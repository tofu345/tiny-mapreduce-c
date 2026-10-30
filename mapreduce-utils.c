#include "mapreduce-utils.h"
#include "rpc/server.h"
#include "rpc/client.h"
#include "rpc/pack.h"

#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/un.h>
#include <unistd.h>

Result newResult(const char * key, uint64_t keylen, const char * val,
                 uint64_t vallen)
{
    char * owned_key = strndup(key, keylen);
    if (owned_key == NULL) die("could not allocate result key:");

    char * owned_val = strndup(val, vallen);
    if (owned_val == NULL) die("could not allocate result val:");

    return (Result){ owned_key, owned_val };
}

void freeResult(Result * r)
{
    free((char *)r->key);
    free((char *)r->val);
    r->key = NULL;
    r->val = NULL;
}

int resultLessThan(const void * a, const void * b)
{
    const Result * _a = a;
    const Result * _b = b;
    return strcmp(_a->key, _b->key);
}

void addResult(Results * buf, Result elem)
{
    ARRAY_APPEND(buf->data, buf->len, buf->cap, elem, sizeof(Result));
}

void freeResults(Results * buf)
{
    for (uint32_t i = 0; i < buf->len; i++)
        freeResult(&buf->data[i]);
    free(buf->data);
    *buf = (Results){0};
}

int getEnvInt(const char * name, int defaultValue)
{
    char * value = getenv(name);
    if (value == NULL)
        return defaultValue;

    char * endptr;
    int num = strtol(value, &endptr, 10);
    if (endptr == value || errno == ERANGE)
        die("could not parse environment variable '%s' value: %s", name, value);
    return num;
}

int numMapTasks(void) {
    int n = getEnvInt("MAPREDUCE_M", 8);
    assert(n > 0);
    return n;
}

int numReduceTasks(void) {
    int n = getEnvInt("MAPREDUCE_R", 4);
    assert(n > 0);
    return n;
}

// TODO: test all this shit code
// TODO: limits on unpacked string lengths

uint32_t packResult(uint8_t * buf, uint32_t len, Result * r)
{
    return pack(buf, len, "ss", r->key, r->val);
}
uint32_t unpackResult(uint8_t * buf, uint32_t len, Result * r)
{
    uint16_t keylen = 0;
    unpack(buf, len, "h", &keylen);
    r->key = malloc(keylen * sizeof(char));
    if (r->key == NULL) return 0;

    uint16_t vallen = 0;
    unpack(buf, len, "h", &vallen);
    r->val = malloc(vallen * sizeof(char));
    if (r->val == NULL) return 0;

    return unpack(buf, len, "ss", r->key, r->val);
}

uint32_t packGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * arg)
{
    return pack(buf, len, "q", arg->workerId);
}
uint32_t unpackGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * res)
{
    return unpack(buf, len, "q", &(res->workerId));
}

uint32_t packTask(uint8_t * buf, uint32_t len, Task * t)
{
    uint32_t initlen = len;

    CHAIN(pack, buf, len, "qqbbqq", t->id, t->workerId, t->status,
          t->funcType, t->startedAt, t->numMapOutputs);

    for (uint64_t i = 0; i < t->numMapOutputs; i++) {
        MapOutput * m = &t->mapOutputs[i];
        CHAIN(pack, buf, len, "sq", m->path, m->size);
    }

    if (t->funcType == FUNC_MAP) {
        CHAIN(pack, buf, len, "sqq", t->inputPath, t->inputOffset,
               t->inputLength);
    }

    return initlen - len;
}

uint32_t unpackTask(uint8_t * buf, uint32_t len, Task * t)
{
    uint32_t initlen = len;
    *t = (Task){0};

    CHAIN(unpack, buf, len, "qqbbqq", &t->id, &t->workerId, &t->status,
          &t->funcType, &t->startedAt, &t->numMapOutputs);

    for (uint64_t i = 0; i < t->numMapOutputs; i++) {
        uint16_t pathlen = 0;
        unpack(buf, len, "h", &pathlen);

        MapOutput * m = &t->mapOutputs[i];
        m->path = malloc(pathlen * sizeof(char));
        if (m->path == NULL)
            return -1;

        CHAIN(unpack, buf, len, "sq", &m->path, &m->size);
    }

    if (t->funcType == FUNC_MAP) {
        uint16_t pathlen = 0;
        unpack(buf, len, "h", &pathlen);

        t->inputPath = malloc(pathlen * sizeof(char));
        if (t->inputPath == NULL)
            return -1;

        CHAIN(unpack, buf, len, "sqq", t->inputPath, &t->inputOffset,
              &t->inputLength);
    }

    return initlen - len;
}

void freeMapOutputs(Task * t)
{
    for (uint64_t i = 0; i < t->numMapOutputs; i++)
        free((char *)t->mapOutputs[i].path);
    free(t->mapOutputs);
    t->mapOutputs = NULL;
    t->numMapOutputs = 0;
}

void freeTask(Task * t)
{
    freeMapOutputs(t);
    free((void *)t->inputPath);
    *t = (Task){0};
}

char * getWorkerSockName(uint64_t workerId) {
    char * buf;
    if (asprintf(&buf, "/tmp/mapreduce-worker-%ld", workerId) == -1)
        return NULL;
    return buf;
}

struct rpc {
    Proc ** procs;
    uint32_t nprocs;
    int socket;
} rpc;

static void * rpcServer(void * arg)
{
    struct rpc * rpc = arg;

    uint8_t buf[4096];
    serveRpc(rpc->socket, rpc->procs, rpc->nprocs, buf, sizeof buf, arg);

    close(rpc->socket);
    free(rpc);
    die("RpcServer: serveRpc should not return");
}

void RpcServer(pthread_t * thread, const char * sockname, Proc * procs[],
               uint32_t nprocs)
{
    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1)
        die("RpcServer socket:");

    remove(sockname);
    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, sockname, sizeof(addr.sun_path) - 1);

    int ret = bind(sock, (const struct sockaddr *) &addr, sizeof(addr));
    if (ret == -1)
        die("RpcServer bind:");

    struct rpc * rpc = malloc(sizeof(struct rpc));
    if (rpc == NULL)
        die("malloc:");

    rpc->procs = procs;
    rpc->nprocs = nprocs;
    rpc->socket = sock;

    ret = pthread_create(thread, NULL, &rpcServer, rpc);
    if (ret != 0)
        die("RpcServer pthread_create: %d", ret);
}

int RpcCall(RpcInt procnum, RpcInt argslen, uint8_t * buf, RpcInt buflen)
{
    int sock = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sock == -1)
        die("RpcCall socket:");

    struct sockaddr_un addr = {};
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, SERVER_SOCK_NAME, sizeof(addr.sun_path) - 1);

    int ret = connect(sock, (const struct sockaddr *)&addr, sizeof(addr));
    if (ret == -1)
        die("RpcCall: connect:");

    ret = callRpc(sock, procnum, argslen, buf, buflen);
    close(sock);
    return ret;
}

void die(const char * fmt, ...)
{
    va_list ap;
    int saved_errno;

    saved_errno = errno;

    // using stderr gives warning:
    // relocation against `stderr@@GLIBC_2.2.5' in read-only section `.text'
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
        printf(" %s", strerror(saved_errno));
    putchar('\n');

    abort();
}

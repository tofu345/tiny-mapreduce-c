#include "mapreduce-utils.h"
#include "rpc/server.h"
#include "rpc/pack.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Result newResult(const char * key, uint64_t keyLen, const char * val, uint64_t valLen)
{
    char * owned_key = strndup(key, keyLen);
    if (owned_key == NULL) die("could not allocate result key");

    char * owned_val = strndup(val, valLen);
    if (owned_val == NULL) die("could not allocate result val");

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

static bool cantfit(size_t size, uint32_t * len)
{
    if (*len < size) return true;
    *len -= size;
    return false;
}

// TODO: test all this shit code

uint32_t packGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * arg)
{
    return pack(buf, len, "l", arg->workerId);
}

int unpackGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * res)
{
    uint32_t buflen = len;

    if (cantfit(sizeof(uint32_t), &len)) return -1;
    unpack(buf, buflen, "l", &(res->workerId));
    return 0;
}

uint32_t packTask(uint8_t * buf, uint32_t len, Task * t)
{
    uint8_t * p = buf;

    p += pack(p, len, "qqbbq", t->id, t->workerId, t->status, t->funcType,
              t->startedAt);

    p += pack(p, len, "q", t->numMapOutputs);
    for (uint64_t i = 0; i < t->numMapOutputs; i++) {
        MapOutput * m = &t->mapOutputs[i];
        p += pack(p, len, "sq", m->path, m->size);
    }

    if (t->funcType == FUNC_MAP) {
        p += pack(p, len, "sqq", t->inputPath, t->inputOffset, t->inputLength);
    }

    return buf - p;
}

int unpackTask(uint8_t * buf, uint32_t len, Task * t)
{
    uint8_t * p = buf;
    uint32_t buflen = len;

    *t = (Task){0};

    if (cantfit((sizeof(uint32_t) * 2) + sizeof(uint64_t) + 2, &len))
        return -1;
    p += unpack(p, buflen, "qqbbq", &t->id, &t->workerId, &t->status,
                &t->funcType, &t->startedAt);

    if (cantfit(sizeof(uint64_t), &len)) return -1;
    p += unpack(p, buflen, "q", &t->numMapOutputs);

    for (uint64_t i = 0; i < t->numMapOutputs; i++) {
        if (cantfit(sizeof(uint16_t), &len)) return -1;
        uint16_t pathlen = 0;
        unpack(p, buflen, "h", &pathlen);
        if (pathlen == 0) return -1;
        if (cantfit(pathlen + sizeof(uint64_t), &len)) return -1;

        MapOutput * m = &t->mapOutputs[i];
        m->path = malloc(pathlen * sizeof(char));
        if (m->path == NULL)
            return -1;

        p += unpack(p, buflen, "sq", &m->path, &m->size);
    }

    if (t->funcType == FUNC_MAP) {
        if (cantfit(sizeof(uint16_t), &len)) return -1;
        uint16_t pathlen = 0;
        unpack(p, buflen, "h", &pathlen);
        if (pathlen == 0) return -1;
        if (cantfit(pathlen + (sizeof(uint64_t) * 2), &len)) return -1;

        t->inputPath = malloc(pathlen * sizeof(char));
        if (t->inputPath == NULL)
            return -1;

        p += unpack(p, buflen, "sqq", t->inputPath, &t->inputOffset,
                    &t->inputLength);
    }

    return 0;
}

void freeMapOutputs(Task * t)
{
    for (uint64_t i = 0; i < t->numMapOutputs; i++)
        free((char *)t->mapOutputs[i].path);
    free(t->mapOutputs);
    t->mapOutputs = NULL;
    t->numMapOutputs = 0;
}

char * getWorkerSockName(uint64_t workerId) {
    char * buf;
    if (asprintf(&buf, "/tmp/mapreduce-worker-%ld", workerId) == -1)
        return NULL;
    return buf;
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

#include <dlfcn.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#include "mapreduce-utils.h"
#include "rpc/client.h"
#include "data_structures/set.h"

#define POLL_INTERVAL 10 // milliseconds
#define MAX_DAIL_MISSES 100

// lists of files with map output partitions
typedef struct {
    char ** data;
    uint32_t len, cap;
} PartFile;

typedef struct {
    pthread_mutex_t mutex;
    WorkerId id;

    int (*map)(char * data, uint64_t len, Results * results);
    int (*reduce)(const char * key, Result * values, uint32_t valuesLen,
                  char * outBuf, int outBufLen);

    uint32_t nReduce;
    PartFile * partFiles;
} Worker;

typedef struct {
    char * data;
    uint64_t length, offset;
    off_t filesize;
} Split;

void workerInit(Worker * w);
void workerFree(Worker * w);
void Run(Worker * w);
void runMap(Worker * w, Task * t);
void runReduce(Worker * w, Task * t);
void complete(Worker * w, Task * t);
void initSplit(Task * t, Split * s);
void freeSplit(Split * s);

RpcInt GetMapOutputs(uint8_t * buf, RpcInt buflen, RpcInt argslen, void * arg);

Proc * procs[] = {
    GetMapOutputs,
};
const uint32_t nprocs = LENGTH(procs);

int main(int argc, char * argv[])
{
    Worker w;
    MapFunc * map;
    ReduceFunc * reduce;
    char * err;
    void * pluginHandle;
    pthread_t rpc;

    if (argc != 2) die("usage: mapreduce-worker funcplugin.so\n");

    pluginHandle = dlopen(argv[1], RTLD_LAZY);
    if (pluginHandle == NULL) die("error loading funcplugin: %s", dlerror());

    // bypass warning: ISO C forbids assignment between function pointer and ‘void *’ [-Wpedantic]
    *(void **)(&map) = dlsym(pluginHandle, "map");
    if ((err = dlerror()) != NULL) die(err);
    *(void **)(&reduce) = dlsym(pluginHandle, "reduce");
    if ((err = dlerror()) != NULL) die(err);

    workerInit(&w);

    char * sockname = getWorkerSockName(w.id);
    RpcServer(&rpc, sockname, procs, nprocs);
    free(sockname);

    Run(&w);

    pthread_cancel(rpc);
    workerFree(&w);
    dlclose(pluginHandle);
    return 0;
}

void Run(Worker * w)
{
    uint8_t buf[1024];
    RpcInt argslen;

    GetTaskArg arg = { .workerId = w->id };
    GetTaskReply reply;

    uint32_t misses = 0;
    bool connected = false;
    struct timespec interval = { .tv_nsec = POLL_INTERVAL * 1'000'000 /* to ms */ };
    int err;

    if ((argslen = packGetTaskArg(buf, sizeof buf, &arg)) == 0)
        die("Run() buffer too small");

    while (1) {
        if ((err = RpcCall(0, argslen, buf, sizeof buf)) != 0) {
            if (!connected) {
                nanosleep(&interval, NULL);
                continue;
            }
            misses++;
            if (misses > MAX_DAIL_MISSES) {
                die("server unreachable, worker %d exiting (rpc err: %d, errno: %s)",
                    w->id, err, strerror(errno));
            }
        }
        connected = true;
        misses = 0;

        if (!unpackGetTaskReply(buf, sizeof buf, &reply))
            die("could not unpackGetTaskReply");

        Task * t = &reply.task;
        if (t->id == (uint64_t)-1)
        {
            nanosleep(&interval, NULL);
        }
        else if (t->funcType == FUNC_MAP)
        {
            runMap(w, t);
        }
        else if (t->funcType == FUNC_REDUCE)
        {
            runReduce(w, t);
        }
    }
}

void initSplit(Task * t, Split * s)
{
    int fd = open(t->inputPath, O_RDONLY);
    if (fd == -1) die("could not open '%s':", t->inputPath);

    off_t size = lseek(fd, 0, SEEK_END);
    if (size == -1) die("could not get file length:");
    else if (size == 0) die("file '%s' cannot be empty", t->inputPath);

    if ((off_t)(t->inputOffset + t->inputLength) > size)
        die("file split greater than file size");

    // TODO: maybe only copy the part of the file needed?
    // man mmap(2): offset must be a multiple of the page size as returned by sysconf(_SC_PAGE_SIZE).
    char * data = mmap(0, t->inputLength, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (data == MAP_FAILED) die("could not perform mmap:");
    if (close(fd) == -1) die("could not close fd:");

    *s = (Split){
        .data = data + t->inputOffset,
        .offset = t->inputOffset,
        .length = t->inputLength,
        .filesize = size,
    };
}

void freeSplit(Split * s)
{
    if (munmap((void *)(s->data - s->offset), s->filesize) == -1)
        die("could not perform munmap:");

    *s = (Split){0};
}

static uint32_t partition(const char * key, uint32_t n) {
    // 32-bit FNV-1a hash
    uint32_t hash = 2166136261;
    for (const char * p = key; *p; ++p) {
        hash ^= (uint32_t)(unsigned char)(*p);
        hash *= 16777619;
    }
    return hash % n;
}

static void flushPartitions(Worker * w, uint64_t mapTaskId, Results * buckets,
                            uint32_t nparts)
{
    for (uint32_t p = 0; p < nparts; p++)
    {
        Results * bucket = &buckets[p];
        if (bucket->len == 0) continue;

        qsort(bucket->data, bucket->len, sizeof(Result), resultLessThan);

        char * name;
        if (asprintf(&name, "mapreduce-part-%ld-%ld-%d", w->id, mapTaskId, p) == -1)
            die("flushPartitions asprintf:");

        FILE * file = fopen(name, "w");
        if (file == NULL) die("could not open file:");

        uint8_t buf[2048];
        uint32_t n = packResults(buf, sizeof buf, bucket);
        if (n == 0) die("flushPartitions: could not packResults");

        if (fwrite(buf, n, 1, file) != n)
            die("flushPartitions: fwrite: %d", ferror(file));

        PartFile * pf = &w->partFiles[p];
        pthread_mutex_lock(&w->mutex);
        ARRAY_APPEND(pf->data, pf->len, pf->cap, name, sizeof(char *));
        pthread_mutex_unlock(&w->mutex);

        if (fclose(file) == -1) die("could not close file:");
    }
}

void complete(Worker * w, Task * t)
{
    (void)w;

    uint8_t buf[1024];
    RpcInt argslen = packTask(buf, sizeof buf, t);
    if (argslen == 0) die("complete() buf too small");

    int err = RpcCall(1, argslen, buf, sizeof buf);
    if (err != 0) {
        printf("could not report completion of task %ld: %d", t->id, err);
    }
}

void runMap(Worker * w, Task * t)
{
    printf("worker %ld running map task %ld (%s)", w->id, t->id, t->inputPath);

    Results mapResult = {0};
    Split split;
    initSplit(t, &split);

    if (w->map(split.data, split.length, &mapResult) != 0)
        die("map failed on %s (offset: %d, length: %d)",
            t->inputPath, t->inputOffset, t->inputLength);

    uint32_t nparts = w->nReduce;
    Results buckets[nparts];
    memset(buckets, 0, sizeof(Results) * nparts);

    for (uint32_t i = 0; i < nparts; i++) {
        uint32_t p = partition(mapResult.data[i].key, nparts);
        addResult(&buckets[p], mapResult.data[i]);
    }

    flushPartitions(w, t->id, buckets, nparts);
    complete(w, t);

    for (uint32_t i = 0; i < nparts; i++)
        freeResults(&buckets[i]);
    freeResults(&mapResult);
}

void runReduce(Worker * w, Task * t)
{
}

RpcInt GetMapOutputs(uint8_t * buf, RpcInt buflen, RpcInt argslen, void * arg)
{
    (void)buf; (void)buflen; (void)arg;
    return argslen;
}

void workerInit(Worker * w)
{
    *w = (Worker){0};
    pthread_mutex_init(&w->mutex, NULL);

    w->id = getpid(); // TODO: leave as 0 and have server assign WorkerID when
                      // getting task

    w->nReduce = numReduceTasks();

    w->partFiles = calloc(w->nReduce, sizeof(PartFile));
    if (w->partFiles == NULL) die("workerInit malloc partFiles");
}

void workerFree(Worker * w)
{
    pthread_mutex_destroy(&w->mutex);

    for (uint32_t i = 0; i < w->nReduce; i++) {
        PartFile * pf = &w->partFiles[i];
        for (uint32_t j = 0; j < pf->len; j++) {
            free(pf->data[j]);
        }
        free(pf->data);
    }
    free(w->partFiles);

    *w = (Worker){0};
}

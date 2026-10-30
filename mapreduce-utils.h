#ifndef INCLUDE_MAPREDUCE_UTILS_H
#define INCLUDE_MAPREDUCE_UTILS_H

#include <stdint.h>
#include <time.h>

#include "rpc/server.h"
#include "rpc/pack.h"

typedef uint64_t WorkerId;

#define SERVER_SOCK_NAME "/tmp/mapreduce-server"
char * getWorkerSockName(WorkerId workerId);

void RpcServer(pthread_t * thread, const char * sockname, Proc * procs[],
               uint32_t nprocs);
int RpcCall(RpcInt procnum, RpcInt argslen, uint8_t * buf, RpcInt buflen);

typedef struct {
    const char * key;
    const char * val;
} Result;

Result newResult(const char * key, uint64_t keylen, const char * val,
                 uint64_t vallen);
void freeResult(Result * r);
int resultLessThan(const void * a, const void * b);

uint32_t packResult(uint8_t * buf, uint32_t len, Result * r);
uint32_t unpackResult(uint8_t * buf, uint32_t len, Result * r);

typedef struct {
    Result * data;
    uint32_t len, cap;
} Results;

typedef int MapFunc(char * data, uint64_t len, Results * results);
typedef int ReduceFunc(const char * key, Result * values, uint32_t valuesLen,
                       char * outBuf, int outBufLen);

void addResult(Results * buf, Result elem);
void freeResults(Results * buf);

static inline uint32_t packResults(uint8_t * buf, uint32_t len, Results * r) {
    uint32_t initlen = len;
    for (uint32_t i = 0; i < r->len; i++)
        CHAIN(packResult, buf, len, &r->data[i]);
    return initlen - len;
}
static inline uint32_t unpackResults(uint8_t * buf, uint32_t len, Results * r) {
    uint32_t initlen = len;
    for (uint32_t i = 0; i < r->len; i++)
        CHAIN(unpackResult, buf, len, &r->data[i]);
    return initlen - len;
}

int getEnvInt(const char * name, int defaultValue);
int numMapTasks(void);
int numReduceTasks(void);

typedef enum {
    FUNC_MAP,
    FUNC_REDUCE,
} FuncType;

typedef struct {
    const char * path;
    uint64_t size;
} MapOutput;

typedef struct {
    uint64_t id;
    WorkerId workerId;
    enum {
        TASK_IDLE,
        TASK_INPROGRESS,
        TASK_DONE,
    } status;
    FuncType funcType;
    time_t startedAt;

    MapOutput * mapOutputs;
    uint32_t numMapOutputs;

    const char * inputPath;
    uint64_t inputOffset;
    uint64_t inputLength;
} Task;

void freeMapOutputs(Task * t);
void freeTask(Task * t);

_Static_assert ((sizeof(time_t) <= sizeof(uint64_t)),
                "time_t must smaller than or equal to uint64_t, for packTask()");

uint32_t packTask(uint8_t * buf, uint32_t len, Task * t);
uint32_t unpackTask(uint8_t * buf, uint32_t len, Task * t);

typedef struct {
    WorkerId workerId;
} GetTaskArg;

typedef struct {
    Task task;
} GetTaskReply;

typedef struct {
    Task task;
} CompleteTaskArgs;

uint32_t packGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * data);
uint32_t unpackGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * result);

static inline uint32_t packGetTaskReply(uint8_t * buf, uint32_t len,
                                        GetTaskReply * data) {
    return packTask(buf, len, &data->task);
}
static inline uint32_t unpackGetTaskReply(uint8_t * buf, uint32_t len,
                                          GetTaskReply * data) {
    return unpackTask(buf, len, &data->task);
}

#define ARRAY_APPEND(data, len, cap, elem, size)                    \
    if (len + 1 > cap)                                              \
    {                                                               \
        cap = cap == 0 ? 4 : cap * 2;                               \
        data = realloc(data, cap * size);                           \
        if (data == NULL) die("ARRAY_APPEND:");                     \
    }                                                               \
    data[len++] = elem;

_Noreturn void die(const char * fmt, ...); // from dwm :p

#endif // INCLUDE_MAPREDUCE_UTILS_H

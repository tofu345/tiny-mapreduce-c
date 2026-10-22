#ifndef INCLUDE_MAPREDUCE_UTILS_H
#define INCLUDE_MAPREDUCE_UTILS_H

#include <stdint.h>
#include <time.h>

#include "rpc/server.h"

typedef uint64_t WorkerId;

#define SERVER_SOCK_NAME "/tmp/mapreduce-server"
char * getWorkerSockName(WorkerId workerId);

typedef struct {
    const char * key;
    const char * val;
} Result;

Result newResult(const char * key, uint64_t keyLen, const char * val, uint64_t valLen);
void freeResult(Result * r);

int resultLessThan(const void * a, const void * b);

typedef struct {
    Result * data;
    uint32_t len, cap;
} Results;

void addResult(Results * buf, Result elem);
void freeResults(Results * buf);

int getEnvInt(const char * name, int defaultValue);

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

_Static_assert ((sizeof(time_t) <= sizeof(uint64_t)),
                "time_t must smaller than or equal to uint64_t, for packTask()");

uint32_t packTask(uint8_t * buf, uint32_t len, Task * t);
int unpackTask(uint8_t * buf, uint32_t len, Task * t);

void freeMapOutputs(Task * t);

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
int unpackGetTaskArg(uint8_t * buf, uint32_t len, GetTaskArg * result);

#define ARRAY_APPEND(data, len, cap, elem, size)                    \
    if (len + 1 > cap)                                              \
    {                                                               \
        cap = cap == 0 ? 4 : cap * 2;                               \
        data = realloc(data, cap * size);                           \
        if (data == NULL) die("ARRAY_APPEND:");                     \
    }                                                               \
    data[len++] = elem;

void die(const char * fmt, ...);

#endif // INCLUDE_MAPREDUCE_UTILS_H

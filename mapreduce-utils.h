#ifndef INCLUDE_MAPREDUCE_UTILS_H
#define INCLUDE_MAPREDUCE_UTILS_H

#include <stdint.h>
#include <time.h>

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

int getEnvOr(const char * name, int defaultValue);

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
    uint64_t workerId;
    enum {
        TASK_IDLE,
        TASK_INPROGRESS,
        TASK_DONE,
    } status;
    FuncType funcType;
    time_t startedAt;

    MapOutput * mapOutputs;
    uint64_t numMapOutputs;

    const char * inputPath;
    uint64_t inputOffset;
    uint64_t inputLength;
} Task;

#define ARRAY_APPEND(data, len, cap, elem)                          \
    if (len + 1 > cap)                                              \
    {                                                               \
        cap = cap == 0 ? 4 : cap * 2;                               \
        data = realloc(data, cap * sizeof(elem));                   \
        if (data == NULL) die("ARRAY_APPEND:");                     \
    }                                                               \
    data[len++] = elem;

void die(const char * fmt, ...);

#endif // INCLUDE_MAPREDUCE_UTILS_H

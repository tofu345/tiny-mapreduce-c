#ifndef INCLUDE_MAPREDUCE_UTILS_H
#define INCLUDE_MAPREDUCE_UTILS_H

#include <stdint.h>

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

void die(const char * fmt, ...);

#endif // INCLUDE_MAPREDUCE_UTILS_H

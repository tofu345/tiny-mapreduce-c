#ifndef INCLUDE_WORDCOUNTER_H
#define INCLUDE_WORDCOUNTER_H

#include "mapreduce-utils.h"

#include <stdint.h>

int map(char * data, uint64_t len, Results * results);
int reduce(const char * key, Result * values, uint32_t valuesLen, char * buf,
           int bufLen);

#endif // INCLUDE_WORDCOUNTER_H

#ifndef INCLUDE_DATA_STRUCTURE_SET_H
#define INCLUDE_DATA_STRUCTURE_SET_H

#include <stdint.h>
#include <stdbool.h>

#ifndef SET_INITIAL_CAPACITY
#   define SET_INITIAL_CAPACITY 16
#endif

typedef struct {
    uint64_t * data;
    uint32_t length, capacity;
} Set;

void SetInit(Set * s);
void SetFree(Set * s);

void SetInsert(Set * s, uint64_t elem);
bool SetContains(Set * s, uint64_t elem);

#endif // INCLUDE_DATA_STRUCTURE_SET_H

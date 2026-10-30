#include "set.h"
#include "../mapreduce-utils.h"

#include <assert.h>
#include <stdlib.h>

void SetInit(Set * s)
{
    *s = (Set){
        .data = calloc(SET_INITIAL_CAPACITY, sizeof(uint64_t)),
        .length = 0,
        .capacity = SET_INITIAL_CAPACITY,
    };
    if (s->data == NULL) die("SetInit calloc");
}

void SetFree(Set * s)
{
    free(s->data);
    *s = (Set){0};
}

static inline uint32_t hashBits(uint64_t hash)
{
    // From https://github.com/wren-lang/wren which cites
    // v8's ComputeLongHash() which in turn cites:
    // Thomas Wang, Integer Hash Functions.
    // http://www.concentric.net/~Ttwang/tech/inthash.htm
    hash = ~hash + (hash << 18);  // hash = (hash << 18) - hash - 1;
    hash = hash ^ (hash >> 31);
    hash = hash * 21;  // hash = (hash + (hash << 2)) + (hash << 4);
    hash = hash ^ (hash >> 11);
    hash = hash + (hash << 6);
    hash = hash ^ (hash >> 22);
    return (uint32_t)(hash & 0x3fffffff);
}

// get a position in `s->data` where key is currently located or could be
static uint64_t *
getEntry(Set * s, uint64_t elem)
{
    uint32_t hash = hashBits(elem);
    uint32_t index = hash & (s->capacity - 1);

    while (s->data[index] != 0)
    {
        if (s->data[index] == elem)
            return &s->data[index];

        index++;
        if (index >= s->capacity)
            index = 0;
    }

    return &s->data[index];
}

static void setExpand(Set * s)
{
    Set new = {
        .data = calloc(s->capacity * 2, sizeof(uint64_t)),
        .length = 0,
        .capacity = s->capacity * 2,
    };
    if (new.data == NULL) die("setExpand calloc");

    for (uint32_t i = 0; i < s->capacity; i++) {
        if (s->data[i] == 0) continue;
        uint64_t * p = getEntry(&new, s->data[i]);
        if (*p != 0) continue;
        *p = s->data[i];
    }

    free(s->data);
    *s = new;
}

void SetInsert(Set * s, uint64_t elem)
{
    assert(elem != 0);

    if (s->length >= s->capacity / 2)
        setExpand(s);

    uint64_t * p = getEntry(s, elem);
    if (*p != 0) return; // key already present
    *p = elem;
}

bool SetContains(Set * s, uint64_t elem)
{
    assert(elem != 0);
    uint64_t * p = getEntry(s, elem);
    return *p == elem;
}

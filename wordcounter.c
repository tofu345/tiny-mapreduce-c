#include "wordcounter.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

int map(char * data, uint64_t len, Results * results)
{
    for (uint64_t i = 0; i < len; i++)
        if (!isalnum(data[i]))
            data[i] = ' ';

    for (uint64_t i = 0; i < len; i++)
    {
        uint64_t j = i;
        for (; j < len && !isspace(data[j]); j++);
        if (j == i) continue;

        Result result = newResult(&data[i], j - i, "1", 1);
        addResult(results, result);

        i = j;
    }
    return 0;
}

int reduce(const char * key, Result * values, uint32_t valuesLen,
           char * outBuf, int outBufLen)
{
    return snprintf(outBuf, outBufLen, "%d", valuesLen);
}

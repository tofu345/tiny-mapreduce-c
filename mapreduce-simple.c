#include <assert.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mapreduce-utils.h"

int main(int argc, char ** argv)
{
    if (argc < 3)
    {
        fprintf(stderr, "usage: mapreduce-simple <functions.so> <inputfile> ...");
        return 1;
    }

    int (*map)(char * data, uint64_t len, Results * results);
    int (*reduce)(const char * key, Result * values, uint32_t valuesLen,
                  char * outBuf, int outBufLen);

    Results results = {0};
    char * err;

    void * pluginHandle = dlopen(argv[1], RTLD_LAZY);
    if (pluginHandle == NULL) die("error loading function: %s", dlerror());

    // bypass warning: ISO C forbids assignment between function pointer and ‘void *’ [-Wpedantic]
    *(void **)(&map) = dlsym(pluginHandle, "map");
    if ((err = dlerror()) != NULL) die(err);
    *(void **)(&reduce) = dlsym(pluginHandle, "reduce");
    if ((err = dlerror()) != NULL) die(err);

    for (int i = 2; i < argc; i++)
    {
        int fd = open(argv[i], O_RDONLY);
        if (fd == -1) die("could not open '%s':", argv[i]);

        off_t data_len = lseek(fd, 0, SEEK_END);
        if (data_len == -1) die("could not get file length:");
        else if (data_len == 0) die("file '%s' cannot be empty", argv[i]);

        char * data = mmap(0, data_len, PROT_WRITE, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) die("could not perform mmap:");

        char * p = data;
        while (*p != '\0')
        {
            char * end = strchr(p, '\n');
            if (end == NULL) break;
            assert(end != NULL);
            uint64_t len = end - p;

            if (map(p, len, &results) != 0)
                die("map failed on line: %.*s\n", (int)len, p);

            p = end + 1;
        }

        if (munmap((void *)data, data_len) == -1) die("could not perform munmap:");
        if (close(fd) == -1) die("could not close fd:");
    }

    qsort(results.data, results.len, sizeof(Result), resultLessThan);

    FILE * outFile = fopen("mapreduce-simple-out", "w");
    if (outFile == NULL) die("could not open output file:");

    uint32_t i = 0;
    while (i < results.len)
    {
        const char * key = results.data[i].key;

        uint32_t j = i + 1;
        while (j < results.len && strcmp(key, results.data[j].key) == 0)
            j++;

        const int bufLen = 1024;
        char buf[bufLen];
        int n = reduce(key, &results.data[i], j - i, buf, bufLen);
        if (n < 0)
            die("reduce failed on key '%s':\n", key);
        else if (n >= bufLen)
            die("reduce failed on key '%s': buffer length too small", key);

        fprintf(outFile, "%s %.*s\n", key, n, buf);

        i = j;
    }

    fclose(outFile);
    dlclose(pluginHandle);
    freeResults(&results);
    return 0;
}

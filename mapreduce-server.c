#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "mapreduce-utils.h"

#define MIN(a, b) ((a < b) ? (a) : (b))

struct Tasks {
    Task * data;
    uint32_t len, cap;
};

typedef struct {
    struct Tasks mapTasks,
                 reduceTasks;
} Server;

void serverInit(Server * s, const char ** inputs, int numInputs);
void addTask(struct Tasks * tasks, Task task);
void serverFree(Server * s);

int main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "usage: mapreduce-server <inputfile> ...");
        return 1;
    }

    Server s;
    serverInit(&s, (const char **)&argv[1], argc - 1);

    serverFree(&s);
    return 0;
}

static void buildMapTasks(struct Tasks * mapTasks, const char ** inputs,
                          int numInputs, int numMapTasks)
{
    off_t totalSize = 0;

    for (int i = 0; i < numInputs; i++)
    {
        struct stat stat_buf;
        if (stat(inputs[i], &stat_buf) == -1)
            die("could not stat '%s':", inputs[i]);
        if (__builtin_add_overflow(totalSize, stat_buf.st_size, &totalSize))
            die("integer overflow summing file sizes"); // necessary?
    }

    off_t splitSize = MIN(1, totalSize / numMapTasks);

    for (int i = 0; i < numInputs; i++)
    {
        int fd = open(inputs[i], O_RDONLY);
        if (fd == -1) die("could not open '%s':", inputs[i]);

        off_t size = lseek(fd, 0, SEEK_END);
        if (size == -1) die("could not get file length:");
        else if (size == 0) die("file '%s' cannot be empty", inputs[i]);

        const char * data = mmap(0, size, PROT_READ, MAP_PRIVATE, fd, 0);
        if (data == MAP_FAILED) die("could not perform mmap:");

        off_t pos = 0;
        while (pos < size)
        {
            off_t end = pos + splitSize;
            if (end < size)
                for (; end < size && data[end] != '\n'; end++);

            addTask(mapTasks, (Task) {
                .id = mapTasks->len,
                .status = TASK_IDLE,
                .funcType = FUNC_MAP,
                .inputPath = inputs[i],
                .inputOffset = pos,
                .inputLength = end - pos,
            });

            pos = end + 1;
        }

        if (munmap((void *)data, size) == -1) die("could not perform munmap:");
        if (close(fd) == -1) die("could not close fd:");
    }
}

void serverInit(Server * s, const char ** inputs, int numInputs)
{
    *s = (Server){0};

    buildMapTasks(&s->mapTasks, inputs, numInputs, getEnvOr("MAPREDUCE_M", 8));

    for (int i = 0; i < getEnvOr("MAPREDUCE_R", 4); i++)
    {
        addTask(&s->reduceTasks, (Task){
            .id = i,
            .status = TASK_IDLE,
            .funcType = FUNC_REDUCE,
        });
    }
}

void addTask(struct Tasks * tasks, Task task)
{
    ARRAY_APPEND(tasks->data, tasks->len, tasks->cap, task);
}

void serverFree(Server * s)
{
    free(s->mapTasks.data);
    free(s->reduceTasks.data);
    *s = (Server){0};
}

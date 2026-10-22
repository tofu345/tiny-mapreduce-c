#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#include "mapreduce-utils.h"
#include "rpc/pack.h"

// in seconds
#define TASK_TIMEOUT 10
#define CHECK_INTERVAL 1

// Stopped 1:16:24

struct Tasks {
    Task * data;
    uint32_t len, cap;
};

typedef struct {
    pthread_mutex_t mutex;
    struct Tasks mapTasks, reduceTasks;
    pthread_t requeuer;
} Server;

void serverInit(Server * s, const char ** inputs, int numInputs);
void addTask(struct Tasks * tasks, Task task);
bool done(Server * s);
void * startRequeueLoop(void * arg);
void * rpcServer(void * arg);
void serverFree(Server * s);

uint32_t getTask(uint8_t * buf, uint32_t buflen, uint32_t argslen, void * arg);
uint32_t completeTask(uint8_t * buf, uint32_t buflen, uint32_t argslen, void * arg);

Proc * procs[] = {
    getTask,
    completeTask,
};
const uint32_t nprocs = LENGTH(procs);

int main(int argc, char ** argv)
{
    int ret;
    pthread_t rpc, requeuer;
    Server s;

    if (argc < 2)
    {
        fprintf(stderr, "usage: mapreduce-server <inputfile> ...");
        return 1;
    }

    serverInit(&s, (const char **)&argv[1], argc - 1);

    ret = pthread_create(&requeuer, NULL, &startRequeueLoop, &s);
    if (ret != 0) {
        fprintf(stderr, "serverInit: pthread_create requeuer: %d", ret);
        exit(1);
    }

    ret = pthread_create(&rpc, NULL, &rpcServer, NULL);
    if (ret != 0) {
        fprintf(stderr, "serverInit: pthread_create rpc: %d", ret);
        exit(1);
    }

    while (!done(&s)) {
        sleep(20);
    }

    printf("all tasks completed, master exiting");

    pthread_cancel(rpc);
    pthread_cancel(requeuer);
    serverFree(&s);
    return 0;
}

void * rpcServer(void * arg)
{
    (void)arg;

    int sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("rpcServer: socket");
        exit(1);
    }

    remove(SERVER_SOCK_NAME);

    struct sockaddr_un name = {};
    name.sun_family = AF_UNIX;
    strncpy(name.sun_path, SERVER_SOCK_NAME, sizeof(name.sun_path) - 1);

    int ret = bind(sock, (const struct sockaddr *) &name, sizeof(name));
    if (ret == -1) {
        perror("bind");
        exit(EXIT_FAILURE);
    }

    uint8_t buf[4096];
    serveRpc(sock, procs, nprocs, buf, sizeof buf, arg);

    close(sock);
    fprintf(stderr, "rpcServer: serveRpc should not return");
    exit(1);
}

static void requeueTasks(struct Tasks * tasks, time_t now)
{
    for (uint32_t i = 0; i < tasks->len; i++) {
        Task * t = &tasks->data[i];
        if (t->status == TASK_INPROGRESS) {
            int64_t time_elapsed = difftime(now, t->startedAt);
            if (time_elapsed > 0 && time_elapsed > TASK_TIMEOUT) {
                printf("requeueing stale task %ld from worker %ld after %ld seconds\n",
                       t->id, t->workerId, time_elapsed);
                t->status = TASK_DONE;
                t->workerId = -1;
            }
        }
    }
}

void requeueStalled(Server * s)
{
    pthread_mutex_lock(&s->mutex);

    time_t now = time(NULL);
    requeueTasks(&s->mapTasks, now);
    requeueTasks(&s->reduceTasks, now);

    pthread_mutex_unlock(&s->mutex);
}

void * startRequeueLoop(void * arg)
{
    Server * s = arg;
    struct timespec interval = { .tv_sec = CHECK_INTERVAL };
    while (1)
    {
        nanosleep(&interval, NULL); // surely no errors?
        requeueStalled(s);
    }
    return NULL;
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
    *s = (Server){};
    pthread_mutex_init(&s->mutex, NULL);

    int numMapTasks = getEnvInt("MAPREDUCE_M", 8);
    assert(numMapTasks > 0);
    buildMapTasks(&s->mapTasks, inputs, numInputs, numMapTasks);

    int numReduceTasks = getEnvInt("MAPREDUCE_R", 4);
    assert(numReduceTasks > 0);

    for (int i = 0; i < numReduceTasks; i++)
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
    ARRAY_APPEND(tasks->data, tasks->len, tasks->cap, task, sizeof(Task));
}

static bool allDone(struct Tasks * tasks) {
    for (uint32_t i = 0; i < tasks->len; i++)
        if (tasks->data[i].status != TASK_DONE)
            return false;
    return true;
}

bool done(Server * s)
{
    pthread_mutex_lock(&s->mutex);
    bool res = allDone(&s->mapTasks) && allDone(&s->reduceTasks);
    pthread_mutex_unlock(&s->mutex);
    return res;
}

static Task * assignFrom(struct Tasks * tasks, uint64_t workerId)
{
    for (uint32_t i = 0; i < tasks->len; i++) {
        Task * t = &tasks->data[i];
        if (t->status == TASK_IDLE) {
            t->status = TASK_INPROGRESS;
            t->workerId = workerId;
            t->startedAt = time(NULL);
            return t;
        }
    }
    return NULL;
}

static bool contains(uint64_t * arr, uint32_t len, uint64_t n)
{
    for (uint32_t i = 0; i < len; i++)
        if (arr[i] == n)
            return true;
    return false;
}

void mapWorkerLocations(Server * s, Task * task)
{
    // TODO: a hashmap would be _much_ better here
    uint64_t * seen = NULL;
    uint32_t seen_len = 0;
    uint32_t seen_cap = 0;

    uint32_t capacity = 0;
    for (uint32_t i = 0; i < s->mapTasks.len; i++) {
        Task * t = &s->mapTasks.data[i];
        if (!contains(seen, seen_len, t->workerId)) {
            ARRAY_APPEND(seen, seen_len, seen_cap, t->workerId, sizeof(t->workerId));

            MapOutput m = { .path = getWorkerSockName(t->workerId) };
            ARRAY_APPEND(task->mapOutputs, task->numMapOutputs, capacity, m, sizeof(MapOutput));
        }
    }
    free(seen);
}

Task * assign(Server * s, WorkerId workerId)
{
    pthread_mutex_lock(&s->mutex);

    Task * task = assignFrom(&s->mapTasks, workerId);
    if (task != NULL) {
        printf("assigned map task %ld to worker %ld\n", task->id, workerId);
        goto exit;
    }

    if (allDone(&s->mapTasks)) {
        task = assignFrom(&s->reduceTasks, workerId);
        if (task != NULL) {
            freeMapOutputs(task); // if any
            mapWorkerLocations(s, task);
            printf("assigned reduce task %ld to worker %ld\n", task->id, workerId);
            goto exit;
        }
    }

exit:
    pthread_mutex_unlock(&s->mutex);
    return task;
}

void complete(Server * s, Task * t)
{
    pthread_mutex_lock(&s->mutex);

    struct Tasks * tasks = &s->mapTasks;
    if (t->funcType == FUNC_REDUCE) {
        tasks = &s->reduceTasks;
    }
    if (t->id == (uint32_t)(-1) || t->id > tasks->len)
        goto exit;

    Task * stored = &tasks->data[t->id];
    if (stored->status == TASK_INPROGRESS && stored->workerId == t->workerId) {
        stored->status = TASK_DONE;

        freeMapOutputs(stored); // if any
        stored->mapOutputs = t->mapOutputs;
        stored->numMapOutputs = t->numMapOutputs;
        t->mapOutputs = NULL;
        t->numMapOutputs = 0;
    }

exit:
    pthread_mutex_unlock(&s->mutex);
}

uint32_t getTask(uint8_t * buf, uint32_t buflen, uint32_t argslen, void * arg)
{
    WorkerId workerId = -1;
    unpack(buf, argslen, "q", &workerId);
    if (workerId == (WorkerId)-1) return 0;

    Server * s = arg;
    Task * t = assign(s, workerId);
    Task reply;
    if (t != NULL)
        reply = *t;
    else
        reply = (Task){ .id = (uint32_t)-1 };

    return packTask(buf, buflen, &reply);
}

uint32_t completeTask(uint8_t * buf, uint32_t buflen, uint32_t argslen, void * arg)
{
    (void)buflen;

    Task t;
    if (!unpackTask(buf, argslen, &t))
        return 0;

    Server * s = arg;
    complete(s, &t);
    return 0;
}

void serverFree(Server * s)
{
    pthread_mutex_destroy(&s->mutex);
    free(s->mapTasks.data);
    free(s->reduceTasks.data);
    *s = (Server){0};
}

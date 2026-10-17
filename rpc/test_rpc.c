/*
 * Sends RPC packets on the main thread, recieving them in a sub thread.
 * The main thread exposes a single method to reverse an incoming string, the
 * sub thread, checks if the response is as expected.
 */

#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "pack.h"
#include "client.h"
#include "server.h"
#include "utils.h"

void * client(void * arg)
{
    int sock = (uintptr_t)arg;

    char * message = "hello there";
    char * expectedResult = "ereht olleh";

    uint8_t buf[1024];
    char result[1024];
    int err = callRpc(sock, 0, message, buf, sizeof buf, &pack_cstring,
                      &unpack_cstring, result);
    if (err != 0) {
        fprintf(stderr, "client: callRpc error: %d (errno: %s)\n",
                err, strerror(errno));
        goto exit;
    }

    if (strcmp(expectedResult, result) != 0) {
        fprintf(stderr, "client: invalid response, want '%s', got '%s'\n",
                expectedResult, result);
        goto exit;
    }

exit:
    // free(result);
    close(sock);
    return NULL;
}

int main(void)
{
    int sockets[2];
    uint8_t buf[1024];
    uint32_t procnum, argslen;
    int ret;
    pthread_t thread;

    ret = socketpair(AF_UNIX, SOCK_STREAM, 0, sockets);
    if (ret == -1) {
        perror("create socketpair");
        return 1;
    }

    ret = pthread_create(&thread, NULL, &client, (void *)(uintptr_t)sockets[1]);
    if (ret != 0) {
        fprintf(stderr, "pthread_create: %d", ret);
        return 1;
    }

    if ((ret = recvRpc(sockets[0], &procnum, &argslen, 1, buf, sizeof buf)) < 0) {
        fprintf(stderr, "server: invalid rpc packet (err: %d): %s\n",
                ret, strerror(errno));
        goto close;
    }

    if (procnum != 0) {
        fprintf(stderr, "server: invalid procnum, must be 0\n");
        goto close;
    }

    uint32_t resplen = reverse(buf, sizeof buf, argslen);
    if (sendRpc(sockets[0], buf, resplen) == -1) {
        fprintf(stderr, "server: error sending response: %s\n",
                strerror(errno));
        goto close;
    }

close:
    close(sockets[0]);

    ret = pthread_join(thread, NULL);
    if (ret != 0)
        perror("pthread_join");
    return 0;
}

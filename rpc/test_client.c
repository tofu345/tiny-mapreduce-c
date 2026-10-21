/*
 * A basic RPC client
 */

#include <errno.h>
#include <netdb.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "client.h"
#include "pack.h"

int main(int argc, char * argv[])
{
    uint8_t buf[4096];
    uint32_t procnum;
    int sock, err;
    char * endptr;
    struct addrinfo hints, *ai;

    if (argc != 4) {
        fprintf(stderr, "usage: test_client port procnum data\n");
        return 1;
    }
    size_t arglen = strlen(argv[3]);
    if (arglen + 2 > sizeof buf) {
        fprintf(stderr,
                "test_client: string must be <= 1022 characters, got %ld\n",
                arglen);
        return 1;
    }

    errno = 0;
    procnum = (uint32_t)strtol(argv[2], &endptr, 0);
    if (endptr == argv[2]) {
        fprintf(stderr, "could not parse as integer: %s\n", argv[2]);
        return 1;
    } else if (errno == ERANGE) {
        fprintf(stderr, "integer out of range: %s\n", argv[2]);
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((err = getaddrinfo("localhost", argv[1], &hints, &ai)) != 0) {
        fprintf(stderr, "test_client: getaddrinfo: %s\n", gai_strerror(err));
        return 1;
    }

    sock = connectTo(ai);
    if (sock == -1) {
        perror("test_client: connectTo");
        return 1;
    }

    uint32_t argslen = pack(buf, sizeof buf, "s", argv[3]);
    if ((err = callRpc(sock, procnum, argslen, buf, sizeof buf)) != 0)
    {
        fprintf(stderr, "test_client: callRpc error: %d (errno: %s)\n",
                err, strerror(errno));
        return 1;
    }

    char result[1024];
    unpack(buf, sizeof buf, "s", result);
    printf("test_client: response '%s'\n", result);

    close(sock);
    freeaddrinfo(ai);
}

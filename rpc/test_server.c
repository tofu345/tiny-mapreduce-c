/*
 * A basic RPC server
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdint.h>

#include "server.h"
#include "pack.h"
#include "utils.h"
#include "common.h"

#define LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

Proc * procs[] = { echo, reverse };

int main(int argc, char * argv[])
{
    struct addrinfo hints, *servinfo;
    int rv;

    if (argc != 2) {
        fprintf(stderr, "usage: test_server port\n");
        return 1;
    }

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE; // use my IP

    if ((rv = getaddrinfo(NULL, argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "test_server: getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    serveRpc(servinfo, procs, LENGTH(procs));

    freeaddrinfo(servinfo);
    fprintf(stderr, "serveRpc should not return");
    return 1;
}

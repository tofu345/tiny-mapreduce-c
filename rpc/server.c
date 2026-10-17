#include "server.h"
#include "common.h"
#include "pack.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

static int bindTo(struct addrinfo * ai)
{
    int sock;
    int yes = 1;
    for (ai = ai; ai != NULL; ai = ai->ai_next) {
        if ((sock = socket(ai->ai_family, ai->ai_socktype,
                           ai->ai_protocol)) == -1) {
            continue;
        }

        if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
            perror("setsockopt");
        }

        if (bind(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
            close(sock);
            continue;
        }

        break;
    }
    if (ai == NULL) return -1;

    return sock;
}

// get sockaddr, IPv4 or IPv6:
static void * getInAddr(struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int recvRpc(int sock, uint32_t * procnum, uint32_t * argslen,
            uint32_t maxprocs, uint8_t * argbuf, size_t argbuflen)
{
    uint8_t buf[RPC_INIT_PACKET_LENGTH];
    ssize_t n;

    if ((n = recv(sock, buf, RPC_INIT_PACKET_LENGTH, 0)) == -1)
        return -1;
    if ((size_t)n != RPC_INIT_PACKET_LENGTH)
        return -2;
    if (strncmp((char *)buf, "RPC", 3) != 0)
        return -3;

    assert(procnum != NULL && argslen != NULL);
    *procnum = 0;
    *argslen = 0;
    unpack(&buf[3], RPC_INIT_PACKET_LENGTH - 3, "ll", procnum, argslen);

    if (*procnum >= maxprocs)
        return -4;
    if (*argslen > argbuflen)
        return -5;

    assert(argbuf != NULL);
    size_t len = *argslen;
    if (len > 0 && recvall(sock, argbuf, &len) == -1)
        return -6;

    return 0;
}

int sendRpc(int sock, uint8_t * resp, uint32_t resplen)
{
    uint8_t buf[sizeof(uint32_t)];
    uint32_t size = pack(buf, sizeof buf, "l", resplen);

    // send response length
    ssize_t n;
    if ((n = send(sock, buf, size, 0)) == -1 || n == 0)
        return -1;

    // send response
    size_t len = resplen;
    if (resplen > 0 && sendall(sock, resp, &len) == -1)
        return -1;

    return 0;
}

void serveRpc(struct addrinfo * ai, Proc * procs[], uint32_t nprocs)
{
    int sock = bindTo(ai);
    if (sock == -1) {
        perror("serveRpc: bind to addrinfo");
        exit(1);
    }

    if (listen(sock, /* connection backlog */ 10) == -1) {
        perror("serveRpc: listen");
        exit(1);
    }

    int r;
    char conn_str[INET6_ADDRSTRLEN];
    uint8_t * buf = calloc(RPC_PROC_BUFLEN, sizeof(uint8_t));
    if (buf == NULL) {
        perror("serveRpc: allocate buffer");
        exit(1);
    }

    while (1)
    {
        struct sockaddr_storage conn_addr; // connector's address info
        socklen_t size = sizeof conn_addr;

        int conn = accept(sock, (struct sockaddr *)&conn_addr, &size);
        if (conn == -1) {
            perror("serveRpc: accept");
            continue;
        }

        inet_ntop(conn_addr.ss_family,
                  getInAddr((struct sockaddr *)&conn_addr),
                  conn_str, sizeof conn_str);
        printf("serveRpc: (fd %d) got connection from %s\n", conn, conn_str);

        errno = 0;
        uint32_t procnum, argslen;
        if ((r = recvRpc(conn, &procnum, &argslen, nprocs, buf, RPC_PROC_BUFLEN)) < 0) {
            fprintf(stderr, "serveRpc: (fd %d) recvRpc error: %d (errno: %s)\n",
                    conn, r, strerror(errno));
            goto close;
        }

        printf("serveRpc: (fd %d) calling procedure %d\n", conn, procnum);

        uint32_t resplen = procs[procnum](buf, RPC_PROC_BUFLEN, argslen);
        if (sendRpc(conn, buf, resplen) == -1) {
            fprintf(stderr, "serveRpc: (fd %d) error sending response: %s\n",
                    conn, strerror(errno));
            goto close;
        }

close:
        printf("serveRpc: (fd %d) closed\n", conn);
        close(conn);
    }

    free(buf);
    close(sock);
}

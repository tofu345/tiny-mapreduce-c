#include "client.h"
#include "pack.h"
#include "utils.h"
#include "common.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int connectTo(struct addrinfo * ai)
{
    int sock;

    for (; ai != NULL; ai = ai->ai_next)
    {
        if ((sock = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol)) < 0) {
            continue;
        }

        if (connect(sock, ai->ai_addr, ai->ai_addrlen) == -1) {
            close(sock);
            continue;
        }

        break;
    }
    if (ai == NULL) return -1;

    return sock;
}

static int initRPC(int sock, RpcInt procnum, RpcInt argslen)
{
    ssize_t n;
    uint8_t buf[RPC_INIT_PACKET_LENGTH];

    strcpy((char *)buf, "RPC");
    pack(&buf[3], RPC_INIT_PACKET_LENGTH - 3, "ll", procnum, argslen);

    // send initial rpc packet
    if ((n = send(sock, buf, RPC_INIT_PACKET_LENGTH, 0)) == -1
            || n != RPC_INIT_PACKET_LENGTH)
        return -1;

    return 0;
}

int callRpc(int sock, RpcInt procnum, RpcInt argslen, uint8_t * buf,
            uint32_t buflen)
{
    size_t len;
    ssize_t n;

    // send initial packet
    if (initRPC(sock, procnum, argslen) == -1)
        return -1;

    if (argslen > 0) {
        // send arguments
        len = argslen;
        if (sendall(sock, buf, &len) == -1)
            return -1;
    }

    // recieve response length
    if ((n = recv(sock, buf, sizeof(RpcInt), 0)) == -1
            || n != sizeof(RpcInt))
        return -2;

    RpcInt resplen = 0;
    unpack(buf, buflen, "l", &resplen);
    if (resplen > buflen) return -3;

    // recieve response
    if (resplen > 0) {
        len = resplen;
        if (recvall(sock, buf, &len) == -1)
            return -2;
    }

    return 0;
}

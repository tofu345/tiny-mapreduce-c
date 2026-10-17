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

static int initRPC(int sock, uint32_t procnum, uint32_t argslen)
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

int callRpc(int sock, uint32_t procnum, void * args, uint8_t * buf, 
            uint32_t buflen, Packer * packer, Unpacker * unpacker,
            void * result)
{
    size_t len;
    ssize_t n;

    assert(result != NULL && "callRpc: result should not be NULL");

    // pack arguments
    uint32_t argslen = packer(buf, buflen, args);

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
    if ((n = recv(sock, buf, sizeof(uint32_t), 0)) == -1
            || n != sizeof(uint32_t))
        return -2;

    uint32_t resplen = 0;
    unpack(buf, buflen, "l", &resplen);

    // recieve response
    resplen = MIN(resplen, buflen);
    if (resplen > 0) {
        len = resplen;
        if (recvall(sock, buf, &len) == -1)
            return -2;
    }

    // unpack response
    if (unpacker(buf, resplen, result) != 0)
        return -3;

    return 0;
}

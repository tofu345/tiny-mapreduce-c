#ifndef INCLUDE_RPC_CLIENT_H
#define INCLUDE_RPC_CLIENT_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "common.h"

int connectTo(struct addrinfo * ai);

/*
 * Call Remote procedure on [sock] with [procnum] and pack()ed arguments in
 * [buf] of [argslen], the response is stored in [buf].
 *
 * TODO: return a length of returned data
 *
 * Return values:
 *   0 ok
 *  -1 failed to send RPC
 *  -2 failed to recieve response
 *  -3 response length greater than [buflen]
 */
int callRpc(int sock, RpcInt procnum, RpcInt argslen, uint8_t * buf,
            RpcInt buflen);

#endif // INCLUDE_RPC_CLIENT_H

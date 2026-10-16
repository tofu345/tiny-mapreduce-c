#ifndef INCLUDE_RPC_SERVER_H
#define INCLUDE_RPC_SERVER_H

#include <stdint.h>
#include <netdb.h>

#include "utils.h"
#include "common.h"

// The size of the buffer serveRpc() will use to recieve RPC arguments and send
// responses to clients.
#ifndef RPC_PROC_BUFLEN
#   define RPC_PROC_BUFLEN 1024
#endif

/*
 * An RPC procedure, it is responsible for unpacking its arguments from [buf]
 * of [argslen], storing its response back in [buf] and returning the length of
 * data to be sent.
 *
 * NOTE: the length of the buffer is RPC_PROC_BUFLEN, which should not be exceeded.
 */
typedef uint32_t Proc(uint8_t * buf, uint32_t argslen);

/*
 * Serve an RPC server on [ai] with [procs].
 */
void serveRpc(struct addrinfo * ai, Proc * procs[], uint32_t nprocs);

/*
 * Return values:
 *   0 : ok
 *  -1 : error reading rpc packet
 *  -2 : received packet of incorrect length (or connection closed prematurely)
 *  -3 : packet does not start with "RPC"
 *  -4 : invalid proc
 *  -5 : argument size greater than [argbuflen]
 *  -6 : error reading arguments
 */
int recvRpc(int sock, uint32_t * procnum, uint32_t * argslen,
            uint32_t maxprocs, uint8_t * argbuf, size_t argbuflen);
int sendRpc(int sock, uint8_t * resp, uint32_t resplen);

#endif // INCLUDE_RPC_SERVER_H

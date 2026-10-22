#ifndef INCLUDE_RPC_SERVER_H
#define INCLUDE_RPC_SERVER_H

#include <stdint.h>
#include <netdb.h>

#include "utils.h"
#include "common.h"

/*
 * An RPC procedure, it is responsible for unpacking its arguments from [buf]
 * of [argslen], storing its response back in [buf] and returning the length of
 * data to be sent.
 *
 * [arg] contains any arbitrary argument passed into serveRpc().
 *
 * NOTE: the length of the buffer is [buflen], which should not be exceeded.
 */
typedef uint32_t Proc(uint8_t * buf, uint32_t buflen, uint32_t argslen, 
                      void * arg);

/*
 * Serve an RPC server on [ai] with [procs].  
 *
 * [buf] will be used for passing arguments and storing responses
 *
 * an arbitrary argument [arg] can be provided which will be passed to
 * any [procs].
 */
void serveRpc(struct addrinfo * ai, Proc * procs[], uint32_t nprocs,
              uint8_t * buf, uint32_t buflen, void * arg);

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

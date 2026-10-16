#ifndef INCLUDE_RPC_CLIENT_H
#define INCLUDE_RPC_CLIENT_H

#include <stdint.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include "common.h"

int connectTo(struct addrinfo * ai);

/*
 * Pack [args] with [packer].  Call Remote procedure on [sock] with [procnum]
 * and packed [args] in [buf], the response is unpacked with [unpacker] and
 * stored in [result].
 *
 * Caveats:
 * - [buf] must be large enough to contain the packed arguments.
 * - [buf] must be large enough to contain the server response or it will be
 *   silently truncated.
 *
 * Return values:
 *   0 ok
 *  -1 failed to send RPC
 *  -2 failed to recieve response
 *  -3 unpacker did not return 0
 */
int callRpc(int sock, uint32_t procnum, void * args, uint8_t * buf, 
            uint32_t buflen, Packer * packer, Unpacker * unpacker,
            void * result);

#endif // INCLUDE_RPC_CLIENT_H

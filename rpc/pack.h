#ifndef INCLUDE_RPC_PACK_H
#define INCLUDE_RPC_PACK_H

#include <arpa/inet.h>
#include <stdint.h>

// https://stackoverflow.com/a/8981006
#define IS_LITTLE_ENDIAN (*(char *)(int[]){ 1 })

/*
** from https://beej.us/guide/bgnet/
**
** pack() -- store data dictated by the format string in [buf] of [buflen]
**
**   bits | unsigned  string
**   -----+-----------------
**      8 |    b
**     16 |    h
**     32 |    l
**     64 |    q
**      - |             s
**
**  (16-bit unsigned length is automatically prepended to strings)
*/
uint32_t pack(uint8_t * buf, uint32_t buflen, char * fmt, ...);

/*
** from https://beej.us/guide/bgnet/
**
** unpack() -- unpack data dictated by the format string into the
**             [buf] of [buflen]
**
**   bits | unsigned  string
**   -----+-----------------
**      8 |    b
**     16 |    h
**     32 |    l
**     64 |    q
**      - |             s
**
**  (string is extracted based on its stored length, but 's' can be
**  prepended with a maximum length)
*/
uint32_t unpack(uint8_t * buf, uint32_t buflen, char * fmt, ...);

uint32_t pack_cstring(uint8_t * buf, uint32_t len, void * data);
int unpack_cstring(uint8_t * response, uint32_t len, void * result);

#endif // INCLUDE_RPC_PACK_H

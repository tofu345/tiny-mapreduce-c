#ifndef INCLUDE_RPC_COMMON_H
#define INCLUDE_RPC_COMMON_H

// Initial Packet:
// "RPC" + procnum + arguments size in bytes
//
// - "RPC": 3 byte string
// - procnum, arguments size: 32 bit unsigned integers
#define RPC_INIT_PACKET_LENGTH (3 + (sizeof(uint32_t) * 2))

#define _Nullable
#define _NotNull

/*
 * The packer is responsible for pack()ing [data] and storing it in [buf].
 * Should return the length of data to send.
 */
typedef uint32_t Packer(uint8_t * buf, void * data);

/*
 * The unpacker is responsible for unpack()ing an RPC response from [buf] and
 * storing the data in [result].  Should return 0 on success.
 */
typedef int Unpacker(uint8_t * buf, uint32_t len, void * result);

_Static_assert ((sizeof(uint16_t) == 2), "what kind of computer is this?");
_Static_assert ((sizeof(size_t) > sizeof(uint32_t)), "32-bit? sorry");

#endif // INCLUDE_RPC_COMMON_H

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

_Static_assert ((sizeof(uint16_t) == 2), "what kind of computer is this?");
_Static_assert ((sizeof(size_t) > sizeof(uint32_t)),
                "This has only been tested on a 64-bit system");

#endif // INCLUDE_RPC_COMMON_H

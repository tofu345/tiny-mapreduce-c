#ifndef INCLUDE_RPC_COMMON_H
#define INCLUDE_RPC_COMMON_H

// NOTE: if you are changing this, make sure to update the pack()ing and
// unpack()ing code in {call,send,recv}Rpc.
typedef uint32_t RpcInt;

// Initial Packet: "RPC" + procnum + arguments size in bytes
#define RPC_INIT_PACKET_LENGTH (3 + (sizeof(RpcInt) * 2))

#define _Nullable
#define _NotNull

_Static_assert ((sizeof(uint16_t) == 2), "what kind of computer is this?");
_Static_assert ((sizeof(size_t) > sizeof(RpcInt)), "This has only been tested with a this assumption");

#endif // INCLUDE_RPC_COMMON_H

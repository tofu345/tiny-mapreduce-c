#ifndef INCLUDE_RPC_UTILS_H
#define INCLUDE_RPC_UTILS_H

#include <stddef.h>
#include <stdint.h>
#include <sys/param.h>

#define LENGTH(arr) (sizeof(arr) / sizeof(arr[0]))

int sendall(int s, uint8_t * buf, size_t * len);
int recvall(int s, uint8_t * buf, size_t * len);

#endif // INCLUDE_RPC_UTILS_H

#include "utils.h"

#include <sys/types.h>
#include <sys/socket.h>

int sendall(int s, uint8_t * buf, size_t * len)
{
    size_t total = 0;        // how many bytes we've sent
    size_t bytesleft = *len; // how many we have left to send
    ssize_t n;

    while (total < *len) {
        n = send(s, buf + total, bytesleft, 0);
        if (n <= 0) break;
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually sent here
    return n <= 0 ? -1 : 0; // return -1 on failure, 0 on success
}

int recvall(int s, uint8_t * buf, size_t * len)
{
    size_t total = 0;        // how many bytes we've recieved
    size_t bytesleft = *len; // how many we still expect
    ssize_t n;

    while (total < *len) {
        n = recv(s, buf + total, bytesleft, 0);
        if (n <= 0) break;
        total += n;
        bytesleft -= n;
    }

    *len = total; // return number actually recieved here
    return n <= 0 ? -1 : 0; // return -1 on failure, 0 on success
}

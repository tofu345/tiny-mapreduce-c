#include "utils.h"

#include "pack.h"

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

uint32_t echo(uint8_t * buf, uint32_t buflen, uint32_t argslen)
{
    (void)buf; (void)buflen;
    return argslen;
}

uint32_t reverse(uint8_t * buf, uint32_t buflen, uint32_t argslen)
{
    (void)argslen;

    uint16_t strLen = 0;
    unpack(buf, argslen, "h", &strLen);
    if (strLen == 0) return 0;

    char s[strLen + 1];
    unpack(buf, argslen, "s", s);

    uint32_t i = 0, j = strLen - 1;
    for (; i < j; i++, j--) {
        char tmp = s[i];
        s[i] = s[j];
        s[j] = tmp;
    }

    return pack(buf, buflen, "s", s);
}

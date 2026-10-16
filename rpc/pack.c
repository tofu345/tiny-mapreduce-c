#include "pack.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

void pack_u16(uint8_t * buf, uint16_t n)
{
    *(uint16_t *)buf = htons(n);
}

uint16_t unpack_u16(uint8_t * buf)
{
    return ntohs(*(uint16_t *)buf);
}

void pack_u32(uint8_t * buf, uint32_t n)
{
    *(uint32_t *)buf = htonl(n);
}

uint32_t unpack_u32(uint8_t * buf)
{
    return ntohl(*(uint32_t *)buf);
}

void pack_u64(uint8_t * buf, uint64_t n)
{
    if (IS_LITTLE_ENDIAN) {
        *buf++ = n >> 56; *buf++ = n >> 48;
        *buf++ = n >> 40; *buf++ = n >> 32;
        *buf++ = n >> 24; *buf++ = n >> 16;
        *buf++ = n >> 8;  *buf++ = n;
    } else {
        *(uint64_t *)buf = n;
    }
}

uint64_t unpack_u64(uint8_t * buf)
{
    if (IS_LITTLE_ENDIAN) {
        return ((uint64_t)buf[0] << 56) | ((uint64_t)buf[1] << 48)
             | ((uint64_t)buf[2] << 40) | ((uint64_t)buf[3] << 32)
             | ((uint64_t)buf[4] << 24) | ((uint64_t)buf[5] << 16)
             | ((uint64_t)buf[6] << 8)  | buf[7];
    } else {
        return *(uint64_t *)buf;
    }
}

uint32_t pack(uint8_t * buf, char * fmt, ...)
{
    va_list ap;

    uint8_t  b;
    uint16_t h;
    uint32_t l;
    uint64_t q;

    const char * s;
    uint16_t len;

    uint32_t size = 0;
    va_start(ap, fmt);

    for (; *fmt != 0; fmt++) {
        switch (*fmt)
        {
            case 'b': // 8-bit
                size += 1;
                b = (uint8_t)va_arg(ap, unsigned int); // promoted
                *buf++ = b;
                break;

            case 'h': // 16-bit
                size += 2;
                h = (uint16_t)va_arg(ap, unsigned int); // promoted
                pack_u16(buf, h);
                buf += 2;
                break;

            case 'l': // 32-bit
                size += 4;
                l = (uint32_t)va_arg(ap, uint32_t);
                pack_u32(buf, l);
                buf += 4;
                break;

            case 'q': // 64-bit
                size += 8;
                q = (uint64_t)va_arg(ap, uint64_t);
                pack_u64(buf, q);
                buf += 8;
                break;

            case 's': // string
                s = va_arg(ap, char *);
                len = strlen(s);
                size += len + 2;
                pack_u16(buf, len);
                buf += 2;
                memcpy(buf, s, len);
                buf += len;
                break;

            default:
                fprintf(stderr, "pack: unknown identifier '%c'\n", *fmt);
                exit(1);
        }
    }

    va_end(ap);
    return size;
}

void unpack(uint8_t * buf, char * fmt, ...)
{
    va_list ap;

    uint8_t  * b;
    uint16_t * h;
    uint32_t * l;
    uint64_t * q;

    char * s;
    uint16_t len, maxstrlen = 0;

    va_start(ap, fmt);

    for (; *fmt != '\0'; fmt++) {
        switch (*fmt)
        {
            case 'b': // 8-bit
                b = va_arg(ap, uint8_t *);
                *b = *buf++;
                break;

            case 'h': // 16-bit
                h = va_arg(ap, uint16_t *);
                *h = unpack_u16(buf);
                buf += 2;
                break;

            case 'l': // 32-bit
                l = va_arg(ap, uint32_t *);
                *l = unpack_u32(buf);
                buf += 4;
                break;

            case 'q': // 64-bit
                q = va_arg(ap, uint64_t *);
                *q = unpack_u64(buf);
                buf += 8;
                break;

            case 's': // string
                s = va_arg(ap, char *);
                len = unpack_u16(buf);
                buf += 2;
                if (maxstrlen > 0 && len > maxstrlen) {
                    len = maxstrlen - 1;
                    maxstrlen = 0; // reset max str len
                }
                memcpy(s, buf, len);
                s[len] = '\0';
                buf += len;
                break;

            default:
                if (isdigit(*fmt)) { // track max str len
                    maxstrlen = maxstrlen * 10 + (*fmt - '0');
                }
        }
    }
}

uint32_t pack_cstring(uint8_t * buf, void * data)
{
    return pack(buf, "s", data);
}

int unpack_cstring(uint8_t * response, uint32_t len, void * result)
{
    char fmt[16];
    snprintf(fmt, sizeof fmt, "%d" "s", len);
    unpack(response, fmt, result);
    return 0;
}

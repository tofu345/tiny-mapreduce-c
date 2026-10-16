/*
 * Basic tests for pack() and unpack()
 */

#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include "pack.h"

#ifndef DEBUG
#   define DEBUG 1
#endif

static void debugprint(char * fmt, ...)
{
#if DEBUG
    va_list ap;
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);
#else
    (void)fmt;
#endif
}

int main(void)
{
    uint8_t buf[1024];

    {
        uint8_t n = 0xa, n1;
        pack(buf, "h", n),
        unpack(buf, "h", &n1);
        if (n != n1) {
            debugprint("uint8_t  bytes: %zu\n", sizeof(uint8_t));
            fprintf(stderr, "uint8_t: %#" PRIx8 " != %#" PRIx8 "\n", n, n1);
        }
    }

    {
        uint16_t n = 0xbeef, n1;
        pack(buf, "h", n),
        unpack(buf, "h", &n1);
        if (n != n1) {
            debugprint("uint16_t bytes: %zu\n", sizeof(uint16_t));
            fprintf(stderr, "uint16_t: %#" PRIx16 " != %#" PRIx16 "\n", n, n1);
        }
    }

    {
        uint32_t n = 0xdeadbeef, n1;
        pack(buf, "l", n),
        unpack(buf, "l", &n1);
        if (n != n1) {
            debugprint("uint32_t bytes: %zu\n", sizeof(uint32_t));
            fprintf(stderr, "uint32_t: %#" PRIx32 " != %#" PRIx32 "\n", n, n1);
        }
    }

    {
        uint64_t n = 0x12345678deadbeef, n1;
        pack(buf, "q", n),
        unpack(buf, "q", &n1);
        if (n != n1) {
            debugprint("uint64_t bytes: %zu\n", sizeof(uint64_t));
            fprintf(stderr, "uint64_t: %#" PRIx64 " != %#" PRIx64 "\n", n, n1);
        }
    }

    {
        char * s = "something, something, something";
        char s1[32] = {};
        char s2[32] = {};
        pack(buf, "s", s);
        unpack(buf, "32s", s1);
        unpack(buf, "s", s2);

        if (strncmp(s, s1, sizeof s1) != 0) {
            fprintf(stderr, "string: '%s' != '%.*s'\n", s, (int)(sizeof s1), s1);
        }
        if (strncmp(s, s2, sizeof s2) != 0) {
            fprintf(stderr, "string: '%s' != '%.*s'\n", s, (int)(sizeof s2), s2);
        }
    }

    return 0;
}

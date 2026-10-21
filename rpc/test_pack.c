/*
 * Basic tests for pack() and unpack()
 */

#include <assert.h>
#include <inttypes.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "pack.h"

typedef struct {
    uint32_t a, b;
    uint64_t c;
    char d[36];
} example;

int main(void)
{
    uint8_t buf[1024];

    printf("uint8_t  bytes: %zu\n", sizeof(uint8_t));
    {
        uint8_t n = 0xa, n1;
        pack(buf, sizeof buf, "h", n),
        unpack(buf, sizeof buf, "h", &n1);
        if (n != n1) {
            fprintf(stderr, "uint8_t: %#" PRIx8 " != %#" PRIx8 "\n", n, n1);
        }
    }

    printf("uint16_t bytes: %zu\n", sizeof(uint16_t));
    {
        uint16_t n = 0xbeef, n1;
        pack(buf, sizeof buf, "h", n),
        unpack(buf, sizeof buf, "h", &n1);
        if (n != n1) {
            fprintf(stderr, "uint16_t: %#" PRIx16 " != %#" PRIx16 "\n", n, n1);
        }
    }

    printf("uint32_t bytes: %zu\n", sizeof(uint32_t));
    {
        uint32_t n = 0xdeadbeef, n1;
        pack(buf, sizeof buf, "l", n),
        unpack(buf, sizeof buf, "l", &n1);
        if (n != n1) {
            fprintf(stderr, "uint32_t: %#" PRIx32 " != %#" PRIx32 "\n", n, n1);
        }
    }

    printf("uint64_t bytes: %zu\n", sizeof(uint64_t));
    {
        uint64_t n = 0x12345678deadbeef, n1;
        pack(buf, sizeof buf, "q", n),
        unpack(buf, sizeof buf, "q", &n1);
        if (n != n1) {
            fprintf(stderr, "uint64_t: %#" PRIx64 " != %#" PRIx64 "\n", n, n1);
        }
    }

    {
        char * s = "something, something, something";
        char s1[32] = {};
        char s2[32] = {};
        pack(buf, sizeof buf, "s", s);
        unpack(buf, sizeof buf, "32s", s1);
        unpack(buf, sizeof buf, "s", s2);

        if (strncmp(s, s1, sizeof s1) != 0) {
            fprintf(stderr, "string: '%s' != '%.*s'\n", s, (int)(sizeof s1), s1);
        }
        if (strncmp(s, s2, sizeof s2) != 0) {
            fprintf(stderr, "string: '%s' != '%.*s'\n", s, (int)(sizeof s2), s2);
        }
    }

    printf("example  bytes: %zu\n", sizeof(example));
    {
        example e1 = { -1, 8978879, UINT_MAX, "abcdefghijklmnopqrstuvwxyz012456789" };
        pack(buf, sizeof buf, "llqs", e1.a, e1.b, e1.c, e1.d);
        example e2;
        unpack(buf, sizeof buf, "llqs", &e2.a, &e2.b, &e2.c, &e2.d);

        if (e1.a != e2.a || e1.b != e2.b || e1.c != e2.c || strcmp(e1.d, e2.d) != 0) {
            fprintf(stderr, "example: %#" PRIx32 ", %#" PRIx32 ", %#" PRIx64 ", '%s'" "\n!=\n"
                            "example: %#" PRIx32 ", %#" PRIx32 ", %#" PRIx64 ", '%s'\n",
                            e1.a, e1.b, e1.c, e1.d, e2.a, e2.b, e2.c, e2.d);
        }
    }

    return 0;
}

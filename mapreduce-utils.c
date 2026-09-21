#include "mapreduce-utils.h"

#include <stdio.h>
#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

Result newResult(const char * key, uint64_t keyLen, const char * val, uint64_t valLen)
{
    char * owned_key = strndup(key, keyLen);
    if (owned_key == NULL) die("could not allocate result key");

    char * owned_val = strndup(val, valLen);
    if (owned_val == NULL) die("could not allocate result val");

    return (Result){ owned_key, owned_val };
}

void freeResult(Result * r)
{
    free((char *)r->key);
    free((char *)r->val);
    r->key = NULL;
    r->val = NULL;
}

int resultLessThan(const void * a, const void * b)
{
    const Result * _a = a;
    const Result * _b = b;
    return strcmp(_a->key, _b->key);
}

void addResult(Results * buf, Result elem)
{
    if (buf->len + 1 > buf->cap)
    {
        buf->cap = buf->cap == 0 ? 4 : buf->cap * 2;
        buf->data = realloc(buf->data, buf->cap * sizeof(Result));
        if (buf->data == NULL) die("appendResult");
    }
    buf->data[buf->len++] = elem;
}

void freeResults(Results * buf)
{
    for (uint32_t i = 0; i < buf->len; i++)
        freeResult(&buf->data[i]);
    free(buf->data);
    *buf = (Results){0};
}

void die(const char * fmt, ...)
{
    va_list ap;
    int saved_errno;

    saved_errno = errno;

    // using stderr gives warning:
    // relocation against `stderr@@GLIBC_2.2.5' in read-only section `.text'
    va_start(ap, fmt);
    vprintf(fmt, ap);
    va_end(ap);

    if (fmt[0] && fmt[strlen(fmt) - 1] == ':')
        printf(" %s", strerror(saved_errno));
    putchar('\n');

    abort();
}

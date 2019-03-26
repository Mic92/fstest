#define _XOPEN_SOURCE 700

#include <stdarg.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <errno.h>
#include <assert.h>
#include <stdlib.h>

#include "fstest_server.h"

int fake_printf(const char* format, ...) {
    va_list argptr;
    va_start(argptr, format);
    size_t size = vsnprintf(NULL, 0, format, argptr) + 1;
    va_end(argptr);

    char *buf = (char*) malloc(size);
    if (!buf) {
        fputs("cannot allocate buf for send_context\n", stderr);
        return -ENOMEM;
    }

    if (send_context.pos >= (sizeof(send_context.sendbuf) / sizeof(struct iovec))) {
        fputs("sendbuf is full\n", stderr);
        return -ENOMEM;
    }

    va_list argptr2;
    va_start(argptr2, format);
    vsnprintf(buf, size, format, argptr2);
    va_end(argptr2);

    send_context.sendbuf[send_context.pos].iov_base = buf;
    send_context.sendbuf[send_context.pos].iov_len = size;
    send_context.pos++;

    return size;
}
void fake_exit(int c) {
    request_cleanup();
    pthread_exit(NULL);
}

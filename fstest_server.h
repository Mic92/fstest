#pragma once
#include <sys/uio.h>

struct send_context {
    int client_fd;
    char** argv;
    struct iovec sendbuf[256];
    int pos;
};

extern struct send_context send_context;
void request_cleanup();

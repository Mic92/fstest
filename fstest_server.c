#define _POSIX_C_SOURCE 200809L

#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/uio.h>
#include <string.h>

#include "fstest_server.h"

#define CBSIZE 0x2000

struct cbuf {
    char buf[CBSIZE];
    int fd;
    unsigned int rpos, wpos;
};

//#define DEBUG 1

#ifdef DEBUG
#define LOG(...) fprintf(stderr, __VA_ARGS__)
#else
#define LOG(x, ...) do {} while (0)
#endif


#define EOF_CHAR 4

int read_msg(struct cbuf *cbuf, char *dst, unsigned int size) {
    unsigned int i = 0;
    ssize_t n;
    while (i < size) {
        if (cbuf->rpos == cbuf->wpos) {
            size_t wpos = cbuf->wpos % CBSIZE;
            if((n = recv(cbuf->fd, cbuf->buf + wpos, (CBSIZE - wpos), 0)) < 0) {
                if (errno == EINTR) {
                    continue;
                }
                return -1;
            } else if (n == 0) {
                return 0;
            }
            cbuf->wpos += n;
        }
        dst[i++] = cbuf->buf[cbuf->rpos++ % CBSIZE];
        if (dst[i - 1] == EOF_CHAR) {
            dst[i - 1] = '\0';
            break;
        }
    }
    if(i == size) {
         fprintf(stderr, "line too large: %d %d\n", i, size);
         return -1;
    }

    dst[i] = '\0';
    return i;
}

int real_main(int argc, char** argv);

void request_cleanup() {
#ifdef DEBUG
    writev(fileno(stderr), send_context.sendbuf, send_context.pos);
#endif
    int rc = writev(send_context.client_fd, send_context.sendbuf, send_context.pos);
    if (rc < 0) {
        fprintf(stderr, "failed to send response: %s", strerror(errno));
    }

    for (int i = 0; i < send_context.pos; i++) {
        free(send_context.sendbuf[i].iov_base);
    }

    free(send_context.argv);
}

struct send_context send_context;
void *handle_request(void* arg) {
    int client_fd = *(int*)arg;
    struct cbuf buf;
    memset(&buf, 0, sizeof(struct cbuf));
    char request_buf[CBSIZE];
    memset(&request_buf, 0, sizeof(request_buf));
    buf.fd = client_fd;
    int n = read_msg(&buf, request_buf, sizeof(request_buf));
    if (n < 0) {
        return NULL;
    }
    char *cwd = request_buf;
    LOG("chdir(%s)\n", cwd);
    if (chdir(cwd) == -1) {
        perror("chdir");
        return NULL;
    };
    char *args;
    for (int i = 0; i < n; i++) {
        if (request_buf[i] == '\0') {
            args = &request_buf[i + 1];
            n -= i + 1;
            break;
        }
    }

    int argc = 0;
    for (int i = 0; i < n; i++) {
        if (args[i] == '\0') {
            argc++;
        };
    }

    memset(&send_context, 0, sizeof(struct send_context));
    send_context.argv = (char**)calloc(argc + 1, sizeof(char*));
    fprintf(stderr, "%s() at %s:%d: %p %p\n", __func__, __FILE__, __LINE__, &send_context.argv, send_context.argv);
    if (!send_context.argv) {
        perror("calloc");
        return NULL;
    }
    send_context.client_fd = client_fd;
    char** argvp = send_context.argv;
    *argvp = args;
    LOG("'%s' ", *argvp);
    argvp++;

    for (int i = 1; i < n; i++) {
        if (args[i - 1] == '\0') {
            *argvp = &args[i];
            LOG("'%s' ", *argvp);
            argvp++;
        }
    }
    LOG("\n");

    optind = 1;
    real_main(argc, send_context.argv);
    request_cleanup();
    return NULL;
}

// Driver function
int main(int argc, char** argv) {
    int sockfd, connfd;
    socklen_t len;

    struct sockaddr_in servaddr = {}, cli = {};

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket creation failed");
        exit(1);
    }

    int yes = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
      perror("setsockopt");
      exit(1);
    }

    // assign IP, PORT
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(6000);

    // Binding newly created socket to given IP and verification
    if ((bind(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))) != 0) {
        perror("socket bind failed");
        return 1;
    }

    // Now server is ready to listen and verification
    if ((listen(sockfd, 5)) != 0) {
        perror("listen failed");
        return 1;
    }
    len = sizeof(cli);

    for (;;) {
        // Accept the data packet from client and verification
        connfd = accept(sockfd, (struct sockaddr*)&cli, &len);
        if (connfd < 0) {
            perror("server acccept failed");
            return 1;
        }

        pthread_t thread;
        int rc = pthread_create(&thread, NULL, &handle_request, &connfd);
        if (rc > 0) {
            perror("pthread_create");
            return 1;
        }
        pthread_join(thread, NULL);

        // After chatting close the socket
        close(connfd);
    }
}

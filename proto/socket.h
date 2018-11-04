#ifndef _PROTO_SOCKET_H_
#define _PROTO_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>

#include "pub/type.h"
#include "pub/time.h"

#define SOCKET_NON_BLOCK_INTERVAL 10

typedef int socket_t;

// read/write with retry on EINTR or EAGAIN
INLINE ssize_t
read_r(int fd, byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = read(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

INLINE ssize_t
write_r(int fd, const byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = write(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

/** Returns true on success, or false if there was an error */
INLINE int
socket_set_block(int fd, bool blocking)
{
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return -1;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return fcntl(fd, F_SETFL, flags);
}

INLINE int
socket_set_timeout(int fd, int sec)
{
    struct timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = 0;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

INLINE int
socket_set_reuse_port(int fd)
{
    int optval = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

INLINE void
pgai_error(int err)
{
    fprintf(stderr, "%s\n", gai_strerror(err));
}

INLINE int
getaddrinfo_r(const char *node, const char *service,
              const struct addrinfo *hints, struct addrinfo **res)
{
    int err = getaddrinfo(node, service, hints, res);

    if (err) {
        pgai_error(err);
        return err;
    }

    return 0;
}

#endif

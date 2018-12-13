#ifndef _PUB_FD_H_
#define _PUB_FD_H_

// an abstraction for file descriptors

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

#include "type.h"

typedef int fd_t;

// read/write with retry on EINTR or EAGAIN
INLINE ssize_t
fd_read(fd_t fd, byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = read(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
    return ret;
}

// non-blocking read
// NOT THREAD-SAFE
// return -2 for EAGAIN
INLINE ssize_t
fd_try_read(fd_t fd, byte_t *buf, size_t size)
{
    int flags = fcntl(fd, F_GETFL, 0);
    ssize_t ret;

    fcntl(fd, F_SETFL, flags | O_NONBLOCK);

    while ((ret = read(fd, buf, size)) == -1 && errno == EINTR);

    if (ret == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        ret = -2;
    }

    fcntl(fd, F_SETFL, flags);

    return ret;
}

INLINE ssize_t
fd_write(fd_t fd, const byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = write(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN || errno == EWOULDBLOCK));
    return ret;
}

#endif

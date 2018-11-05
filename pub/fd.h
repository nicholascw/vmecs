#ifndef _PUB_FD_H_
#define _PUB_FD_H_

// an abstraction for file descriptors

#include <errno.h>

typedef int fd_t;

// read/write with retry on EINTR or EAGAIN
INLINE ssize_t
fd_read(fd_t fd, byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = read(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

INLINE ssize_t
fd_write(fd_t fd, const byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = write(fd, buf, size)) == -1 && (errno == EINTR || errno == EAGAIN));
    return ret;
}

#endif

#ifndef _PROTO_SOCKET_H_
#define _PROTO_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>

#include "pub/type.h"
#include "pub/time.h"

#define SOCKET_NON_BLOCK_INTERVAL 10

typedef int socket_t;

// read/write with retry on EINTR or EAGAIN
INLINE ssize_t read_r(int fd, byte_t *buf, size_t size)
{
    ssize_t ret;
    
    while ((ret = read(fd, buf, size)) == -1) {
        if (errno == EINTR) perror("read");
        else if (errno == EAGAIN) continue;
        else break;
    }

    return ret;
}

INLINE ssize_t write_r(int fd, const byte_t *buf, size_t size)
{
    ssize_t ret;
    while ((ret = write(fd, buf, size)) == -1 && errno == EINTR)
        perror("write");
    return ret;
}

/** Returns true on success, or false if there was an error */
INLINE int socket_set_block(int fd, bool blocking)
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

#endif

#ifndef _PUB_SOCKET_H_
#define _PUB_SOCKET_H_

// an abstraction layer for socket

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
#include "pub/fd.h"

INLINE int
socket_set_block(fd_t fd, bool blocking)
{
   int flags = fcntl(fd, F_GETFL, 0);
   if (flags == -1) return -1;
   flags = blocking ? (flags & ~O_NONBLOCK) : (flags | O_NONBLOCK);
   return fcntl(fd, F_SETFL, flags);
}

INLINE int
socket_set_timeout(fd_t fd, int sec)
{
    struct timespec tv;
    tv.tv_sec = sec;
    tv.tv_nsec = 0;
    return setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

INLINE int
socket_set_reuse_port(fd_t fd)
{
    int optval = 1;
    return setsockopt(fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
}

typedef struct addrinfo socket_addrinfo_t;
typedef struct {
    struct sockaddr_storage addr;
    socklen_t len;
} socket_sockaddr_t;

#define socket_freeaddrinfo freeaddrinfo

// port in host endianness
INLINE void
socket_sockaddr_ipv4(const uint8_t ip[4], uint16_t port, socket_sockaddr_t *addr)
{
    struct sockaddr_in sin;

    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    
    memcpy(&sin.sin_addr, ip, 4);

    addr->len = sizeof(sin);
    memcpy(&addr->addr, &sin, sizeof(sin));
}

INLINE void
socket_sockaddr_ipv6(const uint8_t ip[16], uint16_t port, socket_sockaddr_t *addr)
{
    struct sockaddr_in6 sin;

    memset(&sin, 0, sizeof(sin));

    sin.sin6_family = AF_INET;
    sin.sin6_port = htons(port);
    
    memcpy(&sin.sin6_addr, ip, 16);

    addr->len = sizeof(sin);
    memcpy(&addr->addr, &sin, sizeof(sin));
}

INLINE int
socket_getaddrinfo(const char *node, const char *service,
                   const socket_addrinfo_t *hints, socket_addrinfo_t **res)
{
    int err = getaddrinfo(node, service, hints, res);

    if (err) {
        fprintf(stderr, "%s\n", gai_strerror(err));
        return err;
    }

    return 0;
}

INLINE int
socket_getsockaddr(const char *node, const char *service, socket_sockaddr_t *addr)
{
    socket_addrinfo_t *res;
    int err;

    if ((err = socket_getaddrinfo(node, service, NULL, &res))) {
        socket_freeaddrinfo(res);
        return err;
    }

    memcpy(&addr->addr, res->ai_addr, res->ai_addrlen);
    addr->len = res->ai_addrlen;

    socket_freeaddrinfo(res);

    return 0;
}

INLINE fd_t
socket_stream(int af)
{
    fd_t sock = socket(af, SOCK_STREAM, 0);

    if (sock == -1) {
        perror("socket");
        ASSERT(0, "failed to create socket");
    }

    return sock;
}

INLINE int
socket_bind(fd_t sock, socket_sockaddr_t *addr)
{
    int ret;

    if ((ret = bind(sock, (struct sockaddr *)&addr->addr, addr->len))) {
        perror("bind");
    }

    return ret;
}

INLINE int
socket_connect(fd_t sock, socket_sockaddr_t *addr)
{
    int ret;

    if ((ret = connect(sock, (struct sockaddr *)&addr->addr, addr->len))) {
        perror("connect");
    }

    return ret;
}

INLINE int
socket_listen(fd_t sock, int backlog)
{
    int ret;

    if ((ret = listen(sock, backlog))) {
        perror("listen");
    }

    return ret;
}

INLINE fd_t
socket_accept(fd_t sock, socket_sockaddr_t *addr)
{
#define DO_ACCEPT accept(sock, (struct sockaddr *)(addr ? &addr->addr : NULL), \
                         (socklen_t *)(addr ? &addr->len : NULL))

    fd_t ret;

    while ((ret = DO_ACCEPT) == -1) {
        if (errno == EAGAIN) continue;
        else {
            perror("accept");
            break;
        }
    }

#undef DO_ACCEPT

    return ret;
}

#endif

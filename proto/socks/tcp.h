#ifndef _PROTO_SOCKS_TCP_H_
#define _PROTO_SOCKS_TCP_H_

#include "pub/fd.h"

#include "proto/tcp.h"
#include "proto/common.h"

typedef struct {
    TCP_SOCKET_HEADER
    fd_t sock;

    union {
        target_id_t *target;
        target_id_t *proxy;
    } addr;

    int socks_vers;
} socks_tcp_socket_t;

enum {
    SOCKS_VERSION_4,
    SOCKS_VERSION_5,
    SOCKS_VERSION_ANY
};

INLINE void
socks_tcp_socket_set_proxy(socks_tcp_socket_t *sock,
                           target_id_t *proxy)
{
    target_id_free(sock->addr.proxy);
    sock->addr.proxy = target_id_copy(proxy);
}

INLINE void
socks_tcp_socket_set_target(socks_tcp_socket_t *sock,
                            target_id_t *target)
{
    target_id_free(sock->addr.target);
    sock->addr.target = target_id_copy(target);
}

socks_tcp_socket_t *
socks_tcp_socket_new(int socks_vers);

socks_tcp_socket_t *
socks_tcp_socket_new_fd(int socks_vers, fd_t fd);

// generate a normal socket and free the original socket
fd_t
socks_to_socket(socks_tcp_socket_t *sock);

#endif

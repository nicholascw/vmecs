#ifndef _PROTO_SOCKS5_TCP_H_
#define _PROTO_SOCKS5_TCP_H_

#include "proto/tcp.h"
#include "proto/common.h"

typedef struct {
    TCP_SOCKET_HEADER
    int sock;

    union {
        target_id_t *target;
        target_id_t *proxy;
    } addr;
} socks5_tcp_socket_t;

INLINE void
socks5_tcp_socket_set_proxy(socks5_tcp_socket_t *sock,
                            target_id_t *proxy)
{
    target_id_free(sock->addr.proxy);
    sock->addr.proxy = target_id_copy(proxy);
}

INLINE void
socks5_tcp_socket_set_target(socks5_tcp_socket_t *sock,
                             target_id_t *target)
{
    target_id_free(sock->addr.target);
    sock->addr.target = target_id_copy(target);
}

socks5_tcp_socket_t *
socks5_tcp_socket_new();

socks5_tcp_socket_t *
socks5_tcp_socket_new_fd(int fd);

// generate a normal socket and free the original socket
int
socks5_to_socket(socks5_tcp_socket_t *sock);

/*

hook protocol

overload connect and getaddrinfo

#define AF_DOMAIN (AF_MAX + 1)

struct sockaddr_domain {
    short family = AF_MAX + 1
    short len
    char *domain
}

when getaddrinfo(node, service, hint, res)
    if node is a domain &&
       hints.family = AF_INET &&
       hints.type = SOCK_STREAM // only redirect tcp traffic
        res = sockaddr_domain {
            len = strlen(node)
            domain = strdup(node)
        }
    else
        use original getaddrinfo

when connect(sock, sockaddr *addr, addrlen)
    sock0, sock1 <- socketpair
    dup2(sock0, sock)

start a hook process
socketpair -> sock0, sock1

*/

#endif

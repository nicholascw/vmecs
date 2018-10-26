#ifndef _PROTO_TCP_H_
#define _PROTO_TCP_H_

#include "pub/type.h"

#define TCP_SOCKET_HEADER \
    tcp_socket_read_t read_func; \
    tcp_socket_write_t write_func; \
    tcp_socket_bind_t bind_func; \
    tcp_socket_listen_t listen_func; \
    tcp_socket_accept_t accept_func; \
    tcp_socket_connect_t connect_func; \
    tcp_socket_close_t close_func;

struct tcp_socket_t_tag;

typedef ssize_t (*tcp_socket_read_t)(struct tcp_socket_t_tag *sock, byte_t *buf, size_t size);
typedef ssize_t (*tcp_socket_write_t)(struct tcp_socket_t_tag *sock, const byte_t *buf, size_t size);

typedef int (*tcp_socket_bind_t)(struct tcp_socket_t_tag *sock, const char *node, const char *port);
typedef int (*tcp_socket_listen_t)(struct tcp_socket_t_tag *sock, int backlog);
typedef struct tcp_socket_t_tag *(*tcp_socket_accept_t)(struct tcp_socket_t_tag *sock);

typedef int (*tcp_socket_connect_t)(struct tcp_socket_t_tag *sock, const char *node, const char *port);

typedef int (*tcp_socket_close_t)(struct tcp_socket_t_tag *sock);

// an abstract layer for tcp connection
typedef struct tcp_socket_t_tag {
    TCP_SOCKET_HEADER
} tcp_socket_t;

// wrappers

INLINE ssize_t
tcp_socket_read(tcp_socket_t *sock, byte_t *buf, size_t size)
{
    return sock->read_func(sock, buf, size);
}

INLINE ssize_t
tcp_socket_write(tcp_socket_t *sock, const byte_t *buf, size_t size)
{
    return sock->write_func(sock, buf, size);
}

INLINE int
tcp_socket_bind(tcp_socket_t *sock, const char *node, const char *port)
{
    return sock->bind_func(sock, node, port);
}

INLINE int
tcp_socket_listen(tcp_socket_t *sock, int backlog)
{
    return sock->listen_func(sock, backlog);
}

INLINE tcp_socket_t *
tcp_socket_accept(tcp_socket_t *sock)
{
    return sock->accept_func(sock);
}

INLINE int
tcp_socket_connect(tcp_socket_t *sock, const char *node, const char *port)
{
    return sock->connect_func(sock, node, port);
}

INLINE int
tcp_socket_close(tcp_socket_t *sock)
{
    return sock->close_func(sock);
}

#endif

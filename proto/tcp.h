#ifndef _PROTO_TCP_H_
#define _PROTO_TCP_H_

#include "pub/type.h"
#include "proto/common.h"

#define TCP_SOCKET_HEADER \
    tcp_socket_read_t read_func; \
    tcp_socket_try_read_t try_read_func; \
    tcp_socket_write_t write_func; \
    tcp_socket_bind_t bind_func; \
    tcp_socket_listen_t listen_func; \
    tcp_socket_accept_t accept_func; \
    tcp_socket_handshake_t handshake_func; \
    tcp_socket_connect_t connect_func; \
    tcp_socket_revent_t revent_func; \
    tcp_socket_close_t close_func; \
    tcp_socket_free_t free_func; \
    tcp_socket_target_t target_func; /* optional*/

struct tcp_socket_t_tag;

typedef ssize_t (*tcp_socket_read_t)(struct tcp_socket_t_tag *sock, byte_t *buf, size_t size);
typedef ssize_t (*tcp_socket_try_read_t)(struct tcp_socket_t_tag *sock, byte_t *buf, size_t size);
typedef ssize_t (*tcp_socket_write_t)(struct tcp_socket_t_tag *sock, const byte_t *buf, size_t size);

typedef int (*tcp_socket_bind_t)(struct tcp_socket_t_tag *sock, const char *node, const char *port);
typedef int (*tcp_socket_listen_t)(struct tcp_socket_t_tag *sock, int backlog);
typedef struct tcp_socket_t_tag *(*tcp_socket_accept_t)(struct tcp_socket_t_tag *sock);
typedef int (*tcp_socket_handshake_t)(struct tcp_socket_t_tag *sock);

typedef int (*tcp_socket_connect_t)(struct tcp_socket_t_tag *sock, const char *node, const char *port);

typedef fd_t (*tcp_socket_revent_t)(struct tcp_socket_t_tag *sock);

typedef target_id_t *(*tcp_socket_target_t)(struct tcp_socket_t_tag *sock);

typedef int (*tcp_socket_close_t)(struct tcp_socket_t_tag *sock);
typedef void (*tcp_socket_free_t)(struct tcp_socket_t_tag *sock);

// an abstract layer for tcp connection
typedef struct tcp_socket_t_tag {
    TCP_SOCKET_HEADER
} tcp_socket_t;

// wrappers

INLINE ssize_t
tcp_socket_read(void *sock, byte_t *buf, size_t size)
{
    return ((tcp_socket_t *)sock)->read_func(sock, buf, size);
}

// non-blocking read
INLINE ssize_t
tcp_socket_try_read(void *sock, byte_t *buf, size_t size)
{
    return ((tcp_socket_t *)sock)->try_read_func(sock, buf, size);
}

INLINE ssize_t
tcp_socket_write(void *sock, const byte_t *buf, size_t size)
{
    return ((tcp_socket_t *)sock)->write_func(sock, buf, size);
}

INLINE int
tcp_socket_bind(void *sock, const char *node, const char *port)
{
    return ((tcp_socket_t *)sock)->bind_func(sock, node, port);
}

INLINE int
tcp_socket_listen(void *sock, int backlog)
{
    return ((tcp_socket_t *)sock)->listen_func(sock, backlog);
}

INLINE void *
tcp_socket_accept(void *sock)
{
    return ((tcp_socket_t *)sock)->accept_func(sock);
}

// original accept is splitted into accept and handshake
INLINE int
tcp_socket_handshake(void *sock)
{
    return ((tcp_socket_t *)sock)->handshake_func(sock);
}

INLINE int
tcp_socket_connect(void *sock, const char *node, const char *port)
{
    return ((tcp_socket_t *)sock)->connect_func(sock, node, port);
}

// return a fd for listening to read event
INLINE fd_t
tcp_socket_revent(void *sock)
{
    return ((tcp_socket_t *)sock)->revent_func(sock);
}

INLINE int
tcp_socket_close(void *sock)
{
    return ((tcp_socket_t *)sock)->close_func(sock);
}

INLINE void
tcp_socket_free(void *sock)
{
    ((tcp_socket_t *)sock)->free_func(sock);
}

// only used for proxy protocols
INLINE target_id_t *
tcp_socket_target(void *sock)
{
    ASSERT(((tcp_socket_t *)sock)->target_func, "target function not implemented");
    return ((tcp_socket_t *)sock)->target_func(sock);
}

INLINE int
tcp_socket_bind_target(void *sock, const target_id_t *target)
{
    char node_buf[TARGET_ID_MAX_DOMAIN + 1];
    char serv_buf[TARGET_ID_MAX_PORT + 1];

    target_id_node(target, node_buf);
    target_id_port(target, serv_buf);

    return tcp_socket_bind(sock, node_buf, serv_buf);
}

INLINE int
tcp_socket_connect_target(void *sock, const target_id_t *target)
{
    char node_buf[TARGET_ID_MAX_DOMAIN + 1];
    char serv_buf[TARGET_ID_MAX_PORT + 1];

    target_id_node(target, node_buf);
    target_id_port(target, serv_buf);

    return tcp_socket_connect(sock, node_buf, serv_buf);
}

#endif

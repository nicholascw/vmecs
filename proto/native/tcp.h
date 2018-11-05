#ifndef _PROTO_NATIVE_TCP_H_
#define _PROTO_NATIVE_TCP_H_

#include "pub/fd.h"

#include "proto/tcp.h"

typedef struct {
    TCP_SOCKET_HEADER
    fd_t sock;
} native_tcp_socket_t;

native_tcp_socket_t *
native_tcp_socket_new();

native_tcp_socket_t *
native_tcp_socket_new_fd(fd_t fd);

#endif

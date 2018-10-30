#ifndef _PROTO_NATIVE_TCP_H_
#define _PROTO_NATIVE_TCP_H_

#include "proto/tcp.h"

typedef struct {
    TCP_SOCKET_HEADER
    int sock;
} native_tcp_socket_t;

native_tcp_socket_t *
native_tcp_socket_new();

native_tcp_socket_t *
native_tcp_socket_new_fd(int fd);

#endif

#ifndef _PROTO_VMESS_TCP_H_
#define _PROTO_VMESS_TCP_H_

#include "proto/tcp.h"
#include "proto/buf.h"

#include "vmess.h"

typedef struct {
    TCP_SOCKET_HEADER
    vbuffer_t *read_buf; // client read from here
    vbuffer_t *write_buf; // client write to here

    int sock;
} vmess_tcp_socket_t;

vmess_tcp_socket_t *
vmess_tcp_socket_new();

#endif

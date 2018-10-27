#ifndef _PROTO_VMESS_TCP_H_
#define _PROTO_VMESS_TCP_H_

#include <pthread.h>

#include "proto/tcp.h"
#include "proto/buf.h"

#include "vmess.h"

typedef struct {
    TCP_SOCKET_HEADER
    vbuffer_t *read_buf; // client read from here
    vbuffer_t *write_buf; // client write to here

    vmess_auth_t auth;
    vmess_serial_t *vser;

    vmess_config_t *config;
    target_id_t *target;

    pthread_t reader, writer;

    int sock;
    bool server;
    bool started;
} vmess_tcp_socket_t;

vmess_tcp_socket_t *
vmess_tcp_socket_new(vmess_config_t *config);

INLINE void
vmess_tcp_socket_set_target(vmess_tcp_socket_t *sock,
                            target_id_t *target)
{
    target_id_free(sock->target);
    sock->target = target_id_copy(target);
}

INLINE const target_id_t *
vmess_tcp_socket_get_target(vmess_tcp_socket_t *sock)
{
    return sock->target;
}

INLINE void
vmess_tcp_socket_auth(vmess_tcp_socket_t *sock, uint64_t gen_time)
{
    vmess_auth_init(&sock->auth, sock->config, gen_time);
}

#endif

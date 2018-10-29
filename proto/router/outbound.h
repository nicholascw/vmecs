#ifndef _PROTO_ROUTER_OUTBOUND_H_
#define _PROTO_ROUTER_OUTBOUND_H_

#include "pub/type.h"

#include "proto/tcp.h"
#include "proto/common.h"

// a outbound acts like a client

#define TCP_OUTBOUND_HEADER \
    tcp_outbound_client_t client_func;

struct tcp_outbound_t_tag;

typedef tcp_socket_t *(*tcp_outbound_client_t)(struct tcp_outbound_t_tag *outbound, const target_id_t *target);

typedef struct tcp_outbound_t_tag {
    TCP_OUTBOUND_HEADER
} tcp_outbound_t;

INLINE tcp_socket_t *
tcp_outbound_client(tcp_outbound_t *outbound, const target_id_t *target)
{
    return outbound->client_func(outbound, target);
}

#endif

#ifndef _PROTO_ROUTER_INBOUND_H_
#define _PROTO_ROUTER_INBOUND_H_

#include "pub/type.h"

#include "proto/tcp.h"

// an inbound acts like a server

#define INBOUND_HEADER \
    tcp_inbound_server_t server_func;

typedef tcp_socket_t *(*tcp_inbound_server_t)(struct tcp_inbound_t_tag *inbound);

typedef struct tcp_inbound_t_tag {
    INBOUND_HEADER
} tcp_inbound_t;

INLINE tcp_socket_t *
tcp_inbound_server(tcp_inbound_t *inbound)
{
    return inbound->server_func(inbound);
}

#endif

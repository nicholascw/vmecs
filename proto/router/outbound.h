#ifndef _PROTO_ROUTER_OUTBOUND_H_
#define _PROTO_ROUTER_OUTBOUND_H_

#include "pub/type.h"

#include "proto/tcp.h"

// a outbound acts like a client

#define OUTBOUND_HEADER \
    tcp_outbound_client_t client_func;

typedef tcp_socket_t *(*tcp_outbound_client_t)(struct tcp_outbound_t_tag *outbound);

typedef struct tcp_outbound_t_tag {
    OUTBOUND_HEADER
} tcp_outbound_t;

INLINE tcp_socket_t *
tcp_inbound_client(tcp_outbound_t *outbound)
{
    return outbound->client_func(outbound);
}

#endif

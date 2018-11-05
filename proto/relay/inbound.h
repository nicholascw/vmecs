#ifndef _PROTO_RELAY_INBOUND_H_
#define _PROTO_RELAY_INBOUND_H_

#include "pub/type.h"

#include "proto/tcp.h"

// an inbound acts like a server

#define TCP_INBOUND_HEADER \
    tcp_inbound_server_t server_func; \
    tcp_inbound_free_t free_func;

struct tcp_inbound_t_tag;

typedef tcp_socket_t *(*tcp_inbound_server_t)(struct tcp_inbound_t_tag *inbound);
typedef void (*tcp_inbound_free_t)(struct tcp_inbound_t_tag *inbound);

typedef struct tcp_inbound_t_tag {
    TCP_INBOUND_HEADER
} tcp_inbound_t;

INLINE tcp_socket_t *
tcp_inbound_server(void *inbound)
{
    return ((tcp_inbound_t *)inbound)->server_func(inbound);
}

INLINE void
tcp_inbound_free(void *inbound)
{
    if (inbound) {
        ((tcp_inbound_t *)inbound)->free_func(inbound);
    }
}

#endif

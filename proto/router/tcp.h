#ifndef _PROTO_ROUTER_TCP_H_
#define _PROTO_ROUTER_TCP_H_

#include "inbound.h"
#include "outbound.h"

#define TCP_ROUTER_DEFAULT_MAX_CONNECT_RETRY 5

typedef struct {
    int max_connect_retry;
} tcp_router_config_t;

tcp_router_config_t *
tcp_router_config_new_default();

void
tcp_router_config_free(tcp_router_config_t *config);

tcp_router_config_t *
tcp_router_config_copy(tcp_router_config_t *config);

void
tcp_router(tcp_router_config_t *config,
           tcp_inbound_t *inbound,
           tcp_outbound_t *outbound);

#endif

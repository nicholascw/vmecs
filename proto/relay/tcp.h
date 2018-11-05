#ifndef _PROTO_RELAY_TCP_H_
#define _PROTO_RELAY_TCP_H_

#include "inbound.h"
#include "outbound.h"

#define TCP_RELAY_DEFAULT_MAX_CONNECT_RETRY 5

typedef struct {
    int max_connect_retry;
} tcp_relay_config_t;

tcp_relay_config_t *
tcp_relay_config_new_default();

void
tcp_relay_config_free(tcp_relay_config_t *config);

tcp_relay_config_t *
tcp_relay_config_copy(tcp_relay_config_t *config);

void
tcp_relay(tcp_relay_config_t *config,
          tcp_inbound_t *inbound,
          tcp_outbound_t *outbound);

#endif

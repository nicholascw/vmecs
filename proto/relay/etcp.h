#ifndef _PROTO_RELAY_ETCP_H_
#define _PROTO_RELAY_ETCP_H_

#include "tcp.h"

// tcp relay using epoll

// each connection gives two fd for data notification
// fd0 ready -> read fd0, write fd1
// fd1 ready -> read fd1, write fd0

void
tcp_relay_epoll(tcp_relay_config_t *config,
                tcp_inbound_t *inbound,
                tcp_outbound_t *outbound,
                size_t nthread);

#endif

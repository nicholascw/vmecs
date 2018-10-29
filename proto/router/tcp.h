#ifndef _PROTO_ROUTER_TCP_H_
#define _PROTO_ROUTER_TCP_H_

#include "inbound.h"
#include "outbound.h"

void
tcp_router(tcp_inbound_t *inbound, tcp_outbound_t *outbound);

#endif

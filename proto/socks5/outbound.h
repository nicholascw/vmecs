#ifndef _PROTO_SOCKS5_OUTBOUND_H_
#define _PROTO_SOCKS5_OUTBOUND_H_

#include "pub/type.h"

#include "proto/router/outbound.h"

#include "socks5.h"

typedef struct {
    TCP_OUTBOUND_HEADER
    target_id_t *proxy;
} socks5_tcp_outbound_t;

socks5_tcp_outbound_t *
socks5_tcp_outbound_new(target_id_t *proxy);

void
socks5_tcp_outbound_free(socks5_tcp_outbound_t *outbound);

#endif

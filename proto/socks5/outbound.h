#ifndef _PROTO_SOCKS5_OUTBOUND_H_
#define _PROTO_SOCKS5_OUTBOUND_H_

#include "pub/type.h"

#include "proto/relay/outbound.h"

#include "socks5.h"

typedef struct {
    TCP_OUTBOUND_HEADER
    target_id_t *proxy;
} socks5_tcp_outbound_t;

socks5_tcp_outbound_t *
socks5_tcp_outbound_new(target_id_t *proxy);

#endif

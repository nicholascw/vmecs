#ifndef _PROTO_SOCKS_INBOUND_H_
#define _PROTO_SOCKS_INBOUND_H_

#include "pub/type.h"

#include "proto/relay/inbound.h"

#include "socks5.h"

typedef struct {
    TCP_INBOUND_HEADER
    target_id_t *local; // local bind address
} socks_tcp_inbound_t;

socks_tcp_inbound_t *
socks_tcp_inbound_new(target_id_t *local);

#endif

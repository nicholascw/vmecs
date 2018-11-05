#ifndef _PROTO_NATIVE_OUTBOUND_H_
#define _PROTO_NATIVE_OUTBOUND_H_

#include "pub/type.h"

#include "proto/relay/outbound.h"

typedef struct {
    TCP_OUTBOUND_HEADER
} native_tcp_outbound_t;

native_tcp_outbound_t *
native_tcp_outbound_new();

#endif

#ifndef _PROTO_NATIVE_OUTBOUND_H_
#define _PROTO_NATIVE_OUTBOUND_H_

#include "pub/type.h"

#include "proto/router/outbound.h"

typedef struct {
    TCP_OUTBOUND_HEADER
} native_tcp_outbound_t;

native_tcp_outbound_t *
native_tcp_outbound_new();

void
native_tcp_outbound_free(native_tcp_outbound_t *outbound);

#endif

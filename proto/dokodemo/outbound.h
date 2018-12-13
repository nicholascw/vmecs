#ifndef _PROTO_DOKODEMO_OUTBOUND_H_
#define _PROTO_DOKODEMO_OUTBOUND_H_

#include "pub/type.h"

#include "proto/relay/outbound.h"

typedef struct {
    TCP_OUTBOUND_HEADER
    target_id_t *target;
} dokodemo_tcp_outbound_t;

dokodemo_tcp_outbound_t *
dokodemo_tcp_outbound_new(target_id_t *target);

#endif

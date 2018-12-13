#ifndef _PROTO_DOKODEMO_INBOUND_H_
#define _PROTO_DOKODEMO_INBOUND_H_

#include "pub/type.h"

#include "proto/relay/inbound.h"

typedef struct {
    TCP_INBOUND_HEADER
    target_id_t *local; // local bind address
} dokodemo_tcp_inbound_t;

dokodemo_tcp_inbound_t *
dokodemo_tcp_inbound_new(target_id_t *local);

#endif

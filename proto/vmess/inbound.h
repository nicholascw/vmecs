#ifndef _PROTO_VMESS_INBOUND_H_
#define _PROTO_VMESS_INBOUND_H_

#include "pub/type.h"

#include "proto/relay/inbound.h"

#include "vmess.h"

typedef struct {
    TCP_INBOUND_HEADER
    vmess_config_t *config;
    target_id_t *local; // local bind address
} vmess_tcp_inbound_t;

vmess_tcp_inbound_t *
vmess_tcp_inbound_new(vmess_config_t *config, target_id_t *local);

#endif

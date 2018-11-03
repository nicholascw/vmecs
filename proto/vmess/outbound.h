#ifndef _PROTO_VMESS_OUTBOUND_H_
#define _PROTO_VMESS_OUTBOUND_H_

#include "pub/type.h"

#include "proto/router/outbound.h"

#include "vmess.h"

typedef struct {
    TCP_OUTBOUND_HEADER
    vmess_config_t *config;
    target_id_t *proxy;
} vmess_tcp_outbound_t;

vmess_tcp_outbound_t *
vmess_tcp_outbound_new(vmess_config_t *config, target_id_t *proxy);

#endif

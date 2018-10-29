#include <stdio.h>
#include <signal.h>

#include "pub/type.h"

#include "proto/vmess/inbound.h"
#include "proto/vmess/vmess.h"
#include "proto/vmess/tcp.h"

#include "proto/native/outbound.h"

#include "proto/router/tcp.h"

hash128_t user_id;

int main()
{
    signal(SIGPIPE, SIG_IGN);

    vmess_config_t *config;
    target_id_t *local;
    vmess_tcp_inbound_t *inbound;
    native_tcp_outbound_t *outbound;

    memset(user_id, 0, sizeof(user_id));

    config = vmess_config_new(user_id);
    local = target_id_new_ipv4((byte_t[]) { 127, 0, 0, 1 }, 3132);

    inbound = vmess_tcp_inbound_new(config, local);
    outbound = native_tcp_outbound_new();

    tcp_router((tcp_inbound_t *)inbound, (tcp_outbound_t *)outbound);

    return 0;
}

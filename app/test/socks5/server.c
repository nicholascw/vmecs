#include <stdio.h>
#include <signal.h>

#include "pub/type.h"

#include "proto/socks5/inbound.h"
#include "proto/socks5/socks5.h"
#include "proto/socks5/tcp.h"

#include "proto/native/outbound.h"

#include "proto/router/tcp.h"

int main()
{
    signal(SIGPIPE, SIG_IGN);

    target_id_t *local;
    socks5_tcp_inbound_t *inbound;
    native_tcp_outbound_t *outbound;

    tcp_router_config_t *router_conf;

    local = target_id_new_ipv4((byte_t[]) { 0, 0, 0, 0 }, 3133);

    router_conf = tcp_router_config_new_default();
    inbound = socks5_tcp_inbound_new(local);
    outbound = native_tcp_outbound_new();

    tcp_router(router_conf, (tcp_inbound_t *)inbound, (tcp_outbound_t *)outbound);

    target_id_free(local);
    tcp_router_config_free(router_conf);

    tcp_inbound_free(inbound);
    tcp_outbound_free(outbound);

    return 0;
}

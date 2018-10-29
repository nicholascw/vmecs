#include <time.h>

#include "proto/common.h"
#include "proto/socket.h"

#include "vmess.h"
#include "inbound.h"
#include "tcp.h"

static tcp_socket_t *
_vmess_tcp_inbound_server(tcp_inbound_t *_inbound)
{
    vmess_tcp_inbound_t *inbound = (vmess_tcp_inbound_t *)_inbound;
    tcp_socket_t *sock = (tcp_socket_t *)vmess_tcp_socket_new(inbound->config);
    
    while (tcp_socket_bind_target(sock, inbound->local))
        sleep(1);

    return sock;
}

vmess_tcp_inbound_t *
vmess_tcp_inbound_new(vmess_config_t *config, target_id_t *local)
{
    vmess_tcp_inbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->server_func = _vmess_tcp_inbound_server;
    ret->config = vmess_config_copy(config);
    ret->local = target_id_copy(local);

    return ret;
}

void
vmess_tcp_inbound_free(vmess_tcp_inbound_t *inbound)
{
    if (inbound) {
        vmess_config_free(inbound->config);
        target_id_free(inbound->local);
        free(inbound);
    }
}

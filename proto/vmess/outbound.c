#include <time.h>

#include "proto/common.h"

#include "vmess.h"
#include "outbound.h"
#include "tcp.h"

static tcp_socket_t *
_vmess_tcp_outbound_client(tcp_outbound_t *_outbound, const target_id_t *target)
{
    vmess_tcp_outbound_t *outbound = (vmess_tcp_outbound_t *)_outbound;
    vmess_tcp_socket_t *sock = vmess_tcp_socket_new(outbound->config);

    vmess_tcp_socket_set_proxy(sock, outbound->proxy);
    vmess_tcp_socket_auth(sock, time(NULL));

    if (tcp_socket_connect_target(sock, target)) {
        tcp_socket_close(sock);
        tcp_socket_free(sock);
        return NULL;
    }

    return (tcp_socket_t *)sock;
}

vmess_tcp_outbound_t *
vmess_tcp_outbound_new(vmess_config_t *config, target_id_t *proxy)
{
    vmess_tcp_outbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->client_func = _vmess_tcp_outbound_client;

    ret->config = vmess_config_copy(config);
    ret->proxy = target_id_copy(proxy);

    return ret;
}

void
vmess_tcp_outbound_free(vmess_tcp_outbound_t *outbound)
{
    if (outbound) {
        vmess_config_free(outbound->config);
        target_id_free(outbound->proxy);
        free(outbound);
    }
}

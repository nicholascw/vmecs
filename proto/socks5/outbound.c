#include <time.h>

#include "proto/common.h"

#include "socks5.h"
#include "outbound.h"
#include "tcp.h"

static tcp_socket_t *
_socks5_tcp_outbound_client(tcp_outbound_t *_outbound, const target_id_t *target)
{
    socks5_tcp_outbound_t *outbound = (socks5_tcp_outbound_t *)_outbound;
    socks5_tcp_socket_t *sock = socks5_tcp_socket_new();

    socks5_tcp_socket_set_proxy(sock, outbound->proxy);

    if (tcp_socket_connect_target(sock, target)) {
        tcp_socket_close(sock);
        tcp_socket_free(sock);
        return NULL;
    }

    return (tcp_socket_t *)sock;
}

socks5_tcp_outbound_t *
socks5_tcp_outbound_new(target_id_t *proxy)
{
    socks5_tcp_outbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->client_func = _socks5_tcp_outbound_client;
    ret->proxy = target_id_copy(proxy);

    return ret;
}

void
socks5_tcp_outbound_free(socks5_tcp_outbound_t *outbound)
{
    if (outbound) {
        target_id_free(outbound->proxy);
        free(outbound);
    }
}

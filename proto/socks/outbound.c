#include <time.h>

#include "proto/common.h"

#include "socks5.h"
#include "outbound.h"
#include "tcp.h"

static tcp_socket_t *
_socks_tcp_outbound_client(tcp_outbound_t *_outbound, const target_id_t *target)
{
    socks_tcp_outbound_t *outbound = (socks_tcp_outbound_t *)_outbound;
    socks_tcp_socket_t *sock = socks_tcp_socket_new(outbound->socks_vers);

    socks_tcp_socket_set_proxy(sock, outbound->proxy);

    if (tcp_socket_connect_target(sock, target)) {
        tcp_socket_close(sock);
        tcp_socket_free(sock);
        return NULL;
    }

    return (tcp_socket_t *)sock;
}

static void
_socks_tcp_outbound_free(tcp_outbound_t *_outbound)
{
    socks_tcp_outbound_t *outbound = (socks_tcp_outbound_t *)_outbound;

    if (outbound) {
        target_id_free(outbound->proxy);
        free(outbound);
    }
}

socks_tcp_outbound_t *
socks_tcp_outbound_new(target_id_t *proxy, int socks_vers)
{
    socks_tcp_outbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->client_func = _socks_tcp_outbound_client;
    ret->free_func = _socks_tcp_outbound_free;
    ret->proxy = target_id_copy(proxy);
    ret->socks_vers = socks_vers;

    return ret;
}

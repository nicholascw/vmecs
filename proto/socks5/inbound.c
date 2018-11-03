#include <time.h>

#include "proto/common.h"
#include "proto/socket.h"

#include "socks5.h"
#include "inbound.h"
#include "tcp.h"

static tcp_socket_t *
_socks5_tcp_inbound_server(tcp_inbound_t *_inbound)
{
    socks5_tcp_inbound_t *inbound = (socks5_tcp_inbound_t *)_inbound;
    tcp_socket_t *sock = (tcp_socket_t *)socks5_tcp_socket_new();
    
    while (tcp_socket_bind_target(sock, inbound->local))
        sleep(1);

    return sock;
}

static void
_socks5_tcp_inbound_free(tcp_inbound_t *_inbound)
{
    socks5_tcp_inbound_t *inbound = (socks5_tcp_inbound_t *)_inbound;

    if (inbound) {
        target_id_free(inbound->local);
        free(inbound);
    }
}

socks5_tcp_inbound_t *
socks5_tcp_inbound_new(target_id_t *local)
{
    socks5_tcp_inbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->server_func = _socks5_tcp_inbound_server;
    ret->free_func = _socks5_tcp_inbound_free;
    ret->local = target_id_copy(local);

    return ret;
}

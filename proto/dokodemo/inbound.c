#include "pub/socket.h"

#include "proto/common.h"
#include "proto/native/tcp.h"

#include "inbound.h"

static tcp_socket_t *
_dokodemo_tcp_inbound_server(tcp_inbound_t *_inbound)
{
    dokodemo_tcp_inbound_t *inbound = (dokodemo_tcp_inbound_t *)_inbound;
    tcp_socket_t *sock = (tcp_socket_t *)native_tcp_socket_new();

    while (tcp_socket_bind_target(sock, inbound->local))
        sleep(1);

    return sock;
}

static void
_dokodemo_tcp_inbound_free(tcp_inbound_t *_inbound)
{
    dokodemo_tcp_inbound_t *inbound = (dokodemo_tcp_inbound_t *)_inbound;

    if (inbound) {
        target_id_free(inbound->local);
        free(inbound);
    }
}

dokodemo_tcp_inbound_t *
dokodemo_tcp_inbound_new(target_id_t *local)
{
    dokodemo_tcp_inbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->server_func = _dokodemo_tcp_inbound_server;
    ret->free_func = _dokodemo_tcp_inbound_free;
    ret->local = target_id_copy(local);

    return ret;
}

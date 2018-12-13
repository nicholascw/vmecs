#include "proto/native/tcp.h"

#include "outbound.h"

static tcp_socket_t *
_dokodemo_tcp_outbound_client(tcp_outbound_t *_outbound, const target_id_t *_)
{
    dokodemo_tcp_outbound_t *outbound = (dokodemo_tcp_outbound_t *)_outbound;
    tcp_socket_t *sock = (tcp_socket_t *)native_tcp_socket_new();
    
    if (tcp_socket_connect_target(sock, outbound->target)) {
        tcp_socket_close(sock);
        tcp_socket_free(sock);
        return NULL;
    }

    return sock;
}

static void
_dokodemo_tcp_outbound_free(tcp_outbound_t *_outbound)
{
    dokodemo_tcp_outbound_t *outbound = (dokodemo_tcp_outbound_t *)_outbound;

    if (outbound) {
        target_id_free(outbound->target);
        free(outbound);
    }
}

dokodemo_tcp_outbound_t *
dokodemo_tcp_outbound_new(target_id_t *target)
{
    dokodemo_tcp_outbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->client_func = _dokodemo_tcp_outbound_client;
    ret->free_func = _dokodemo_tcp_outbound_free;
    
    ret->target = target_id_copy(target);

    return ret;
}

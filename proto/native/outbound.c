#include "outbound.h"
#include "tcp.h"

static tcp_socket_t *
_native_tcp_outbound_client(tcp_outbound_t *_outbound, const target_id_t *target)
{
    tcp_socket_t *sock = (tcp_socket_t *)native_tcp_socket_new();
    
    if (tcp_socket_connect_target(sock, target)) {
        tcp_socket_close(sock);
        tcp_socket_free(sock);
        return NULL;
    }

    return sock;
}

native_tcp_outbound_t *
native_tcp_outbound_new()
{
    native_tcp_outbound_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->client_func = _native_tcp_outbound_client;
    
    return ret;
}

void
native_tcp_outbound_free(native_tcp_outbound_t *outbound)
{
    if (outbound) {
        free(outbound);
    }
}

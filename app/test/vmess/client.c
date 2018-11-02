#include <stdio.h>

#include "pub/type.h"

#include "proto/vmess/outbound.h"
#include "proto/vmess/vmess.h"
#include "proto/vmess/tcp.h"
#include "proto/router/tcp.h"

hash128_t user_id;

int main()
{
    vmess_config_t *config;
    target_id_t *proxy;
    vmess_tcp_socket_t *sock;
    
    byte_t data[] = 
        "GET / HTTP/1.1\r\n"
        "Host: www.google.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    byte_t buf[4096];
    ssize_t size;

    memset(user_id, 0, sizeof(user_id));

    config = vmess_config_new(user_id);
    proxy = target_id_new_ipv4((byte_t []) { 127, 0, 0, 1 }, 3131);

    sock = vmess_tcp_socket_new(config);

    vmess_tcp_socket_set_proxy(sock, proxy);
    vmess_tcp_socket_auth(sock, time(NULL));

    while (tcp_socket_connect(sock, "www.google.com", "80")) {
        sleep(1);
    }

    tcp_socket_write(sock, data, sizeof(data) - 1);

    while ((size = tcp_socket_read(sock, buf, sizeof(buf)))) {
        hexdump("client received", buf, size);

        if (size >= 4 && memcmp(buf + size - 4, "\r\n\r\n", 4) == 0) {
            TRACE("break!");
            break;
        }
    }

    tcp_socket_close(sock);
    tcp_socket_free(sock);

    vmess_config_free(config);
    target_id_free(proxy);

    TRACE("client end");

    return 0;
}

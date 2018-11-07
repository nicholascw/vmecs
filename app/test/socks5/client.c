#include <stdio.h>

#include "pub/type.h"

#include "proto/socks/outbound.h"
#include "proto/socks/socks5.h"
#include "proto/socks/tcp.h"
#include "proto/relay/tcp.h"

int main()
{
    target_id_t *proxy;
    socks_tcp_socket_t *sock;
    
    byte_t data[] = 
        "GET / HTTP/1.1\r\n"
        "Host: www.google.com\r\n"
        "Connection: close\r\n"
        "\r\n";

    byte_t buf[4096];
    ssize_t size;

    proxy = target_id_new_ipv4((byte_t []) { 127, 0, 0, 1 }, 3133);

    sock = socks_tcp_socket_new(SOCKS_VERSION_5);

    socks_tcp_socket_set_proxy(sock, proxy);

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

    target_id_free(proxy);

    TRACE("client end");

    return 0;
}

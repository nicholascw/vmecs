#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "proto/vmess/vmess.h"
#include "proto/vmess/decoding.h"
#include "proto/vmess/tcp.h"
#include "proto/native/tcp.h"
#include "proto/buf.h"
#include "proto/socket.h"

#define PORT 3137
#define PORT_STR "3137"

void hexdump(char *desc, void *addr, int len);

hash128_t user_id = {0};

void server()
{
    // vmess_config_t *config = vmess_config_new(user_id);
    // vmess_tcp_socket_t *sock = vmess_tcp_socket_new(config);
    // vmess_tcp_socket_t *client;

    native_tcp_socket_t *sock = native_tcp_socket_new();
    native_tcp_socket_t *client;

    byte_t buf[2];

    while (tcp_socket_bind(sock, "127.0.0.1", PORT_STR)) {
        sleep(1);
    }
    
    tcp_socket_listen(sock, 5);

    client = tcp_socket_accept(sock);
    ASSERT(client, "server accept failed");

    printf("accepted client %p\n", client);

    tcp_socket_read(client, buf, 1);

    buf[1] = 0;
    printf("server read: %s\n", buf);

    tcp_socket_write(client, "y", 1);

    // print_target(vmess_tcp_socket_get_target(client));

    sleep(1);

    printf("closing server\n");
    tcp_socket_close(client);
    tcp_socket_close(sock);
    tcp_socket_free(client);
    tcp_socket_free(sock);
    printf("server closed\n");

    // vmess_config_free(config);
}

void client()
{
    // vmess_config_t *config = vmess_config_new(user_id);
    // vmess_tcp_socket_t *sock = vmess_tcp_socket_new(config);
    // target_id_t *target = target_id_new_ipv4((byte_t[]){ 127, 0, 0, 1 }, PORT);

    // vmess_tcp_socket_set_proxy(sock, target);
    // vmess_tcp_socket_auth(sock, time(NULL));

    native_tcp_socket_t *sock = native_tcp_socket_new();

    byte_t buf[2];

    while (tcp_socket_connect(sock, "127.0.0.1", PORT_STR)) {
        sleep(1);
    }

    tcp_socket_write(sock, "b", 1);
    tcp_socket_read(sock, buf, 1);

    buf[1] = 0;
    printf("client read: %s\n", buf);

    printf("closing client\n");
    tcp_socket_close(sock);
    tcp_socket_free(sock);
    printf("client closed\n");

    // vmess_config_free(config);
    // target_id_free(target);
}

void *run(void *f)
{
    ((void (*)())f)();
    return NULL;
}

int main()
{
    pthread_t client_tid, server_tid;
    
    pthread_create(&client_tid, NULL, run, client);
    pthread_create(&server_tid, NULL, run, server);

    pthread_join(client_tid, NULL);
    pthread_join(server_tid, NULL);

    return 0;
}

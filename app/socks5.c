#include <time.h>

#include "pub/type.h"

#include "proto/socket.h"
#include "proto/buf.h"
#include "proto/socks5/socks5.h"

int main()
{
    int sock, client;
    rbuffer_t *rbuf;

    data_trunk_t supported;
    byte_t *tmp;
    size_t size;

    socks5_request_t req;
    socks5_response_t resp;

    target_id_t *local = target_id_new_ipv4((byte_t []) { 127, 0, 0, 1 }, 3133);
    struct addrinfo *ai;

    sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (sock == -1) {
        perror("socket");
        return 0;
    }

    ai = target_id_resolve(local);

    while (bind(sock, ai->ai_addr, ai->ai_addrlen)) {
        perror("bind");
        sleep(1);
    }

    freeaddrinfo(ai);

    listen(sock, 10);

    TRACE("listening");

    rbuf = rbuffer_new(1024);
    client = accept(sock, NULL, NULL);

    TRACE("accepted %d", client);

    switch (rbuffer_read(rbuf, client, socks5_auth_sel_decoder,
                         NULL, &supported)) {
        case RBUFFER_SUCCESS:
            break;

        case RBUFFER_ERROR:
            TRACE("invalid header");
            goto ERROR1;

        case RBUFFER_INCOMPLETE:
            TRACE("incomplete header");
            goto ERROR1;
    }

    for (size_t i = 0; i < supported.size; i++) {
        TRACE("supported method %d", supported.data[i]);
    }

    // acknowledge auth method
    tmp = socks5_encode_auth_ack(0, &size);
    write(client, tmp, size);

    // read request
    switch (rbuffer_read(rbuf, client, socks5_request_decoder,
                         NULL, &req)) {
        case RBUFFER_SUCCESS:
            break;

        case RBUFFER_ERROR:
            TRACE("invalid header");
            goto ERROR1;

        case RBUFFER_INCOMPLETE:
            TRACE("incomplete header");
            goto ERROR1;
    }

    print_target(req.target);
    TRACE("request: cmd %d", req.cmd);

    resp = (socks5_response_t) {
        .rep = SOCKS5_REP_SUCCESS
    };

    tmp = socks5_encode_response(&resp, &size);
    write(client, tmp, size);

    char http_data[] =
        "lalalal\r\n\r\n";

    write(client, http_data, sizeof(http_data) - 1);

ERROR1:

    close(client);
    close(sock);

    return 0;
}

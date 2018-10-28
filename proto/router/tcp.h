#ifndef _PROTO_ROUTER_TCP_H_
#define _PROTO_ROUTER_TCP_H_

// tcp router
/*

tcp_inbound_t *inbound;
tcp_outbound_t *outbound;

thread reader((client, target_socket)) {
    buf;

    while (1) {
        read(target_socket, buf);
        write(client, buf);
    }
}

thread writer((client, target_socket)) {
    buf;

    while (1) {
        read(client, buf);
        writer(target_socket, buf);
    }
}

thread f(client) {
    target_id_t *target = target(client);
    tcp_socket_t *target_socket = client(outbound);

    connect(target_socket, target);
    
    new_thread(read, (client, target_socket));
    new_thread(writer, (client, target_socket));

    join two threads;
}

tcp_socket_t *server, *client;

server = tcp_inbound_server(inbound);

bind(server);
listen(server, backlog);

while (1) {
    client = accept(server);
    new_thread(f, client);
}

*/

#endif

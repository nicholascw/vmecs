#include "proto/socket.h"
#include "proto/buf.h"

#include "socks5.h"
#include "tcp.h"

#define DEFAULT_BUFFER 32

static ssize_t
_socks5_tcp_socket_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    return read_r(sock->sock, buf, size);
}

static ssize_t
_socks5_tcp_socket_write(tcp_socket_t *_sock, const byte_t *buf, size_t size)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    return write_r(sock->sock, buf, size);
}

static int
_socks5_tcp_socket_bind(tcp_socket_t *_sock, const char *node, const char *port)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;

    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;

    if (getaddrinfo_r(node, port, &hints, &list)) {
        return -1;
    }

    socket_set_reuse_port(sock->sock);

    if (bind(sock->sock, list->ai_addr, list->ai_addrlen)) {
        perror("bind");
        freeaddrinfo(list);
        return -1;
    }

    freeaddrinfo(list);

    return 0;
}

static int
_socks5_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    return listen(sock->sock, backlog);
}

static bool
_socks5_tcp_socket_handshake(socks5_tcp_socket_t *sock, target_id_t *target)
{
    data_trunk_t auth_sel;
    byte_t auth_ack;
    
    socks5_request_t req;
    socks5_response_t resp;

    byte_t *tmp;
    size_t size;

    size_t i;
    bool found;

    rbuffer_t *rbuf = rbuffer_new(DEFAULT_BUFFER);

#define CLEARUP() \
    do { \
        rbuffer_free(rbuf); \
    } while (0);

#define READ_DECODE(name, decoder, acceptor) \
    switch (rbuffer_read(rbuf, sock->sock, (decoder), NULL, (acceptor))) { \
        case RBUFFER_SUCCESS: \
            break; \
 \
        case RBUFFER_ERROR: \
            TRACE(name " decoding failed, rejecting connection"); \
            CLEARUP(); \
            return false; \
 \
        case RBUFFER_INCOMPLETE: \
            TRACE("incomplete " name ", rejecting connection"); \
            CLEARUP(); \
            return false; \
    }

    if (!target) {
        // server

        READ_DECODE("auth sel", socks5_auth_sel_decoder, &auth_sel);

        found = false;

        for (i = 0; i < auth_sel.size; i++) {
            if (auth_sel.data[i] == SOCKS5_AUTH_METHOD_NO_AUTH) {
                found = true;
                break;
            }
        }

        data_trunk_destroy(&auth_sel);

        if (!found) {
            TRACE("no supported auth method given");
            CLEARUP();
            return false;
        }

        tmp = socks5_encode_auth_ack(SOCKS5_AUTH_METHOD_NO_AUTH, &size);
        write(sock->sock, tmp, size);
        free(tmp);

        READ_DECODE("request", socks5_request_decoder, &req);

        if (req.cmd != SOCKS5_CMD_CONNECT) {
            TRACE("unsupported request command");
            socks5_request_destroy(&req);
            CLEARUP();
            return false;
        }

        // set target
        socks5_tcp_socket_set_target(sock, req.target);
        socks5_request_destroy(&req);

        resp = (socks5_response_t) {
            .rep = SOCKS5_REP_SUCCESS
        };

        // write response
        tmp = socks5_encode_response(&resp, &size);
        write(sock->sock, tmp, size);
        free(tmp);
    } else {
        // client

        // only support no auth for now
        tmp = socks5_encode_auth_sel((byte_t []) { SOCKS5_AUTH_METHOD_NO_AUTH }, 1, &size);
        write(sock->sock, tmp, size);
        free(tmp);

        READ_DECODE("auth ack", socks5_auth_ack_decoder, &auth_ack);

        if (auth_ack != SOCKS5_AUTH_METHOD_NO_AUTH) {
            TRACE("invalid auth ack");
            CLEARUP();
            return false;
        }

        req = (socks5_request_t) {
            .cmd = SOCKS5_CMD_CONNECT,
            .target = target
        };

        // send request
        tmp = socks5_encode_request(&req, &size);
        write(sock->sock, tmp, size);
        free(tmp);

        READ_DECODE("response", socks5_response_decoder, &resp);

        if (resp.rep != SOCKS5_REP_SUCCESS) {
            TRACE("socks5 server rejected with code %d", resp.rep);
            CLEARUP();
            return false;
        }
    }

#undef READ_DECODE

    CLEARUP();

    return true;
}

static tcp_socket_t *
_socks5_tcp_socket_accept(tcp_socket_t *_sock)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    socks5_tcp_socket_t *ret;
    int client;

    client = accept(sock->sock, NULL, NULL);

    if (client == -1) {
        return NULL;
    }

    ret = socks5_tcp_socket_new_fd(client);
    
    if (!_socks5_tcp_socket_handshake(ret, NULL)) {
        tcp_socket_close((tcp_socket_t *)ret);
        tcp_socket_free((tcp_socket_t *)ret);
        return NULL;
    }

    return (tcp_socket_t *)ret;
}

static int
_socks5_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    struct addrinfo *list;
    target_id_t *target;

    ASSERT(sock->addr.proxy, "socks5 proxy server not set");

    list = target_id_resolve(sock->addr.proxy);

    if (connect(sock->sock, list->ai_addr, list->ai_addrlen)) {
        perror("connect");
        freeaddrinfo(list);
        return -1;
    }

    freeaddrinfo(list);

    target = target_id_parse(node, port);

    if (!_socks5_tcp_socket_handshake(sock, target)) {
        target_id_free(target);
        return -1;
    }

    target_id_free(target);

    return 0;
}

static int
_socks5_tcp_socket_close(tcp_socket_t *_sock)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    return close(sock->sock);
}

static void
_socks5_tcp_socket_free(tcp_socket_t *_sock)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;

    if (sock) {
        target_id_free(sock->addr.proxy);
        free(sock);
    }
}

static target_id_t *
_socks5_tcp_socket_target(tcp_socket_t *_sock)
{
    socks5_tcp_socket_t *sock = (socks5_tcp_socket_t *)_sock;
    ASSERT(sock->addr.target, "socks5 target not set");
    return target_id_copy(sock->addr.target);
}

socks5_tcp_socket_t *
socks5_tcp_socket_new_fd(int fd)
{
    socks5_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->read_func = _socks5_tcp_socket_read;
    ret->write_func = _socks5_tcp_socket_write;
    ret->bind_func = _socks5_tcp_socket_bind;
    ret->listen_func = _socks5_tcp_socket_listen;
    ret->accept_func = _socks5_tcp_socket_accept;
    ret->connect_func = _socks5_tcp_socket_connect;
    ret->close_func = _socks5_tcp_socket_close;
    ret->free_func = _socks5_tcp_socket_free;
    ret->target_func = _socks5_tcp_socket_target;

    ret->sock = fd;
    ret->addr.proxy = NULL;
    ret->addr.target = NULL;

    return ret;
}

socks5_tcp_socket_t *
socks5_tcp_socket_new()
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT(fd != -1, "failed to create socket");
    
    socket_set_timeout(fd, 1);
    
    return socks5_tcp_socket_new_fd(fd);
}

int
socks5_to_socket(socks5_tcp_socket_t *sock)
{
    int fd = sock->sock;
    tcp_socket_free(sock);
    return fd;
}

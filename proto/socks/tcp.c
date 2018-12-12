#include "pub/socket.h"

#include "proto/buf.h"

#include "socks5.h"
#include "socks4.h"
#include "tcp.h"

#define DEFAULT_BUFFER 32

static ssize_t
_socks_tcp_socket_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    return fd_read(sock->sock, buf, size);
}

static ssize_t
_socks_tcp_socket_try_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    return fd_try_read(sock->sock, buf, size);
}

static ssize_t
_socks_tcp_socket_write(tcp_socket_t *_sock, const byte_t *buf, size_t size)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    return fd_write(sock->sock, buf, size);
}

static int
_socks_tcp_socket_bind(tcp_socket_t *_sock, const char *node, const char *port)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    socket_sockaddr_t addr;

    if (socket_getsockaddr(node, port, &addr)) {
        return -1;
    }

    socket_set_reuse_port(sock->sock);

    return socket_bind(sock->sock, &addr);
}

static int
_socks_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    return socket_listen(sock->sock, backlog);
}

static bool
_socks_tcp_socket_handshake_c(socks_tcp_socket_t *sock, target_id_t *target)
{
    data_trunk_t auth_sel;
    byte_t auth_ack;
    
    socks5_request_t req;
    socks5_response_t resp;

    socks4_request_t req4;
    socks4_response_t resp4;

    byte_t *tmp;
    size_t size;

    byte_t vers;

    size_t i;
    bool found;

    socket_sockaddr_t addr;

    rbuffer_t *rbuf = rbuffer_new(DEFAULT_BUFFER);

    target_id_t *tmp_target;

#define CLEANUP() \
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
            CLEANUP(); \
            return false; \
 \
        case RBUFFER_INCOMPLETE: \
            TRACE("incomplete " name ", rejecting connection"); \
            CLEANUP(); \
            return false; \
    }

    if (!target) {
        // server

        if (!fd_read(sock->sock, &vers, sizeof(vers))) {
            TRACE("nothing received");
            CLEANUP();
            return false;
        }

        rbuffer_push(rbuf, &vers, sizeof(vers));

        if (vers == SOCKS5_VERSION) {
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
                CLEANUP();
                return false;
            }

            tmp = socks5_encode_auth_ack(SOCKS5_AUTH_METHOD_NO_AUTH, &size);
            write(sock->sock, tmp, size);
            free(tmp);

            READ_DECODE("socks5 request", socks5_request_decoder, &req);

            if (req.cmd != SOCKS5_CMD_CONNECT) {
                TRACE("unsupported request command");
                socks5_request_destroy(&req);
                CLEANUP();
                return false;
            }

            // set target
            socks_tcp_socket_set_target(sock, req.target);
            socks5_request_destroy(&req);

            resp = (socks5_response_t) {
                .rep = SOCKS5_REP_SUCCESS
            };

            // write response
            tmp = socks5_encode_response(&resp, &size);
            fd_write(sock->sock, tmp, size);
            free(tmp);
        } else if (vers == SOCKS4_VERSION) {
            READ_DECODE("socks4 request", socks4_request_decoder, &req4);
            
            if (req4.cmd != SOCKS4_CMD_CONNECT) {
                TRACE("unsupported request command");
                socks4_request_destroy(&req4);
                CLEANUP();
                return false;
            }
            
            // fprintf(stderr, "socks4 target: ");
            // print_target(req4.target);

            socks_tcp_socket_set_target(sock, req4.target);

            // send back response
            resp4 = (socks4_response_t) {
                .rep = SOCKS4_REP_SUCCESS
            };

            tmp = socks4_encode_response(&resp4, &size);
            fd_write(sock->sock, tmp, size);
            free(tmp);
        } else {
            TRACE("unrecognized version number %d", vers);
            return false;
        }
    } else {
        // client
        if (sock->socks_vers == SOCKS_VERSION_5) {
            // only support no auth for now
            tmp = socks5_encode_auth_sel((byte_t []) { SOCKS5_AUTH_METHOD_NO_AUTH }, 1, &size);
            fd_write(sock->sock, tmp, size);
            free(tmp);

            READ_DECODE("auth ack", socks5_auth_ack_decoder, &auth_ack);

            if (auth_ack != SOCKS5_AUTH_METHOD_NO_AUTH) {
                TRACE("invalid auth ack");
                CLEANUP();
                return false;
            }

            req = (socks5_request_t) {
                .cmd = SOCKS5_CMD_CONNECT,
                .target = target
            };

            // send request
            tmp = socks5_encode_request(&req, &size);
            fd_write(sock->sock, tmp, size);
            free(tmp);

            READ_DECODE("socks5 response", socks5_response_decoder, &resp);

            if (resp.rep != SOCKS5_REP_SUCCESS) {
                TRACE("socks5 server rejected with code %d", resp.rep);
                CLEANUP();
                return false;
            }
        } else if (sock->socks_vers == SOCKS_VERSION_4) {
            if (!target_id_resolve(target, &addr)) {
                TRACE("failed to resolve target");
                CLEANUP();
                return false;
            }

            if (!socket_sockaddr_is_ipv4(&addr)) {
                TRACE("socks4 only supports ipv4 addresses");
                CLEANUP();
                return false;
            }

            tmp_target = target_id_new_ipv4(socket_sockaddr_get_ipv4(&addr),
                                            socket_sockaddr_port(&addr));

            req4 = (socks4_request_t) {
                .cmd = SOCKS4_CMD_CONNECT,
                .target = tmp_target
            };

            tmp = socks4_encode_request(&req4, &size);
            fd_write(sock->sock, tmp, size);
            free(tmp);
            target_id_free(tmp_target);

            READ_DECODE("socks4 response", socks4_response_decoder, &resp4);

            if (resp4.rep != SOCKS4_REP_SUCCESS) {
                TRACE("socks4 server rejected with code %d", resp4.rep);
                CLEANUP();
                return false;
            }
        } else {
            ASSERT(0, "must specify socks version in client");
        }
    }

#undef READ_DECODE

    CLEANUP();

    return true;
}

static tcp_socket_t *
_socks_tcp_socket_accept(tcp_socket_t *_sock)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    socks_tcp_socket_t *ret;
    fd_t client;

    // TRACE("accepting");

    client = socket_accept(sock->sock, NULL);

    if (client == -1) {
        return NULL;
    }

    // TRACE("accepted connection");

    ret = socks_tcp_socket_new_fd(sock->socks_vers, client);

    return (tcp_socket_t *)ret;
}

static int
_socks_tcp_socket_handshake(tcp_socket_t *_sock)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;

    if (!_socks_tcp_socket_handshake_c(sock, NULL))
        return -1;
    else
        return 0;
}

static int
_socks_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    socket_sockaddr_t addr;
    target_id_t *target;
    
    ASSERT(sock->addr.proxy, "socks proxy server not set");

    if (!target_id_resolve(sock->addr.proxy, &addr))
        return -1;

    print_target("connecting proxy", sock->addr.proxy);

    if (socket_connect(sock->sock, &addr))
        return -1;

    print_target("proxy connected", sock->addr.proxy);

    target = target_id_parse(node, port);

    if (!target) {
        TRACE("invalid node/port");
        return -1;
    }

    print_target("handshaking", target);

    if (!_socks_tcp_socket_handshake_c(sock, target)) {
        target_id_free(target);
        return -1;
    }

    print_target("connected", target);

    target_id_free(target);

    return 0;
}

static fd_t
_socks_tcp_socket_revent(tcp_socket_t *_sock)
{
    return ((socks_tcp_socket_t *)_sock)->sock;
}

static int
_socks_tcp_socket_close(tcp_socket_t *_sock)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    return socket_shutdown_write(sock->sock);
}

static void
_socks_tcp_socket_free(tcp_socket_t *_sock)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;

    if (sock) {
        target_id_free(sock->addr.proxy);
        
        if (sock->sock != -1) {
            if (close(sock->sock)) {
                perror("close");
            }
        }

        free(sock);
    }
}

static target_id_t *
_socks_tcp_socket_target(tcp_socket_t *_sock)
{
    socks_tcp_socket_t *sock = (socks_tcp_socket_t *)_sock;
    ASSERT(sock->addr.target, "socks target not set");
    return target_id_copy(sock->addr.target);
}

socks_tcp_socket_t *
socks_tcp_socket_new_fd(int socks_vers, fd_t fd)
{
    socks_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->read_func = _socks_tcp_socket_read;
    ret->try_read_func = _socks_tcp_socket_try_read;
    ret->write_func = _socks_tcp_socket_write;
    ret->bind_func = _socks_tcp_socket_bind;
    ret->listen_func = _socks_tcp_socket_listen;
    ret->accept_func = _socks_tcp_socket_accept;
    ret->handshake_func = _socks_tcp_socket_handshake;
    ret->connect_func = _socks_tcp_socket_connect;
    ret->revent_func = _socks_tcp_socket_revent;
    ret->close_func = _socks_tcp_socket_close;
    ret->free_func = _socks_tcp_socket_free;
    ret->target_func = _socks_tcp_socket_target;

    ret->sock = fd;
    ret->addr.proxy = NULL;
    ret->addr.target = NULL;

    ret->socks_vers = socks_vers;

    return ret;
}

socks_tcp_socket_t *
socks_tcp_socket_new(int socks_vers)
{
    fd_t fd = socket_stream(AF_INET);
    ASSERT(fd != -1, "failed to create socket");
    
    socket_set_timeout(fd, 1);
    
    return socks_tcp_socket_new_fd(socks_vers, fd);
}

fd_t
socks_to_socket(socks_tcp_socket_t *sock)
{
    fd_t fd = sock->sock;
    sock->sock = -1;
    tcp_socket_free(sock);
    return fd;
}

#include <pthread.h>

#include "pub/type.h"

#include "proto/socket.h"
#include "proto/buf.h"

#include "tcp.h"
#include "decoding.h"

#define TCP_DEFAULT_BUF 1024
#define RBUF_SIZE 1024

static ssize_t
_vmess_tcp_socket_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    return vbuffer_read(sock->read_buf, buf, size);
}

static ssize_t
_vmess_tcp_socket_write(tcp_socket_t *_sock, const byte_t *buf, size_t size)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    vbuffer_write(sock->write_buf, buf, size);
    return size;
}

static int
_vmess_tcp_socket_bind(tcp_socket_t *_sock, const char *node, const char *port)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;

    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(node, port, &hints, &list)) {
        perror("getaddrinfo");
        return -1;
    }

    if (bind(sock->sock, list->ai_addr, list->ai_addrlen)) {
        perror("bind");
        return -1;
    }

    return 0;
}

static int
_vmess_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;

    if (listen(sock->sock, backlog)) {
        perror("listen");
        return -1;
    }

    return 0;
}

static void
_vmess_tcp_socket_free(vmess_tcp_socket_t *sock)
{
    if (sock) {
        vbuffer_free(sock->read_buf);
        vbuffer_free(sock->write_buf);
        vmess_config_free(sock->config);
        vmess_serial_free(sock->vser);
        target_id_free(sock->target);
        free(sock);
    }
}

static int
_vmess_tcp_socket_close(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;

    vbuffer_close(sock->read_buf);
    vbuffer_close(sock->write_buf);

    if (close(sock->sock)) {
        perror("close");
    }

    if (sock->started) {
        pthread_join(sock->reader, NULL);
        pthread_join(sock->writer, NULL);
    }

    _vmess_tcp_socket_free(sock);

    return 0;
}

static bool
_vmess_tcp_socket_handshake(vmess_tcp_socket_t *sock)
{
    rbuffer_t *rbuf;

    byte_t *trunk;
    size_t size;
    vmess_serial_t *vser = NULL;

    vmess_request_t req;
    vmess_response_t resp;

    rbuf = rbuffer_new(RBUF_SIZE);

    // handshake first
    if (sock->server) {
        // server
        // read request header
        switch (rbuffer_read(rbuf, sock->sock, vmess_request_decoder,
                         VMESS_DECODER_CTX(sock->config, &sock->auth), &req)) {
            case RBUFFER_SUCCESS:
                break;

            case RBUFFER_ERROR:
                goto ERROR1;

            case RBUFFER_INCOMPLETE:
                goto ERROR1;
        }

        // write response header
        resp = (vmess_response_t) {
            .opt = 1
        };

        vser = vmess_serial_new(&sock->auth);
        vmess_serial_response(vser, sock->config, &resp);

        while ((trunk = vmess_serial_digest(vser, &size))) {
            write_r(sock->sock, trunk, size);
            free(trunk);
        }

        vmess_tcp_socket_set_target(sock, req.target);
        vmess_request_destroy(&req);
    } else {
        ASSERT(sock->target, "client: socket target not set");

        // send request
        req = (vmess_request_t) {
            .target = sock->target,
            .vers = 1,
            .crypt = VMESS_CRYPT_AES_128_CFB,
            .cmd = VMESS_REQ_CMD_TCP,
            .opt = 1
        };

        vser = vmess_serial_new(&sock->auth);

        vmess_serial_request(vser, sock->config, &req);

        while ((trunk = vmess_serial_digest(vser, &size))) {
            write_r(sock->sock, trunk, size);
            free(trunk);
        }

        // read response header
        switch (rbuffer_read(rbuf, sock->sock, vmess_response_decoder,
                            VMESS_DECODER_CTX(sock->config, &sock->auth), &resp)) {
            case RBUFFER_SUCCESS:
                break;

            case RBUFFER_ERROR:
                goto ERROR1;

            case RBUFFER_INCOMPLETE:
                goto ERROR1;
        }
    }

    rbuffer_free(rbuf);

    // set serializer for future use
    sock->vser = vser;

    return true;

ERROR1:
    vmess_serial_free(vser);
    rbuffer_free(rbuf);
    return false;
}

static void *
_vmess_tcp_socket_reader(void *arg)
{
    vmess_tcp_socket_t *sock = arg;
    rbuffer_t *rbuf = rbuffer_new(RBUF_SIZE);
    data_trunk_t trunk;
    bool closed = false;

    // read from remote and prepare read_buf

    // printf("reader %p\n", sock);

    socket_set_block(sock->sock, false);

    while (!closed) {
        switch (rbuffer_read(rbuf, sock->sock, vmess_data_decoder,
                             VMESS_DECODER_CTX(sock->config, &sock->auth), &trunk)) {
            case RBUFFER_SUCCESS:
                hexdump("data read", trunk.data, trunk.size);

                if (trunk.size == 0) {
                    // remote sent end signal
                    // no more read is needed
                    closed = true;
                } else {
                    vbuffer_write(sock->read_buf, trunk.data, trunk.size);
                }

                data_trunk_destroy(&trunk);
                break;

            case RBUFFER_ERROR:
            case RBUFFER_INCOMPLETE:
                closed = true;
                break;
        }
    }

    rbuffer_free(rbuf);

    // printf("reader exited %p\n", sock);

    return NULL;
}

static void *
_vmess_tcp_socket_writer(void *arg)
{
    vmess_tcp_socket_t *sock = arg;

    ASSERT(sock->vser, "socket not connected");

    byte_t *trunk;
    byte_t buf[RBUF_SIZE];
    size_t size;

    bool closed = false;

    while (!closed && (size = vbuffer_read(sock->write_buf, buf, RBUF_SIZE))) {
        vmess_serial_write(sock->vser, buf, size);
        
        // flush all
        while ((trunk = vmess_serial_digest(sock->vser, &size))) {
            if (write_r(sock->sock, trunk, size) == -1) {
                closed = true;
            }

            free(trunk);
        }
    }

    // printf("writer exited %p\n", sock);

    // vbuffer closed

    return NULL;
}

static vmess_tcp_socket_t *
_vmess_tcp_socket_new_fd(vmess_config_t *config, int fd);

static tcp_socket_t *
_vmess_tcp_socket_accept(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    vmess_tcp_socket_t *ret;
    int client;

    client = accept(sock->sock, NULL, NULL);

    if (client == -1) {
        perror("accept");
        return NULL;
    }

    ret = _vmess_tcp_socket_new_fd(sock->config, client);
    ret->server = true;
    
    if (!_vmess_tcp_socket_handshake(ret)) {
        _vmess_tcp_socket_close((tcp_socket_t *)ret);
        return NULL;
    }

    pthread_create(&ret->reader, NULL, _vmess_tcp_socket_reader, ret);
    pthread_create(&ret->writer, NULL, _vmess_tcp_socket_writer, ret);

    ret->started = true;

    return (tcp_socket_t *)ret;
}

static int
_vmess_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(node, port, &hints, &list)) {
        perror("getaddrinfo");
        return -1;
    }

    if (connect(sock->sock, list->ai_addr, list->ai_addrlen)) {
        perror("connect");
        return -1;
    }

    sock->server = false;

    if (!_vmess_tcp_socket_handshake(sock)) {
        return -1;
    }

    pthread_create(&sock->reader, NULL, _vmess_tcp_socket_reader, sock);
    pthread_create(&sock->writer, NULL, _vmess_tcp_socket_writer, sock);

    sock->started = true;

    return 0;

    // create universal auth
    // start two threads
    // handshake with the remote(send the request header and wait for response header)
    // 1. read from the write_buf, encode, and send it to server
    //    if write_buf returned 0, exit thread(local closed)
    //    if sending falied, exit thread(remote closed)
    // 2. read from the server, decode, and push back to the read_buf
    //    if read server returned 0, exit thread(remote closed)
    //    if remote returned end packet, exit thread(remote closed)
}

static vmess_tcp_socket_t *
_vmess_tcp_socket_new_fd(vmess_config_t *config, int fd)
{
    vmess_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->sock = fd;

    ret->server = false; // undetermined at this point
    ret->started = false; // if the threads have been started

    ret->config = vmess_config_copy(config);
    ret->target = NULL;
    ret->vser = NULL;

    ret->read_func = _vmess_tcp_socket_read;
    ret->write_func = _vmess_tcp_socket_write;
    ret->bind_func = _vmess_tcp_socket_bind;
    ret->listen_func = _vmess_tcp_socket_listen;
    ret->accept_func = _vmess_tcp_socket_accept;
    ret->connect_func = _vmess_tcp_socket_connect;
    ret->close_func = _vmess_tcp_socket_close;

    ret->read_buf = vbuffer_new(TCP_DEFAULT_BUF);
    ret->write_buf = vbuffer_new(TCP_DEFAULT_BUF);

    return ret;
}

vmess_tcp_socket_t *
vmess_tcp_socket_new(vmess_config_t *config)
{
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT(fd != -1, "failed to create socket");
    return _vmess_tcp_socket_new_fd(config, fd);
}

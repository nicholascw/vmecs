#include "pub/type.h"
#include "pub/thread.h"
#include "pub/socket.h"

#include "proto/buf.h"

#include "tcp.h"
#include "decoding.h"

#define TCP_DEFAULT_BUF 4096
#define RBUF_SIZE 4096

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
    socket_sockaddr_t addr;
    
    if (socket_getsockaddr(node, port, &addr)) {
        return -1;
    }
    
    socket_set_reuse_port(sock->sock);

    return socket_bind(sock->sock, &addr);
}

static int
_vmess_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    return socket_listen(sock->sock, backlog);
}

static void
_vmess_tcp_socket_free(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;

    vbuffer_free(sock->read_buf);
    vbuffer_free(sock->write_buf);
    vmess_config_free(sock->config);
    vmess_serial_free(sock->vser);
    target_id_free(sock->addr.proxy);
    mutex_free(sock->write_mut);
    
    free(sock);
}

static int
_vmess_tcp_socket_close(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;

    // send closind packet

    size_t size;
    const byte_t *trunk = vmess_serial_end(&size);

    // close write and read buffer to
    // prevent further read/write
    vbuffer_close(sock->write_buf);
    vbuffer_close(sock->read_buf);

    // wait until all write buffer is drained
    vbuffer_drain(sock->write_buf);

    // write mutex is here to ensure all data is written
    // before the end chunk is sent
    mutex_lock(sock->write_mut);
    fd_write(sock->sock, trunk, size);
    mutex_unlock(sock->write_mut);

    // TRACE("end written");

    if (sock->started) {
        thread_join(sock->reader);
        thread_join(sock->writer);
    }

    if (close(sock->sock)) {
        perror("close");
    }

    return 0;
}

static bool
_vmess_tcp_socket_handshake(vmess_tcp_socket_t *sock, target_id_t *target)
{
    rbuffer_t *rbuf;

    byte_t *trunk;
    size_t size;
    vmess_serial_t *vser = NULL;

    vmess_request_t req;
    vmess_response_t resp;

    rbuf = rbuffer_new(RBUF_SIZE);

    // handshake first
    if (!target) {
        // server
        // read request header
        switch (rbuffer_read(rbuf, sock->sock, vmess_request_decoder,
                             VMESS_DECODER_CTX(sock->config, &sock->auth), &req)) {
            case RBUFFER_SUCCESS:
                break;

            case RBUFFER_ERROR:
                TRACE("header decoding failed, rejecting connection");
                goto ERROR1;

            case RBUFFER_INCOMPLETE:
                TRACE("incomplete header, rejecting connection");
                goto ERROR1;
        }

        // write response header
        resp = (vmess_response_t) {
            .opt = 1
        };

        vser = vmess_serial_new(&sock->auth);
        vmess_serial_response(vser, sock->config, &resp);

        while ((trunk = vmess_serial_digest(vser, &size))) {
            fd_write(sock->sock, trunk, size);
            free(trunk);
        }

        vmess_tcp_socket_set_target(sock, req.target);
        vmess_request_destroy(&req);
    } else {
        // send request
        req = (vmess_request_t) {
            .target = target,
            .vers = 1,
            .crypt = VMESS_CRYPT_AES_128_CFB,
            .cmd = VMESS_REQ_CMD_TCP,
            .opt = 1
        };

        vser = vmess_serial_new(&sock->auth);

        vmess_serial_request(vser, sock->config, &req);

        while ((trunk = vmess_serial_digest(vser, &size))) {
            fd_write(sock->sock, trunk, size);
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
    bool end = false;

    // read from remote and prepare read_buf

    while (!end) {
        switch (rbuffer_read(rbuf, sock->sock, vmess_data_decoder,
                             VMESS_DECODER_CTX(sock->config, &sock->auth), &trunk)) {
            case RBUFFER_SUCCESS:
                // hexdump("data read", trunk.data, trunk.size);

                if (trunk.size == 0) {
                    // remote sent end signal
                    // no more read is needed
                    end = true;
                } else {
                    vbuffer_write(sock->read_buf, trunk.data, trunk.size);
                }

                data_trunk_destroy(&trunk);
                break;

            case RBUFFER_ERROR:
                TRACE("decoding failed exiting");

            case RBUFFER_INCOMPLETE:
                end = true;
                break;
        }
    }

    TRACE("reader exited %p", (void *)sock);

    rbuffer_free(rbuf);

    vbuffer_close(sock->read_buf);
    vbuffer_close(sock->write_buf);

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
    bool end = false;

    while (1) {
        // TRACE("writer waiting for lock");
        mutex_lock(sock->write_mut);

        size = vbuffer_read(sock->write_buf, buf, RBUF_SIZE);

        // TRACE("writer got new data");
        if (!size) {
            mutex_unlock(sock->write_mut);
            break;
        }

        if (!end) {
            vmess_serial_write(sock->vser, buf, size);
            
            // flush all
            while ((trunk = vmess_serial_digest(sock->vser, &size))) {
                if (fd_write(sock->sock, trunk, size) == -1) {
                    end = true;
                }

                free(trunk);
            }
        }

        mutex_unlock(sock->write_mut);

        // TRACE("chunk written");
    }

    TRACE("writer exited %p", (void *)sock);

    // vbuffer closed

    return NULL;
}

static vmess_tcp_socket_t *
_vmess_tcp_socket_new_fd(vmess_config_t *config, fd_t fd);

static tcp_socket_t *
_vmess_tcp_socket_accept(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    vmess_tcp_socket_t *ret;
    fd_t client;

    client = socket_accept(sock->sock, NULL);

    if (client == -1) {
        return NULL;
    }

    ret = _vmess_tcp_socket_new_fd(sock->config, client);
    
    if (!_vmess_tcp_socket_handshake(ret, NULL)) {
        tcp_socket_close((tcp_socket_t *)ret);
        tcp_socket_free((tcp_socket_t *)ret);
        return NULL;
    }

    ret->reader = thread_new(_vmess_tcp_socket_reader, ret);
    ret->writer = thread_new(_vmess_tcp_socket_writer, ret);

    ret->started = true;

    return (tcp_socket_t *)ret;
}

static int
_vmess_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    socket_sockaddr_t addr;
    target_id_t *target;

    ASSERT(sock->addr.proxy, "proxy server not set");

    if (!target_id_resolve(sock->addr.proxy, &addr))
        return -1;

    if (socket_connect(sock->sock, &addr))
        return -1;

    target = target_id_parse(node, port);

    if (!target) {
        TRACE("invalid node/port");
        return -1;
    }

    if (!_vmess_tcp_socket_handshake(sock, target)) {
        target_id_free(target);
        return -1;
    }

    target_id_free(target);

    sock->reader = thread_new(_vmess_tcp_socket_reader, sock);
    sock->writer = thread_new(_vmess_tcp_socket_writer, sock);

    sock->started = true;

    return 0;
}

static target_id_t *
_vmess_tcp_socket_target(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    ASSERT(sock->addr.target, "vmess target not set");
    return target_id_copy(sock->addr.target);
}

static vmess_tcp_socket_t *
_vmess_tcp_socket_new_fd(vmess_config_t *config, fd_t fd)
{
    vmess_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->sock = fd;

    ret->started = false; // if the threads have been started
    
    ret->config = vmess_config_copy(config);
    ret->addr.proxy = NULL;
    ret->addr.target = NULL;
    ret->vser = NULL;

    ret->write_mut = mutex_new();

    ret->read_func = _vmess_tcp_socket_read;
    ret->write_func = _vmess_tcp_socket_write;
    ret->bind_func = _vmess_tcp_socket_bind;
    ret->listen_func = _vmess_tcp_socket_listen;
    ret->accept_func = _vmess_tcp_socket_accept;
    ret->connect_func = _vmess_tcp_socket_connect;
    ret->close_func = _vmess_tcp_socket_close;
    ret->free_func = _vmess_tcp_socket_free;
    ret->target_func = _vmess_tcp_socket_target;

    ret->read_buf = vbuffer_new(TCP_DEFAULT_BUF);
    ret->write_buf = vbuffer_new(TCP_DEFAULT_BUF);

    return ret;
}

vmess_tcp_socket_t *
vmess_tcp_socket_new(vmess_config_t *config)
{
    fd_t fd = socket_stream(AF_INET);
    ASSERT(fd != -1, "failed to create socket");

    socket_set_timeout(fd, 1);

    return _vmess_tcp_socket_new_fd(config, fd);
}

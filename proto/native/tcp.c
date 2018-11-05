#include "pub/socket.h"

#include "tcp.h"

static ssize_t
_native_tcp_socket_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return fd_read(sock->sock, buf, size);
}

static ssize_t
_native_tcp_socket_write(tcp_socket_t *_sock, const byte_t *buf, size_t size)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return fd_write(sock->sock, buf, size);
}

static int
_native_tcp_socket_bind(tcp_socket_t *_sock, const char *node, const char *port)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    socket_sockaddr_t addr;

    if (socket_getsockaddr(node, port, &addr)) {
        return -1;
    }

    socket_set_reuse_port(sock->sock);

    return socket_bind(sock->sock, &addr);
}

static int
_native_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return socket_listen(sock->sock, backlog);
}

static tcp_socket_t *
_native_tcp_socket_accept(tcp_socket_t *_sock)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    fd_t fd;

    fd = socket_accept(sock->sock, NULL);
    if (fd == -1) return NULL;

    return (tcp_socket_t *)native_tcp_socket_new_fd(fd);
}

static int
_native_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    socket_sockaddr_t addr;

    if (socket_getsockaddr(node, port, &addr))
        return -1;

    return socket_connect(sock->sock, &addr);
}

static int
_native_tcp_socket_close(tcp_socket_t *_sock)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return close(sock->sock);
}

static void
_native_tcp_socket_free(tcp_socket_t *_sock)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    free(sock);
}

native_tcp_socket_t *
native_tcp_socket_new_fd(fd_t fd)
{
    native_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->read_func = _native_tcp_socket_read;
    ret->write_func = _native_tcp_socket_write;
    ret->bind_func = _native_tcp_socket_bind;
    ret->listen_func = _native_tcp_socket_listen;
    ret->accept_func = _native_tcp_socket_accept;
    ret->connect_func = _native_tcp_socket_connect;
    ret->close_func = _native_tcp_socket_close;
    ret->free_func = _native_tcp_socket_free;
    ret->target_func = NULL;

    ret->sock = fd;

    return ret;
}

native_tcp_socket_t *
native_tcp_socket_new()
{
    fd_t fd = socket_stream(AF_INET);
    ASSERT(fd != -1, "failed to create socket");
    
    socket_set_timeout(fd, 1);
    
    return native_tcp_socket_new_fd(fd);
}

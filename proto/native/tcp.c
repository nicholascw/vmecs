#include "proto/socket.h"

#include "tcp.h"

static ssize_t
_native_tcp_socket_read(tcp_socket_t *_sock, byte_t *buf, size_t size)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return read_r(sock->sock, buf, size);
}

static ssize_t
_native_tcp_socket_write(tcp_socket_t *_sock, const byte_t *buf, size_t size)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return write_r(sock->sock, buf, size);
}

static int
_native_tcp_socket_bind(tcp_socket_t *_sock, const char *node, const char *port)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;

    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    if (getaddrinfo(node, port, &hints, &list)) {
        return -1;
    }

    if (bind(sock->sock, list->ai_addr, list->ai_addrlen)) {
        freeaddrinfo(list);
        return -1;
    } else {
        freeaddrinfo(list);
        return 0;
    }
}

static int
_native_tcp_socket_listen(tcp_socket_t *_sock, int backlog)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    return listen(sock->sock, backlog);
}

static tcp_socket_t *
_native_tcp_socket_accept(tcp_socket_t *_sock)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;
    int fd;

    fd = accept(sock->sock, NULL, NULL);
    if (fd == -1) return NULL;

    return (tcp_socket_t *)native_tcp_socket_new_fd(fd);
}

static int
_native_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    native_tcp_socket_t *sock = (native_tcp_socket_t *)_sock;

    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;

    if (getaddrinfo(node, port, &hints, &list)) {
        freeaddrinfo(list);
        return -1;
    }

    if (connect(sock->sock, list->ai_addr, list->ai_addrlen)) {
        freeaddrinfo(list);
        return -1;
    } else {
        freeaddrinfo(list);
        return 0;
    }
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
native_tcp_socket_new_fd(int fd)
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
    int fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT(fd != -1, "failed to create socket");
    
    socket_set_timeout(fd, 1);
    
    return native_tcp_socket_new_fd(fd);
}

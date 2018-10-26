#include "pub/type.h"

#include "proto/socket.h"

#include "tcp.h"

#define TCP_DEFAULT_BUF 1024

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

static int
_vmess_tcp_socket_close(tcp_socket_t *_sock)
{
    vmess_tcp_socket_t *sock = (vmess_tcp_socket_t *)_sock;
    vbuffer_close(sock->read_buf);
    vbuffer_close(sock->write_buf);
}

static tcp_socket_t *
_vmess_tcp_socket_accept(tcp_socket_t *_sock)
{
    
}

static int
_vmess_tcp_socket_connect(tcp_socket_t *_sock, const char *node, const char *port)
{
    // create universal auth
    // start two threads
    // 1. handshake with the remote(send the request header and wait for response header)
    //    read from the write_buf, encode, and send it to server
    //    if write_buf returned 0, exit thread(local closed)
    //    if sending falied, exit thread(remote closed)
    // 2. read from the server, decode, and push back to the read_buf
    //    if read server returned 0, exit thread(remote closed)
    //    if remote returned end packet, exit thread(remote closed)
}

vmess_tcp_socket_t *
vmess_tcp_socket_new()
{
    vmess_tcp_socket_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    ASSERT(ret->sock != -1, "failed to create socket");

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

#include "pub/err.h"
#include "pub/epoll.h"

#include "etcp.h"

#define DEFAULT_BACKLOG 128
#define DEFAULT_BUFFER (8 * 1024)

typedef struct etcp_relay_conn_t_tag {
    fd_t in_fd;
    fd_t out_fd;

    tcp_socket_t *in_sock;
    tcp_socket_t *out_sock; // if outbound == NULL, do accept on inbound instead of copying data from inbound to outbound

    struct etcp_relay_conn_t_tag *pair;

    int nclosed;
} etcp_relay_conn_t;

static etcp_relay_conn_t *
etcp_relay_conn_new(fd_t in_fd, fd_t out_fd, tcp_socket_t *in, tcp_socket_t *out)
{
    etcp_relay_conn_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->in_fd = in_fd;
    ret->out_fd = out_fd;

    ret->in_sock = in;
    ret->out_sock = out;

    ret->pair = NULL;

    ret->nclosed = 0;

    return ret;
}

static void
etcp_relay_conn_link(etcp_relay_conn_t *c1, etcp_relay_conn_t *c2)
{
    c1->pair = c2;
    c2->pair = c1;
}

static void
etcp_relay_conn_free(etcp_relay_conn_t *conn)
{
    free(conn);
}

static void
etcp_remove_conn(epoll_t epfd, etcp_relay_conn_t *conn)
{
    fd_epoll_ctl(epfd, FD_EPOLL_DEL, conn->in_fd, NULL);
    fd_epoll_ctl(epfd, FD_EPOLL_DEL, conn->out_fd, NULL);

    tcp_socket_close(conn->in_sock);
    tcp_socket_close(conn->out_sock);

    tcp_socket_free(conn->in_sock);
    tcp_socket_free(conn->out_sock);

    etcp_relay_conn_free(conn->pair);
    etcp_relay_conn_free(conn);
}

static void
etcp_handle(epoll_t epfd, tcp_outbound_t *outbound, etcp_relay_conn_t *conn)
{
    tcp_socket_t *new_in, *new_out;
    etcp_relay_conn_t *conn1, *conn2;
    target_id_t *target;

    fd_t in_fd, out_fd;

    epoll_event_t event;

    byte_t *buf;
    ssize_t res;

    if (!conn->out_sock) {
        // server
        // TRACE("accepting");

        new_in = tcp_socket_accept(conn->in_sock);

        if (new_in) {
            if (tcp_socket_handshake(new_in)) {
                tcp_socket_close(new_in);
                tcp_socket_free(new_in);

                TRACE("handshake failed");

                return;
            }

            target = tcp_socket_target(new_in);
            print_target("request", target);

            if (!(new_out = tcp_outbound_client(outbound, target))) {
                // failed to connect
                tcp_socket_close(new_in);
                tcp_socket_free(new_in);
                target_id_free(target);
                return;
            }

            target_id_free(target);
        
            in_fd = tcp_socket_revent(new_in);
            out_fd = tcp_socket_revent(new_out);

            conn1 = etcp_relay_conn_new(in_fd, out_fd, new_in, new_out);
            conn2 = etcp_relay_conn_new(out_fd, in_fd, new_out, new_in); // reversed

            etcp_relay_conn_link(conn1, conn2);

            event = (epoll_event_t) {
                .events = FD_EPOLL_READ | FD_EPOLL_ET,
                .data = {
                    .ptr = conn1
                }
            };

            fd_epoll_ctl(epfd, FD_EPOLL_ADD, in_fd, &event);

            event = (epoll_event_t) {
                .events = FD_EPOLL_READ | FD_EPOLL_ET,
                .data = {
                    .ptr = conn2
                }
            };

            fd_epoll_ctl(epfd, FD_EPOLL_ADD, out_fd, &event);
        }
    } else {
        // pipe

        // TRACE("piping");

        buf = malloc(DEFAULT_BUFFER);
        ASSERT(buf, "out of mem");

        while (1) {
            res = tcp_socket_try_read(conn->in_sock, buf, DEFAULT_BUFFER);

            // TRACE("read %ld", res);

            if (res > 0) {
                if (tcp_socket_write(conn->out_sock, buf, res) <= 0) {
                    // TRACE("write failed");
                    etcp_remove_conn(epfd, conn);
                    break;
                }
            } else if (res == 0) {
                // in_sock is closed

                // if (++conn->nclosed == 2) {
                // both sides are closed
                // TRACE("closing 2");
                etcp_remove_conn(epfd, conn);
                // }

                break;
            } else if (res == -1) {
                // error, close connection
                // TRACE("closing");
                etcp_remove_conn(epfd, conn);
                break;
            } else if (res == -2) {
                // no more data to read, finish handler
                break;
            }
        }

        free(buf);
    }
}

void
tcp_relay_epoll(tcp_relay_config_t *config,
                tcp_inbound_t *inbound,
                tcp_outbound_t *outbound)
{
    tcp_socket_t *server;
    fd_t server_fd;

    epoll_t epfd;
    epoll_event_t event;

    int res;

    TRACE("relay started");

    server = tcp_inbound_server(inbound);

    tcp_socket_listen(server, DEFAULT_BACKLOG);

    TRACE("relay started listening");

    server_fd = tcp_socket_revent(server);

    // set up epoll
    epfd = fd_epoll_create();

    event = (epoll_event_t) {
        .events = FD_EPOLL_READ,
        .data = {
            .ptr = etcp_relay_conn_new(server_fd, -1, server, NULL)
        }
    };

    fd_epoll_ctl(epfd, FD_EPOLL_ADD, server_fd, &event);

    while (1) {
        res = fd_epoll_wait(epfd, &event, 1, -1);

        if (res > 0) {
            // TRACE("handle start");
            etcp_handle(epfd, outbound, event.data.ptr);
            // TRACE("handle end");
        } else if (res == -1) {
            perror("epoll_wait");
        }
    }
}

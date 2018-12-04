#include "pub/err.h"
#include "pub/epoll.h"
#include "pub/thread.h"

#include "etcp.h"

#define DEFAULT_BACKLOG 128
#define DEFAULT_BUFFER (8 * 1024)

#define EPOLL_TIMEOUT 1

typedef struct etcp_relay_conn_t_tag {
    fd_t in_fd;
    fd_t out_fd;

    tcp_socket_t *in_sock;
    tcp_socket_t *out_sock; // if outbound == NULL, do accept on inbound instead of copying data from inbound to outbound

    struct etcp_relay_conn_t_tag *pair;

    int nclosed;
} etcp_relay_conn_t;

typedef struct {
    epoll_t epfd;
    tcp_outbound_t *outbound;
    size_t id;
    bool stop;
} etcp_job_t;

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

    if (!conn->nclosed) {
        tcp_socket_close(conn->in_sock);
        tcp_socket_close(conn->out_sock);
    } // else already closed

    tcp_socket_free(conn->in_sock);
    tcp_socket_free(conn->out_sock);

    etcp_relay_conn_free(conn->pair);
    etcp_relay_conn_free(conn);
}

static void
etcp_handle(epoll_t epfd, tcp_outbound_t *outbound, etcp_relay_conn_t *conn, size_t id)
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
            fprintf(stderr, "thread %ld: ", id);
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
                .events = FD_EPOLL_READ | FD_EPOLL_RDHUP | FD_EPOLL_ET,
                .data = {
                    .ptr = conn1
                }
            };

            fd_epoll_ctl(epfd, FD_EPOLL_ADD, in_fd, &event);

            event = (epoll_event_t) {
                .events = FD_EPOLL_READ | FD_EPOLL_RDHUP | FD_EPOLL_ET,
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

        do {
            res = tcp_socket_try_read(conn->in_sock, buf, DEFAULT_BUFFER);
            // TRACE("read %ld", res);
        } while (res > 0 && tcp_socket_write(conn->out_sock, buf, res) > 0);

        free(buf);

        switch (res) {
            case 0:
                // read end cloesd writing
                // shutdown write to out_sock

                tcp_socket_close(conn->out_sock);

                conn->nclosed++;
                conn->pair->nclosed++;

                if (conn->nclosed == 2) {
                    TRACE("conn closed %p %p", (void *)conn, (void *)conn->pair);
                    etcp_remove_conn(epfd, conn);
                } else {
                    TRACE("conn in sock write closed %p", (void *)conn);
                }

                break;

            case -2: break; // EAGAIN

            case -1:
            default:
                // res > 0, but write failed
                etcp_remove_conn(epfd, conn);
        }
    }
}

// every thread has its own epoll instance
static void *
etcp_worker(void *arg)
{
    etcp_job_t *job = arg;
    epoll_event_t event;

    int res;

    while (!job->stop) {
        res = fd_epoll_wait(job->epfd, &event, 1, -1);

        if (res > 0) {
            // TRACE("thread %ld", job->id);
            etcp_handle(job->epfd, job->outbound, event.data.ptr, job->id);
            // TRACE("handle end");
        } else if (res == -1) {
            perror("epoll_wait");
        }
    }

    return NULL;
}

void
tcp_relay_epoll(tcp_relay_config_t *config,
                tcp_inbound_t *inbound,
                tcp_outbound_t *outbound,
                size_t nthread)
{
    etcp_job_t *jobs;
    thread_t *threads;
    tcp_socket_t **servers;

    size_t i;
    fd_t server_fd;
    epoll_t epfd;
    epoll_event_t event;

    // create threads
    jobs = malloc(sizeof(*jobs) * nthread);
    threads = malloc(sizeof(*threads) * nthread);
    servers = malloc(sizeof(*servers) * nthread);

    ASSERT(jobs, "out of mem");
    ASSERT(threads, "out of mem");
    ASSERT(servers, "out of mem");

    for (i = 0; i < nthread; i++) {
        TRACE("starting thread %ld", i);

        servers[i] = tcp_inbound_server(inbound);

        ASSERT(servers[i], "failed to start server");

        tcp_socket_listen(servers[i], DEFAULT_BACKLOG);

        server_fd = tcp_socket_revent(servers[i]);

        epfd = fd_epoll_create();
        ASSERT(epfd != -1, "failed to create epoll instance");

        event = (epoll_event_t) {
            .events = FD_EPOLL_READ,
            .data = {
                .ptr = etcp_relay_conn_new(server_fd, -1, servers[i], NULL)
            }
        };

        fd_epoll_ctl(epfd, FD_EPOLL_ADD, server_fd, &event);

        jobs[i] = (etcp_job_t) {
            .epfd = epfd,
            .outbound = outbound,
            .id = i,
            .stop = false
        };

        threads[i] = thread_new(etcp_worker, jobs + i);
    }

    for (i = 0; i < nthread; i++) {
        thread_join(threads[i]);

        tcp_socket_close(servers[i]);
        tcp_socket_free(servers[i]);
    }

    free(jobs);
    free(threads);

    return;
}

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

#include <time.h>
#include <pthread.h>

#include "tcp.h"

#define DEFAULT_BACKLOG 10
#define DEFAULT_BUF 1024

typedef struct {
    tcp_socket_t *in_sock;
    tcp_socket_t *out_sock;
    tcp_outbound_t *outbound;
} tcp_router_job_t;

static void *
_tcp_router_reader(void *arg)
{
    tcp_router_job_t *job = arg;
    byte_t buf[DEFAULT_BUF];
    ssize_t n_read;
    ssize_t n_write;

    // read out_sock
    // write in_sock

    while ((n_read = tcp_socket_read(job->out_sock, buf, sizeof(buf))) &&
           n_read != -1) {
        // printf("reader got data\n");
        if ((n_write = tcp_socket_write(job->in_sock, buf, n_read)) &&
            n_write != -1) {
            hexdump("router reader routed", buf, n_read);
        } else {
            // write error
            break;
        }
    }

    printf("router reader closed\n");
    tcp_socket_close(job->in_sock);
    printf("router reader closed 2\n"); 

    return NULL;
}

static void *
_tcp_router_writer(void *arg)
{
    tcp_router_job_t *job = arg;
    byte_t buf[DEFAULT_BUF];
    ssize_t n_read;
    ssize_t n_write;

    // read in_sock
    // write out_sock

    while ((n_read = tcp_socket_read(job->in_sock, buf, sizeof(buf))) &&
           n_read != -1) {
        // printf("writer got data\n");
        if ((n_write = tcp_socket_write(job->out_sock, buf, n_read)) &&
            n_write != -1) {
            hexdump("router writer routed", buf, n_read);
        } else {
            // write error
            break;
        }
    }

    printf("router writer closed\n");
    tcp_socket_close(job->out_sock);
    printf("router writer closed 2\n");
    
    return NULL;
}

static void *
_tcp_router_handler(void *arg)
{
    tcp_router_job_t *job = arg;
    target_id_t *target = tcp_socket_target(job->in_sock);

    pthread_t reader, writer;

    print_target(target);

    while (!(job->out_sock = tcp_outbound_client(job->outbound, target))) {
        perror("connect");
        sleep(1);
    }

    target_id_free(target);

    pthread_create(&reader, NULL, _tcp_router_reader, job);
    pthread_create(&writer, NULL, _tcp_router_writer, job);

    pthread_join(reader, NULL);
    pthread_join(writer, NULL);

    printf("reader/writer joined\n");

    tcp_socket_free(job->in_sock);
    tcp_socket_free(job->out_sock);
    
    free(job);

    printf("connection closed\n");

    return NULL;
}

void
tcp_router(tcp_inbound_t *inbound, tcp_outbound_t *outbound)
{
    tcp_socket_t *server, *client;
    pthread_t tid;
    tcp_router_job_t *job;

    printf("router started\n");

    server = tcp_inbound_server(inbound);

    tcp_socket_listen(server, DEFAULT_BACKLOG);

    printf("router started listening\n");

    while (1) {
        client = tcp_socket_accept(server);

        if (!client) continue;

        printf("router accepted client %p\n", (void *)client);

        job = malloc(sizeof(*job));
        ASSERT(job, "out of mem");
        job->in_sock = client;
        job->out_sock = NULL;
        job->outbound = outbound;

        pthread_create(&tid, NULL, _tcp_router_handler, job);
        pthread_detach(tid);
    }
}

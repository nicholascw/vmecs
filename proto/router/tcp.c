#include <time.h>
#include <pthread.h>

#include "pub/err.h"

#include "tcp.h"

#define DEFAULT_BACKLOG 128
#define DEFAULT_BUFFER (8 * 1024)

tcp_router_config_t *
tcp_router_config_new_default()
{
    tcp_router_config_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->max_connect_retry = TCP_ROUTER_DEFAULT_MAX_CONNECT_RETRY;

    return ret;
}

void
tcp_router_config_free(tcp_router_config_t *config)
{
    free(config);
}

tcp_router_config_t *
tcp_router_config_copy(tcp_router_config_t *config)
{
    tcp_router_config_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    memcpy(ret, config, sizeof(*ret));

    return ret;
}

typedef struct {
    tcp_socket_t *in_sock;
    tcp_socket_t *out_sock;
    tcp_outbound_t *outbound;

    tcp_router_config_t *config;
} tcp_router_job_t;

static void *
_tcp_router_reader(void *arg)
{
    tcp_router_job_t *job = arg;
    byte_t *buf;
    ssize_t n_read;
    ssize_t n_write;

    // read out_sock
    // write in_sock

    buf = malloc(DEFAULT_BUFFER);
    ASSERT(buf, "out of mem");

    while ((n_read = tcp_socket_read(job->out_sock, buf, sizeof(buf))) &&
           n_read != -1) {
        // TRACE("reader got data");
        if ((n_write = tcp_socket_write(job->in_sock, buf, n_read)) &&
            n_write != -1) {
            TRACE("inbound <- outbound");
            // hexdump("outbound -> inbound", buf, n_read);
        } else {
            // write error
            break;
        }
    }

    TRACE("router reader closing");
    tcp_socket_close(job->in_sock);
    TRACE("router reader closed"); 

    free(buf);

    return NULL;
}

static void *
_tcp_router_writer(void *arg)
{
    tcp_router_job_t *job = arg;
    byte_t *buf;
    ssize_t n_read;
    ssize_t n_write;

    // read in_sock
    // write out_sock

    buf = malloc(DEFAULT_BUFFER);
    ASSERT(buf, "out of mem");

    while ((n_read = tcp_socket_read(job->in_sock, buf, sizeof(buf))) &&
           n_read != -1) {
        // TRACE("writer got data");
        if ((n_write = tcp_socket_write(job->out_sock, buf, n_read)) &&
            n_write != -1) {
            TRACE("inbound -> outbound");
            // hexdump("inbound -> outbound", buf, n_read);
        } else {
            // write error
            break;
        }
    }

    TRACE("router writer closing");
    tcp_socket_close(job->out_sock);
    TRACE("router writer closed");

    free(buf);
    
    return NULL;
}

static void
_tcp_router_job_free(tcp_router_job_t *job)
{
    if (job) {
        tcp_router_config_free(job->config);
        free(job);
    }
}

static void *
_tcp_router_handler(void *arg)
{
    tcp_router_job_t *job = arg;
    target_id_t *target;

    pthread_t reader, writer;
    int retry = 0;

    target = tcp_socket_target(job->in_sock);
    print_target(target);

    while (!(job->out_sock = tcp_outbound_client(job->outbound, target))) {
        perror("connect");
        sleep(1);
        retry++;

        if (retry >= job->config->max_connect_retry) {
            // retry failed, clean up an go

            tcp_socket_close(job->in_sock);
            tcp_socket_free(job->in_sock);
            
            target_id_free(target);
            _tcp_router_job_free(job);
            
            TRACE("connection failed");

            return NULL;
        }
    }

    pthread_create(&reader, NULL, _tcp_router_reader, job);
    pthread_create(&writer, NULL, _tcp_router_writer, job);

    pthread_join(reader, NULL);
    pthread_join(writer, NULL);

    TRACE("reader/writer joined");

    tcp_socket_free(job->in_sock);
    tcp_socket_free(job->out_sock);
    
    target_id_free(target);
    _tcp_router_job_free(job);

    TRACE("connection closed");

    return NULL;
}

void
tcp_router(tcp_router_config_t *config,
           tcp_inbound_t *inbound,
           tcp_outbound_t *outbound)
{
    tcp_socket_t *server, *client;
    pthread_t tid;
    tcp_router_job_t *job;

    TRACE("router started");

    server = tcp_inbound_server(inbound);

    tcp_socket_listen(server, DEFAULT_BACKLOG);

    TRACE("router started listening");

    while (1) {
        client = tcp_socket_accept(server);

        if (!client) continue;

        TRACE("router accepted client %p", (void *)client);

        job = malloc(sizeof(*job));
        ASSERT(job, "out of mem");
        job->in_sock = client;
        job->out_sock = NULL;
        job->outbound = outbound;
        job->config = tcp_router_config_copy(config);

        pthread_create(&tid, NULL, _tcp_router_handler, job);
        pthread_detach(tid);
    }
}

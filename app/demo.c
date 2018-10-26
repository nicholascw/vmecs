#include <stdio.h>
#include <pthread.h>
#include <time.h>

#include "proto/vmess/vmess.h"
#include "proto/vmess/decoding.h"
#include "proto/buf.h"
#include "proto/socket.h"

#define PORT "3137"

void hexdump(char *desc, void *addr, int len);

void server_proc(int fd)
{
    hash128_t user_id = {0};
    vmess_config_t *config = vmess_config_new(user_id);
    vmess_state_t *state = vmess_state_new();

    vmess_auth_t auth;
    vmess_request_t req;
    vmess_response_t resp;

    vmess_serial_t *vser;

    rbuffer_t *rbuf;
    data_trunk_t trunk;
    bool end = false;

    rbuf = rbuffer_new(1024);

    switch (rbuffer_read(rbuf, fd, vmess_request_decoder,
                         VMESS_DECODER_CTX(config, &auth), &req)) {
        case RBUFFER_SUCCESS:
            break;

        case RBUFFER_ERROR:
            printf("invalid header\n");
            goto ERROR1;

        case RBUFFER_INCOMPLETE:
            printf("incomplete header\n");
            goto ERROR1;
    }

    while (!end) {
        switch (rbuffer_read(rbuf, fd, vmess_data_decoder,
                             VMESS_DECODER_CTX(config, &auth), &trunk)) {
            case RBUFFER_SUCCESS:
                hexdump("data read", trunk.data, trunk.size);

                if (trunk.size == 0) end = true;
                data_trunk_destroy(&trunk);
                break;

            case RBUFFER_ERROR:
                printf("data read error\n");
                end = true;
                break;

            case RBUFFER_INCOMPLETE:
                printf("incomplete data\n");
                end = true;
                break;
        }
    }

    {
        byte_t *trunk;
        size_t size;

        // respond
        resp = (vmess_response_t) {
            .opt = 1
        };

        vser = vmess_serial_new(&auth);
        vmess_serial_response(vser, config, &resp);
        vmess_serial_write(vser, "yes?", 4);

        while ((trunk = vmess_serial_digest(vser, &size))) {
            printf("respond data with trunk of size %lu\n", size);
            write(fd, trunk, size);
            free(trunk);
        }

        trunk = vmess_serial_end(&size);
        write(fd, trunk, size);
        vmess_serial_free(vser);
    }

ERROR1:

    close(fd);
    vmess_config_free(config);
    vmess_state_free(state);
    rbuffer_free(rbuf);

    return;
}

void server()
{
    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_flags = AI_PASSIVE;
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(NULL, PORT, &hints, &list)) {
        perror("getaddrinfo");
        return;
    }

    int fd = socket(AF_INET, SOCK_STREAM, hints.ai_protocol);
    int cli;

    if (fd == -1) {
        perror("socket");
        return;
    }

    while (bind(fd, list->ai_addr, list->ai_addrlen)) {
        perror("bind");
        sleep(1);
    }

    while (listen(fd, 5)) {
        perror("listen");
        sleep(1);
    }

    printf("server start listening\n");

    while (1) {
        cli = accept(fd, NULL, NULL);

        printf("server accepted\n");
        server_proc(cli);
        printf("client disconnected\n");
        
        break;
    }

    close(fd);
}

void client()
{
    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", PORT, &hints, &list)) {
        perror("getaddrinfo");
        return;
    }

    int fd = socket(AF_INET, SOCK_STREAM, hints.ai_protocol);

    if (fd == -1) {
        perror("socket");
        return;
    }

    while (connect(fd, list->ai_addr, list->ai_addrlen)) {
        perror("connect");
        sleep(1);
    }

    printf("client connected\n");

    // init vmess connection
    hash128_t user_id = {0};
    vmess_config_t *config = vmess_config_new(user_id);
    vmess_state_t *state = vmess_state_new();

    vmess_auth_t auth;
    vmess_request_t req;
    vmess_response_t resp;

    vmess_serial_t *vser;
    target_id_t *target;

    byte_t *trunk;
    size_t size;

    // init auth
    vmess_auth_init(&auth, config, time(NULL));
    vmess_auth_set_nonce(&auth, vmess_state_next_nonce(state));

    // init serializer
    vser = vmess_serial_new(&auth);

    // init request
    target = target_id_new_ipv4((byte_t[]){ 127, 0, 0, 1 }, 3151);
    req = (vmess_request_t) {
        .target = target,
        .vers = 1,
        .crypt = VMESS_CRYPT_AES_128_CFB,
        .cmd = VMESS_REQ_CMD_TCP,
        .opt = 1
    };

    vmess_serial_request(vser, config, &req);
    vmess_serial_write(vser, (byte_t *)"hello", 5);

    while ((trunk = vmess_serial_digest(vser, &size))) {
        printf("write trunk of size %lu\n", size);
        write(fd, trunk, size);
        free(trunk);
    }

    vmess_serial_write(vser, (byte_t *)"hi", 2);

    while ((trunk = vmess_serial_digest(vser, &size))) {
        printf("write trunk of size %lu\n", size);
        write(fd, trunk, size);
        free(trunk);
    }

    // write the end data trunk
    trunk = vmess_serial_end(&size);
    write(fd, trunk, size);

    {
        rbuffer_t *rbuf;
        data_trunk_t trunk;
        bool end = false;

        rbuf = rbuffer_new(1024);

        switch (rbuffer_read(rbuf, fd, vmess_response_decoder,
                            VMESS_DECODER_CTX(config, &auth), &resp)) {
            case RBUFFER_SUCCESS:
                break;

            case RBUFFER_ERROR:
                printf("invalid response header\n");
                goto ERROR1;

            case RBUFFER_INCOMPLETE:
                printf("incomplete response header\n");
                goto ERROR1;
        }

        while (!end) {
            switch (rbuffer_read(rbuf, fd, vmess_data_decoder,
                                    VMESS_DECODER_CTX(config, &auth), &trunk)) {
                case RBUFFER_SUCCESS:
                    hexdump("data read", trunk.data, trunk.size);

                    if (trunk.size == 0) end = true;
                    data_trunk_destroy(&trunk);
                    break;

                case RBUFFER_ERROR:
                    printf("data read error\n");
                    end = true;
                    break;

                case RBUFFER_INCOMPLETE:
                    printf("incomplete data\n");
                    end = true;
                    break;
            }
        }

    ERROR1:;

        rbuffer_free(rbuf);
    }

    close(fd);

    vmess_config_free(config);
    vmess_state_free(state);
    vmess_serial_free(vser);

    printf("client finished\n");
}

void *run(void *f)
{
    ((void (*)())f)();
    return NULL;
}

void test_vmess();

// pattern
// loop:
// read some data
// try to decode
//     -> need more -> goto loop
//     -> error -> end connection

int main()
{
    // test_vmess_request();
    // test_vmess_response();

    pthread_t client_tid, server_tid;
    
    pthread_create(&client_tid, NULL, run, client);
    pthread_create(&server_tid, NULL, run, server);

    pthread_join(client_tid, NULL);
    pthread_join(server_tid, NULL);

    return 0;
}

#include <stdio.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <arpa/inet.h>

#include "proto/vmess/vmess.h"

void server()
{
    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", "3131", &hints, &list)) {
        perror("getaddrinfo");
        return;
    }

    int fd = socket(AF_INET, SOCK_STREAM, hints.ai_protocol);
    int cli;

    byte_t buf[3];

    if (fd == -1) {
        perror("socket");
        return;
    }

    bind(fd, list->ai_addr, list->ai_addrlen);
    listen(fd, 5);

    printf("server start listening\n");

    while (1) {
        cli = accept(fd, NULL, NULL);

        printf("server accepted");

        read(cli, buf, 2);
        buf[3] = 0;

        printf("%s\n", buf);

        close(cli);
    }
}

void client()
{
    struct addrinfo hints, *list;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo("127.0.0.1", "3131", &hints, &list)) {
        perror("getaddrinfo");
        return;
    }

    int fd = socket(AF_INET, SOCK_STREAM, hints.ai_protocol);

    if (fd == -1) {
        perror("socket");
        return;
    }

    sleep(1);

    if (connect(fd, list->ai_addr, list->ai_addrlen)) {
        perror("connect");
        return;
    }

    printf("client connected\n");

    write(fd, "hi", 2);

    close(fd);

    printf("client finished\n");
}

void *run(void *f)
{
    ((void (*)())f)();
    return NULL;
}

void test_vmess();

int main()
{
    // pthread_t client_tid, server_tid;

    test_vmess();

    // pthread_create(&client_tid, NULL, run, client);
    // pthread_create(&server_tid, NULL, run, server);

    // pthread_exit(0);

    return 0;
}

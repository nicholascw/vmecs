#ifndef _PROTO_COMMON_H_
#define _PROTO_COMMON_H_

#include <string.h>

#include "pub/err.h"
#include "pub/type.h"
#include "socket.h"

enum {
    ADDR_TYPE_IPV4 = 1,
    ADDR_TYPE_DOMAIN = 2,
    ADDR_TYPE_IPV6 = 3
};

typedef enum {
    PROTO_UDP,
    PROTO_TCP
} proto_type_t;

typedef struct {
    union {
        uint8_t ipv4[4];
        char *domain;
        uint8_t ipv6[16];
    } addr;

    uint16_t port; // stored in local endianness

    byte_t addr_type;
} target_id_t;

target_id_t *target_id_new_ipv4(uint8_t addr[4], uint16_t port);
target_id_t *target_id_new_ipv6(uint8_t addr[16], uint16_t port);
target_id_t *target_id_new_domain(const char *domain, uint64_t port);

target_id_t *target_id_parse(const char *node, const char *service);

INLINE target_id_t *target_id_copy(const target_id_t *target)
{
    target_id_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    memcpy(ret, target, sizeof(*ret));
    
    if (ret->addr_type == ADDR_TYPE_DOMAIN) {
        ret->addr.domain = strdup(ret->addr.domain);
    }

    return ret;
}

struct addrinfo *target_id_resolve(const target_id_t *target);
void target_id_free(target_id_t *target);

#define TARGET_ID_MAX_DOMAIN 256
#define TARGET_ID_MAX_PORT 8

INLINE void
target_id_node(const target_id_t *target, char buf[TARGET_ID_MAX_DOMAIN + 1])
{
    switch (target->addr_type) {
        case ADDR_TYPE_DOMAIN:
            sprintf(buf, "%s", target->addr.domain);
            break;

        case ADDR_TYPE_IPV4:
            sprintf(buf, "%d.%d.%d.%d",
                    target->addr.ipv4[0], target->addr.ipv4[1],
                    target->addr.ipv4[2], target->addr.ipv4[3]);
            break;

        case ADDR_TYPE_IPV6:
            ASSERT(0, "not implemented");
            break;
    }
}

INLINE void
target_id_port(const target_id_t *target, char buf[TARGET_ID_MAX_PORT + 1])
{
    sprintf(buf, "%d", target->port);
}

INLINE void
print_target(const target_id_t *target)
{
    switch (target->addr_type) {
        case ADDR_TYPE_IPV4:
            printf("%d.%d.%d.%d:%d\n",
                   target->addr.ipv4[0], target->addr.ipv4[1],
                   target->addr.ipv4[2], target->addr.ipv4[3],
                   target->port);

            break;

        case ADDR_TYPE_DOMAIN:
            printf("%s:%d\n",
                   target->addr.domain, target->port);
            break;

        case ADDR_TYPE_IPV6:
            ASSERT(0, "unimplemented");
    }
}

typedef struct {
    target_id_t *target;
    proto_type_t type;

    size_t data_len;
    byte_t *data;
} packet_t;

typedef struct {
    byte_t *data;
    size_t size;
} data_trunk_t;

typedef ssize_t (*decoder_t)(void *context, void *result, const byte_t *dat, size_t dat_size);

INLINE void
data_trunk_destroy(data_trunk_t *trunk)
{
    if (trunk) {
        free(trunk->data);
        trunk->data = NULL;
        trunk->size = 0;
    }
}

#endif

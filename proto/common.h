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
    byte_t domain_len; // used only if addr type == ADDR_TYPE_DOMAIN
} target_id_t;

target_id_t *target_id_new_ipv4(uint8_t addr[4], uint16_t port);
target_id_t *target_id_new_ipv6(uint8_t addr[16], uint16_t port);
target_id_t *target_id_new_domain(const char *domain, uint64_t port);

void target_id_free(target_id_t *target);

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

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

INLINE char *strdup(const char *str)
{
    size_t len = strlen(str);
    char *ret = malloc(len + 1);
    ASSERT(ret, "out of mem");
    memcpy(ret, str, len + 1);
    return ret;
}

#endif

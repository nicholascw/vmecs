#include <string.h>
#include <stdlib.h>

#include "pub/err.h"

#include "common.h"

target_id_t *_target_id_new(byte_t type, uint16_t port)
{
    target_id_t *id = malloc(sizeof(*id));
    ASSERT(id, "out of mem");

    id->port = port;
    id->addr_type = type;

    return id;
}

target_id_t *target_id_new_ipv4(uint8_t addr[4], uint16_t port)
{
    target_id_t *id = _target_id_new(ADDR_TYPE_IPV4, port);
    memcpy(id->addr.ipv4, addr, sizeof(id->addr.ipv4));
    return id;
}

target_id_t *target_id_new_ipv6(uint8_t addr[16], uint16_t port)
{
    target_id_t *id = _target_id_new(ADDR_TYPE_IPV6, port);
    memcpy(id->addr.ipv6, addr, sizeof(id->addr.ipv6));
    return id;
}

target_id_t *target_id_new_domain(const char *domain, uint64_t port)
{
    target_id_t *id = _target_id_new(ADDR_TYPE_DOMAIN, port);
    id->addr.domain = strdup(domain);
    return id;
}

void target_id_free(target_id_t *target)
{
    if (target) {
        if (target->addr_type == ADDR_TYPE_DOMAIN) {
            free(target->addr.domain);
        }

        free(target);
    }
}

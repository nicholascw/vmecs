#include <string.h>

#include "pub/err.h"

#include "common.h"

target_id_t *target_id_new_ipv4(uint8_t addr[4], uint16_t port)
{
    target_id_t *id = malloc(sizeof(*id));
    ASSERT(id, "out of mem");

    memcpy(id->addr.ipv4, addr, sizeof(id->addr.ipv4));
    id->port = port;
    id->addr_type = ADDR_TYPE_IPV4;

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

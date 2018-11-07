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

target_id_t *target_id_new_ipv4(const uint8_t addr[4], uint16_t port)
{
    target_id_t *id = _target_id_new(ADDR_TYPE_IPV4, port);
    memcpy(id->addr.ipv4, addr, sizeof(id->addr.ipv4));
    return id;
}

target_id_t *target_id_new_ipv6(const uint8_t addr[16], uint16_t port)
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

target_id_t *target_id_parse(const char *node, const char *service)
{
    struct in_addr ipv4;
    struct in6_addr ipv6;

    int port;

    if (!sscanf(service, "%d", &port)) {
        return NULL;
    }

    if (inet_pton(AF_INET, node, &ipv4)) {
        return target_id_new_ipv4((uint8_t *)&ipv4, port);
    }

    if (inet_pton(AF_INET6, node, &ipv6)) {
        return target_id_new_ipv6((uint8_t *)&ipv6, port);
    }

    return target_id_new_domain(node, port);
}

bool target_id_resolve(const target_id_t *target, socket_sockaddr_t *addr)
{
    char port[TARGET_ID_MAX_PORT];
    
    switch (target->addr_type) {
        case ADDR_TYPE_IPV4:
            socket_sockaddr_ipv4(target->addr.ipv4, target->port, addr);
            return true;

        case ADDR_TYPE_DOMAIN:
            target_id_port(target, port);
            return socket_getsockaddr(target->addr.domain, port, addr) == 0;

        case ADDR_TYPE_IPV6:
            socket_sockaddr_ipv6(target->addr.ipv6, target->port, addr);
            return true;
    }

    ASSERT(0, "unsupported address type");
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

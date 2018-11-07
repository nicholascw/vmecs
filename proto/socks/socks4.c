#include "pub/type.h"

#include "proto/common.h"
#include "socks4.h"

byte_t *
socks4_encode_request(const socks4_request_t *req, size_t *size_p)
{
    byte_t *ret = malloc(1 + 1 + 2 + 4 + 1);
    ASSERT(ret, "out of mem");

    ret[0] = SOCKS4_VERSION;
    ret[1] = req->cmd;

    ASSERT(req->target && req->target->addr_type == ADDR_TYPE_IPV4, "invalid target id(socks4 only supports ipv4)");

    *(uint16_t *)(ret + 2) = be16(req->target->port);
    memcpy(ret + 4, &req->target->addr.ipv4, sizeof(req->target->addr.ipv4));

    ret[8] = 0;

    if (size_p) *size_p = 9;

    return ret;
}

byte_t *
socks4_encode_response(const socks4_response_t *resp, size_t *size_p)
{
    byte_t *ret = malloc(8);
    ASSERT(ret, "out of mem");

    memset(ret, 0, 8);

    ret[0] = 0;
    ret[1] = resp->rep;

    if (size_p) *size_p = 8;

    return ret;
}

ssize_t
socks4_decode_request(socks4_request_t *req,
                      const byte_t *data, size_t size)
{
    uint16_t port;
    target_id_t *target;
    size_t i;

    if (size < 9) {
        return 0;
    }

    if (data[0] != SOCKS4_VERSION) {
        TRACE("wrong socks4 version %d", data[0]);
        return -1;
    }

    req->cmd = data[1];

    port = from_be16(*(uint16_t *)(data + 2));
    target = target_id_new_ipv4(data + 4, port);

    for (i = 8; i < size; i++) {
        if (data[i] == 0) {
            // found null
            req->target = target;
            return i + 1;
        }
    }

    target_id_free(target);

    return 0;
}

ssize_t
socks4_decode_response(socks4_response_t *resp,
                       const byte_t *data, size_t size)
{
    if (size < 8) {
        return 0;
    }

    if (data[0] != 0) {
        TRACE("wrong socks4 reply version %d", data[0]);
        return -1;
    }

    resp->rep = data[1];
    return 8;
}

ssize_t
socks4_request_decoder(void *ctx, void *result,
                       const byte_t *data, size_t size)
{
    return socks4_decode_request(result, data, size);
}

ssize_t
socks4_response_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size)
{
    return socks4_decode_response(result, data, size);
}

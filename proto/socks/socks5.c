#include <stdlib.h>

#include "pub/err.h"
#include "pub/serial.h"

#include "socks5.h"

byte_t *
socks5_encode_auth_sel(const byte_t *supported, byte_t len, size_t *size_p)
{
    byte_t *res = malloc(len + 2);
    ASSERT(res, "out of mem");

    res[0] = SOCKS5_VERSION;
    res[1] = len;
    memcpy(res + 2, supported, len);

    if (size_p) *size_p = len + 2;

    return res;
}

byte_t *
socks5_encode_auth_ack(byte_t method, size_t *size_p)
{
    byte_t *res = malloc(2);
    ASSERT(res, "out of mem");

    res[0] = SOCKS5_VERSION;
    res[1] = method;

    if (size_p) *size_p = 2;

    return res;
}

byte_t *
socks5_encode_request(const socks5_request_t *req, size_t *size_p)
{
    byte_t domain_len;
    serial_t ser;

    serial_init(&ser, NULL, 0, 0);

    serial_write_u8(&ser, SOCKS5_VERSION);
    serial_write_u8(&ser, req->cmd);
    serial_write_u8(&ser, 0); // resv

    ASSERT(req->target, "request target not specified");

    switch (req->target->addr_type) {
        case ADDR_TYPE_IPV4:
            serial_write_u8(&ser, SOCKS5_ADDR_TYPE_IPV4);
            serial_write(&ser, &req->target->addr.ipv4, 4);
            break;

        case ADDR_TYPE_DOMAIN:
            serial_write_u8(&ser, SOCKS5_ADDR_TYPE_DOMAIN);
            domain_len = strlen(req->target->addr.domain);
            serial_write_u8(&ser, domain_len);
            serial_write(&ser, req->target->addr.domain, domain_len);
            break;

        case ADDR_TYPE_IPV6:
            serial_write_u8(&ser, SOCKS5_ADDR_TYPE_IPV6);
            serial_write(&ser, &req->target->addr.ipv6, 16);
            break;

        default: ASSERT(0, "unsupported address type");
    }

    serial_write_u16(&ser, be16(req->target->port));

    if (size_p) *size_p = serial_size(&ser);

    return serial_final(&ser);
}

byte_t *
socks5_encode_response(const socks5_response_t *resp, size_t *size_p)
{
    serial_t ser;

    serial_init(&ser, NULL, 0, 0);

    serial_write_u8(&ser, SOCKS5_VERSION);
    serial_write_u8(&ser, resp->rep);
    serial_write_u8(&ser, 0); // resv
    serial_write_u8(&ser, SOCKS5_ADDR_TYPE_IPV4);

    // ASSERT(!resp->bound, "bound address not supported");
    serial_write_u32(&ser, 0); // always 0
    serial_write_u16(&ser, 0);

    if (size_p) *size_p = serial_size(&ser);

    return serial_final(&ser);
}

ssize_t
socks5_decode_auth_sel(data_trunk_t *trunk,
                       const byte_t *data, size_t size)
{
    byte_t len;

    if (size < 2) {
        return 0;
    }

    if (data[0] != SOCKS5_VERSION) {
        TRACE("wrong socks5 version %d", data[0]);
        return -1; // wrong version
    }

    len = data[1];

    if (len == 0) {
        TRACE("no method given");
        return -1; // no supported method given
    }

    if (size - 2 < len) {
        return 0;
    }

    trunk->data = malloc(len);
    trunk->size = len;
    ASSERT(trunk->data, "out of mem");

    // store all supported auth methods
    memcpy(trunk->data, data + 2, len);

    return 2 + len;
}

ssize_t
socks5_decode_auth_ack(byte_t *method,
                       const byte_t *data, size_t size)
{
    if (size < 2) {
        return 0;
    }

    if (data[0] != SOCKS5_VERSION) {
        return -1; // invlaid version
    }

    *method = data[1];

    return 2;
}

ssize_t
socks5_decode_request(socks5_request_t *req,
                      const byte_t *data, size_t size)
{
    serial_t ser;
    byte_t ver;
    byte_t cmd;
    byte_t resv;
    byte_t addr_type;
    uint16_t port;

    uint8_t ipv4[4];
    uint8_t ipv6[16];

    byte_t len;
    char *domain = NULL;

    size_t n_read;

    target_id_t *target;

#define DECODE_FAIL(status) \
    do { \
        serial_destroy(&ser); \
        free(domain); \
        return (status); \
    } while (0)

    serial_init(&ser, (byte_t *)data, size, 1);

    if (!serial_read(&ser, &ver, 1)) DECODE_FAIL(0);
    if (ver != SOCKS5_VERSION) DECODE_FAIL(-1);

    if (!serial_read(&ser, &cmd, 1)) DECODE_FAIL(0);
    if (!serial_read(&ser, &resv, 1)) DECODE_FAIL(0);
    if (!serial_read(&ser, &addr_type, 1)) DECODE_FAIL(0);

    switch (addr_type) {
        case SOCKS5_ADDR_TYPE_IPV4:
            if (!serial_read(&ser, ipv4, sizeof(ipv4))) DECODE_FAIL(0);
            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);
            target = target_id_new_ipv4(ipv4, from_be16(port));
            break;

        case SOCKS5_ADDR_TYPE_DOMAIN:
            if (!serial_read(&ser, &len, sizeof(len))) DECODE_FAIL(0);

            domain = malloc(len + 1);
            ASSERT(domain, "out of mem");

            if (!serial_read(&ser, domain, len)) DECODE_FAIL(0);
            domain[len] = 0;

            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);

            target = target_id_new_domain(domain, from_be16(port));

            free(domain);
            domain = NULL;

            break;

        case SOCKS5_ADDR_TYPE_IPV6:
            if (!serial_read(&ser, ipv6, sizeof(ipv6))) DECODE_FAIL(0);
            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);
            target = target_id_new_ipv6(ipv6, from_be16(port));
            break;

        default:
            TRACE("unsupported socks5 addr_type %d", addr_type);
            DECODE_FAIL(-1);
    }

    req->target = target;
    req->cmd = cmd;

    n_read = serial_read_idx(&ser);
    serial_destroy(&ser);

    return n_read;
}

ssize_t
socks5_decode_response(socks5_response_t *resp,
                       const byte_t *data, size_t size)
{
    serial_t ser;
    byte_t ver;
    byte_t rep;
    byte_t resv;
    byte_t addr_type;
    uint16_t port;

    uint8_t ipv4[4];
    uint8_t ipv6[16];

    byte_t len;
    char *domain = NULL;

    size_t n_read;

    target_id_t *target;
    // target here is just a replaceholder
    // that don't really do anything and is freed after decoding

#define DECODE_FAIL(status) \
    do { \
        serial_destroy(&ser); \
        free(domain); \
        return (status); \
    } while (0)

    serial_init(&ser, (byte_t *)data, size, 1);

    if (!serial_read(&ser, &ver, 1)) DECODE_FAIL(0);
    if (ver != SOCKS5_VERSION) DECODE_FAIL(-1);

    if (!serial_read(&ser, &rep, 1)) DECODE_FAIL(0);
    if (!serial_read(&ser, &resv, 1)) DECODE_FAIL(0);
    if (!serial_read(&ser, &addr_type, 1)) DECODE_FAIL(0);

    switch (addr_type) {
        case SOCKS5_ADDR_TYPE_IPV4:
            if (!serial_read(&ser, ipv4, sizeof(ipv4))) DECODE_FAIL(0);
            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);
            target = target_id_new_ipv4(ipv4, port);
            break;

        case SOCKS5_ADDR_TYPE_DOMAIN:
            if (!serial_read(&ser, &len, sizeof(len))) DECODE_FAIL(0);

            domain = malloc(len + 1);
            ASSERT(domain, "out of mem");

            if (!serial_read(&ser, domain, len)) DECODE_FAIL(0);
            domain[len] = 0;

            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);

            target = target_id_new_domain(domain, port);

            free(domain);
            domain = NULL;

            break;

        case SOCKS5_ADDR_TYPE_IPV6:
            if (!serial_read(&ser, ipv6, sizeof(ipv6))) DECODE_FAIL(0);
            if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL(0);
            target = target_id_new_ipv6(ipv6, port);
            break;

        default:
            TRACE("unsupported socks5 addr_type %d", addr_type);
            DECODE_FAIL(-1);
    }

    // req->target = target;
    target_id_free(target);
    resp->rep = rep;

    n_read = serial_read_idx(&ser);
    serial_destroy(&ser);

    return n_read;
}

ssize_t
socks5_auth_sel_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size)
{
    return socks5_decode_auth_sel(result, data, size);
}

ssize_t
socks5_auth_ack_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size)
{
    return socks5_decode_auth_ack(result, data, size);
}

ssize_t
socks5_request_decoder(void *ctx, void *result,
                       const byte_t *data, size_t size)
{
    return socks5_decode_request(result, data, size);
}

ssize_t
socks5_response_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size)
{
    return socks5_decode_response(result, data, size);
}

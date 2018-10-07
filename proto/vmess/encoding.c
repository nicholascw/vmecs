#include "pub/serial.h"

#include "encoding.h"

vmess_request_t *
vmess_decode_request(const byte_t *data, size_t size)
{
    serial_t ser;
    hash128_t valid_code;

    hash128_t key, iv;

    byte_t vers;
    byte_t nonce;
    byte_t opt;

    byte_t p_sec;
    byte_t resv;
    byte_t cmd;
    byte_t addr_type;
    byte_t len;

    uint16_t port;

    uint8_t ipv4[4];
    uint8_t ipv6[16];

    target_id_t *target = NULL;
    char *domain = NULL;

    uint32_t checksum;

    vmess_request_t *req;

#define DECODE_FAIL \
    do { \
        serial_destroy(&ser); \
        target_id_free(target); \
        free(domain); \
        return NULL; \
    } while (0)

    serial_init(&ser, (byte_t *)data, size, 1);

    if (!serial_read(&ser, valid_code, sizeof(valid_code))) DECODE_FAIL;
    if (!serial_read(&ser, &vers, sizeof(vers))) DECODE_FAIL;
    if (!serial_read(&ser, &iv, sizeof(iv))) DECODE_FAIL;
    if (!serial_read(&ser, &key, sizeof(key))) DECODE_FAIL;

    if (!serial_read(&ser, &nonce, sizeof(nonce))) DECODE_FAIL;
    if (!serial_read(&ser, &opt, sizeof(opt))) DECODE_FAIL;
    if (!serial_read(&ser, &p_sec, sizeof(p_sec))) DECODE_FAIL;
    if (!serial_read(&ser, &resv, sizeof(resv))) DECODE_FAIL;
    if (!serial_read(&ser, &cmd, sizeof(cmd))) DECODE_FAIL;

    if (!serial_read(&ser, &port, sizeof(port))) DECODE_FAIL;
    port = from_be16(port);

    if (!serial_read(&ser, &addr_type, sizeof(addr_type))) DECODE_FAIL;

    switch (addr_type) {
        case ADDR_TYPE_IPV4:
            if (!serial_read(&ser, ipv4, sizeof(ipv4))) DECODE_FAIL;
            target = target_id_new_ipv4(ipv4, port);
            break;

        case ADDR_TYPE_DOMAIN:
            if (!serial_read(&ser, &len, sizeof(len))) DECODE_FAIL;

            domain = malloc(len + 1);
            ASSERT(domain, "out of mem");

            if (!serial_read(&ser, domain, len)) DECODE_FAIL;
            target = target_id_new_domain(domain, port);

            free(domain);
            domain = NULL;

            break;

        case ADDR_TYPE_IPV6:
            if (!serial_read(&ser, ipv6, sizeof(ipv6))) DECODE_FAIL;
            target = target_id_new_ipv6(ipv6, port);
            break;

        default: DECODE_FAIL;
    }

    // read off p bytes of empty value
    if (!serial_read(&ser, NULL, p_sec >> 4)) DECODE_FAIL;

    if (!serial_read(&ser, &checksum, sizeof(checksum))) DECODE_FAIL;

    // TODO: check checksum

    serial_destroy(&ser);

    req = malloc(sizeof(*req));
    ASSERT(req, "out of mem");

    req->target = target;
    req->vers = vers;
    req->crypt = p_sec & 0xf;
    req->cmd = cmd;
    req->nonce = nonce;
    req->opt = opt;

    return req;
}

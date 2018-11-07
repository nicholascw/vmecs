#ifndef _PROTO_SOCKS_SOCKS5_H_
#define _PROTO_SOCKS_SOCKS5_H_

/*

client:
    auth_sel
        1: ver 5
        1: len
        len: [ ... methods ]

server:
    auth_ack
        1: ver 5
        1: method(we will always use 0)

client:
    request:
        1: ver 5
        1: cmd(assuming CONNECT)
        1: resv
        1: addr type
        x: addr
        2: port

server:
    response:
        1: ver 5
        1: rep: 0 succeed or 7 command not supported
        1: resv
        1: addr type(always ip4)
        4: always 0.0.0.0
        2: always 0

<the rest are regular tcp data>

*/

#include "pub/type.h"
#include "proto/common.h"

#define SOCKS5_VERSION 5

enum {
    SOCKS5_AUTH_METHOD_NO_AUTH = 0
};

enum {
    SOCKS5_CMD_CONNECT = 1
};

enum {
    SOCKS5_REP_SUCCESS = 0,
    SOCKS5_REP_CMD_NOT_SUPPORTED = 7
};

// NOTE: this is different from vmess
enum {
    SOCKS5_ADDR_TYPE_IPV4 = 1,
    SOCKS5_ADDR_TYPE_DOMAIN = 3,
    SOCKS5_ADDR_TYPE_IPV6 = 4
};

typedef struct {
    target_id_t *target;
    byte_t cmd;
} socks5_request_t;

typedef struct {
    byte_t rep;
    // target_id_t *bound; // always NULL for now
} socks5_response_t;

INLINE void
socks5_request_destroy(socks5_request_t *req)
{
    if (req) {
        target_id_free(req->target);
    }
}

byte_t *
socks5_encode_auth_sel(const byte_t *supported, byte_t len, size_t *size_p);

byte_t *
socks5_encode_auth_ack(byte_t method, size_t *size_p);

byte_t *
socks5_encode_request(const socks5_request_t *req, size_t *size_p);

byte_t *
socks5_encode_response(const socks5_response_t *resp, size_t *size_p);

ssize_t
socks5_decode_auth_sel(data_trunk_t *trunk,
                       const byte_t *data, size_t size);

ssize_t
socks5_decode_auth_ack(byte_t *method,
                       const byte_t *data, size_t size);

ssize_t
socks5_decode_request(socks5_request_t *req,
                      const byte_t *data, size_t size);

ssize_t
socks5_decode_response(socks5_response_t *resp,
                       const byte_t *data, size_t size);

// decoder interface

ssize_t
socks5_auth_sel_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size);

ssize_t
socks5_auth_ack_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size);

ssize_t
socks5_request_decoder(void *ctx, void *result,
                       const byte_t *data, size_t size);

ssize_t
socks5_response_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size);

#endif

#ifndef _PROTO_SOCKS_SOCKS4_H_
#define _PROTO_SOCKS_SOCKS4_H_

#include "pub/type.h"

#define SOCKS4_VERSION 4

enum {
    SOCKS4_CMD_CONNECT = 1
};

enum {
    SOCKS4_REP_SUCCESS = 90
};

typedef struct {
    target_id_t *target;
    byte_t cmd;
} socks4_request_t;

typedef struct {
    byte_t rep;
} socks4_response_t;

INLINE void
socks4_request_destroy(socks4_request_t *req)
{
    if (req) {
        target_id_free(req->target);
    }
}

byte_t *
socks4_encode_request(const socks4_request_t *req, size_t *size_p);

byte_t *
socks4_encode_response(const socks4_response_t *resp, size_t *size_p);

ssize_t
socks4_decode_request(socks4_request_t *req,
                      const byte_t *data, size_t size);

ssize_t
socks4_decode_response(socks4_response_t *resp,
                       const byte_t *data, size_t size);

ssize_t
socks4_request_decoder(void *ctx, void *result,
                       const byte_t *data, size_t size);

ssize_t
socks4_response_decoder(void *ctx, void *result,
                        const byte_t *data, size_t size);

#endif

#ifndef _PROTO_VMESS_DECODING_H_
#define _PROTO_VMESS_DECODING_H_

#include "vmess.h"

ssize_t
vmess_decode_data(vmess_config_t *config, const vmess_auth_t *auth,
                  data_trunk_t *trunk,
                  const byte_t *data, size_t size);

// return -1 if wrong checksum
// return 0 if decoding failed
// return number of bytes read otherwise
ssize_t
vmess_decode_request(vmess_config_t *config,
                     vmess_auth_t *auth, vmess_request_t *req,
                     const byte_t *data, size_t size);

ssize_t
vmess_decode_response(vmess_config_t *config,
                      const vmess_auth_t *auth,
                      vmess_response_t *resp,
                      const byte_t *data, size_t size);

typedef struct {
    vmess_config_t *config;
    vmess_auth_t *auth;
} vmess_decoder_ctx_t;

// put a ctx on stack
#define VMESS_DECODER_CTX(config, auth) \
    ((void *)((vmess_decoder_ctx_t []) { { (config), (auth) } }))

// wrapper for decoder interface
ssize_t
vmess_request_decoder(void *context, void *result, const byte_t *data, size_t size);

ssize_t
vmess_response_decoder(void *context, void *result, const byte_t *data, size_t size);

ssize_t
vmess_data_decoder(void *context, void *result, const byte_t *data, size_t size);

#endif

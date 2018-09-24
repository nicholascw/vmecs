#ifndef _PROTO_VMESS_H_
#define _PROTO_VMESS_H_

#include "crypto/hash.h"

#include "common.h"

typedef struct {
    hash128_t user_id;
    hash128_t magic_no;
    int p_max;
} vmess_config_t;

typedef struct {
    int dumb;
} vmess_state_t;

enum {
    VMESS_CRYPT_AES_128_CFB = 0,
    VMESS_CRYPT_NONE = 1,
    VMESS_CRYPT_AES_128_GCM = 2,
    VMESS_CRYPT_CHACHA20_POLY1305 = 3
};

enum {
    VMESS_ADDR_TYPE_IPV4 = 1,
    VMESS_ADDR_TYPE_DOMAIN = 2,
    VMESS_ADDR_TYPE_IPV6 = 3
};

typedef struct {
    union {
        uint32_t ipv4;
        
        struct {
            byte_t len;
            char *val;
        } domain;

        uint32_t ipv6[4];
    } addr;

    size_t data_len;
    byte_t *data;

    uint32_t checksum;

    uint16_t port;

    byte_t vers: 2;
    byte_t crypt: 2;
    byte_t cmd: 2;
    byte_t addr_type: 2;

    byte_t nonce;
    byte_t opt;
} vmess_request_t;

typedef struct {
    hash128_t key, iv;

    size_t header_size;
    byte_t *header;

    size_t data_len;
    byte_t *data;
} vmess_encoded_msg_t;

vmess_encoded_msg_t *
vmess_encode_request(vmess_config_t *config,
                     vmess_state_t *state,
                     vmess_request_t *req);

// read and encode a single trunk from the remaining data
// return NULL if there is no more data to encode
byte_t *vmess_encode_trunk(vmess_request_t *req,
                           vmess_encoded_msg_t *msg);

void vmess_destroy_msg(vmess_encoded_msg_t *msg);
void vmess_destroy_request(vmess_request_t *req);

#endif

#ifndef _PROTO_VMESS_VMESS_H_
#define _PROTO_VMESS_VMESS_H_

#include "crypto/hash.h"
#include "pub/type.h"
#include "proto/common.h"

#define VMESS_MAGIC_NO ((hash128_t) { \
        0xc4, 0x86, 0x19, 0xfe, \
        0x8f, 0x02, 0x49, 0xe0, \
        0xb9, 0xe9, 0xed, 0xf7, \
        0x63, 0xe1, 0x7e, 0x21  \
    })

// cur_time +- 30s
#define VMESS_TIME_DELTA 30

#define VMESS_P_MAX 16

typedef struct {
    hash128_t user_id;
    hash128_t magic_no;
    uint64_t time_delta;
    int p_max;
} vmess_config_t;

// state contains the validation code map
typedef struct {
    byte_t nonce;
} vmess_state_t;

enum {
    VMESS_CRYPT_AES_128_CFB = 0,
    VMESS_CRYPT_NONE = 1,
    VMESS_CRYPT_AES_128_GCM = 2,
    VMESS_CRYPT_CHACHA20_POLY1305 = 3
};

enum {
    VMESS_REQ_CMD_TCP = 1,
    VMESS_REQ_CMD_UDP = 2
};

enum {
    VMESS_RES_CMD_NONE = 0,
    VMESS_RES_CMD_DYN_PORT = 1
};

typedef struct {
    target_id_t *target;

    byte_t vers: 2;         // vmess version
    byte_t crypt: 4;        // encryption method
    byte_t cmd: 2;          // UDP/TCP

    byte_t opt;             // options
} vmess_request_t;

typedef struct {
    byte_t opt;
    // byte_t crypt;
    // byte_t cmd;
    // byte_t cmd_length;
} vmess_response_t;

// authorization info
// unique to each connection
typedef struct {
    hash128_t key, iv;
    uint64_t gen_time;
    byte_t nonce;
} vmess_auth_t;

// vmess serializer
typedef struct {
    vmess_auth_t auth;

    size_t header_size;
    byte_t *header;

    // vmess_serial_t is
    // not in charge of freeing these
    // two fields
    size_t buf_len;
    size_t buf_ofs; // digested length
    byte_t *buf;

    byte_t crypt;
} vmess_serial_t;

/*

    usage:

    config = vmess_config_new();
    state = vmess_state_new();

    // set up request/response
    ...

    vser = vmess_serial_new(auth);
    vmess_serial_response/request(...);
    vmess_serial_write(vser, data, size);

    while ((trunk = vmess_serial_digest(vser))) {
        send trunk;
    }

 */

vmess_config_t *vmess_config_new(hash128_t user_id);
void vmess_config_free(vmess_config_t *config);

vmess_config_t *vmess_config_copy(vmess_config_t *config);

vmess_state_t *vmess_state_new();

INLINE byte_t
vmess_state_next_nonce(vmess_state_t *state)
{
    return state->nonce++;
}

void vmess_state_free(vmess_state_t *state);

vmess_serial_t *vmess_serial_new(vmess_auth_t *auth);

void
vmess_serial_write(vmess_serial_t *vser,
                   const byte_t *buf, size_t len);

// generate a init serializer using
// the request provided
// init may alter the 'nonce' field
void
vmess_serial_request(vmess_serial_t *vser,
                     vmess_config_t *config,
                     vmess_request_t *req);

void
vmess_serial_response(vmess_serial_t *vser,
                      vmess_config_t *config,
                      vmess_response_t *resp);

// generate a data chunk from buffer
// return NULL if there no data left
byte_t *vmess_serial_digest(vmess_serial_t *vser, size_t *size_p);

// generate the end message
const byte_t *vmess_serial_end(size_t *size_p);

void vmess_serial_free(vmess_serial_t *vser);
void vmess_request_free(vmess_request_t *req);
void vmess_request_destroy(vmess_request_t *req);

void
vmess_gen_validation_code(const hash128_t user_id, uint64_t timestamp, hash128_t out);

void vmess_gen_key(vmess_config_t *config, hash128_t key);
void vmess_gen_iv(vmess_config_t *config, uint64_t time, hash128_t iv);

INLINE void
vmess_auth_init(vmess_auth_t *auth, vmess_config_t *config, uint64_t gen_time)
{
    vmess_gen_key(config, auth->key);
    vmess_gen_iv(config, gen_time, auth->iv);
    auth->gen_time = gen_time;
    auth->nonce = 0;
}

INLINE void
vmess_auth_set_nonce(vmess_auth_t *auth, byte_t nonce)
{
    auth->nonce = nonce;
}

INLINE void
vmess_auth_copy(vmess_auth_t *dst, vmess_auth_t *src)
{
    memcpy(dst, src, sizeof(*dst));
}

#endif

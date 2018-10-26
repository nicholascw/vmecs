#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "pub/err.h"
#include "pub/type.h"
#include "pub/random.h"
#include "pub/serial.h"

#include "crypto/aes.h"
#include "crypto/hash.h"

#include "proto/common.h"
#include "vmess.h"

#define AES_128_CFB_TRUNK (((uint16_t)~0) - 1 - 4) // 2^16 - 1 - 4(checksum)
#define NO_ENC_TRUNK (((uint16_t)~0) - 1) // 2^16 - 1

void vmess_gen_key(vmess_config_t *config, hash128_t key)
{
    byte_t tmp_data[sizeof(hash128_t) * 2];

    memcpy(tmp_data, config->user_id, sizeof(config->user_id));
    memcpy(tmp_data + sizeof(config->user_id), config->magic_no, sizeof(config->magic_no));

    if (crypto_md5(tmp_data, sizeof(tmp_data), key)) {
        ASSERT(0, "md5 failed");
    }
}

void vmess_gen_iv(vmess_config_t *config, uint64_t time, hash128_t iv)
{
    uint64_t tmp_data[4];

    tmp_data[0] = tmp_data[1] = tmp_data[2] = tmp_data[3] = be64(time);

    if (crypto_md5((byte_t *)tmp_data, sizeof(tmp_data), iv)) {
        ASSERT(0, "md5 failed");
    }
}

vmess_config_t *vmess_config_new(hash128_t user_id)
{
    vmess_config_t *ret = malloc(sizeof(*ret));

    memcpy(ret->user_id, user_id, sizeof(ret->user_id));
    memcpy(ret->magic_no, VMESS_MAGIC_NO, sizeof(ret->magic_no));
    ret->time_delta = VMESS_TIME_DELTA;
    ret->p_max = VMESS_P_MAX;

    return ret;
}

void vmess_config_free(vmess_config_t *config)
{
    free(config);
}

vmess_state_t *vmess_state_new()
{
    vmess_state_t *ret = malloc(sizeof(*ret));
    ret->nonce = 0;
    return ret;
}

void vmess_state_free(vmess_state_t *state)
{
    free(state);
}

vmess_serial_t *vmess_serial_new(vmess_auth_t *auth)
{
    vmess_serial_t *vser = calloc(1, sizeof(*vser));
    ASSERT(vser, "out of mem");

    vmess_auth_copy(&vser->auth, auth);

    return vser;
}

void
vmess_serial_write(vmess_serial_t *vser,
                   const byte_t *buf, size_t len)
{
    vser->buf = realloc(vser->buf, vser->buf_len + len);
    memcpy(vser->buf + vser->buf_len, buf, len);
    vser->buf_len += len;
}

void
vmess_gen_validation_code(const hash128_t user_id, uint64_t timestamp, hash128_t out)
{
    timestamp = be64(timestamp);

    ASSERT(crypto_hmac_md5(user_id, sizeof(hash128_t),
                           (byte_t *)&timestamp, sizeof(timestamp),
                           out) == 0, "hmac md5 failed");
}

// generate a data chunk from buffer
// return NULL if there no data left(and size is not set)
byte_t *vmess_serial_digest(vmess_serial_t *vser, size_t *size_p)
{
    size_t trunk_size, out_size;
    size_t remain_size, total_size;
    byte_t *data, *enc_data;
    byte_t *trunk = NULL;

    // send and clear header first
    if (vser->header) {
        trunk = vser->header;
        *size_p = vser->header_size;

        vser->header_size = 0;
        vser->header = NULL;
        return trunk;
    }

    if (!vser->buf) return NULL;

    // if (!msg->data_len) {
    //     trunk = calloc(2, 1); // last package is a empty package
    //     msg->data_len = -1; // set length to -1 to denote the end of data
    //     return trunk;
    // }

    switch (vser->crypt) {
        case VMESS_CRYPT_AES_128_CFB: trunk_size = AES_128_CFB_TRUNK; break;
        case VMESS_CRYPT_NONE: trunk_size = NO_ENC_TRUNK; break;
        default: ASSERT(0, "unsupported encryption");
    }

    remain_size = vser->buf_len - vser->buf_ofs;
    data = vser->buf + vser->buf_ofs;

    // consume at most one trunk of data
    trunk_size = trunk_size > remain_size ? remain_size : trunk_size;

    switch (vser->crypt) {
        case VMESS_CRYPT_AES_128_CFB:
            total_size = 2 + 4 + trunk_size;

            trunk = malloc(total_size);
            ASSERT(trunk, "out of mem");

            *((uint16_t *)trunk) = be16(trunk_size + 4);
            *((uint32_t *)(trunk + 2)) = be32(crypto_fnv1a(data, trunk_size));
            memcpy(trunk + 6, data, trunk_size);

            // encrypt
            enc_data = crypto_aes_128_cfb_enc(vser->auth.key, vser->auth.iv, trunk + 2, trunk_size + 4, &out_size);
            ASSERT(out_size == trunk_size + 4, "wrong padding");

            memcpy(trunk + 2, enc_data, trunk_size + 4);

            free(enc_data);

            break;

        case VMESS_CRYPT_NONE:
            total_size = 2 + trunk_size;

            trunk = malloc(2 + trunk_size);
            ASSERT(trunk, "out of mem");

            *((uint16_t *)trunk) = trunk_size + 4;
            memcpy(trunk + 2, data, trunk_size);
            break;
    }

    vser->buf_ofs += trunk_size;

    if (vser->buf_ofs == vser->buf_len) {
        // all data in the buffer digested
        free(vser->buf);
        vser->buf_len = vser->buf_ofs = 0;
        vser->buf = NULL;
    }

    *size_p = total_size;

    return trunk;
}

void vmess_serial_free(vmess_serial_t *vser)
{
    if (vser) {
        free(vser->header);
        free(vser->buf);
        free(vser);
    }
}

void vmess_request_free(vmess_request_t *req)
{
    if (req) {
        target_id_free(req->target);
        free(req);
    }
}

const byte_t *vmess_serial_end(size_t *size_p)
{
    static byte_t end[] = { 0, 0 };
    if (size_p) *size_p = sizeof(end);
    return end;
}

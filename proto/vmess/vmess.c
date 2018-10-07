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

static void _vmess_gen_key(vmess_config_t *config, hash128_t key)
{
    byte_t tmp_data[sizeof(hash128_t) * 2];

    memcpy(tmp_data, config->user_id, sizeof(config->user_id));
    memcpy(tmp_data + sizeof(config->user_id), config->magic_no, sizeof(config->magic_no));

    if (crypto_md5(tmp_data, sizeof(tmp_data), key)) {
        ASSERT(0, "md5 failed");
    }
}

static void _vmess_gen_iv(vmess_config_t *config, uint64_t time, hash128_t iv)
{
    uint64_t tmp_data[4];

    tmp_data[0] = tmp_data[1] = tmp_data[2] = tmp_data[3] = time;

    if (crypto_md5((byte_t *)tmp_data, sizeof(tmp_data), iv)) {
        ASSERT(0, "md5 failed");
    }
}

vmess_config_t *vmess_config_new(hash128_t user_id)
{
    vmess_config_t *ret = malloc(sizeof(*ret));

    memcpy(ret->user_id, user_id, sizeof(ret->user_id));
    memcpy(ret->magic_no, VMESS_MAGIC_NO, sizeof(ret->magic_no));
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

vmess_connection_t *vmess_conn_new()
{
    vmess_connection_t *conn = calloc(1, sizeof(*conn));
    ASSERT(conn, "out of mem");
    return conn;
}

void
vmess_conn_write(vmess_connection_t *conn,
                 const byte_t *buf, size_t len)
{
    conn->buf = realloc(conn->buf, conn->buf_len + len);
    memcpy(conn->buf + conn->buf_len, buf, len);
    conn->buf_len += len;
}

void
vmess_conn_request(vmess_connection_t *conn,
                   vmess_config_t *config,
                   vmess_state_t *state,
                   vmess_request_t *req)
{
    hash128_t valid_code;
    uint64_t gen_time = be64(time(NULL));
    size_t cmd_size;
    size_t out_size;
    byte_t *cmd, *enc_cmd;

    uint32_t checksum;

    serial_t ser;

    int i, p = random_in(0, config->p_max), tmp;
    
    // TODO: add option M support
    ASSERT(req->opt == 1, "unsupported option");

    serial_init(&ser, NULL, 0, 0);

    /******* part 1, validation code *******/

    // gen validation code
    tmp = crypto_hmac_md5(config->user_id, sizeof(config->user_id),
                          (byte_t *)&gen_time, sizeof(gen_time),
                          valid_code);
    ASSERT(!tmp, "hmac md5 failed");

    serial_write(&ser, valid_code, sizeof(valid_code));

    /******* part 2, header *******/
    // version byte

    serial_write_u8(&ser, req->vers);

    // gen iv and key
    _vmess_gen_key(config, conn->key);
    _vmess_gen_iv(config, gen_time, conn->iv);

    serial_write(&ser, conn->iv, sizeof(conn->iv));
    serial_write(&ser, conn->key, sizeof(conn->key));

    req->nonce = state->nonce++;
    serial_write_u8(&ser, req->nonce); // inc and set nonce
    serial_write_u8(&ser, req->opt);
    serial_write_u8(&ser, p << 4 | req->crypt);
    serial_write_u8(&ser, 0); // reserved
    serial_write_u8(&ser, req->cmd);

    serial_write_u16(&ser, be16(req->target->port));
    serial_write_u8(&ser, req->target->addr_type);

    switch (req->target->addr_type) {
        case ADDR_TYPE_IPV4:
            serial_write(&ser, &req->target->addr.ipv4, 4);
            break;

        case ADDR_TYPE_DOMAIN:
            serial_write_u8(&ser, req->target->domain_len);
            serial_write(&ser, req->target->addr.domain, req->target->domain_len);
            break;

        case ADDR_TYPE_IPV6:
            serial_write(&ser, &req->target->addr.ipv6, 16);
            break;

        default: ASSERT(0, "invalid address type");
    }

    for (i = 0; i < p; i++) {
        serial_write_u8(&ser, random_in(0, 0xff));
    }

    checksum = crypto_fnv1a(serial_ofs(&ser, sizeof(valid_code)), serial_size(&ser) - sizeof(valid_code));
    serial_write_u32(&ser, checksum);

    // aes-128-cfb encrypt header
    cmd = serial_ofs(&ser, sizeof(valid_code));
    cmd_size = serial_size(&ser) - sizeof(valid_code);
    enc_cmd = crypto_aes_128_cfb_enc(conn->key, conn->iv, cmd, cmd_size, &out_size);
    memcpy(cmd, enc_cmd, cmd_size);
    free(enc_cmd);

    // set up connection
    conn->state = VMESS_CONN_INIT;
    conn->header_size = serial_size(&ser);
    conn->header = serial_final(&ser);
    conn->crypt = req->crypt;
}

// generate a data chunk from buffer
// return NULL if there no data left(and size is not set)
byte_t *vmess_conn_digest(vmess_connection_t *conn, size_t *size_p)
{
    size_t trunk_size, out_size;
    size_t remain_size, total_size;
    byte_t *data, *enc_data;
    byte_t *trunk = NULL;

    // send and clear header first
    if (conn->header) {
        trunk = conn->header;
        *size_p = conn->header_size;

        conn->header_size = 0;
        conn->header = NULL;
        return trunk;
    }

    if (!conn->buf) return NULL;

    // if (!msg->data_len) {
    //     trunk = calloc(2, 1); // last package is a empty package
    //     msg->data_len = -1; // set length to -1 to denote the end of data
    //     return trunk;
    // }

    switch (conn->crypt) {
        case VMESS_CRYPT_AES_128_CFB: trunk_size = AES_128_CFB_TRUNK; break;
        case VMESS_CRYPT_NONE: trunk_size = NO_ENC_TRUNK; break;
        default: ASSERT(0, "unsupported encryption");
    }

    remain_size = conn->buf_len - conn->buf_ofs;
    data = conn->buf + conn->buf_ofs;

    // consume at most one trunk of data
    trunk_size = trunk_size > remain_size ? remain_size : trunk_size;

    switch (conn->crypt) {
        case VMESS_CRYPT_AES_128_CFB:
            total_size = 2 + 4 + trunk_size;

            trunk = malloc(total_size);
            ASSERT(trunk, "out of mem");

            *((uint16_t *)trunk) = trunk_size + 4;
            *((uint32_t *)(trunk + 2)) = crypto_fnv1a(data, trunk_size);
            memcpy(trunk + 6, data, trunk_size);

            // encrypt
            enc_data = crypto_aes_128_cfb_enc(conn->key, conn->iv, trunk + 2, trunk_size + 4, &out_size);
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

    conn->buf_ofs += trunk_size;

    if (conn->buf_ofs == conn->buf_len) {
        // all data in the buffer digested
        free(conn->buf);
        conn->buf_len = conn->buf_ofs = 0;
        conn->buf = NULL;
    }

    *size_p = total_size;

    return trunk;
}

void vmess_conn_free(vmess_connection_t *conn)
{
    if (conn) {
        free(conn->header);
        free(conn->buf);
        free(conn);
    }
}

void vmess_request_free(vmess_request_t *req)
{
    if (req) {
        target_id_free(req->target);
        free(req);
    }
}

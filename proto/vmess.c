#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "pub/err.h"
#include "pub/type.h"
#include "pub/random.h"

#include "crypto/aes.h"
#include "crypto/hash.h"

#include "vmess.h"
#include "common.h"

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

vmess_encoded_msg_t *
vmess_encode_request(vmess_config_t *config,
                     vmess_state_t *state,
                     vmess_request_t *req)
{
    hash128_t valid_code;
    uint64_t gen_time = be64(time(NULL));
    size_t header_size;
    size_t addr_size;
    size_t out_size;
    byte_t *ret, *header, *header_it, *enc_header;
    int i, p = random_in(0, config->p_max), tmp;

    vmess_encoded_msg_t *msg;

    switch (req->addr_type) {
        case VMESS_ADDR_TYPE_IPV4: addr_size = 4; break;
        case VMESS_ADDR_TYPE_DOMAIN: addr_size = 1 + req->addr.domain.len; break;
        case VMESS_ADDR_TYPE_IPV6: addr_size = 8; break;
        default: ASSERT(0, "invalid address type");
    }
    
    // TODO: add option M support
    ASSERT(req->opt == 1, "unsupported option");

    header_size =
        1 + // version
        16 + // iv
        16 + // key
        1 + // nonce
        1 + // option
        1 + // p: 4 + crypt: 4
        1 + // reserved
        1 + // cmd
        2 + // port
        1 + // addr type
        addr_size +
        p +
        4; // checksum

    // // using standard data type
    // switch (req->crypt) {
    //     case VMESS_CRYPT_AES_128_CFB:
    //         // every packet can hold 2^16 - 1 - 4 bytes
    //         n_packet = (req->data_len + AES_128_CFB_PACKET - 1) / AES_128_CFB_PACKET;
    //         // 2 bytes for length, 4 bytes for checksum
    //         data_size = 6 * n_packet + req->data_len;
    //         break;

    //     case VMESS_CRYPT_NONE:
    //         // every packet can hold 2^16 - 1 bytes
    //         n_packet = (req->data_len + NO_ENC_PACKET - 1) / NO_ENC_PACKET;
    //         // 2 bytes for length
    //         data_size = 2 * n_packet + req->data_len;
    //         break;

    //     default: ASSERT(0, "unsupported encryption");
    // }

    ret = malloc(sizeof(valid_code) + header_size);
    ASSERT(ret, "out of mem");
    
    msg = malloc(sizeof(msg));
    ASSERT(msg, "out of mem");

    msg->header = ret;
    msg->data_len = req->data_len;
    msg->data = req->data;

    /******* part 1, validation code *******/

    // gen validation code
    tmp = crypto_hmac_md5(config->user_id, sizeof(config->user_id),
                          (byte_t *)&gen_time, sizeof(gen_time),
                          valid_code);
    ASSERT(!tmp, "hmac md5 failed");

    memcpy(ret, valid_code, sizeof(valid_code));

    /******* part 2, header *******/
    header_it = header = ret + sizeof(valid_code);

    // version byte
    *header_it++ = req->vers;

    // gen iv and key
    _vmess_gen_key(config, msg->key);
    _vmess_gen_iv(config, gen_time, msg->iv);

    memcpy(header_it, msg->iv, sizeof(msg->iv));
    memcpy(header_it + sizeof(msg->iv), msg->key, sizeof(msg->key));
    header_it += sizeof(msg->iv) + sizeof(msg->key);

    // nonce
    *header_it++ = req->nonce;
    *header_it++ = req->opt;
    *header_it++ = p << 4 | req->crypt;
    *header_it++ = 0; // reserved
    *header_it++ = req->cmd;

    // port
    *((uint16_t *)header_it) = req->port;
    header_it += 2;

    *header_it++ = req->addr_type;

    switch (req->addr_type) {
        case VMESS_ADDR_TYPE_IPV4:
            *((uint32_t *)header_it) = req->addr.ipv4;
            header_it += 4;
            break;

        case VMESS_ADDR_TYPE_DOMAIN:
            *header_it++ = req->addr.domain.len;
            memcpy(header_it, req->addr.domain.val, req->addr.domain.len);
            header_it += req->addr.domain.len;
            break;


        case VMESS_ADDR_TYPE_IPV6:
            memcpy(header_it, req->addr.ipv6, sizeof(req->addr.ipv6));
            header_it += sizeof(req->addr.ipv6);
            break;


        default: ASSERT(0, "invalid address type");
    }

    for (i = 0; i < p; i++) {
        *header_it++ = random_in(0, 0xff);
    }

    ASSERT(header_it - header == header_size - 4, "impossible");

    *((uint32_t *)header) = crypto_fnv1a(header, header_size - 4);

    // aes-128-cfb encrypt header
    enc_header = crypto_aes_128_cfb_enc(msg->key, msg->iv, header, header_size, &out_size);
    ASSERT(out_size == header_size, "wrong padding");

    memcpy(header, enc_header, header_size);
    free(enc_header);

    return msg;
}

byte_t *vmess_encode_trunk(vmess_request_t *req,
                           vmess_encoded_msg_t *msg)
{
    size_t trunk_size, out_size;
    byte_t *data, *enc_data;
    byte_t *trunk = NULL;

    if (!msg->data_len) return NULL;
    
    switch (req->crypt) {
        case VMESS_CRYPT_AES_128_CFB: trunk_size = AES_128_CFB_TRUNK; break;
        case VMESS_CRYPT_NONE: trunk_size = NO_ENC_TRUNK; break;
        default: ASSERT(0, "unsupported encryption");
    }

    // consume at most one trunk of data
    trunk_size = trunk_size > msg->data_len ? msg->data_len : trunk_size;
    data = msg->data;

    msg->data += trunk_size;
    msg->data_len -= trunk_size;

    switch (req->crypt) {
        case VMESS_CRYPT_AES_128_CFB:
            trunk = malloc(2 + 4 + trunk_size);
            ASSERT(trunk, "out of mem");

            *((uint16_t *)trunk) = trunk_size + 4;
            *((uint32_t *)(trunk + 2)) = crypto_fnv1a(data, trunk_size);
            memcpy(trunk + 6, data, trunk_size);

            // encrypt
            enc_data = crypto_aes_128_cfb_enc(msg->key, msg->iv, trunk + 2, trunk_size + 4, &out_size);
            ASSERT(out_size == trunk_size + 4, "wrong padding");

            memcpy(trunk + 2, enc_data, trunk_size + 4);
            break;

        case VMESS_CRYPT_NONE:
            trunk = malloc(2 + trunk_size);
            ASSERT(trunk, "out of mem");

            *((uint16_t *)trunk) = trunk_size + 4;
            memcpy(trunk + 2, data, trunk_size);
            break;
    }

    return trunk;
}

void vmess_destroy_msg(vmess_encoded_msg_t *msg)
{
    if (msg) {
        free(msg->header);
        free(msg);
    }
}

void vmess_destroy_request(vmess_request_t *req)
{
    if (req) {
        if (req->addr_type == VMESS_ADDR_TYPE_DOMAIN) {
            free(req->addr.domain.val);
        }

        free(req->data);
        free(req);
    }
}

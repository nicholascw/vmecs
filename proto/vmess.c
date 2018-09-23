#include <time.h>
#include <stdlib.h>
#include <string.h>

#include "pub/err.h"
#include "pub/type.h"
#include "pub/random.h"
#include "crypto/hash.h"

#include "vmess.h"
#include "common.h"

#define AES_128_CFB_PACKET (((uint16_t)~0) - 1 - 4) // 2^16 - 1 - 4(checksum)
#define NO_ENC_PACKET (((uint16_t)~0) - 1) // 2^16 - 1

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

byte_t *
vmess_encode_request(vmess_config_t *config,
                     vmess_state_t *state,
                     vmess_request_t *req)
{
    hash128_t valid_code;
    uint64_t gen_time = be64(time(NULL));
    size_t header_size;
    size_t data_size;
    size_t addr_size;
    size_t n_packet;
    byte_t *ret, *header, *header_it, *data;
    int i, p = random_in(0, config->p_max), tmp;

    hash128_t iv, key;

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

    // using standard data type
    switch (req->crypt) {
        case VMESS_CRYPT_AES_128_CFB:
            // every packet can hold 2^16 - 1 - 4 bytes
            n_packet = (req->data_len + AES_128_CFB_PACKET - 1) / AES_128_CFB_PACKET;
            // 2 bytes for length, 4 bytes for checksum
            data_size = 6 * n_packet + req->data_len;
            break;

        case VMESS_CRYPT_NONE:
            // every packet can hold 2^16 - 1 bytes
            n_packet = (req->data_len + NO_ENC_PACKET - 1) / NO_ENC_PACKET;
            // 2 bytes for length
            data_size = 2 * n_packet + req->data_len;
            break;

        default: ASSERT(0, "unsupported encryption");
    }

    ret = malloc(sizeof(valid_code) + header_size + data_size);
    ASSERT(ret, "out of mem");

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
    _vmess_gen_key(config, key);
    _vmess_gen_iv(config, gen_time, iv);

    memcpy(header_it, iv, sizeof(iv));
    memcpy(header_it + sizeof(iv), key, sizeof(key));
    header_it += sizeof(iv) + sizeof(key);

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

    /******* part 3, data *******/
    data = ret + sizeof(valid_code) + header_size;
    (void)data;
    // TODO

    return ret;
}

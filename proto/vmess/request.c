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

void
vmess_serial_request(vmess_serial_t *vser,
                     vmess_config_t *config,
                     vmess_request_t *req)
{
    hash128_t valid_code;
    uint64_t gen_time = vser->auth.gen_time;
    size_t cmd_size;
    size_t out_size;
    byte_t *cmd, *enc_cmd;
    byte_t domain_len;

    uint32_t checksum;

    serial_t ser;

    int i, p = random_in(0, config->p_max);
    
    // TODO: add option M support
    ASSERT(req->opt == 1, "unsupported option");

    serial_init(&ser, NULL, 0, 0);

    /******* part 1, validation code *******/

    // gen validation code
    vmess_gen_validation_code(config->user_id, gen_time, valid_code);

    serial_write(&ser, valid_code, sizeof(valid_code));

    /******* part 2, header *******/
    // version byte

    serial_write_u8(&ser, req->vers);

    // gen iv and key
    serial_write(&ser, vser->auth.iv, sizeof(vser->auth.iv));
    serial_write(&ser, vser->auth.key, sizeof(vser->auth.key));

    serial_write_u8(&ser, vser->auth.nonce); // inc and set nonce
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
            domain_len = strlen(req->target->addr.domain);
            serial_write_u8(&ser, domain_len);
            serial_write(&ser, req->target->addr.domain, domain_len);
            break;

        case ADDR_TYPE_IPV6:
            serial_write(&ser, &req->target->addr.ipv6, 16);
            break;

        default: ASSERT(0, "invalid address type");
    }

    for (i = 0; i < p; i++) {
        serial_write_u8(&ser, random_in(0, 0xff));
    }

    checksum = be32(crypto_fnv1a(serial_ofs(&ser, sizeof(valid_code)), serial_size(&ser) - sizeof(valid_code)));
    serial_write_u32(&ser, checksum);

    // aes-128-cfb encrypt header
    cmd = serial_ofs(&ser, sizeof(valid_code));
    cmd_size = serial_size(&ser) - sizeof(valid_code);

    // hexdump("not encoded", cmd, cmd_size);
    // printf("checksum: %d\n", checksum);

    enc_cmd = crypto_aes_128_cfb_enc(vser->auth.key, vser->auth.iv, cmd, cmd_size, &out_size);
    memcpy(cmd, enc_cmd, cmd_size);
    free(enc_cmd);

    // set up serializer
    vser->header_size = serial_size(&ser);
    vser->header = serial_final(&ser);
    vser->crypt = req->crypt;
}

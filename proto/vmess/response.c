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
vmess_serial_response(vmess_serial_t *vser,
                      vmess_config_t *config,
                      vmess_response_t *resp)
{
    byte_t header[4];
    byte_t *header_enc;

    header[0] = vser->auth.nonce;
    header[1] = resp->opt;
    header[2] = 0; // cmd
    header[3] = 0; // cmd length

    header_enc = crypto_aes_128_cfb_enc(vser->auth.key, vser->auth.iv, header, sizeof(header), NULL);
    ASSERT(header_enc, "aes-128-cfb failed");

    vser->header_size = 4;
    vser->header = header_enc;
    vser->crypt = VMESS_CRYPT_AES_128_CFB; // TODO
}

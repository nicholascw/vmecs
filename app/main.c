#include <stdio.h>
#include <time.h>

#include "pub/err.h"

#include "crypto/aes.h"
#include "crypto/hash.h"

#include "proto/vmess/vmess.h"
#include "proto/vmess/decoding.h"

void print_byte(byte_t b)
{
    printf("%.2x", b);
}

void print_hex(byte_t *data, size_t len)
{
    for (int i = 0; i < len; i++) {
        print_byte(data[i]);
    }

    puts("");
}

void print_hash128(hash128_t hash)
{
    print_hex(hash, 16);
}

void test_vmess_response()
{
    // vmess_config_t *config = vmess_config_new(user_id);
    // vmess_state_t *state = vmess_state_new();

    // vmess_serial_t *vser;
    // vmess_response_t req;

    
}

void test_vmess_request()
{
    // hash128_t user_id = {0};
    // vmess_config_t *config = vmess_config_new(user_id);
    // vmess_state_t *state = vmess_state_new();
    // vmess_serial_t *vser;
    // target_id_t *target = target_id_new_ipv4((byte_t[]){ 127, 0, 0, 1 }, 3151);

    // vmess_request_t req = {
    //     .target = target,
    //     .vers = 1,
    //     .crypt = VMESS_CRYPT_AES_128_CFB,
    //     .cmd = VMESS_REQ_CMD_TCP,
    //     .opt = 1
    // };

    // size_t size;
    // byte_t *trunk;

    // byte_t *header;
    // byte_t *buf;
    // size_t buf_size;

    // vmess_request_t req_read;
    // vmess_auth_t auth;
    
    // vmess_auth_init(&auth, config, time(NULL));
    // vmess_auth_set_nonce(&auth, vmess_state_next_nonce(state));

    // vser = vmess_serial_new(&auth);

    // vmess_serial_request(vser, config, &req);
    // vmess_serial_write(vser, (byte_t *)"hello", 5);

    // ////////////// decode/encode header

    // header = vmess_serial_digest(vser, &size);
    // hexdump("header", header, size);
    // size = vmess_decode_request(config, &auth, &req_read, header, size);
    // TRACE("read: %lu", size);
    // free(header);

    // ////////////// decode/encode data
    // trunk = vmess_serial_digest(vser, &size);
    // hexdump("trunk", trunk, size);
    // size = vmess_decode_data(config, &auth, trunk, size, &buf, &buf_size);
    // TRACE("read: %lu, data size: %lu", size, buf_size);
    // hexdump("data", buf, buf_size);
    // free(buf);
    // free(trunk);

    // vmess_serial_free(vser);
    // target_id_free(target);

    // vmess_config_free(config);
    // vmess_state_free(state);
}

void test_crypto()
{
    hash128_t hash;

    byte_t key[16] = {0};
    byte_t iv[16] = {0};
    size_t out_size;

    byte_t *ctext = crypto_aes_128_cfb_enc(key, iv, (byte_t *)"hi", 2, &out_size);
    print_hex(ctext, out_size);

    byte_t *ptext = crypto_aes_128_cfb_dec(key, iv, ctext, out_size, &out_size);
    print_hex(ptext, out_size);

    free(ptext);
    free(ctext);

    // crypto_md5("hi", 2, hash);
    // crypto_shake128((byte_t *)"hi", 2, hash);

    print_hash128(hash);
}

// int main()
// {
//     test_crypto();
//     test_vmess();
//     return 0;
// }

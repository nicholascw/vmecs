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

void hexdump(char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        printf ("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }

    if (len < 0) {
        printf("  NEGATIVE LENGTH: %i\n",len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf ("  %s\n", buff);

            // Output the offset.
            printf ("  %04x ", i);
        }

        // Now the hex code for the specific character.
        printf (" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf ("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf ("  %s\n", buff);
}

void test_vmess()
{
    hash128_t user_id = {0};
    vmess_config_t *config = vmess_config_new(user_id);
    vmess_state_t *state = vmess_state_new();
    vmess_serial_t *vser;
    target_id_t *target = target_id_new_ipv4((byte_t[]){ 127, 0, 0, 1 }, 3151);

    vmess_request_t req = {
        .target = target,
        .vers = 1,
        .crypt = VMESS_CRYPT_AES_128_CFB,
        .cmd = VMESS_REQ_CMD_TCP,
        .opt = 1
    };

    size_t size;
    byte_t *trunk;

    byte_t *header;
    byte_t *buf;
    size_t buf_size;

    vmess_request_t req_read;
    vmess_auth_t auth;
    
    vmess_auth_init(&auth, config, time(NULL));

    vser = vmess_serial_new(&auth);

    vmess_serial_request(vser, config, &req);
    vmess_serial_write(vser, (byte_t *)"hello", 5);

    ////////////// decode/encode header

    header = vmess_serial_digest(vser, &size);
    hexdump("header", header, size);
    size = vmess_decode_request(config, &auth, &req_read, header, size);
    printf("read: %lu\n", size);
    free(header);

    ////////////// decode/encode data
    trunk = vmess_serial_digest(vser, &size);
    hexdump("trunk", trunk, size);
    size = vmess_decode_data(config, &auth, trunk, size, &buf, &buf_size);
    printf("read: %lu, data size: %lu\n", size, buf_size);
    hexdump("data", buf, buf_size);
    free(buf);
    free(trunk);

    ////////////// decode/encode response


    vmess_serial_free(vser);
    target_id_free(target);

    vmess_config_free(config);
    vmess_state_free(state);
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
    crypto_shake128((byte_t *)"hi", 2, hash);

    print_hash128(hash);
}

// int main()
// {
//     test_crypto();
//     test_vmess();
//     return 0;
// }

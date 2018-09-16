#include <stdio.h>

#include "crypto/aes.h"
#include "crypto/hash.h"

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

int main()
{
    hash128_t hash;

    byte_t key[16] = {0};
    byte_t iv[16] = {0};
    size_t out_size;

    byte_t *ctext = crypto_aes_128_cfb_enc(key, iv, "hi", 2, &out_size);
    print_hex(ctext, out_size);

    byte_t *ptext = crypto_aes_128_cfb_dec(key, iv, ctext, out_size, &out_size);
    print_hex(ptext, out_size);

    free(ptext);
    free(ctext);

    // crypto_md5("hi", 2, hash);
    crypto_shake128("hi", 2, hash);

    print_hash128(hash);

    return 0;
}

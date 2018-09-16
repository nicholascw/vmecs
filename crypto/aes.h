#ifndef _CRYPTO_AES_H_
#define _CRYPTO_AES_H_

#include "pub/type.h"

#define GEN_AES_SIG(mode) \
    byte_t *crypto_aes_128_##mode##_enc(const byte_t *key, const byte_t *iv, \
                                        const byte_t *data, size_t data_size, \
                                        size_t *out_size_p); \
    byte_t *crypto_aes_128_##mode##_dec(const byte_t *key, const byte_t *iv, \
                                        const byte_t *ctext, size_t ctext_size, \
                                        size_t *out_size_p);

GEN_AES_SIG(cfb)

#undef GEN_AES_SIG

#endif

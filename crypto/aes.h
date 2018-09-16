#ifndef _CRYPTO_AES_H_
#define _CRYPTO_AES_H_

#include "pub/type.h"

// this function allocates the memory needed for encrypted text
byte_t *crypto_aes_128_cfb_enc(const byte_t *key, const byte_t *iv,
                               const byte_t *data, size_t data_size,
                               size_t *out_size_p);

#endif

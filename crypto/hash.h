#ifndef _VMECS_HASH_H_
#define _VMECS_HASH_H_

#include "pub/type.h"

typedef byte_t hash128_t[16];
typedef byte_t hash256_t[32];

int crypto_md5(const byte_t *data, size_t size, hash128_t hash);

int crypto_hmac_md5(const byte_t *key, size_t key_size,
                    const byte_t *data, size_t data_size,
                    hash128_t hash);

int crypto_shake128(const byte_t *data, size_t data_size, hash128_t hash);

#endif

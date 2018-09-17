#include <openssl/md5.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/hmac.h>

#include "hash.h"

int crypto_md5(const byte_t *data, size_t size, hash128_t hash)
{
    if (!MD5(data, size, hash)) {
        ERR_print_errors_fp(stderr);
        return -1;
    }

    return 0;
}

int crypto_hmac_md5(const byte_t *key, size_t key_size,
                    const byte_t *data, size_t data_size,
                    hash128_t hash)
{
    const EVP_MD *md5 = EVP_md5();

    if (!HMAC(md5, key, key_size, data, data_size, hash, NULL)) {
        ERR_print_errors_fp(stderr);
        return -1;
    }
    
    return 0;
}

int _crypto_evp_md(const EVP_MD *evp, const byte_t *data, size_t data_size, byte_t *hash)
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();

#define CLEAR_EXIT(code) \
    do { \
        int _c = (code); \
        if (_c) { \
            ERR_print_errors_fp(stderr); \
        } \
        EVP_MD_CTX_free(ctx); \
        return _c; \
    } while (0)

    if (!EVP_DigestInit(ctx, evp)) CLEAR_EXIT(-1);
    if (!EVP_DigestUpdate(ctx, data, data_size)) CLEAR_EXIT(-1);
    if (!EVP_DigestFinal(ctx, hash, NULL)) CLEAR_EXIT(-1);

    CLEAR_EXIT(0);

#undef CLEAR_EXIT
}

int crypto_shake128(const byte_t *data, size_t data_size, hash128_t hash)
{
    return _crypto_evp_md(EVP_shake128(), data, data_size, hash);
}

uint32_t crypto_fnv1a(const byte_t *data, size_t data_size)
{
    uint32_t hash = 0x811c9dc5;
    const uint32_t p = 0x01000193;

    while (data_size--)
        hash = (hash ^ *data++) + p;

    return hash;
}

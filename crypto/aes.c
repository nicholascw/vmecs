#include <openssl/evp.h>
#include <openssl/err.h>

#include "pub/err.h"

#include "aes.h"

// encrypt if enc == 1
// decrypt if enc == 0
// the caller should allocate enough mem for output
ssize_t _crypto_evp_cipher(const EVP_CIPHER *evp,
                           const byte_t *key, const byte_t *iv,
                           const byte_t *data, size_t data_size,
                           byte_t *out, int enc)
{
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    int len1, len2;

#define CLEAR_EXIT(code) \
    do { \
        int _c = (code); \
        if (_c) { \
            ERR_print_errors_fp(stderr); \
        } \
        EVP_CIPHER_CTX_free(ctx); \
        return _c; \
    } while (0)

    if (!EVP_CipherInit(ctx, evp, key, iv, enc)) CLEAR_EXIT(-1);
    if (!EVP_CipherUpdate(ctx, out, &len1, data, data_size)) CLEAR_EXIT(-1);
    if (!EVP_CipherFinal(ctx, out + len1, &len2)) CLEAR_EXIT(-1);

    CLEAR_EXIT(len1 + len2);

#undef CLEAR_EXIT
}

byte_t *crypto_aes_128_cfb_enc(const byte_t *key, const byte_t *iv,
                               const byte_t *data, size_t data_size,
                               size_t *out_size_p)
{
    const EVP_CIPHER *type = EVP_aes_128_cfb();
    size_t out_size = data_size + (16 - (data_size % 16));
    byte_t *ret = malloc(out_size);

    ASSERT(ret, "malloc failed");

    out_size = _crypto_evp_cipher(type, key, iv, data, data_size, ret, 1);
    ret = realloc(ret, out_size);

    if (out_size_p)
        *out_size_p = out_size;

    return ret;
}

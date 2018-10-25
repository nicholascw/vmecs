#ifndef _PUB_TYPE_H_
#define _PUB_TYPE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "err.h"

typedef uint8_t byte_t;
typedef signed long ssize_t;

#ifdef __GNUC__
	#if (__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1))
		#define INLINE static __inline__ __attribute__((always_inline))
	#else
		#define INLINE static __inline__
	#endif
#elif defined(_MSC_VER)
	#define INLINE static __forceinline
#elif (defined(__BORLANDC__) || defined(__WATCOMC__))
	#define INLINE static __inline
#else
	#define INLINE static inline
#endif

#define B_AT(n, i) (((n) >> (i) * 8) & 0xff)
#define BE_GEN(bit) \
    INLINE uint##bit##_t be##bit(uint##bit##_t n) \
    { \
        uint##bit##_t ret; \
        for (int i = 0; i < sizeof(ret); i++) { \
            ((byte_t *)&ret)[i] = B_AT(n, sizeof(ret) - i - 1); \
        } \
        return ret; \
    }

BE_GEN(16)
BE_GEN(32)
BE_GEN(64)

#undef B_AT

INLINE uint16_t
from_be16(uint16_t n)
{
    uint8_t *p = (uint8_t *)&n;
    return ((uint16_t)p[0] << 8) | (uint16_t)p[1];
}

// INLINE char *strdup(const char *str)
// {
//     size_t len = strlen(str);
//     char *ret = malloc(len + 1);
//     ASSERT(ret, "out of mem");
//     memcpy(ret, str, len + 1);
//     return ret;
// }

#endif

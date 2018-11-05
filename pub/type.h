#ifndef _PUB_TYPE_H_
#define _PUB_TYPE_H_

#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

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
        for (size_t i = 0; i < sizeof(ret); i++) { \
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

INLINE void
hexdump(char *desc, void *addr, size_t len)
{
    size_t i;
    byte_t buff[17];
    byte_t *pc = (byte_t *)addr;

    // Output description if given.
    if (desc != NULL)
        printf("%s:\n", desc);

    if (len == 0) {
        printf("  ZERO LENGTH\n");
        return;
    }

    if (len < 0) {
        printf("  NEGATIVE LENGTH: %lu\n", len);
        return;
    }

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                printf("  %s\n", buff);

            // Output the offset.
            printf("  %04lx ", i);
        }

        // Now the hex code for the specific character.
        printf(" %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];

        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        printf("   ");
        i++;
    }

    // And print the final ASCII bit.
    printf("  %s\n", buff);
}

#endif

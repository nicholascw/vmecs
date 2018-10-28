#ifndef _PUB_ERR_H_
#define _PUB_ERR_H_

#include <stdio.h>
#include <stdlib.h>

#define ASSERT(cond, ...) \
    if (!(cond)) { \
        fprintf(stderr, "%s: line %d: `%s` failed: ", __FILE__, __LINE__, #cond); \
        fprintf(stderr, __VA_ARGS__); \
        puts(""); \
        abort(); \
    }

#endif

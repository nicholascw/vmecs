#ifndef _PROTO_RBUF_H_
#define _PROTO_RBUF_H_

#include "pub/type.h"

#include "common.h"

// read buffer
typedef struct {
    byte_t *buf;
    size_t w_idx;
    size_t size;
} rbuffer_t;

typedef enum {
    RBUFFER_SUCCESS,
    RBUFFER_ERROR,
    RBUFFER_INCOMPLETE
} rbuffer_result_t;

rbuffer_t *
rbuffer_new(size_t init);

rbuffer_result_t
rbuffer_read(rbuffer_t *buf, int fd, decoder_t decoder, void *context, void *result);

void
rbuffer_free(rbuffer_t *buf);

#endif

#ifndef _PROTO_BUF_H_
#define _PROTO_BUF_H_

#include <pthread.h>

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

// a unidirectional variable buffer
typedef struct {
    pthread_mutex_t mut;
    pthread_cond_t cond;

    byte_t *buf;
    size_t w_idx;
    size_t size;

    bool eof;
} vbuffer_t;

vbuffer_t *
vbuffer_new(size_t init);

// may block if no data is ready
size_t
vbuffer_read(vbuffer_t *vbuf, byte_t *buf, size_t buf_size);

// write doesn't block
void
vbuffer_write(vbuffer_t *vbuf, const byte_t *buf, size_t buf_size);

void
vbuffer_close(vbuffer_t *vbuf);

void
vbuffer_free(vbuffer_t *vbuf);

#endif

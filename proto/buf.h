#ifndef _PROTO_BUF_H_
#define _PROTO_BUF_H_

#include "pub/type.h"
#include "pub/thread.h"

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
rbuffer_read(rbuffer_t *buf, fd_t fd, decoder_t decoder, void *context, void *result);

// non-blocking read
// will drain existing data in the fd
rbuffer_result_t
rbuffer_try_read(rbuffer_t *buf, fd_t fd, decoder_t decoder, void *context, void *result);

void
rbuffer_push(rbuffer_t *buf, const byte_t *data, size_t size);

void
rbuffer_free(rbuffer_t *buf);

// a unidirectional variable buffer
typedef struct {
    mutex_t *mut;
    cv_t *data;
    cv_t *drain;

    byte_t *buf;
    size_t w_idx;
    size_t size;

    bool eof;
} vbuffer_t;

vbuffer_t *
vbuffer_new(size_t init);

void
vbuffer_wait_drain(vbuffer_t *vbuf);

// flush out all the data
// this function will call vbuffer_close
void
vbuffer_drain(vbuffer_t *vbuf);

size_t
vbuffer_try_read(vbuffer_t *vbuf, byte_t *buf, size_t buf_size);

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

#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "buf.h"
#include "socket.h"

rbuffer_t *
rbuffer_new(size_t init)
{
    rbuffer_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    ret->size = init > 1 ? init : 1;
    ret->w_idx = 0;
    ret->buf = malloc(ret->size);
    ASSERT(ret->buf, "out of mem");

    return ret;
}

void
_rbuffer_expand(rbuffer_t *buf)
{
    buf->size <<= 1;
    buf->buf = realloc(buf->buf, buf->size);
}

rbuffer_result_t
rbuffer_read(rbuffer_t *buf, int fd, decoder_t decoder, void *context, void *result)
{
    size_t rest;
    ssize_t n_read = 0;
    ssize_t n_decoded;

    bool should_read = false;

    while (1) {
        if (should_read || buf->w_idx == 0) {
            // try read more iff should_read flag is set
            // or we have run out of data
            if (buf->w_idx >= buf->size) {
                _rbuffer_expand(buf);
            }

            rest = buf->size - buf->w_idx;
            n_read = read_r(fd, buf->buf + buf->w_idx, rest);

            if (n_read == -1) {
                return RBUFFER_ERROR;
            } // else hexdump("received", buf->buf + buf->w_idx, n_read);

            if (n_read == 0) {
                return RBUFFER_INCOMPLETE;
            }

            // actual data read, try decoding
            buf->w_idx += n_read;
            should_read = false;
        }
        
        // TRACE("decoding trunk of size %lu", buf->w_idx);
        n_decoded = decoder(context, result, buf->buf, buf->w_idx);
        // TRACE("decoded: %ld", n_decoded);

        if (n_decoded == 0) {
            should_read = true;
            // no enough data, continue
        } else if (n_decoded == -1) {
            // error
            return RBUFFER_ERROR;
        } else {
            // success, truncate the decoded part
            buf->w_idx -= n_decoded;
            memmove(buf->buf, buf->buf + n_decoded, buf->w_idx);

            // if (buf->w_idx) {
            //     hexdump("rest", buf->buf, buf->w_idx);
            // } else TRACE("no rest");

            return RBUFFER_SUCCESS;
        }
    }
}

void
rbuffer_free(rbuffer_t *buf)
{
    if (buf) {
        free(buf->buf);
        free(buf);
    }
}

vbuffer_t *
vbuffer_new(size_t init)
{
    vbuffer_t *ret = malloc(sizeof(*ret));
    ASSERT(ret, "out of mem");

    pthread_mutex_init(&ret->mut, NULL);
    pthread_cond_init(&ret->cond, NULL);
    pthread_cond_init(&ret->drain, NULL);

    ret->size = init > 1 ? init : 1;
    ret->w_idx = 0;
    ret->buf = malloc(ret->size);
    ret->eof = false;
    ASSERT(ret->buf, "out of mem");

    return ret;
}

void
vbuffer_wait_drain(vbuffer_t *vbuf)
{
    pthread_mutex_lock(&vbuf->mut);

    while (vbuf->w_idx) {
        pthread_cond_wait(&vbuf->drain, &vbuf->mut);
    }

    pthread_mutex_unlock(&vbuf->mut);
}

void
vbuffer_drain(vbuffer_t *vbuf)
{
    vbuffer_close(vbuf);

    pthread_mutex_lock(&vbuf->mut);

    vbuf->w_idx = 0;
    pthread_cond_broadcast(&vbuf->drain);

    pthread_mutex_unlock(&vbuf->mut);
}

size_t
vbuffer_read(vbuffer_t *vbuf, byte_t *buf, size_t buf_size)
{
    size_t n_read;

    pthread_mutex_lock(&vbuf->mut);

    while (!vbuf->w_idx && !vbuf->eof) {
        pthread_cond_wait(&vbuf->cond, &vbuf->mut);
    }

    if (!vbuf->w_idx && vbuf->eof) {
        pthread_mutex_unlock(&vbuf->mut);
        return 0;
    }

    n_read = buf_size > vbuf->w_idx ? vbuf->w_idx : buf_size;

    memcpy(buf, vbuf->buf, n_read);
    memmove(vbuf->buf, vbuf->buf + n_read, vbuf->w_idx - n_read);

    vbuf->w_idx -= n_read;

    if (vbuf->w_idx == 0) {
        pthread_cond_broadcast(&vbuf->drain);
    }

    pthread_mutex_unlock(&vbuf->mut);

    return n_read;
}

void
_vbuffer_extend(vbuffer_t *vbuf)
{
    vbuf->size <<= 1;
    vbuf->buf = realloc(vbuf->buf, vbuf->size);
}

void
vbuffer_write(vbuffer_t *vbuf, const byte_t *buf, size_t buf_size)
{
    pthread_mutex_lock(&vbuf->mut);

    if (vbuf->eof) {
        pthread_mutex_unlock(&vbuf->mut);
        return;
    }

    while (vbuf->w_idx + buf_size > vbuf->size) {
        _vbuffer_extend(vbuf);
    }

    memcpy(vbuf->buf + vbuf->w_idx, buf, buf_size);
    vbuf->w_idx += buf_size;

    pthread_cond_broadcast(&vbuf->cond);

    pthread_mutex_unlock(&vbuf->mut);
}

void
vbuffer_close(vbuffer_t *vbuf)
{
    pthread_mutex_lock(&vbuf->mut);

    if (!vbuf->eof) {
        vbuf->eof = true;
        pthread_cond_broadcast(&vbuf->cond);
    }

    pthread_mutex_unlock(&vbuf->mut);
}

void
vbuffer_free(vbuffer_t *vbuf)
{
    if (vbuf) {
        pthread_mutex_destroy(&vbuf->mut);
        pthread_cond_destroy(&vbuf->cond);
        pthread_cond_destroy(&vbuf->drain);
        free(vbuf->buf);
        free(vbuf);
    }
}

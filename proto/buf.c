#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "pub/socket.h"

#include "buf.h"

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

void
rbuffer_push(rbuffer_t *buf, const byte_t *data, size_t size)
{
    while (buf->w_idx + size >= buf->size) {
        _rbuffer_expand(buf);
    }

    memcpy(buf->buf + buf->w_idx, data, size);
    buf->w_idx += size;
}

rbuffer_result_t
rbuffer_try_read(rbuffer_t *buf, fd_t fd, decoder_t decoder, void *context, void *result)
{
    size_t rest;
    ssize_t n_read;
    ssize_t n_decoded;

    // read all we have
    do {
        if (buf->w_idx >= buf->size) {
            _rbuffer_expand(buf);
        }

        rest = buf->size - buf->w_idx;
        n_read = fd_try_read(fd, buf->buf + buf->w_idx, rest);

        // TRACE("n_read %ld", n_read);

        if (n_read == -1) return RBUFFER_ERROR;
        else if (n_read == -2) break;
        else {
            buf->w_idx += n_read;
        }
    } while (n_read > 0);

    n_decoded = decoder(context, result, buf->buf, buf->w_idx);

    if (n_decoded == 0) {
        // no enough data
        return RBUFFER_INCOMPLETE;
    } else if (n_decoded == -1) {
        // error
        return RBUFFER_ERROR;
    } else {
        // success
        buf->w_idx -= n_decoded;
        memmove(buf->buf, buf->buf + n_decoded, buf->w_idx);
        return RBUFFER_SUCCESS;
    }
}

rbuffer_result_t
rbuffer_read(rbuffer_t *buf, fd_t fd, decoder_t decoder, void *context, void *result)
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
            n_read = fd_read(fd, buf->buf + buf->w_idx, rest);

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

    ret->mut = mutex_new();
    ret->data = cv_new();
    ret->drain = cv_new();

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
    mutex_lock(vbuf->mut);

    while (vbuf->w_idx) {
        cv_wait(vbuf->drain, vbuf->mut);
    }

    mutex_unlock(vbuf->mut);
}

void
vbuffer_drain(vbuffer_t *vbuf)
{
    vbuffer_close(vbuf);

    mutex_lock(vbuf->mut);

    vbuf->w_idx = 0;
    cv_broadcast(vbuf->drain);

    mutex_unlock(vbuf->mut);
}

ssize_t
vbuffer_try_read(vbuffer_t *vbuf, byte_t *buf, size_t buf_size)
{
    ssize_t n_read;

    mutex_lock(vbuf->mut);

    if (vbuf->eof) {
        n_read = -1;
    } else if (vbuf->w_idx) {
        n_read = buf_size > vbuf->w_idx ? vbuf->w_idx : buf_size;

        memcpy(buf, vbuf->buf, n_read);
        memmove(vbuf->buf, vbuf->buf + n_read, vbuf->w_idx - n_read);

        vbuf->w_idx -= n_read;
    } else {
        n_read = 0;
    }

    mutex_unlock(vbuf->mut);

    return n_read;
}

size_t
vbuffer_read(vbuffer_t *vbuf, byte_t *buf, size_t buf_size)
{
    size_t n_read;

    mutex_lock(vbuf->mut);

    while (!vbuf->w_idx && !vbuf->eof) {
        cv_wait(vbuf->data, vbuf->mut);
    }

    if (!vbuf->w_idx && vbuf->eof) {
        mutex_unlock(vbuf->mut);
        return 0;
    }

    n_read = buf_size > vbuf->w_idx ? vbuf->w_idx : buf_size;

    memcpy(buf, vbuf->buf, n_read);
    memmove(vbuf->buf, vbuf->buf + n_read, vbuf->w_idx - n_read);

    vbuf->w_idx -= n_read;

    if (vbuf->w_idx == 0) {
        cv_broadcast(vbuf->drain);
    }

    mutex_unlock(vbuf->mut);

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
    mutex_lock(vbuf->mut);

    if (vbuf->eof) {
        mutex_unlock(vbuf->mut);
        return;
    }

    while (vbuf->w_idx + buf_size > vbuf->size) {
        _vbuffer_extend(vbuf);
    }

    memcpy(vbuf->buf + vbuf->w_idx, buf, buf_size);
    vbuf->w_idx += buf_size;

    cv_broadcast(vbuf->data);

    mutex_unlock(vbuf->mut);
}

void
vbuffer_close(vbuffer_t *vbuf)
{
    mutex_lock(vbuf->mut);

    if (!vbuf->eof) {
        vbuf->eof = true;
        cv_broadcast(vbuf->data);
    }

    mutex_unlock(vbuf->mut);
}

void
vbuffer_free(vbuffer_t *vbuf)
{
    if (vbuf) {
        mutex_free(vbuf->mut);
        cv_free(vbuf->data);
        cv_free(vbuf->drain);
        free(vbuf->buf);
        free(vbuf);
    }
}

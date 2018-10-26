#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

#include "rbuf.h"

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

            n_read = read(fd, buf->buf + buf->w_idx, rest);

            if (n_read == -1) {
                if (errno == EAGAIN || errno == EINTR) {
                    // read again
                    continue;
                } else {
                    // fail and return
                    return RBUFFER_ERROR;
                }
            }
        }

        // actual data read, try decoding
        buf->w_idx += n_read;
        
        n_decoded = decoder(context, result, buf->buf, buf->w_idx);

        if (n_decoded == 0) {
            if (n_read == 0) {
                if (should_read) {
                    return RBUFFER_INCOMPLETE;
                } else {
                    should_read = true;
                }
            }

            // no enough data, continue
        } else if (n_decoded == -1) {
            // error
            return RBUFFER_ERROR;
        } else {
            // success, truncate the decoded part
            buf->w_idx -= n_decoded;
            memmove(buf->buf, buf->buf + n_decoded, buf->w_idx);
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

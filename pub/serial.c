#include <stdlib.h>
#include <string.h>

#include "err.h"
#include "serial.h"

#define SERIAL_INIT_BUF 8

void serial_init(serial_t *ser, byte_t *buf, size_t size, int copy)
{
    ser->cap = ser->size = size;

    if (copy || (size && !buf)) {
        ser->buf = malloc(size);
        ASSERT(ser->buf, "out of mem");

        if (buf) memcpy(ser->buf, buf, size);
    } else {
        ser->buf = buf;
    }

    ser->r_idx = ser->w_idx = 0;
}

void serial_destroy(serial_t *ser)
{
    if (ser) {
        free(ser->buf);
    }
}

byte_t *serial_final(serial_t *ser)
{
    byte_t *ret = realloc(ser->buf, ser->size);

    ser->cap = ser->size = 0;
    ser->buf = NULL;
    ser->r_idx = ser->w_idx = 0;

    return ret;
}

INLINE void
serial_ensure(serial_t *ser, size_t size)
{
    size_t final = ser->cap ? ser->cap : SERIAL_INIT_BUF;

    while (ser->size + size > final) {
        final <<= 1;
    }

    if (final != ser->cap) {
        ser->buf = realloc(ser->buf, final);
        ASSERT(ser->buf, "out of mem");
        ser->cap = final;
    }
}

void serial_write_u8(serial_t *ser, uint8_t val)
{
    serial_ensure(ser, 1);
    ser->buf[ser->w_idx] = val;
    ser->size++;
    ser->w_idx++;
}

void serial_write_u16(serial_t *ser, uint16_t val)
{
    serial_ensure(ser, 2);
    *(uint16_t *)(ser->buf + ser->w_idx) = val;
    ser->size += 2;
    ser->w_idx += 2;
}

void serial_write_u32(serial_t *ser, uint32_t val)
{
    serial_ensure(ser, 4);
    *(uint32_t *)(ser->buf + ser->w_idx) = val;
    ser->size += 4;
    ser->w_idx += 4;
}

void serial_write_u64(serial_t *ser, uint64_t val)
{
    serial_ensure(ser, 8);
    *(uint64_t *)(ser->buf + ser->w_idx) = val;
    ser->size += 8;
    ser->w_idx += 8;
}

void serial_write(serial_t *ser, const void *data, size_t size)
{
    serial_ensure(ser, size);
    memcpy(ser->buf + ser->w_idx, data, size);
    ser->size += size;
    ser->w_idx += size;
}

// return the length of read data
int serial_read(serial_t *ser, void *buf, size_t size)
{
    if (ser->r_idx + size > ser->size) return 0;

    size_t remain = ser->size - ser->r_idx;
    size_t read = size > remain ? remain : size;

    memcpy(buf, ser->buf + ser->r_idx, read);

    return read;
}

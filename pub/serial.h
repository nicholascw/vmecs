#ifndef _PUB_SERIAL_H_
#define _PUB_SERIAL_H_

#include "type.h"

typedef struct {
    size_t cap;
    size_t size;
    byte_t *buf;

    size_t r_idx;
    size_t w_idx;
} serial_t;

void serial_init(serial_t *ser, byte_t *buf, size_t size, int copy);
void serial_destroy(serial_t *ser);
byte_t *serial_final(serial_t *ser); // no need to call destroy after final

INLINE size_t serial_size(serial_t *ser)
{
    return ser->size;
}

INLINE byte_t *serial_ofs(serial_t *ser, size_t ofs)
{
    return ser->buf + ofs;
}

void serial_write_u8(serial_t *ser, uint8_t val);
void serial_write_u16(serial_t *ser, uint16_t val);
void serial_write_u32(serial_t *ser, uint32_t val);
void serial_write_u64(serial_t *ser, uint64_t val);
void serial_write(serial_t *ser, const void *data, size_t size);

// return 1 if succeed
int serial_read(serial_t *ser, void *buf, size_t size);

#endif

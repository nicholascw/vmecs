#ifndef _PROTO_VMESS_ENCODING_H_
#define _PROTO_VMESS_ENCODING_H_

#include "vmess.h"

vmess_request_t *
vmess_decode_request(const byte_t *data, size_t size);

#endif

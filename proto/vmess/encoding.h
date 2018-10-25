#ifndef _PROTO_VMESS_ENCODING_H_
#define _PROTO_VMESS_ENCODING_H_

#include "vmess.h"

ssize_t
vmess_decode_data(vmess_config_t *config, uint64_t gen_time,
                  const byte_t *data, size_t size,
                  byte_t **result, size_t *res_size);

// return -1 if wrong checksum
// return 0 if decoding failed
// return number of bytes read otherwise
ssize_t
vmess_decode_request(vmess_config_t *config, vmess_request_t * req,
                     const byte_t *data, size_t size);

#endif

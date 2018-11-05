#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

#include "pub/type.h"
#include "pub/fd.h"

#include "toml/toml.h"
#include "crypto/hash.h"

#include "proto/relay/tcp.h"

#include "proto/vmess/vmess.h"
#include "proto/vmess/inbound.h"
#include "proto/vmess/outbound.h"

#include "proto/socks5/inbound.h"
#include "proto/socks5/outbound.h"

#include "proto/native/outbound.h"

#define HELP \
    "usage: %s <config>\n"

void print_help(const char *prog)
{
    fprintf(stderr, HELP, prog ? prog : "vmecs");
}

toml_object_t *
load_config(const char *path)
{
    FILE *fp = fopen(path, "rb");
    size_t size;
    char *buf;
    toml_object_t *conf;
    toml_err_t err;

    if (!fp) {
        perror("fopen");
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    buf = malloc(size + 1);
    assert(buf);
    
    if (fread(buf, 1, size, fp) != size) {
        fprintf(stderr, "failed to read file\n");
        fclose(fp);
        free(buf);
        return NULL;
    }

    fclose(fp);

    buf[size] = 0;

    conf = toml_load(buf, &err);
    free(buf);

    if (!conf) {
        fprintf(stderr, "parse error: %s\n", err.msg);
        return NULL;
    }

    return conf;
}

bool
config_lookup_string(toml_object_t *conf, const char *query, const char **res)
{
    object_t *found = toml_lookup(conf, query, NULL);

    if (!found || !IS_TYPE(found, STRING)) {
        return false;
    }

    *res = ((string_object_t *)found)->str;
    return true;
}

bool
config_lookup_int(toml_object_t *conf, const char *query, long long *res)
{
    object_t *found = toml_lookup(conf, query, NULL);

    if (!found || !IS_TYPE(found, INT)) {
        return false;
    }

    *res = ((int_object_t *)found)->val;
    return true;
}

bool
config_lookup_bool(toml_object_t *conf, const char *query, bool *res)
{
    object_t *found = toml_lookup(conf, query, NULL);

    if (!found || !IS_TYPE(found, BOOL)) {
        return false;
    }

    *res = ((bool_object_t *)found)->val;
    return true;
}

typedef tcp_inbound_t *(*inbound_builder_t)(toml_object_t *config);
typedef tcp_outbound_t *(*outbound_builder_t)(toml_object_t *config);

target_id_t *
load_inbound_param(toml_object_t *config)
{
    const char *local;
    const char *port;
    target_id_t *ret;

    if (!config_lookup_string(config, "inbound.local", &local)) {
        fprintf(stderr, "inbound.local not defined\n");
        return NULL;
    }

    if (!config_lookup_string(config, "inbound.port", &port)) {
        fprintf(stderr, "inbound.port not defined\n");
        return NULL;
    }

    ret = target_id_parse(local, port);

    if (!ret) {
        fprintf(stderr, "failed to parse inbound.local/inbound.port\n");
        return NULL;
    }

    return ret;
}

target_id_t *
load_outbound_param(toml_object_t *config)
{
    const char *proxy;
    const char *port;
    target_id_t *ret;

    if (!config_lookup_string(config, "outbound.proxy", &proxy)) {
        fprintf(stderr, "outbound.proxy not defined\n");
        return NULL;
    }

    if (!config_lookup_string(config, "outbound.port", &port)) {
        fprintf(stderr, "outbound.port not defined\n");
        return NULL;
    }

    ret = target_id_parse(proxy, port);

    if (!ret) {
        fprintf(stderr, "failed to parse outbound.local/outbound.port\n");
        return NULL;
    }

    return ret;
}

tcp_inbound_t *vmess_inbound_builder(toml_object_t *config)
{
    const char *pass;
    size_t pass_len;
    target_id_t *local;
    vmess_config_t *vmess_config;
    vmess_tcp_inbound_t *inbound;
    
    data128_t user_id;

    if (!config_lookup_string(config, "inbound.pass", &pass)) {
        fprintf(stderr, "inbound.pass not defined\n");
        return NULL;
    }

    local = load_inbound_param(config);
    if (!local) return NULL;

    pass_len = strlen(pass);
    ASSERT(crypto_hmac_md5((byte_t *)pass, pass_len,
                           (byte_t *)pass, pass_len, user_id) == 0, "hmac md5 failed");

    vmess_config = vmess_config_new(user_id);
    inbound = vmess_tcp_inbound_new(vmess_config, local);

    vmess_config_free(vmess_config);
    target_id_free(local);

    return (tcp_inbound_t *)inbound;
}

tcp_outbound_t *vmess_outbound_builder(toml_object_t *config)
{
    const char *pass;
    size_t pass_len;
    target_id_t *proxy;
    vmess_config_t *vmess_config;
    vmess_tcp_outbound_t *outbound;
    
    data128_t user_id;

    if (!config_lookup_string(config, "outbound.pass", &pass)) {
        fprintf(stderr, "outbound.pass not defined\n");
        return NULL;
    }

    proxy = load_outbound_param(config);
    if (!proxy) return NULL;

    pass_len = strlen(pass);
    ASSERT(crypto_hmac_md5((byte_t *)pass, pass_len,
                           (byte_t *)pass, pass_len, user_id) == 0, "hmac md5 failed");

    vmess_config = vmess_config_new(user_id);
    outbound = vmess_tcp_outbound_new(vmess_config, proxy);

    vmess_config_free(vmess_config);
    target_id_free(proxy);

    return (tcp_outbound_t *)outbound;
}

tcp_inbound_t *socks5_inbound_builder(toml_object_t *config)
{
    target_id_t *local;
    socks5_tcp_inbound_t *inbound;
    
    local = load_inbound_param(config);
    if (!local) return NULL;

    inbound = socks5_tcp_inbound_new(local);

    target_id_free(local);

    return (tcp_inbound_t *)inbound;
}

tcp_outbound_t *socks5_outbound_builder(toml_object_t *config)
{
    target_id_t *proxy;
    socks5_tcp_outbound_t *outbound;
    
    proxy = load_outbound_param(config);
    if (!proxy) return NULL;

    outbound = socks5_tcp_outbound_new(proxy);

    target_id_free(proxy);

    return (tcp_outbound_t *)outbound;
}

tcp_outbound_t *native_outbound_builder(toml_object_t *config)
{
    return (tcp_outbound_t *)native_tcp_outbound_new();
}

struct {
    const char *proto;
    inbound_builder_t in_build;
    outbound_builder_t out_build;
} proto_map[] = {
    { "vmess", vmess_inbound_builder, vmess_outbound_builder },
    { "socks5", socks5_inbound_builder, socks5_outbound_builder },
    { "native", NULL, native_outbound_builder }
};

#define PROTO_MAP_COUNT (sizeof(proto_map) / sizeof(*proto_map))

tcp_inbound_t *
load_inbound(toml_object_t *config)
{
    const char *proto;
    size_t i;

    if (!config_lookup_string(config, "inbound.proto", &proto)) {
        fprintf(stderr, "inbound.proto not defined\n");
        return NULL;
    }

    for (i = 0; i < PROTO_MAP_COUNT; i++) {
        if (strcmp(proto_map[i].proto, proto) == 0) {
            if (proto_map[i].in_build) {
                return proto_map[i].in_build(config);
            } else {
                fprintf(stderr, "protocol %s cannot be an inbound protocol\n", proto);
            }
        }
    }

    fprintf(stderr, "inbound protocol %s not found\n", proto);
    
    return NULL;
}

tcp_outbound_t *
load_outbound(toml_object_t *config)
{
    const char *proto;
    size_t i;

    if (!config_lookup_string(config, "outbound.proto", &proto)) {
        fprintf(stderr, "outbound.proto not defined\n");
        return NULL;
    }

    for (i = 0; i < PROTO_MAP_COUNT; i++) {
        if (strcmp(proto_map[i].proto, proto) == 0) {
            if (proto_map[i].out_build) {
                return proto_map[i].out_build(config);
            } else {
                fprintf(stderr, "protocol %s cannot be an outbound protocol\n", proto);
            }
        }
    }

    fprintf(stderr, "outbound protocol %s not found\n", proto);
    
    return NULL;
}

tcp_relay_config_t *
load_relay_config(toml_object_t *config)
{
    return tcp_relay_config_new_default();
}

void sigpipe_handler(int sig)
{
    const char msg[] = "pipe broken\n";
    fd_write(STDERR_FILENO, (byte_t *)msg, sizeof(msg) - 1);
}

int main(int argc, const char **argv)
{
    toml_object_t *config;

    tcp_relay_config_t *relay_config;

    tcp_inbound_t *inbound;
    tcp_outbound_t *outbound;

    signal(SIGPIPE, sigpipe_handler);

    if (argc < 2) {
        print_help(argv[0]);
        return 1;
    }

    config = load_config(argv[1]);

    if (!config) {
        fprintf(stderr, "failed to load config file %s\n", argv[1]);
        return 1;
    }

    inbound = load_inbound(config);
    outbound = load_outbound(config);
    relay_config = load_relay_config(config);

    if (!inbound || !outbound) {
        fprintf(stderr, "failed to load inbound/outbound\n");

        tcp_inbound_free(inbound);
        tcp_outbound_free(outbound);
        tcp_relay_config_free(relay_config);

        toml_free(config);
        return 1;
    }

    tcp_relay(relay_config, inbound, outbound);

    tcp_inbound_free(inbound);
    tcp_outbound_free(outbound);
    tcp_relay_config_free(relay_config);

    toml_free(config);

    return 0;
}

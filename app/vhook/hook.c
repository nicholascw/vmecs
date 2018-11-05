#define _GNU_SOURCE // enable RTLD_NEXT
#include <dlfcn.h>

#include <fcntl.h>

#include <string.h>

#include "pub/type.h"
#include "pub/socket.h"

#include "proto/socks5/tcp.h"

#include "hook.h"

/* helper functions */

static bool proxy_on = true;
static bool init = false;
static target_id_t *target_proxy = NULL;

// define hook types
#define HOOK(name, ret, param, block) \
    typedef ret (*REAL_TYPE(name))param; \
    DEF_REAL(name);

#include "hook.def"

#undef HOOK

// define hook functions
#define HOOK(name, ret, param, block) ret name param block

#include "hook.def"

#undef HOOK

void *load_libc(const char *name)
{
    void *func = dlsym(RTLD_NEXT, name);
    if (!func) return NULL;
    return func;
}

__attribute__((constructor))
static void init_hook()
{
    const char *proxy;
    const char *port;

    if (init) return;
    init = true;

    // load libc functions
#define HOOK(name, ret, param, block) LOAD_REAL(name);
#include "hook.def"
#undef HOOK

    proxy = getenv(PROXY_ENV_VAR);
    port = getenv(PORT_ENV_VAR);

    if (!proxy || !port) {
        TRACE("no proxy given");
        exit(1);
    }

    TRACE("init hook, proxy %s:%s", proxy, port);

    // set target proxy
    target_proxy = target_id_parse(proxy, port);

    if (!target_proxy) {
        TRACE("failed to parse proxy address, exiting");
        exit(1);
    }
}

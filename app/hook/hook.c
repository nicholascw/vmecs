#define _GNU_SOURCE // enable RTLD_NEXT
#include <dlfcn.h>

#include <string.h>

#include "pub/type.h"

#include "proto/socket.h"
#include "proto/socks5/tcp.h"

#include "hook.h"

/* helper functions */

static bool proxy_on = true;

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

static bool init = false;

__attribute__((constructor))
static void init_hook()
{
    if (init) return;
    init = true;

    TRACE("init hook");

    // load libc functions
#define HOOK(name, ret, param, block) LOAD_REAL(name);
#include "hook.def"
#undef HOOK
}

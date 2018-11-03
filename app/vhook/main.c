#include <unistd.h>
#include <stdlib.h>

#include "pub/type.h"
#include "hook.h"

#ifndef VHOOK_CORE
    #error VHOOK_CORE not defined
#endif

char *base_path(const char *path)
{
    size_t found = 0;
    size_t i, len = strlen(path);
    char *ret;

    for (i = 0; i < len; i++)
        if (path[i] == '/')
            found = i;

    ret = malloc(found + 1);
    ASSERT(ret, "out of mem");

    memcpy(ret, path, found);
    ret[found] = 0;

    return ret;
}

int main(int argc, char **argv)
{
    if (argc < 4) {
        TRACE("no enough argument");
        TRACE("usage: %s <proxy> <port> <client program> [[arg 1] ...]", argv[0] ? argv[0] : "vhook");
        return 1;
    }

    const char *proxy = argv[1];
    const char *port = argv[2];

    char *path = base_path(argv[0]);
    size_t len = strlen(path);
    path = realloc(path, len + sizeof(VHOOK_CORE) + 1);

    path[len] = '/';
    memcpy(path + len + 1, VHOOK_CORE, sizeof(VHOOK_CORE)); // last nil is included
    // path = base path + '/' + VHOOK_CORE

    setenv("LD_PRELOAD", path, 1);
    free(path);

    setenv(PROXY_ENV_VAR, proxy, 1);
    setenv(PORT_ENV_VAR, port, 1);

    execvp(argv[3], argv + 3);
    TRACE("exec failed");

    return 1;
}

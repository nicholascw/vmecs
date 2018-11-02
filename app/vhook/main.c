#include <unistd.h>
#include <stdlib.h>

#include "pub/type.h"

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
    if (argc < 2) {
        TRACE("no enough argument");
        return 1;
    }

    char *path = base_path(argv[0]);
    size_t len = strlen(path);
    path = realloc(path, len + sizeof(VHOOK_CORE) + 1);

    path[len] = '/';
    memcpy(path + len + 1, VHOOK_CORE, sizeof(VHOOK_CORE)); // last nil is included

    setenv("LD_PRELOAD", path, 1);
    free(path);

    execvp(argv[1], argv + 2);
    TRACE("exec failed");

    return 1;
}

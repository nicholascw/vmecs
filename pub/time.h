#ifndef _PUB_TIME_H_
#define _PUB_TIME_H_

#include <time.h>

#include "type.h"

// sleep millisecond
INLINE void msleep(long msec)
{
    struct timespec t;
    t.tv_sec = msec / 1000l;
    t.tv_nsec = msec * 1000000l;
    while (nanosleep(&t, &t));
}

#endif

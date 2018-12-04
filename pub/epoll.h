#ifndef _PUB_EPOLL_H_
#define _PUB_EPOLL_H_

#include <sys/epoll.h>

#include "type.h"
#include "fd.h"

typedef fd_t epoll_t;

typedef struct epoll_event epoll_event_t;

INLINE epoll_t
fd_epoll_create()
{
    return epoll_create1(0);
}

#define fd_epoll_ctl epoll_ctl
#define fd_epoll_wait epoll_wait

#define FD_EPOLL_ADD EPOLL_CTL_ADD
#define FD_EPOLL_MOD EPOLL_CTL_MOD
#define FD_EPOLL_DEL EPOLL_CTL_DEL

#define FD_EPOLL_READ EPOLLIN
#define FD_EPOLL_WRITE EPOLLOUT
#define FD_EPOLL_ET EPOLLET
#define FD_EPOLL_HUP EPOLLHUP
#define FD_EPOLL_RDHUP EPOLLRDHUP

#endif

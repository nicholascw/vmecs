#ifndef _PUB_THREAD_H_
#define _PUB_THREAD_H_

// an abstraction layer for threading

#include <pthread.h>
#include <stdlib.h>
#include <errno.h>

#include "type.h"

typedef pthread_t thread_t;
typedef void *(*thread_func_t)(void *);

typedef pthread_mutex_t mutex_t;
typedef pthread_cond_t cv_t;

INLINE thread_t
thread_new(thread_func_t func, void *arg)
{
    thread_t tid;
    int err;

    while ((err = pthread_create(&tid, NULL, func, arg)) && err == EAGAIN) {
        perror("pthread_create");
    }

    ASSERT(!err, "failed to create thread");

    return tid;
}

INLINE void
thread_detach(thread_t tid)
{
    if (pthread_detach(tid)) {
        perror("pthread_detach");
    }
}

INLINE void *
thread_join(thread_t tid)
{
    void *res = NULL;

    if (pthread_join(tid, &res)) {
        perror("pthread_join");
    }

    return res;
}

INLINE mutex_t *
mutex_new()
{
    mutex_t *mut = malloc(sizeof(*mut));
    ASSERT(mut, "out of mem");

    pthread_mutex_init(mut, NULL);

    return mut;
}

INLINE void
mutex_free(mutex_t *mut)
{
    if (mut) {
        pthread_mutex_destroy(mut);
        free(mut);
    }
}

INLINE void
mutex_lock(mutex_t *mut)
{
    pthread_mutex_lock(mut);
}

INLINE void
mutex_unlock(mutex_t *mut)
{
    pthread_mutex_unlock(mut);
}

INLINE cv_t *
cv_new()
{
    cv_t *cv = malloc(sizeof(*cv));
    ASSERT(cv, "out of mem");

    pthread_cond_init(cv, NULL);

    return cv;
}

INLINE void
cv_free(cv_t *cv)
{
    if (cv) {
        pthread_cond_destroy(cv);
        free(cv);
    }
}

INLINE void
cv_wait(cv_t *cv, mutex_t *mut)
{
    pthread_cond_wait(cv, mut);
}

INLINE void
cv_broadcast(cv_t *cv)
{
    pthread_cond_broadcast(cv);
}

INLINE void
cv_signal(cv_t *cv)
{
    pthread_cond_signal(cv);
}

#endif

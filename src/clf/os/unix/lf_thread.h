
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_THREAD_H_INCLUDED_
#define _lf_THREAD_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>

#if (lf_THREADS)

#include <pthread.h>


typedef pthread_mutex_t  lf_thread_mutex_t;

lf_int_t lf_thread_mutex_create(lf_thread_mutex_t *mtx, lf_log_t *log);
lf_int_t lf_thread_mutex_destroy(lf_thread_mutex_t *mtx, lf_log_t *log);
lf_int_t lf_thread_mutex_lock(lf_thread_mutex_t *mtx, lf_log_t *log);
lf_int_t lf_thread_mutex_unlock(lf_thread_mutex_t *mtx, lf_log_t *log);


typedef pthread_cond_t  lf_thread_cond_t;

lf_int_t lf_thread_cond_create(lf_thread_cond_t *cond, lf_log_t *log);
lf_int_t lf_thread_cond_destroy(lf_thread_cond_t *cond, lf_log_t *log);
lf_int_t lf_thread_cond_signal(lf_thread_cond_t *cond, lf_log_t *log);
lf_int_t lf_thread_cond_wait(lf_thread_cond_t *cond, lf_thread_mutex_t *mtx,
    lf_log_t *log);


#if (lf_LINUX)

typedef pid_t      lf_tid_t;
#define lf_TID_T_FMT         "%P"

#elif (lf_FREEBSD)

typedef uint32_t   lf_tid_t;
#define lf_TID_T_FMT         "%uD"

#elif (lf_DARWIN)

typedef uint64_t   lf_tid_t;
#define lf_TID_T_FMT         "%uL"

#else

typedef uint64_t   lf_tid_t;
#define lf_TID_T_FMT         "%uL"

#endif

lf_tid_t lf_thread_tid(void);

#define lf_log_tid           lf_thread_tid()

#else

#define lf_log_tid           0
#define lf_TID_T_FMT         "%d"

#endif


#endif /* _lf_THREAD_H_INCLUDED_ */

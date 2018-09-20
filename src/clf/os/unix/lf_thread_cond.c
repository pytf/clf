
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


lf_int_t
lf_thread_cond_create(lf_thread_cond_t *cond, lf_log_t *log)
{
    lf_err_t  err;

    err = pthread_cond_init(cond, NULL);
    if (err == 0) {
        return lf_OK;
    }

    lf_log_error(lf_LOG_EMERG, log, err, "pthread_cond_init() failed");
    return lf_ERROR;
}


lf_int_t
lf_thread_cond_destroy(lf_thread_cond_t *cond, lf_log_t *log)
{
    lf_err_t  err;

    err = pthread_cond_destroy(cond);
    if (err == 0) {
        return lf_OK;
    }

    lf_log_error(lf_LOG_EMERG, log, err, "pthread_cond_destroy() failed");
    return lf_ERROR;
}


lf_int_t
lf_thread_cond_signal(lf_thread_cond_t *cond, lf_log_t *log)
{
    lf_err_t  err;

    err = pthread_cond_signal(cond);
    if (err == 0) {
        return lf_OK;
    }

    lf_log_error(lf_LOG_EMERG, log, err, "pthread_cond_signal() failed");
    return lf_ERROR;
}


lf_int_t
lf_thread_cond_wait(lf_thread_cond_t *cond, lf_thread_mutex_t *mtx,
    lf_log_t *log)
{
    lf_err_t  err;

    err = pthread_cond_wait(cond, mtx);

#if 0
    lf_time_update();
#endif

    if (err == 0) {
        return lf_OK;
    }

    lf_log_error(lf_LOG_ALERT, log, err, "pthread_cond_wait() failed");

    return lf_ERROR;
}

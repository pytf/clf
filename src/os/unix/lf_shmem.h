
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_SHMEM_H_INCLUDED_
#define _lf_SHMEM_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


typedef struct {
    u_char      *addr;
    size_t       size;
    lf_str_t    name;
    lf_log_t   *log;
    lf_uint_t   exists;   /* unsigned  exists:1;  */
} lf_shm_t;


lf_int_t lf_shm_alloc(lf_shm_t *shm);
void lf_shm_free(lf_shm_t *shm);


#endif /* _lf_SHMEM_H_INCLUDED_ */

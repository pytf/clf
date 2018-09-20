
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_USER_H_INCLUDED_
#define _lf_USER_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


typedef uid_t  lf_uid_t;
typedef gid_t  lf_gid_t;


lf_int_t lf_libc_crypt(lf_pool_t *pool, u_char *key, u_char *salt,
    u_char **encrypted);


#endif /* _lf_USER_H_INCLUDED_ */

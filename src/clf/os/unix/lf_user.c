
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_CRYPT)

#if (lf_HAVE_GNU_CRYPT_R)

lf_int_t
lf_libc_crypt(lf_pool_t *pool, u_char *key, u_char *salt, u_char **encrypted)
{
    char               *value;
    size_t              len;
    struct crypt_data   cd;

    cd.initialized = 0;

    value = crypt_r((char *) key, (char *) salt, &cd);

    if (value) {
        len = lf_strlen(value) + 1;

        *encrypted = lf_pnalloc(pool, len);
        if (*encrypted == NULL) {
            return lf_ERROR;
        }

        lf_memcpy(*encrypted, value, len);
        return lf_OK;
    }

    lf_log_error(lf_LOG_CRIT, pool->log, lf_errno, "crypt_r() failed");

    return lf_ERROR;
}

#else

lf_int_t
lf_libc_crypt(lf_pool_t *pool, u_char *key, u_char *salt, u_char **encrypted)
{
    char       *value;
    size_t      len;
    lf_err_t   err;

    value = crypt((char *) key, (char *) salt);

    if (value) {
        len = lf_strlen(value) + 1;

        *encrypted = lf_pnalloc(pool, len);
        if (*encrypted == NULL) {
            return lf_ERROR;
        }

        lf_memcpy(*encrypted, value, len);
        return lf_OK;
    }

    err = lf_errno;

    lf_log_error(lf_LOG_CRIT, pool->log, err, "crypt() failed");

    return lf_ERROR;
}

#endif

#endif /* lf_CRYPT */

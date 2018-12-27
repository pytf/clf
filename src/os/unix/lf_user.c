#include <lf_config.h>
#include <lf_core.h>


#if (lf_CRYPT)

#if (lf_HAVE_GNU_CRYPT_R)

int lf_libc_crypt(lf_pool_t *pool, unchar *key, unchar *salt, unchar **encrypted)
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
            return RET_ERR;
        }

        lf_memcpy(*encrypted, value, len);
        return RET_OK;
    }

    lf_log_error(lf_LOG_CRIT, pool->log, lf_errno, "crypt_r() failed");

    return RET_ERR;
}

#else

int lf_libc_crypt(lf_pool_t *pool, unchar *key, unchar *salt, unchar **encrypted)
{
    char       *value;
    size_t      len;
    int   err;

    value = crypt((char *) key, (char *) salt);

    if (value) {
        len = lf_strlen(value) + 1;

        *encrypted = lf_pnalloc(pool, len);
        if (*encrypted == NULL) {
            return RET_ERR;
        }

        lf_memcpy(*encrypted, value, len);
        return RET_OK;
    }

    err = lf_errno;

    lf_log_std(LOG_LEVEL_CRIT, "crypt() failed");

    return RET_ERR;
}

#endif

#endif /* lf_CRYPT */

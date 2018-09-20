
/*
 * 
 * 
 */


#include <lf_config.h>
#include <lf_core.h>


/*
 * The strerror() messages are copied because:
 *
 * 1) strerror() and strerror_r() functions are not Async-Signal-Safe,
 *    therefore, they cannot be used in signal handlers;
 *
 * 2) a direct sys_errlist[] array may be used instead of these functions,
 *    but Linux linker warns about its usage:
 *
 * warning: `sys_errlist' is deprecated; use `strerror' or `strerror_r' instead
 * warning: `sys_nerr' is deprecated; use `strerror' or `strerror_r' instead
 *
 *    causing false bug reports.
 */


static lf_str_t  *lf_sys_errlist;
static lf_str_t   lf_unknown_error = lf_string("Unknown error");


u_char *
lf_strerror(lf_err_t err, u_char *errstr, size_t size)
{
    lf_str_t  *msg;

    msg = ((lf_uint_t) err < lf_SYS_NERR) ? &lf_sys_errlist[err]:
                                              &lf_unknown_error;
    size = lf_min(size, msg->len);

    return lf_cpymem(errstr, msg->data, size);
}


lf_int_t
lf_strerror_init(void)
{
    char       *msg;
    u_char     *p;
    size_t      len;
    lf_err_t   err;

    /*
     * lf_strerror() is not ready to work at this stage, therefore,
     * malloc() is used and possible errors are logged using strerror().
     */

    len = lf_SYS_NERR * sizeof(lf_str_t);

    lf_sys_errlist = malloc(len);
    if (lf_sys_errlist == NULL) {
        goto failed;
    }

    for (err = 0; err < lf_SYS_NERR; err++) {
        msg = strerror(err);
        len = lf_strlen(msg);

        p = malloc(len);
        if (p == NULL) {
            goto failed;
        }

        lf_memcpy(p, msg, len);
        lf_sys_errlist[err].len = len;
        lf_sys_errlist[err].data = p;
    }

    return lf_OK;

failed:

    err = errno;
    lf_log_stderr(0, "malloc(%uz) failed (%d: %s)", len, err, strerror(err));

    return lf_ERROR;
}

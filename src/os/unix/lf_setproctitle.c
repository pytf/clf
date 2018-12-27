
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_SETPROCTITLE_USES_ENV)

/*
 * To change the process title in Linux and Solaris we have to set argv[1]
 * to NULL and to copy the title to the same place where the argv[0] points to.
 * However, argv[0] may be too small to hold a new title.  Fortunately, Linux
 * and Solaris store argv[] and environ[] one after another.  So we should
 * ensure that is the continuous memory and then we allocate the new memory
 * for environ[] and copy it.  After this we could use the memory starting
 * from argv[0] for our process title.
 *
 * The Solaris's standard /bin/ps does not show the changed process title.
 * You have to use "/usr/ucb/ps -w" instead.  Besides, the UCB ps does not
 * show a new title if its length less than the origin command line length.
 * To avoid it we append to a new title the origin command line in the
 * parenthesis.
 */

extern char **environ;

static char *lf_os_argv_last;

lf_int_t
lf_init_setproctitle(lf_log_t *log)
{
    u_char      *p;
    size_t       size;
    lf_uint_t   i;

    size = 0;

    for (i = 0; environ[i]; i++) {
        size += lf_strlen(environ[i]) + 1;
    }

    p = lf_alloc(size, log);
    if (p == NULL) {
        return lf_ERROR;
    }

    lf_os_argv_last = lf_os_argv[0];

    for (i = 0; lf_os_argv[i]; i++) {
        if (lf_os_argv_last == lf_os_argv[i]) {
            lf_os_argv_last = lf_os_argv[i] + lf_strlen(lf_os_argv[i]) + 1;
        }
    }

    for (i = 0; environ[i]; i++) {
        if (lf_os_argv_last == environ[i]) {

            size = lf_strlen(environ[i]) + 1;
            lf_os_argv_last = environ[i] + size;

            lf_cpystrn(p, (u_char *) environ[i], size);
            environ[i] = (char *) p;
            p += size;
        }
    }

    lf_os_argv_last--;

    return lf_OK;
}


void
lf_setproctitle(char *title)
{
    u_char     *p;

#if (lf_SOLARIS)

    lf_int_t   i;
    size_t      size;

#endif

    lf_os_argv[1] = NULL;

    p = lf_cpystrn((u_char *) lf_os_argv[0], (u_char *) "nginx: ",
                    lf_os_argv_last - lf_os_argv[0]);

    p = lf_cpystrn(p, (u_char *) title, lf_os_argv_last - (char *) p);

#if (lf_SOLARIS)

    size = 0;

    for (i = 0; i < lf_argc; i++) {
        size += lf_strlen(lf_argv[i]) + 1;
    }

    if (size > (size_t) ((char *) p - lf_os_argv[0])) {

        /*
         * lf_setproctitle() is too rare operation so we use
         * the non-optimized copies
         */

        p = lf_cpystrn(p, (u_char *) " (", lf_os_argv_last - (char *) p);

        for (i = 0; i < lf_argc; i++) {
            p = lf_cpystrn(p, (u_char *) lf_argv[i],
                            lf_os_argv_last - (char *) p);
            p = lf_cpystrn(p, (u_char *) " ", lf_os_argv_last - (char *) p);
        }

        if (*(p - 1) == ' ') {
            *(p - 1) = ')';
        }
    }

#endif

    if (lf_os_argv_last - (char *) p) {
        lf_memset(p, lf_SETPROCTITLE_PAD, lf_os_argv_last - (char *) p);
    }

    lf_log_debug1(lf_LOG_DEBUG_CORE, lf_cycle->log, 0,
                   "setproctitle: \"%s\"", lf_os_argv[0]);
}

#endif /* lf_SETPROCTITLE_USES_ENV */

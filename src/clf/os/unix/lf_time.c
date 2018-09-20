
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


/*
 * FreeBSD does not test /etc/localtime change, however, we can workaround it
 * by calling tzset() with TZ and then without TZ to update timezone.
 * The trick should work since FreeBSD 2.1.0.
 *
 * Linux does not test /etc/localtime change in localtime(),
 * but may stat("/etc/localtime") several times in every strftime(),
 * therefore we use it to update timezone.
 *
 * Solaris does not test /etc/TIMEZONE change too and no workaround available.
 */

void
lf_timezone_update(void)
{
#if (lf_FREEBSD)

    if (getenv("TZ")) {
        return;
    }

    putenv("TZ=UTC");

    tzset();

    unsetenv("TZ");

    tzset();

#elif (lf_LINUX)
    time_t      s;
    struct tm  *t;
    char        buf[4];

    s = time(0);

    t = localtime(&s);

    strftime(buf, 4, "%H", t);

#endif
}


void
lf_localtime(time_t s, lf_tm_t *tm)
{
#if (lf_HAVE_LOCALTIME_R)
    (void) localtime_r(&s, tm);

#else
    lf_tm_t  *t;

    t = localtime(&s);
    *tm = *t;

#endif

    tm->lf_tm_mon++;
    tm->lf_tm_year += 1900;
}


void
lf_libc_localtime(time_t s, struct tm *tm)
{
#if (lf_HAVE_LOCALTIME_R)
    (void) localtime_r(&s, tm);

#else
    struct tm  *t;

    t = localtime(&s);
    *tm = *t;

#endif
}


void
lf_libc_gmtime(time_t s, struct tm *tm)
{
#if (lf_HAVE_LOCALTIME_R)
    (void) gmtime_r(&s, tm);

#else
    struct tm  *t;

    t = gmtime(&s);
    *tm = *t;

#endif
}

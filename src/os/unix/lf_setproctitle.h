
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_SETPROCTITLE_H_INCLUDED_
#define _lf_SETPROCTITLE_H_INCLUDED_


#if (lf_HAVE_SETPROCTITLE)

/* FreeBSD, NetBSD, OpenBSD */

#define lf_init_setproctitle(log) lf_OK
#define lf_setproctitle(title)    setproctitle("%s", title)


#else /* !lf_HAVE_SETPROCTITLE */

#if !defined lf_SETPROCTITLE_USES_ENV

#if (lf_SOLARIS)

#define lf_SETPROCTITLE_USES_ENV  1
#define lf_SETPROCTITLE_PAD       ' '

lf_int_t lf_init_setproctitle(lf_log_t *log);
void lf_setproctitle(char *title);

#elif (lf_LINUX) || (lf_DARWIN)

#define lf_SETPROCTITLE_USES_ENV  1
#define lf_SETPROCTITLE_PAD       '\0'

lf_int_t lf_init_setproctitle(lf_log_t *log);
void lf_setproctitle(char *title);

#else

#define lf_init_setproctitle(log) lf_OK
#define lf_setproctitle(title)

#endif /* OSes */

#endif /* lf_SETPROCTITLE_USES_ENV */

#endif /* lf_HAVE_SETPROCTITLE */


#endif /* _lf_SETPROCTITLE_H_INCLUDED_ */

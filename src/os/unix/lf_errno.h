
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_ERRNO_H_INCLUDED_
#define _lf_ERRNO_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


typedef int               lf_err_t;

#define lf_EPERM         EPERM
#define lf_ENOENT        ENOENT
#define lf_ENOPATH       ENOENT
#define lf_ESRCH         ESRCH
#define lf_EINTR         EINTR
#define lf_ECHILD        ECHILD
#define lf_ENOMEM        ENOMEM
#define lf_EACCES        EACCES
#define lf_EBUSY         EBUSY
#define lf_EEXIST        EEXIST
#define lf_EEXIST_FILE   EEXIST
#define lf_EXDEV         EXDEV
#define lf_ENOTDIR       ENOTDIR
#define lf_EISDIR        EISDIR
#define lf_EINVAL        EINVAL
#define lf_ENFILE        ENFILE
#define lf_EMFILE        EMFILE
#define lf_ENOSPC        ENOSPC
#define lf_EPIPE         EPIPE
#define lf_EINPROGRESS   EINPROGRESS
#define lf_ENOPROTOOPT   ENOPROTOOPT
#define lf_EOPNOTSUPP    EOPNOTSUPP
#define lf_EADDRINUSE    EADDRINUSE
#define lf_ECONNABORTED  ECONNABORTED
#define lf_ECONNRESET    ECONNRESET
#define lf_ENOTCONN      ENOTCONN
#define lf_ETIMEDOUT     ETIMEDOUT
#define lf_ECONNREFUSED  ECONNREFUSED
#define lf_ENAMETOOLONG  ENAMETOOLONG
#define lf_ENETDOWN      ENETDOWN
#define lf_ENETUNREACH   ENETUNREACH
#define lf_EHOSTDOWN     EHOSTDOWN
#define lf_EHOSTUNREACH  EHOSTUNREACH
#define lf_ENOSYS        ENOSYS
#define lf_ECANCELED     ECANCELED
#define lf_EILSEQ        EILSEQ
#define lf_ENOMOREFILES  0
#define lf_ELOOP         ELOOP
#define lf_EBADF         EBADF

#if (lf_HAVE_OPENAT)
#define lf_EMLINK        EMLINK
#endif

#if (__hpux__)
#define lf_EAGAIN        EWOULDBLOCK
#else
#define lf_EAGAIN        EAGAIN
#endif


#define lf_errno                  errno
#define lf_socket_errno           errno
#define lf_set_errno(err)         errno = err
#define lf_set_socket_errno(err)  errno = err


u_char *lf_strerror(lf_err_t err, u_char *errstr, size_t size);
lf_int_t lf_strerror_init(void);


#endif /* _lf_ERRNO_H_INCLUDED_ */

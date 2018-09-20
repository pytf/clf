
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_SOCKET_H_INCLUDED_
#define _lf_SOCKET_H_INCLUDED_


#include <lf_config.h>


#define lf_WRITE_SHUTDOWN SHUT_WR

typedef int  lf_socket_t;

#define lf_socket          socket
#define lf_socket_n        "socket()"


#if (lf_HAVE_FIONBIO)

int lf_nonblocking(lf_socket_t s);
int lf_blocking(lf_socket_t s);

#define lf_nonblocking_n   "ioctl(FIONBIO)"
#define lf_blocking_n      "ioctl(!FIONBIO)"

#else

#define lf_nonblocking(s)  fcntl(s, F_SETFL, fcntl(s, F_GETFL) | O_NONBLOCK)
#define lf_nonblocking_n   "fcntl(O_NONBLOCK)"

#define lf_blocking(s)     fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK)
#define lf_blocking_n      "fcntl(!O_NONBLOCK)"

#endif

int lf_tcp_nopush(lf_socket_t s);
int lf_tcp_push(lf_socket_t s);

#if (lf_LINUX)

#define lf_tcp_nopush_n   "setsockopt(TCP_CORK)"
#define lf_tcp_push_n     "setsockopt(!TCP_CORK)"

#else

#define lf_tcp_nopush_n   "setsockopt(TCP_NOPUSH)"
#define lf_tcp_push_n     "setsockopt(!TCP_NOPUSH)"

#endif


#define lf_shutdown_socket    shutdown
#define lf_shutdown_socket_n  "shutdown()"

#define lf_close_socket    close
#define lf_close_socket_n  "close() socket"


#endif /* _lf_SOCKET_H_INCLUDED_ */

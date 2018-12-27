
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


/*
 * ioctl(FIONBIO) sets a non-blocking mode with the single syscall
 * while fcntl(F_SETFL, O_NONBLOCK) needs to learn the current state
 * using fcntl(F_GETFL).
 *
 * ioctl() and fcntl() are syscalls at least in FreeBSD 2.x, Linux 2.2
 * and Solaris 7.
 *
 * ioctl() in Linux 2.4 and 2.6 uses BKL, however, fcntl(F_SETFL) uses it too.
 */


#if (lf_HAVE_FIONBIO)

int
lf_nonblocking(lf_socket_t s)
{
    int  nb;

    nb = 1;

    return ioctl(s, FIONBIO, &nb);
}


int
lf_blocking(lf_socket_t s)
{
    int  nb;

    nb = 0;

    return ioctl(s, FIONBIO, &nb);
}

#endif


#if (lf_FREEBSD)

int
lf_tcp_nopush(lf_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}


int
lf_tcp_push(lf_socket_t s)
{
    int  tcp_nopush;

    tcp_nopush = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_NOPUSH,
                      (const void *) &tcp_nopush, sizeof(int));
}

#elif (lf_LINUX)


int
lf_tcp_nopush(lf_socket_t s)
{
    int  cork;

    cork = 1;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}


int
lf_tcp_push(lf_socket_t s)
{
    int  cork;

    cork = 0;

    return setsockopt(s, IPPROTO_TCP, TCP_CORK,
                      (const void *) &cork, sizeof(int));
}

#else

int
lf_tcp_nopush(lf_socket_t s)
{
    return 0;
}


int
lf_tcp_push(lf_socket_t s)
{
    return 0;
}

#endif

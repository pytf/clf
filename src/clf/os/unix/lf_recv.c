
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>


ssize_t
lf_unix_recv(lf_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    lf_err_t     err;
    lf_event_t  *rev;

    rev = c->read;

#if (lf_HAVE_KQUEUE)

    if (lf_event_flags & lf_USE_KQUEUE_EVENT) {
        lf_log_debug3(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "recv: eof:%d, avail:%d, err:%d",
                       rev->pending_eof, rev->available, rev->kq_errno);

        if (rev->available == 0) {
            if (rev->pending_eof) {
                rev->ready = 0;
                rev->eof = 1;

                if (rev->kq_errno) {
                    rev->error = 1;
                    lf_set_socket_errno(rev->kq_errno);

                    return lf_connection_error(c, rev->kq_errno,
                               "kevent() reported about an closed connection");
                }

                return 0;

            } else {
                rev->ready = 0;
                return lf_AGAIN;
            }
        }
    }

#endif

#if (lf_HAVE_EPOLLRDHUP)

    if (lf_event_flags & lf_USE_EPOLL_EVENT) {
        lf_log_debug2(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "recv: eof:%d, avail:%d",
                       rev->pending_eof, rev->available);

        if (!rev->available && !rev->pending_eof) {
            rev->ready = 0;
            return lf_AGAIN;
        }
    }

#endif

    do {
        n = recv(c->fd, buf, size, 0);

        lf_log_debug3(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "recv: fd:%d %z of %uz", c->fd, n, size);

        if (n == 0) {
            rev->ready = 0;
            rev->eof = 1;

#if (lf_HAVE_KQUEUE)

            /*
             * on FreeBSD recv() may return 0 on closed socket
             * even if kqueue reported about available data
             */

            if (lf_event_flags & lf_USE_KQUEUE_EVENT) {
                rev->available = 0;
            }

#endif

            return 0;
        }

        if (n > 0) {

#if (lf_HAVE_KQUEUE)

            if (lf_event_flags & lf_USE_KQUEUE_EVENT) {
                rev->available -= n;

                /*
                 * rev->available may be negative here because some additional
                 * bytes may be received between kevent() and recv()
                 */

                if (rev->available <= 0) {
                    if (!rev->pending_eof) {
                        rev->ready = 0;
                    }

                    rev->available = 0;
                }

                return n;
            }

#endif

#if (lf_HAVE_EPOLLRDHUP)

            if ((lf_event_flags & lf_USE_EPOLL_EVENT)
                && lf_use_epoll_rdhup)
            {
                if ((size_t) n < size) {
                    if (!rev->pending_eof) {
                        rev->ready = 0;
                    }

                    rev->available = 0;
                }

                return n;
            }

#endif

            if ((size_t) n < size
                && !(lf_event_flags & lf_USE_GREEDY_EVENT))
            {
                rev->ready = 0;
            }

            return n;
        }

        err = lf_socket_errno;

        if (err == lf_EAGAIN || err == lf_EINTR) {
            lf_log_debug0(lf_LOG_DEBUG_EVENT, c->log, err,
                           "recv() not ready");
            n = lf_AGAIN;

        } else {
            n = lf_connection_error(c, err, "recv() failed");
            break;
        }

    } while (err == lf_EINTR);

    rev->ready = 0;

    if (n == lf_ERROR) {
        rev->error = 1;
    }

    return n;
}

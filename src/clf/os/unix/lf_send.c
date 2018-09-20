
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>


ssize_t
lf_unix_send(lf_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    lf_err_t     err;
    lf_event_t  *wev;

    wev = c->write;

#if (lf_HAVE_KQUEUE)

    if ((lf_event_flags & lf_USE_KQUEUE_EVENT) && wev->pending_eof) {
        (void) lf_connection_error(c, wev->kq_errno,
                               "kevent() reported about an closed connection");
        wev->error = 1;
        return lf_ERROR;
    }

#endif

    for ( ;; ) {
        n = send(c->fd, buf, size, 0);

        lf_log_debug3(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "send: fd:%d %z of %uz", c->fd, n, size);

        if (n > 0) {
            if (n < (ssize_t) size) {
                wev->ready = 0;
            }

            c->sent += n;

            return n;
        }

        err = lf_socket_errno;

        if (n == 0) {
            lf_log_error(lf_LOG_ALERT, c->log, err, "send() returned zero");
            wev->ready = 0;
            return n;
        }

        if (err == lf_EAGAIN || err == lf_EINTR) {
            wev->ready = 0;

            lf_log_debug0(lf_LOG_DEBUG_EVENT, c->log, err,
                           "send() not ready");

            if (err == lf_EAGAIN) {
                return lf_AGAIN;
            }

        } else {
            wev->error = 1;
            (void) lf_connection_error(c, err, "send() failed");
            return lf_ERROR;
        }
    }
}

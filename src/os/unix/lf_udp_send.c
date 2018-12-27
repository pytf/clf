
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>


ssize_t
lf_udp_unix_send(lf_connection_t *c, u_char *buf, size_t size)
{
    ssize_t       n;
    lf_err_t     err;
    lf_event_t  *wev;

    wev = c->write;

    for ( ;; ) {
        n = sendto(c->fd, buf, size, 0, c->sockaddr, c->socklen);

        lf_log_debug4(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "sendto: fd:%d %z of %uz to \"%V\"",
                       c->fd, n, size, &c->addr_text);

        if (n >= 0) {
            if ((size_t) n != size) {
                wev->error = 1;
                (void) lf_connection_error(c, 0, "sendto() incomplete");
                return lf_ERROR;
            }

            c->sent += n;

            return n;
        }

        err = lf_socket_errno;

        if (err == lf_EAGAIN) {
            wev->ready = 0;
            lf_log_debug0(lf_LOG_DEBUG_EVENT, c->log, lf_EAGAIN,
                           "sendto() not ready");
            return lf_AGAIN;
        }

        if (err != lf_EINTR) {
            wev->error = 1;
            (void) lf_connection_error(c, err, "sendto() failed");
            return lf_ERROR;
        }
    }
}

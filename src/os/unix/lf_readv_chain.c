
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>


ssize_t
lf_readv_chain(lf_connection_t *c, lf_chain_t *chain, off_t limit)
{
    u_char        *prev;
    ssize_t        n, size;
    lf_err_t      err;
    lf_array_t    vec;
    lf_event_t   *rev;
    struct iovec  *iov, iovs[lf_IOVS_PREALLOCATE];

    rev = c->read;

#if (lf_HAVE_KQUEUE)

    if (lf_event_flags & lf_USE_KQUEUE_EVENT) {
        lf_log_debug3(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "readv: eof:%d, avail:%d, err:%d",
                       rev->pending_eof, rev->available, rev->kq_errno);

        if (rev->available == 0) {
            if (rev->pending_eof) {
                rev->ready = 0;
                rev->eof = 1;

                lf_log_error(lf_LOG_INFO, c->log, rev->kq_errno,
                              "kevent() reported about an closed connection");

                if (rev->kq_errno) {
                    rev->error = 1;
                    lf_set_socket_errno(rev->kq_errno);
                    return lf_ERROR;
                }

                return 0;

            } else {
                return lf_AGAIN;
            }
        }
    }

#endif

#if (lf_HAVE_EPOLLRDHUP)

    if (lf_event_flags & lf_USE_EPOLL_EVENT) {
        lf_log_debug2(lf_LOG_DEBUG_EVENT, c->log, 0,
                       "readv: eof:%d, avail:%d",
                       rev->pending_eof, rev->available);

        if (!rev->available && !rev->pending_eof) {
            return lf_AGAIN;
        }
    }

#endif

    prev = NULL;
    iov = NULL;
    size = 0;

    vec.elts = iovs;
    vec.nelts = 0;
    vec.size = sizeof(struct iovec);
    vec.nalloc = lf_IOVS_PREALLOCATE;
    vec.pool = c->pool;

    /* coalesce the neighbouring bufs */

    while (chain) {
        n = chain->buf->end - chain->buf->last;

        if (limit) {
            if (size >= limit) {
                break;
            }

            if (size + n > limit) {
                n = (ssize_t) (limit - size);
            }
        }

        if (prev == chain->buf->last) {
            iov->iov_len += n;

        } else {
            if (vec.nelts >= IOV_MAX) {
                break;
            }

            iov = lf_array_push(&vec);
            if (iov == NULL) {
                return lf_ERROR;
            }

            iov->iov_base = (void *) chain->buf->last;
            iov->iov_len = n;
        }

        size += n;
        prev = chain->buf->end;
        chain = chain->next;
    }

    lf_log_debug2(lf_LOG_DEBUG_EVENT, c->log, 0,
                   "readv: %ui, last:%uz", vec.nelts, iov->iov_len);

    do {
        n = readv(c->fd, (struct iovec *) vec.elts, vec.nelts);

        if (n == 0) {
            rev->ready = 0;
            rev->eof = 1;

#if (lf_HAVE_KQUEUE)

            /*
             * on FreeBSD readv() may return 0 on closed socket
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
                 * bytes may be received between kevent() and readv()
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
                if (n < size) {
                    if (!rev->pending_eof) {
                        rev->ready = 0;
                    }

                    rev->available = 0;
                }

                return n;
            }

#endif

            if (n < size && !(lf_event_flags & lf_USE_GREEDY_EVENT)) {
                rev->ready = 0;
            }

            return n;
        }

        err = lf_socket_errno;

        if (err == lf_EAGAIN || err == lf_EINTR) {
            lf_log_debug0(lf_LOG_DEBUG_EVENT, c->log, err,
                           "readv() not ready");
            n = lf_AGAIN;

        } else {
            n = lf_connection_error(c, err, "readv() failed");
            break;
        }

    } while (err == lf_EINTR);

    rev->ready = 0;

    if (n == lf_ERROR) {
        c->read->error = 1;
    }

    return n;
}


/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_THREADS)
#include <lf_thread_pool.h>
static void lf_thread_read_handler(void *data, lf_log_t *log);
static void lf_thread_write_chain_to_file_handler(void *data, lf_log_t *log);
#endif

static lf_chain_t *lf_chain_to_iovec(lf_iovec_t *vec, lf_chain_t *cl);
static ssize_t lf_writev_file(lf_file_t *file, lf_iovec_t *vec,
    off_t offset);


#if (lf_HAVE_FILE_AIO)

lf_uint_t  lf_file_aio = 1;

#endif


ssize_t
lf_read_file(lf_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t  n;

    lf_log_debug4(lf_LOG_DEBUG_CORE, file->log, 0,
                   "read: %d, %p, %uz, %O", file->fd, buf, size, offset);

#if (lf_HAVE_PREAD)

    n = pread(file->fd, buf, size, offset);

    if (n == -1) {
        lf_log_error(lf_LOG_CRIT, file->log, lf_errno,
                      "pread() \"%s\" failed", file->name.data);
        return lf_ERROR;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            lf_log_error(lf_LOG_CRIT, file->log, lf_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->sys_offset = offset;
    }

    n = read(file->fd, buf, size);

    if (n == -1) {
        lf_log_error(lf_LOG_CRIT, file->log, lf_errno,
                      "read() \"%s\" failed", file->name.data);
        return lf_ERROR;
    }

    file->sys_offset += n;

#endif

    file->offset += n;

    return n;
}


#if (lf_THREADS)

typedef struct {
    lf_fd_t       fd;
    lf_uint_t     write;   /* unsigned  write:1; */

    u_char        *buf;
    size_t         size;
    lf_chain_t   *chain;
    off_t          offset;

    size_t         nbytes;
    lf_err_t      err;
} lf_thread_file_ctx_t;


ssize_t
lf_thread_read(lf_file_t *file, u_char *buf, size_t size, off_t offset,
    lf_pool_t *pool)
{
    lf_thread_task_t      *task;
    lf_thread_file_ctx_t  *ctx;

    lf_log_debug4(lf_LOG_DEBUG_CORE, file->log, 0,
                   "thread read: %d, %p, %uz, %O",
                   file->fd, buf, size, offset);

    task = file->thread_task;

    if (task == NULL) {
        task = lf_thread_task_alloc(pool, sizeof(lf_thread_file_ctx_t));
        if (task == NULL) {
            return lf_ERROR;
        }

        file->thread_task = task;
    }

    ctx = task->ctx;

    if (task->event.complete) {
        task->event.complete = 0;

        if (ctx->write) {
            lf_log_error(lf_LOG_ALERT, file->log, 0,
                          "invalid thread call, read instead of write");
            return lf_ERROR;
        }

        if (ctx->err) {
            lf_log_error(lf_LOG_CRIT, file->log, ctx->err,
                          "pread() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        return ctx->nbytes;
    }

    task->handler = lf_thread_read_handler;

    ctx->write = 0;

    ctx->fd = file->fd;
    ctx->buf = buf;
    ctx->size = size;
    ctx->offset = offset;

    if (file->thread_handler(task, file) != lf_OK) {
        return lf_ERROR;
    }

    return lf_AGAIN;
}


#if (lf_HAVE_PREAD)

static void
lf_thread_read_handler(void *data, lf_log_t *log)
{
    lf_thread_file_ctx_t *ctx = data;

    ssize_t  n;

    lf_log_debug0(lf_LOG_DEBUG_CORE, log, 0, "thread read handler");

    n = pread(ctx->fd, ctx->buf, ctx->size, ctx->offset);

    if (n == -1) {
        ctx->err = lf_errno;

    } else {
        ctx->nbytes = n;
        ctx->err = 0;
    }

#if 0
    lf_time_update();
#endif

    lf_log_debug4(lf_LOG_DEBUG_CORE, log, 0,
                   "pread: %z (err: %d) of %uz @%O",
                   n, ctx->err, ctx->size, ctx->offset);
}

#else

#error pread() is required!

#endif

#endif /* lf_THREADS */


ssize_t
lf_write_file(lf_file_t *file, u_char *buf, size_t size, off_t offset)
{
    ssize_t    n, written;
    lf_err_t  err;

    lf_log_debug4(lf_LOG_DEBUG_CORE, file->log, 0,
                   "write: %d, %p, %uz, %O", file->fd, buf, size, offset);

    written = 0;

#if (lf_HAVE_PWRITE)

    for ( ;; ) {
        n = pwrite(file->fd, buf + written, size, offset);

        if (n == -1) {
            err = lf_errno;

            if (err == lf_EINTR) {
                lf_log_debug0(lf_LOG_DEBUG_CORE, file->log, err,
                               "pwrite() was interrupted");
                continue;
            }

            lf_log_error(lf_LOG_CRIT, file->log, err,
                          "pwrite() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        offset += n;
        size -= n;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            lf_log_error(lf_LOG_CRIT, file->log, lf_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->sys_offset = offset;
    }

    for ( ;; ) {
        n = write(file->fd, buf + written, size);

        if (n == -1) {
            err = lf_errno;

            if (err == lf_EINTR) {
                lf_log_debug0(lf_LOG_DEBUG_CORE, file->log, err,
                               "write() was interrupted");
                continue;
            }

            lf_log_error(lf_LOG_CRIT, file->log, err,
                          "write() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->sys_offset += n;
        file->offset += n;
        written += n;

        if ((size_t) n == size) {
            return written;
        }

        size -= n;
    }
#endif
}


lf_fd_t
lf_open_tempfile(u_char *name, lf_uint_t persistent, lf_uint_t access)
{
    lf_fd_t  fd;

    fd = open((const char *) name, O_CREAT|O_EXCL|O_RDWR,
              access ? access : 0600);

    if (fd != -1 && !persistent) {
        (void) unlink((const char *) name);
    }

    return fd;
}


ssize_t
lf_write_chain_to_file(lf_file_t *file, lf_chain_t *cl, off_t offset,
    lf_pool_t *pool)
{
    ssize_t        total, n;
    lf_iovec_t    vec;
    struct iovec   iovs[lf_IOVS_PREALLOCATE];

    /* use pwrite() if there is the only buf in a chain */

    if (cl->next == NULL) {
        return lf_write_file(file, cl->buf->pos,
                              (size_t) (cl->buf->last - cl->buf->pos),
                              offset);
    }

    total = 0;

    vec.iovs = iovs;
    vec.nalloc = lf_IOVS_PREALLOCATE;

    do {
        /* create the iovec and coalesce the neighbouring bufs */
        cl = lf_chain_to_iovec(&vec, cl);

        /* use pwrite() if there is the only iovec buffer */

        if (vec.count == 1) {
            n = lf_write_file(file, (u_char *) iovs[0].iov_base,
                               iovs[0].iov_len, offset);

            if (n == lf_ERROR) {
                return n;
            }

            return total + n;
        }

        n = lf_writev_file(file, &vec, offset);

        if (n == lf_ERROR) {
            return n;
        }

        offset += n;
        total += n;

    } while (cl);

    return total;
}


static lf_chain_t *
lf_chain_to_iovec(lf_iovec_t *vec, lf_chain_t *cl)
{
    size_t         total, size;
    u_char        *prev;
    lf_uint_t     n;
    struct iovec  *iov;

    iov = NULL;
    prev = NULL;
    total = 0;
    n = 0;

    for ( /* void */ ; cl; cl = cl->next) {

        if (lf_buf_special(cl->buf)) {
            continue;
        }

        size = cl->buf->last - cl->buf->pos;

        if (prev == cl->buf->pos) {
            iov->iov_len += size;

        } else {
            if (n == vec->nalloc) {
                break;
            }

            iov = &vec->iovs[n++];

            iov->iov_base = (void *) cl->buf->pos;
            iov->iov_len = size;
        }

        prev = cl->buf->pos + size;
        total += size;
    }

    vec->count = n;
    vec->size = total;

    return cl;
}


static ssize_t
lf_writev_file(lf_file_t *file, lf_iovec_t *vec, off_t offset)
{
    ssize_t    n;
    lf_err_t  err;

    lf_log_debug3(lf_LOG_DEBUG_CORE, file->log, 0,
                   "writev: %d, %uz, %O", file->fd, vec->size, offset);

#if (lf_HAVE_PWRITEV)

eintr:

    n = pwritev(file->fd, vec->iovs, vec->count, offset);

    if (n == -1) {
        err = lf_errno;

        if (err == lf_EINTR) {
            lf_log_debug0(lf_LOG_DEBUG_CORE, file->log, err,
                           "pwritev() was interrupted");
            goto eintr;
        }

        lf_log_error(lf_LOG_CRIT, file->log, err,
                      "pwritev() \"%s\" failed", file->name.data);
        return lf_ERROR;
    }

    if ((size_t) n != vec->size) {
        lf_log_error(lf_LOG_CRIT, file->log, 0,
                      "pwritev() \"%s\" has written only %z of %uz",
                      file->name.data, n, vec->size);
        return lf_ERROR;
    }

#else

    if (file->sys_offset != offset) {
        if (lseek(file->fd, offset, SEEK_SET) == -1) {
            lf_log_error(lf_LOG_CRIT, file->log, lf_errno,
                          "lseek() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->sys_offset = offset;
    }

eintr:

    n = writev(file->fd, vec->iovs, vec->count);

    if (n == -1) {
        err = lf_errno;

        if (err == lf_EINTR) {
            lf_log_debug0(lf_LOG_DEBUG_CORE, file->log, err,
                           "writev() was interrupted");
            goto eintr;
        }

        lf_log_error(lf_LOG_CRIT, file->log, err,
                      "writev() \"%s\" failed", file->name.data);
        return lf_ERROR;
    }

    if ((size_t) n != vec->size) {
        lf_log_error(lf_LOG_CRIT, file->log, 0,
                      "writev() \"%s\" has written only %z of %uz",
                      file->name.data, n, vec->size);
        return lf_ERROR;
    }

    file->sys_offset += n;

#endif

    file->offset += n;

    return n;
}


#if (lf_THREADS)

ssize_t
lf_thread_write_chain_to_file(lf_file_t *file, lf_chain_t *cl, off_t offset,
    lf_pool_t *pool)
{
    lf_thread_task_t      *task;
    lf_thread_file_ctx_t  *ctx;

    lf_log_debug3(lf_LOG_DEBUG_CORE, file->log, 0,
                   "thread write chain: %d, %p, %O",
                   file->fd, cl, offset);

    task = file->thread_task;

    if (task == NULL) {
        task = lf_thread_task_alloc(pool,
                                     sizeof(lf_thread_file_ctx_t));
        if (task == NULL) {
            return lf_ERROR;
        }

        file->thread_task = task;
    }

    ctx = task->ctx;

    if (task->event.complete) {
        task->event.complete = 0;

        if (!ctx->write) {
            lf_log_error(lf_LOG_ALERT, file->log, 0,
                          "invalid thread call, write instead of read");
            return lf_ERROR;
        }

        if (ctx->err || ctx->nbytes == 0) {
            lf_log_error(lf_LOG_CRIT, file->log, ctx->err,
                          "pwritev() \"%s\" failed", file->name.data);
            return lf_ERROR;
        }

        file->offset += ctx->nbytes;
        return ctx->nbytes;
    }

    task->handler = lf_thread_write_chain_to_file_handler;

    ctx->write = 1;

    ctx->fd = file->fd;
    ctx->chain = cl;
    ctx->offset = offset;

    if (file->thread_handler(task, file) != lf_OK) {
        return lf_ERROR;
    }

    return lf_AGAIN;
}


static void
lf_thread_write_chain_to_file_handler(void *data, lf_log_t *log)
{
    lf_thread_file_ctx_t *ctx = data;

#if (lf_HAVE_PWRITEV)

    off_t          offset;
    ssize_t        n;
    lf_err_t      err;
    lf_chain_t   *cl;
    lf_iovec_t    vec;
    struct iovec   iovs[lf_IOVS_PREALLOCATE];

    vec.iovs = iovs;
    vec.nalloc = lf_IOVS_PREALLOCATE;

    cl = ctx->chain;
    offset = ctx->offset;

    ctx->nbytes = 0;
    ctx->err = 0;

    do {
        /* create the iovec and coalesce the neighbouring bufs */
        cl = lf_chain_to_iovec(&vec, cl);

eintr:

        n = pwritev(ctx->fd, iovs, vec.count, offset);

        if (n == -1) {
            err = lf_errno;

            if (err == lf_EINTR) {
                lf_log_debug0(lf_LOG_DEBUG_CORE, log, err,
                               "pwritev() was interrupted");
                goto eintr;
            }

            ctx->err = err;
            return;
        }

        if ((size_t) n != vec.size) {
            ctx->nbytes = 0;
            return;
        }

        ctx->nbytes += n;
        offset += n;
    } while (cl);

#else

    ctx->err = lf_ENOSYS;
    return;

#endif
}

#endif /* lf_THREADS */


lf_int_t
lf_set_file_time(u_char *name, lf_fd_t fd, time_t s)
{
    struct timeval  tv[2];

    tv[0].tv_sec = lf_time();
    tv[0].tv_usec = 0;
    tv[1].tv_sec = s;
    tv[1].tv_usec = 0;

    if (utimes((char *) name, tv) != -1) {
        return lf_OK;
    }

    return lf_ERROR;
}


lf_int_t
lf_create_file_mapping(lf_file_mapping_t *fm)
{
    fm->fd = lf_open_file(fm->name, lf_FILE_RDWR, lf_FILE_TRUNCATE,
                           lf_FILE_DEFAULT_ACCESS);

    if (fm->fd == lf_INVALID_FILE) {
        lf_log_error(lf_LOG_CRIT, fm->log, lf_errno,
                      lf_open_file_n " \"%s\" failed", fm->name);
        return lf_ERROR;
    }

    if (ftruncate(fm->fd, fm->size) == -1) {
        lf_log_error(lf_LOG_CRIT, fm->log, lf_errno,
                      "ftruncate() \"%s\" failed", fm->name);
        goto failed;
    }

    fm->addr = mmap(NULL, fm->size, PROT_READ|PROT_WRITE, MAP_SHARED,
                    fm->fd, 0);
    if (fm->addr != MAP_FAILED) {
        return lf_OK;
    }

    lf_log_error(lf_LOG_CRIT, fm->log, lf_errno,
                  "mmap(%uz) \"%s\" failed", fm->size, fm->name);

failed:

    if (lf_close_file(fm->fd) == lf_FILE_ERROR) {
        lf_log_error(lf_LOG_ALERT, fm->log, lf_errno,
                      lf_close_file_n " \"%s\" failed", fm->name);
    }

    return lf_ERROR;
}


void
lf_close_file_mapping(lf_file_mapping_t *fm)
{
    if (munmap(fm->addr, fm->size) == -1) {
        lf_log_error(lf_LOG_CRIT, fm->log, lf_errno,
                      "munmap(%uz) \"%s\" failed", fm->size, fm->name);
    }

    if (lf_close_file(fm->fd) == lf_FILE_ERROR) {
        lf_log_error(lf_LOG_ALERT, fm->log, lf_errno,
                      lf_close_file_n " \"%s\" failed", fm->name);
    }
}


lf_int_t
lf_open_dir(lf_str_t *name, lf_dir_t *dir)
{
    dir->dir = opendir((const char *) name->data);

    if (dir->dir == NULL) {
        return lf_ERROR;
    }

    dir->valid_info = 0;

    return lf_OK;
}


lf_int_t
lf_read_dir(lf_dir_t *dir)
{
    dir->de = readdir(dir->dir);

    if (dir->de) {
#if (lf_HAVE_D_TYPE)
        dir->type = dir->de->d_type;
#else
        dir->type = 0;
#endif
        return lf_OK;
    }

    return lf_ERROR;
}


lf_int_t
lf_open_glob(lf_glob_t *gl)
{
    int  n;

    n = glob((char *) gl->pattern, 0, NULL, &gl->pglob);

    if (n == 0) {
        return lf_OK;
    }

#ifdef GLOB_NOMATCH

    if (n == GLOB_NOMATCH && gl->test) {
        return lf_OK;
    }

#endif

    return lf_ERROR;
}


lf_int_t
lf_read_glob(lf_glob_t *gl, lf_str_t *name)
{
    size_t  count;

#ifdef GLOB_NOMATCH
    count = (size_t) gl->pglob.gl_pathc;
#else
    count = (size_t) gl->pglob.gl_matchc;
#endif

    if (gl->n < count) {

        name->len = (size_t) lf_strlen(gl->pglob.gl_pathv[gl->n]);
        name->data = (u_char *) gl->pglob.gl_pathv[gl->n];
        gl->n++;

        return lf_OK;
    }

    return lf_DONE;
}


void
lf_close_glob(lf_glob_t *gl)
{
    globfree(&gl->pglob);
}


lf_err_t
lf_trylock_fd(lf_fd_t fd)
{
    struct flock  fl;

    lf_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return lf_errno;
    }

    return 0;
}


lf_err_t
lf_lock_fd(lf_fd_t fd)
{
    struct flock  fl;

    lf_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_WRLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLKW, &fl) == -1) {
        return lf_errno;
    }

    return 0;
}


lf_err_t
lf_unlock_fd(lf_fd_t fd)
{
    struct flock  fl;

    lf_memzero(&fl, sizeof(struct flock));
    fl.l_type = F_UNLCK;
    fl.l_whence = SEEK_SET;

    if (fcntl(fd, F_SETLK, &fl) == -1) {
        return  lf_errno;
    }

    return 0;
}


#if (lf_HAVE_POSIX_FADVISE) && !(lf_HAVE_F_READAHEAD)

lf_int_t
lf_read_ahead(lf_fd_t fd, size_t n)
{
    int  err;

    err = posix_fadvise(fd, 0, 0, POSIX_FADV_SEQUENTIAL);

    if (err == 0) {
        return 0;
    }

    lf_set_errno(err);
    return lf_FILE_ERROR;
}

#endif


#if (lf_HAVE_O_DIRECT)

lf_int_t
lf_directio_on(lf_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return lf_FILE_ERROR;
    }

    return fcntl(fd, F_SETFL, flags | O_DIRECT);
}


lf_int_t
lf_directio_off(lf_fd_t fd)
{
    int  flags;

    flags = fcntl(fd, F_GETFL);

    if (flags == -1) {
        return lf_FILE_ERROR;
    }

    return fcntl(fd, F_SETFL, flags & ~O_DIRECT);
}

#endif


#if (lf_HAVE_STATFS)

size_t
lf_fs_bsize(u_char *name)
{
    struct statfs  fs;

    if (statfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_bsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_bsize;
}

#elif (lf_HAVE_STATVFS)

size_t
lf_fs_bsize(u_char *name)
{
    struct statvfs  fs;

    if (statvfs((char *) name, &fs) == -1) {
        return 512;
    }

    if ((fs.f_frsize % 512) != 0) {
        return 512;
    }

    return (size_t) fs.f_frsize;
}

#else

size_t
lf_fs_bsize(u_char *name)
{
    return 512;
}

#endif

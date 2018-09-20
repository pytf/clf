
/*
 * 封装内存分配
 */


#include <lf_config.h>
#include <lf_core.h>

/*
 * 封装malloc
 */
void *
lf_alloc(size_t size, lf_log_t *log)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        lf_log_error(lf_LOG_EMERG, log, lf_errno,
                      "malloc(%uz) failed", size);
    }

    lf_log_debug2(lf_LOG_DEBUG_ALLOC, log, 0, "malloc: %p:%uz", p, size);

    return p;
}

/*
 * 分配内存并初始化
 */
void *
lf_calloc(size_t size, lf_log_t *log)
{
    void  *p;

    p = lf_alloc(size, log);

    if (p) {
        lf_memzero(p, size);
    }

    return p;
}

/*
 * 数据对齐函数封装
 */
#if (lf_HAVE_POSIX_MEMALIGN)

void *
lf_memalign(size_t alignment, size_t size, lf_log_t *log)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        lf_log_error(lf_LOG_EMERG, log, err,
                      "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    lf_log_debug3(lf_LOG_DEBUG_ALLOC, log, 0,
                   "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (lf_HAVE_MEMALIGN)

void *
lf_memalign(size_t alignment, size_t size, lf_log_t *log)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
        lf_log_error(lf_LOG_EMERG, log, lf_errno,
                      "memalign(%uz, %uz) failed", alignment, size);
    }

    lf_log_debug3(lf_LOG_DEBUG_ALLOC, log, 0,
                   "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif

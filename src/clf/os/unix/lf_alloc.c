/*
 * 封装内存分配
 */
#inlude "lf_alloc.h"

/*
 * 封装malloc
 */
void *lf_alloc(size_t size)
{
    void  *p;

    p = malloc(size);
    if (p == NULL) {
        lf_log_std(LOG_LEVEL_ERR, "malloc(%uz) failed", size);
    }

    lf_log_std(LOG_LEVEL_DEBUG, "malloc: %p:%uz", p, size);

    return p;
}

/*
 * 分配内存并初始化
 */
void *lf_calloc(size_t size)
{
    void  *p;

    p = lf_alloc(size);

    if (p) {
        lf_memzero(p, size);
    }

    return p;
}

/*
 * 数据对齐函数封装
 */
#if (lf_HAVE_POSIX_MEMALIGN)

void *lf_memalign(size_t alignment, size_t size)
{
    void  *p;
    int    err;

    err = posix_memalign(&p, alignment, size);

    if (err) {
        lf_log_std(LOG_LEVEL_ERR, "posix_memalign(%uz, %uz) failed", alignment, size);
        p = NULL;
    }

    lf_log_std(LOG_LEVEL_DEBUG, "posix_memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#elif (lf_HAVE_MEMALIGN)

void *lf_memalign(size_t alignment, size_t size)
{
    void  *p;

    p = memalign(alignment, size);
    if (p == NULL) {
        lf_log_std(LOG_LEVEL_ERR, "memalign(%uz, %uz) failed", alignment, size);
    }

    lf_log_std(LOG_LEVEL_DEBUG, "memalign: %p:%uz @%uz", p, size, alignment);

    return p;
}

#endif

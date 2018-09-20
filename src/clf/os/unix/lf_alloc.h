
/*
 * 
 * 
 */


#ifndef _lf_ALLOC_H_INCLUDED_
#define _lf_ALLOC_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


void *lf_alloc(size_t size, lf_log_t *log);
void *lf_calloc(size_t size, lf_log_t *log);

#define lf_free          free


/*
 * Linux has memalign() or posix_memalign()
 * Solaris has memalign()
 * FreeBSD 7.0 has posix_memalign(), besides, early version's malloc()
 * aligns allocations bigger than page size at the page boundary
 */

#if (lf_HAVE_POSIX_MEMALIGN || lf_HAVE_MEMALIGN)

void *lf_memalign(size_t alignment, size_t size, lf_log_t *log);

#else

#define lf_memalign(alignment, size, log)  lf_alloc(size, log)

#endif

#endif /* _lf_ALLOC_H_INCLUDED_ */

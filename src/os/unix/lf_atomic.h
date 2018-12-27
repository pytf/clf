
/*
 * Linux的原子操作与同步机制
 * https://www.cnblogs.com/fanzhidongyzby/p/3654855.html
 */


#ifndef _lf_ATOMIC_H_INCLUDED_
#define _lf_ATOMIC_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


#if (lf_HAVE_LIBATOMIC)

#define AO_REQUIRE_CAS
#include <atomic_ops.h>

#define lf_HAVE_ATOMIC_OPS  1

typedef long                        lf_atomic_int_t;
typedef AO_t                        lf_atomic_uint_t;
typedef volatile lf_atomic_uint_t  lf_atomic_t;

#if (lf_PTR_SIZE == 8)
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

#define lf_atomic_cmp_set(lock, old, new)                                    \
    AO_compare_and_swap(lock, old, new)
#define lf_atomic_fetch_add(value, add)                                      \
    AO_fetch_and_add(value, add)
#define lf_memory_barrier()        AO_nop()
#define lf_cpu_pause()


#elif (lf_DARWIN_ATOMIC)

/*
 * use Darwin 8 atomic(3) and barrier(3) operations
 * optimized at run-time for UP and SMP
 */

#include <libkern/OSAtomic.h>

/* "bool" conflicts with perl's CORE/handy.h */
#if 0
#undef bool
#endif


#define lf_HAVE_ATOMIC_OPS  1

#if (lf_PTR_SIZE == 8)

typedef int64_t                     lf_atomic_int_t;
typedef uint64_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#define lf_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap64Barrier(old, new, (int64_t *) lock)

#define lf_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd64(add, (int64_t *) value) - add)

#else

typedef int32_t                     lf_atomic_int_t;
typedef uint32_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#define lf_atomic_cmp_set(lock, old, new)                                    \
    OSAtomicCompareAndSwap32Barrier(old, new, (int32_t *) lock)

#define lf_atomic_fetch_add(value, add)                                      \
    (OSAtomicAdd32(add, (int32_t *) value) - add)

#endif

#define lf_memory_barrier()        OSMemoryBarrier()

#define lf_cpu_pause()

typedef volatile lf_atomic_uint_t  lf_atomic_t;


#elif (lf_HAVE_GCC_ATOMIC)

/* GCC 4.1 builtin atomic operations */

#define lf_HAVE_ATOMIC_OPS  1

typedef long                        lf_atomic_int_t;
typedef unsigned long               lf_atomic_uint_t;

#if (lf_PTR_SIZE == 8)
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)
#else
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)
#endif

typedef volatile lf_atomic_uint_t  lf_atomic_t;


#define lf_atomic_cmp_set(lock, old, set)                                    \
    __sync_bool_compare_and_swap(lock, old, set)

#define lf_atomic_fetch_add(value, add)                                      \
    __sync_fetch_and_add(value, add)

#define lf_memory_barrier()        __sync_synchronize()

#if ( __i386__ || __i386 || __amd64__ || __amd64 )
#define lf_cpu_pause()             __asm__ ("pause")
#else
#define lf_cpu_pause()
#endif


#elif ( __i386__ || __i386 )

typedef int32_t                     lf_atomic_int_t;
typedef uint32_t                    lf_atomic_uint_t;
typedef volatile lf_atomic_uint_t  lf_atomic_t;
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


#if ( __SUNPRO_C )

#define lf_HAVE_ATOMIC_OPS  1

lf_atomic_uint_t
lf_atomic_cmp_set(lf_atomic_t *lock, lf_atomic_uint_t old,
    lf_atomic_uint_t set);

lf_atomic_int_t
lf_atomic_fetch_add(lf_atomic_t *value, lf_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so lf_cpu_pause is declared in src/os/unix/lf_sunpro_x86.il
 */

void
lf_cpu_pause(void);

/* the code in src/os/unix/lf_sunpro_x86.il */

#define lf_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define lf_HAVE_ATOMIC_OPS  1

#include "lf_gcc_atomic_x86.h"

#endif


#elif ( __amd64__ || __amd64 )

typedef int64_t                     lf_atomic_int_t;
typedef uint64_t                    lf_atomic_uint_t;
typedef volatile lf_atomic_uint_t  lf_atomic_t;
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)


#if ( __SUNPRO_C )

#define lf_HAVE_ATOMIC_OPS  1

lf_atomic_uint_t
lf_atomic_cmp_set(lf_atomic_t *lock, lf_atomic_uint_t old,
    lf_atomic_uint_t set);

lf_atomic_int_t
lf_atomic_fetch_add(lf_atomic_t *value, lf_atomic_int_t add);

/*
 * Sun Studio 12 exits with segmentation fault on '__asm ("pause")',
 * so lf_cpu_pause is declared in src/os/unix/lf_sunpro_amd64.il
 */

void
lf_cpu_pause(void);

/* the code in src/os/unix/lf_sunpro_amd64.il */

#define lf_memory_barrier()        __asm (".volatile"); __asm (".nonvolatile")


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define lf_HAVE_ATOMIC_OPS  1

#include "lf_gcc_atomic_amd64.h"

#endif


#elif ( __sparc__ || __sparc || __sparcv9 )

#if (lf_PTR_SIZE == 8)

typedef int64_t                     lf_atomic_int_t;
typedef uint64_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     lf_atomic_int_t;
typedef uint32_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile lf_atomic_uint_t  lf_atomic_t;


#if ( __SUNPRO_C )

#define lf_HAVE_ATOMIC_OPS  1

#include "lf_sunpro_atomic_sparc64.h"


#else /* ( __GNUC__ || __INTEL_COMPILER ) */

#define lf_HAVE_ATOMIC_OPS  1

#include "lf_gcc_atomic_sparc64.h"

#endif


#elif ( __powerpc__ || __POWERPC__ )

#define lf_HAVE_ATOMIC_OPS  1

#if (lf_PTR_SIZE == 8)

typedef int64_t                     lf_atomic_int_t;
typedef uint64_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-9223372036854775808") - 1)

#else

typedef int32_t                     lf_atomic_int_t;
typedef uint32_t                    lf_atomic_uint_t;
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)

#endif

typedef volatile lf_atomic_uint_t  lf_atomic_t;


#include "lf_gcc_atomic_ppc.h"

#endif


#if !(lf_HAVE_ATOMIC_OPS)

#define lf_HAVE_ATOMIC_OPS  0

typedef int32_t                     lf_atomic_int_t;
typedef uint32_t                    lf_atomic_uint_t;
typedef volatile lf_atomic_uint_t  lf_atomic_t;
#define lf_ATOMIC_T_LEN            (sizeof("-2147483648") - 1)


static lf_inline lf_atomic_uint_t
lf_atomic_cmp_set(lf_atomic_t *lock, lf_atomic_uint_t old,
    lf_atomic_uint_t set)
{
    if (*lock == old) {
        *lock = set;
        return 1;
    }

    return 0;
}


static lf_inline lf_atomic_int_t
lf_atomic_fetch_add(lf_atomic_t *value, lf_atomic_int_t add)
{
    lf_atomic_int_t  old;

    old = *value;
    *value += add;

    return old;
}

#define lf_memory_barrier()
#define lf_cpu_pause()

#endif


void lf_spinlock(lf_atomic_t *lock, lf_atomic_int_t value, lf_uint_t spin);

#define lf_trylock(lock)  (*(lock) == 0 && lf_atomic_cmp_set(lock, 0, 1))
#define lf_unlock(lock)    *(lock) = 0


#endif /* _lf_ATOMIC_H_INCLUDED_ */

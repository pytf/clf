
/*
 * Copyright (C) Nginx, Inc.
 */

#ifndef _lf_SETAFFINITY_H_INCLUDED_
#define _lf_SETAFFINITY_H_INCLUDED_


#if (lf_HAVE_SCHED_SETAFFINITY || lf_HAVE_CPUSET_SETAFFINITY)

#define lf_HAVE_CPU_AFFINITY 1

#if (lf_HAVE_SCHED_SETAFFINITY)

typedef cpu_set_t  lf_cpuset_t;

#elif (lf_HAVE_CPUSET_SETAFFINITY)

#include <sys/cpuset.h>

typedef cpuset_t  lf_cpuset_t;

#endif

void lf_setaffinity(lf_cpuset_t *cpu_affinity, lf_log_t *log);

#else

#define lf_setaffinity(cpu_affinity, log)

typedef uint64_t  lf_cpuset_t;

#endif


#endif /* _lf_SETAFFINITY_H_INCLUDED_ */

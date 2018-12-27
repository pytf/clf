
/*
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_HAVE_CPUSET_SETAFFINITY)

void
lf_setaffinity(lf_cpuset_t *cpu_affinity, lf_log_t *log)
{
    lf_uint_t  i;

    for (i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, cpu_affinity)) {
            lf_log_error(lf_LOG_NOTICE, log, 0,
                          "cpuset_setaffinity(): using cpu #%ui", i);
        }
    }

    if (cpuset_setaffinity(CPU_LEVEL_WHICH, CPU_WHICH_PID, -1,
                           sizeof(cpuset_t), cpu_affinity) == -1)
    {
        lf_log_error(lf_LOG_ALERT, log, lf_errno,
                      "cpuset_setaffinity() failed");
    }
}

#elif (lf_HAVE_SCHED_SETAFFINITY)

void
lf_setaffinity(lf_cpuset_t *cpu_affinity, lf_log_t *log)
{
    lf_uint_t  i;

    for (i = 0; i < CPU_SETSIZE; i++) {
        if (CPU_ISSET(i, cpu_affinity)) {
            lf_log_error(lf_LOG_NOTICE, log, 0,
                          "sched_setaffinity(): using cpu #%ui", i);
        }
    }

    if (sched_setaffinity(0, sizeof(cpu_set_t), cpu_affinity) == -1) {
        lf_log_error(lf_LOG_ALERT, log, lf_errno,
                      "sched_setaffinity() failed");
    }
}

#endif

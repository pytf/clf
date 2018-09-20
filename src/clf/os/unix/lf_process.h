
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_PROCESS_H_INCLUDED_
#define _lf_PROCESS_H_INCLUDED_


#include <lf_setaffinity.h>
#include <lf_setproctitle.h>


typedef pid_t       lf_pid_t;

#define lf_INVALID_PID  -1

typedef void (*lf_spawn_proc_pt) (lf_cycle_t *cycle, void *data);

typedef struct {
    lf_pid_t           pid;
    int                 status;
    lf_socket_t        channel[2];

    lf_spawn_proc_pt   proc;
    void               *data;
    char               *name;

    unsigned            respawn:1;
    unsigned            just_spawn:1;
    unsigned            detached:1;
    unsigned            exiting:1;
    unsigned            exited:1;
} lf_process_t;


typedef struct {
    char         *path;
    char         *name;
    char *const  *argv;
    char *const  *envp;
} lf_exec_ctx_t;


#define lf_MAX_PROCESSES         1024

#define lf_PROCESS_NORESPAWN     -1
#define lf_PROCESS_JUST_SPAWN    -2
#define lf_PROCESS_RESPAWN       -3
#define lf_PROCESS_JUST_RESPAWN  -4
#define lf_PROCESS_DETACHED      -5


#define lf_getpid   getpid
#define lf_getppid  getppid

#ifndef lf_log_pid
#define lf_log_pid  lf_pid
#endif


lf_pid_t lf_spawn_process(lf_cycle_t *cycle,
    lf_spawn_proc_pt proc, void *data, char *name, lf_int_t respawn);
lf_pid_t lf_execute(lf_cycle_t *cycle, lf_exec_ctx_t *ctx);
lf_int_t lf_init_signals(lf_log_t *log);
void lf_debug_point(void);


#if (lf_HAVE_SCHED_YIELD)
#define lf_sched_yield()  sched_yield()
#else
#define lf_sched_yield()  usleep(1)
#endif


extern int            lf_argc;
extern char         **lf_argv;
extern char         **lf_os_argv;

extern lf_pid_t      lf_pid;
extern lf_pid_t      lf_parent;
extern lf_socket_t   lf_channel;
extern lf_int_t      lf_process_slot;
extern lf_int_t      lf_last_process;
extern lf_process_t  lf_processes[lf_MAX_PROCESSES];


#endif /* _lf_PROCESS_H_INCLUDED_ */

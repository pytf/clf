
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>
#include <lf_channel.h>


typedef struct {
    int     signo;
    char   *signame;
    char   *name;
    void  (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
} lf_signal_t;



static void lf_execute_proc(lf_cycle_t *cycle, void *data);
static void lf_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);
static void lf_process_get_status(void);
static void lf_unlock_mutexes(lf_pid_t pid);


int              lf_argc;
char           **lf_argv;
char           **lf_os_argv;

lf_int_t        lf_process_slot;
lf_socket_t     lf_channel;
lf_int_t        lf_last_process;
lf_process_t    lf_processes[lf_MAX_PROCESSES];


lf_signal_t  signals[] = {
    { lf_signal_value(lf_RECONFIGURE_SIGNAL),
      "SIG" lf_value(lf_RECONFIGURE_SIGNAL),
      "reload",
      lf_signal_handler },

    { lf_signal_value(lf_REOPEN_SIGNAL),
      "SIG" lf_value(lf_REOPEN_SIGNAL),
      "reopen",
      lf_signal_handler },

    { lf_signal_value(lf_NOACCEPT_SIGNAL),
      "SIG" lf_value(lf_NOACCEPT_SIGNAL),
      "",
      lf_signal_handler },

    { lf_signal_value(lf_TERMINATE_SIGNAL),
      "SIG" lf_value(lf_TERMINATE_SIGNAL),
      "stop",
      lf_signal_handler },

    { lf_signal_value(lf_SHUTDOWN_SIGNAL),
      "SIG" lf_value(lf_SHUTDOWN_SIGNAL),
      "quit",
      lf_signal_handler },

    { lf_signal_value(lf_CHANGEBIN_SIGNAL),
      "SIG" lf_value(lf_CHANGEBIN_SIGNAL),
      "",
      lf_signal_handler },

    { SIGALRM, "SIGALRM", "", lf_signal_handler },

    { SIGINT, "SIGINT", "", lf_signal_handler },

    { SIGIO, "SIGIO", "", lf_signal_handler },

    { SIGCHLD, "SIGCHLD", "", lf_signal_handler },

    { SIGSYS, "SIGSYS, SIG_IGN", "", NULL },

    { SIGPIPE, "SIGPIPE, SIG_IGN", "", NULL },

    { 0, NULL, "", NULL }
};


lf_pid_t
lf_spawn_process(lf_cycle_t *cycle, lf_spawn_proc_pt proc, void *data,
    char *name, lf_int_t respawn)
{
    u_long     on;
    lf_pid_t  pid;
    lf_int_t  s;

    if (respawn >= 0) {
        s = respawn;

    } else {
        for (s = 0; s < lf_last_process; s++) {
            if (lf_processes[s].pid == -1) {
                break;
            }
        }

        if (s == lf_MAX_PROCESSES) {
            lf_log_error(lf_LOG_ALERT, cycle->log, 0,
                          "no more than %d processes can be spawned",
                          lf_MAX_PROCESSES);
            return lf_INVALID_PID;
        }
    }


    if (respawn != lf_PROCESS_DETACHED) {

        /* Solaris 9 still has no AF_LOCAL */

        if (socketpair(AF_UNIX, SOCK_STREAM, 0, lf_processes[s].channel) == -1)
        {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "socketpair() failed while spawning \"%s\"", name);
            return lf_INVALID_PID;
        }

        lf_log_debug2(lf_LOG_DEBUG_CORE, cycle->log, 0,
                       "channel %d:%d",
                       lf_processes[s].channel[0],
                       lf_processes[s].channel[1]);

        if (lf_nonblocking(lf_processes[s].channel[0]) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          lf_nonblocking_n " failed while spawning \"%s\"",
                          name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        if (lf_nonblocking(lf_processes[s].channel[1]) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          lf_nonblocking_n " failed while spawning \"%s\"",
                          name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        on = 1;
        if (ioctl(lf_processes[s].channel[0], FIOASYNC, &on) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "ioctl(FIOASYNC) failed while spawning \"%s\"", name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        if (fcntl(lf_processes[s].channel[0], F_SETOWN, lf_pid) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "fcntl(F_SETOWN) failed while spawning \"%s\"", name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        if (fcntl(lf_processes[s].channel[0], F_SETFD, FD_CLOEXEC) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        if (fcntl(lf_processes[s].channel[1], F_SETFD, FD_CLOEXEC) == -1) {
            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "fcntl(FD_CLOEXEC) failed while spawning \"%s\"",
                           name);
            lf_close_channel(lf_processes[s].channel, cycle->log);
            return lf_INVALID_PID;
        }

        lf_channel = lf_processes[s].channel[1];

    } else {
        lf_processes[s].channel[0] = -1;
        lf_processes[s].channel[1] = -1;
    }

    lf_process_slot = s;


    pid = fork();

    switch (pid) {

    case -1:
        lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                      "fork() failed while spawning \"%s\"", name);
        lf_close_channel(lf_processes[s].channel, cycle->log);
        return lf_INVALID_PID;

    case 0:
        lf_parent = lf_pid;
        lf_pid = lf_getpid();
        proc(cycle, data);
        break;

    default:
        break;
    }

    lf_log_error(lf_LOG_NOTICE, cycle->log, 0, "start %s %P", name, pid);

    lf_processes[s].pid = pid;
    lf_processes[s].exited = 0;

    if (respawn >= 0) {
        return pid;
    }

    lf_processes[s].proc = proc;
    lf_processes[s].data = data;
    lf_processes[s].name = name;
    lf_processes[s].exiting = 0;

    switch (respawn) {

    case lf_PROCESS_NORESPAWN:
        lf_processes[s].respawn = 0;
        lf_processes[s].just_spawn = 0;
        lf_processes[s].detached = 0;
        break;

    case lf_PROCESS_JUST_SPAWN:
        lf_processes[s].respawn = 0;
        lf_processes[s].just_spawn = 1;
        lf_processes[s].detached = 0;
        break;

    case lf_PROCESS_RESPAWN:
        lf_processes[s].respawn = 1;
        lf_processes[s].just_spawn = 0;
        lf_processes[s].detached = 0;
        break;

    case lf_PROCESS_JUST_RESPAWN:
        lf_processes[s].respawn = 1;
        lf_processes[s].just_spawn = 1;
        lf_processes[s].detached = 0;
        break;

    case lf_PROCESS_DETACHED:
        lf_processes[s].respawn = 0;
        lf_processes[s].just_spawn = 0;
        lf_processes[s].detached = 1;
        break;
    }

    if (s == lf_last_process) {
        lf_last_process++;
    }

    return pid;
}


lf_pid_t
lf_execute(lf_cycle_t *cycle, lf_exec_ctx_t *ctx)
{
    return lf_spawn_process(cycle, lf_execute_proc, ctx, ctx->name,
                             lf_PROCESS_DETACHED);
}


static void
lf_execute_proc(lf_cycle_t *cycle, void *data)
{
    lf_exec_ctx_t  *ctx = data;

    if (execve(ctx->path, ctx->argv, ctx->envp) == -1) {
        lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                      "execve() failed while executing %s \"%s\"",
                      ctx->name, ctx->path);
    }

    exit(1);
}


lf_int_t
lf_init_signals(lf_log_t *log)
{
    lf_signal_t      *sig;
    struct sigaction   sa;

    for (sig = signals; sig->signo != 0; sig++) {
        lf_memzero(&sa, sizeof(struct sigaction));

        if (sig->handler) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;

        } else {
            sa.sa_handler = SIG_IGN;
        }

        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
#if (lf_VALGRIND)
            lf_log_error(lf_LOG_ALERT, log, lf_errno,
                          "sigaction(%s) failed, ignored", sig->signame);
#else
            lf_log_error(lf_LOG_EMERG, log, lf_errno,
                          "sigaction(%s) failed", sig->signame);
            return lf_ERROR;
#endif
        }
    }

    return lf_OK;
}


static void
lf_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    char            *action;
    lf_int_t        ignore;
    lf_err_t        err;
    lf_signal_t    *sig;

    ignore = 0;

    err = lf_errno;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    lf_time_sigsafe_update();

    action = "";

    switch (lf_process) {

    case lf_PROCESS_MASTER:
    case lf_PROCESS_SINGLE:
        switch (signo) {

        case lf_signal_value(lf_SHUTDOWN_SIGNAL):
            lf_quit = 1;
            action = ", shutting down";
            break;

        case lf_signal_value(lf_TERMINATE_SIGNAL):
        case SIGINT:
            lf_terminate = 1;
            action = ", exiting";
            break;

        case lf_signal_value(lf_NOACCEPT_SIGNAL):
            if (lf_daemonized) {
                lf_noaccept = 1;
                action = ", stop accepting connections";
            }
            break;

        case lf_signal_value(lf_RECONFIGURE_SIGNAL):
            lf_reconfigure = 1;
            action = ", reconfiguring";
            break;

        case lf_signal_value(lf_REOPEN_SIGNAL):
            lf_reopen = 1;
            action = ", reopening logs";
            break;

        case lf_signal_value(lf_CHANGEBIN_SIGNAL):
            if (lf_getppid() == lf_parent || lf_new_binary > 0) {

                /*
                 * Ignore the signal in the new binary if its parent is
                 * not changed, i.e. the old binary's process is still
                 * running.  Or ignore the signal in the old binary's
                 * process if the new binary's process is already running.
                 */

                action = ", ignoring";
                ignore = 1;
                break;
            }

            lf_change_binary = 1;
            action = ", changing binary";
            break;

        case SIGALRM:
            lf_sigalrm = 1;
            break;

        case SIGIO:
            lf_sigio = 1;
            break;

        case SIGCHLD:
            lf_reap = 1;
            break;
        }

        break;

    case lf_PROCESS_WORKER:
    case lf_PROCESS_HELPER:
        switch (signo) {

        case lf_signal_value(lf_NOACCEPT_SIGNAL):
            if (!lf_daemonized) {
                break;
            }
            lf_debug_quit = 1;
            /* fall through */
        case lf_signal_value(lf_SHUTDOWN_SIGNAL):
            lf_quit = 1;
            action = ", shutting down";
            break;

        case lf_signal_value(lf_TERMINATE_SIGNAL):
        case SIGINT:
            lf_terminate = 1;
            action = ", exiting";
            break;

        case lf_signal_value(lf_REOPEN_SIGNAL):
            lf_reopen = 1;
            action = ", reopening logs";
            break;

        case lf_signal_value(lf_RECONFIGURE_SIGNAL):
        case lf_signal_value(lf_CHANGEBIN_SIGNAL):
        case SIGIO:
            action = ", ignoring";
            break;
        }

        break;
    }

    if (siginfo && siginfo->si_pid) {
        lf_log_error(lf_LOG_NOTICE, lf_cycle->log, 0,
                      "signal %d (%s) received from %P%s",
                      signo, sig->signame, siginfo->si_pid, action);

    } else {
        lf_log_error(lf_LOG_NOTICE, lf_cycle->log, 0,
                      "signal %d (%s) received%s",
                      signo, sig->signame, action);
    }

    if (ignore) {
        lf_log_error(lf_LOG_CRIT, lf_cycle->log, 0,
                      "the changing binary signal is ignored: "
                      "you should shutdown or terminate "
                      "before either old or new binary's process");
    }

    if (signo == SIGCHLD) {
        lf_process_get_status();
    }

    lf_set_errno(err);
}


static void
lf_process_get_status(void)
{
    int              status;
    char            *process;
    lf_pid_t        pid;
    lf_err_t        err;
    lf_int_t        i;
    lf_uint_t       one;

    one = 0;

    for ( ;; ) {
        pid = waitpid(-1, &status, WNOHANG);

        if (pid == 0) {
            return;
        }

        if (pid == -1) {
            err = lf_errno;

            if (err == lf_EINTR) {
                continue;
            }

            if (err == lf_ECHILD && one) {
                return;
            }

            /*
             * Solaris always calls the signal handler for each exited process
             * despite waitpid() may be already called for this process.
             *
             * When several processes exit at the same time FreeBSD may
             * erroneously call the signal handler for exited process
             * despite waitpid() may be already called for this process.
             */

            if (err == lf_ECHILD) {
                lf_log_error(lf_LOG_INFO, lf_cycle->log, err,
                              "waitpid() failed");
                return;
            }

            lf_log_error(lf_LOG_ALERT, lf_cycle->log, err,
                          "waitpid() failed");
            return;
        }


        one = 1;
        process = "unknown process";

        for (i = 0; i < lf_last_process; i++) {
            if (lf_processes[i].pid == pid) {
                lf_processes[i].status = status;
                lf_processes[i].exited = 1;
                process = lf_processes[i].name;
                break;
            }
        }

        if (WTERMSIG(status)) {
#ifdef WCOREDUMP
            lf_log_error(lf_LOG_ALERT, lf_cycle->log, 0,
                          "%s %P exited on signal %d%s",
                          process, pid, WTERMSIG(status),
                          WCOREDUMP(status) ? " (core dumped)" : "");
#else
            lf_log_error(lf_LOG_ALERT, lf_cycle->log, 0,
                          "%s %P exited on signal %d",
                          process, pid, WTERMSIG(status));
#endif

        } else {
            lf_log_error(lf_LOG_NOTICE, lf_cycle->log, 0,
                          "%s %P exited with code %d",
                          process, pid, WEXITSTATUS(status));
        }

        if (WEXITSTATUS(status) == 2 && lf_processes[i].respawn) {
            lf_log_error(lf_LOG_ALERT, lf_cycle->log, 0,
                          "%s %P exited with fatal code %d "
                          "and cannot be respawned",
                          process, pid, WEXITSTATUS(status));
            lf_processes[i].respawn = 0;
        }

        lf_unlock_mutexes(pid);
    }
}


static void
lf_unlock_mutexes(lf_pid_t pid)
{
    lf_uint_t        i;
    lf_shm_zone_t   *shm_zone;
    lf_list_part_t  *part;
    lf_slab_pool_t  *sp;

    /*
     * unlock the accept mutex if the abnormally exited process
     * held it
     */

    if (lf_accept_mutex_ptr) {
        (void) lf_shmtx_force_unlock(&lf_accept_mutex, pid);
    }

    /*
     * unlock shared memory mutexes if held by the abnormally exited
     * process
     */

    part = (lf_list_part_t *) &lf_cycle->shared_memory.part;
    shm_zone = part->elts;

    for (i = 0; /* void */ ; i++) {

        if (i >= part->nelts) {
            if (part->next == NULL) {
                break;
            }
            part = part->next;
            shm_zone = part->elts;
            i = 0;
        }

        sp = (lf_slab_pool_t *) shm_zone[i].shm.addr;

        if (lf_shmtx_force_unlock(&sp->mutex, pid)) {
            lf_log_error(lf_LOG_ALERT, lf_cycle->log, 0,
                          "shared memory zone \"%V\" was locked by %P",
                          &shm_zone[i].shm.name, pid);
        }
    }
}


void
lf_debug_point(void)
{
    lf_core_conf_t  *ccf;

    ccf = (lf_core_conf_t *) lf_get_conf(lf_cycle->conf_ctx,
                                           lf_core_module);

    switch (ccf->debug_points) {

    case lf_DEBUG_POINTS_STOP:
        raise(SIGSTOP);
        break;

    case lf_DEBUG_POINTS_ABORT:
        lf_abort();
    }
}


lf_int_t
lf_os_signal_process(lf_cycle_t *cycle, char *name, lf_pid_t pid)
{
    lf_signal_t  *sig;

    for (sig = signals; sig->signo != 0; sig++) {
        if (lf_strcmp(name, sig->name) == 0) {
            if (kill(pid, sig->signo) != -1) {
                return 0;
            }

            lf_log_error(lf_LOG_ALERT, cycle->log, lf_errno,
                          "kill(%P, %d) failed", pid, sig->signo);
        }
    }

    return 1;
}

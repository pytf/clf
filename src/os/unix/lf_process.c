#include <lf_config.h>
#include <lf_core.h>
#include <lf_event.h>
#include <lf_channel.h>

sig_atomic_t lf_reap;
sig_atomic_t lf_sigio;
sig_atomic_t lf_quit;
sig_atomic_t lf_terminate;
sig_atomic_t lf_reconfigure;
sig_atomic_t lf_reopen;
sig_atomic_t lf_sigalrm;

typedef struct {
    int     signo;
    char   *signame;
    char   *name;
    void  (*handler)(int signo, siginfo_t *siginfo, void *ucontext);
} lf_signal_t;

static void lf_signal_handler(int signo, siginfo_t *siginfo, void *ucontext);

lf_signal_t  signals[] = {
    { SIGHUP,       "SIGHUP",           "reload",    lf_signal_handler },
    { SIGUSR1,      "SIGUSR1",          "reopen",    lf_signal_handler },
    { SIGTERM,      "SIGTERM",          "stop",      lf_signal_handler },
    { SIGQUIT,      "SIGQUIT",          "quit",      lf_signal_handler },
    { SIGALRM,      "SIGALRM",          "",          lf_signal_handler },
    { SIGINT,       "SIGINT",           "",          lf_signal_handler },
    { SIGIO,        "SIGIO",            "",          lf_signal_handler },
    { SIGCHLD,      "SIGCHLD",          "",          lf_signal_handler },
    { SIGSYS,       "SIGSYS, SIG_IGN",  "",          NULL },
    { SIGPIPE,      "SIGPIPE, SIG_IGN", "",          NULL },
    { 0,            NULL,               "",          NULL }
};

int lf_spawn_process(lf_spawn_proc_pt proc, char *name)
{
    pid_t pid;
    pid = fork();

    switch (pid) {

    case -1:
        lf_log_std(LOG_LEVEL_ERR, "fork() failed while spawning \"%s\"", name);
        return -1;

    case 0:
        proc();
        break;

    default:
        break;
    }

    return pid;
}

/*
 *function : Initial signal handler, add signals[]
*/
int lf_init_signals()
{
    lf_signal_t      *sig;
    struct sigaction   sa;

    for (sig = signals; sig->signo != 0; sig++) {
        memset(&sa, 0, sizeof(struct sigaction));

        if (sig->handler) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags = SA_SIGINFO;

        } else {
            sa.sa_handler = SIG_IGN;
        }

        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1)
        {
            lf_log_std(LOG_LEVEL_ERR, "sigaction(%s) failed", sig->signame);
            return RET_ERR;
        }
    }

    return RET_OK;
}

/*
 * function : handle signals[]
*/
static void lf_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{
    char           *action;
    int            ignore;
    int            err;
    lf_signal_t    *sig;

    ignore = 0;

    err = errno;

    for (sig = signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    action = "";

    switch (signo)
    {
        case SIGQUIT:
            lf_quit = 1;
            action = ", shutting down";
            break;

        case SIGTERM:
        case SIGINT:
            lf_terminate = 1;
            action = ", exiting";
            break;

        case SIGHUP:
            lf_reconfigure = 1;
            action = ", reconfiguring";
            break;

        case SIGUSR1:
            lf_reopen = 1;
            action = ", reopening logs";
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

    if (siginfo && siginfo->si_pid)
    {
        lf_log_std(LOG_LEVEL_NOTICE, "signal %d (%s) received from %P%s",signo, sig->signame, siginfo->si_pid, action);

    } else
    {
        lf_log_std(LOG_LEVEL_NOTICE, "signal %d (%s) received%s", signo, sig->signame, action);
    }

    if (ignore)
    {
        lf_log_std(LOG_LEVEL_CRIT, "the changing binary signal is ignored: "
                                "you should shutdown or terminate "
                                "before either old or new binary's process");
    }

    lf_set_errno(err);
}


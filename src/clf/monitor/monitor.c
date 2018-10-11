#include "lf_monitor.h"

static unchar lf_monitor_process[] = "lf_master_process";

void monitorFunc()
{
    int ret = RET_OK;
    unint seconds = 1;
    unint delay = 0;
    sigset_t set;
    struct itimerval   itv;
    
    sigemptyset(&set);
    sigaddset(&set, SIGCHLD);
    sigaddset(&set, SIGALRM);
    sigaddset(&set, SIGIO);
    sigaddset(&set, SIGINT);
    sigaddset(&set, SIGUP);
    sigaddset(&set, SIGUSR1);
    sigaddset(&set, SIGQUIT);
    sigaddset(&set, SIGTERM);
    sigaddset(&set, SIGUSR2);
    sigaddset(&set, SIGWINCH);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        lf_log_std(LOG_LEVEL_ERR, "sigprocmask() failed");
    }

    sigemptyset(&set);
    
    for(;;)
    {
        //time_setitimer(delay);
        if (delay) {
            if (lf_sigalrm) {
                delay *= 2;
                lf_sigalrm = 0;
            }

            lf_log_std(LOG_LEVEL_DEBUG, "termination cycle: %M", delay);

            itv.it_interval.tv_sec = 0;
            itv.it_interval.tv_usec = 0;
            itv.it_value.tv_sec = delay / 1000;
            itv.it_value.tv_usec = (delay % 1000 ) * 1000;

            if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
                lf_log_std(LOG_LEVEL_ERR, "setitimer() failed");
            }
        }

        sigsuspend(&set);
        
        if (lf_reap) {
            lf_reap = 0;
            lf_log_std(LOG_LEVEL_DEBUG, "reap children");
            /*Reap the child process*/
        }
        /*Reivce term to other processes,go to delay,wait to active the processe*/
        if (lf_terminate) {
            if (delay == 0) {
                delay = 50;
            }
            lf_log_std(LOG_LEVEL_NOTICE, "terminate other process");
            /*if timout,force to kill*/

            continue;
        }
        /*Reivce term to other processes,go to delay,wait to active the processe*/
        if (lf_quit) {
            lf_log_std(LOG_LEVEL_NOTICE, "quit software");
            continue;
        }

        if (lf_reconfigure) {
            lf_reconfigure = 0;
            lf_log_std(LOG_LEVEL_NOTICE, "reconfigure software");

        }

        if (lf_reopen) {
            lf_reopen = 0;
            lf_log_std(LOG_LEVEL_NOTICE, "reopening logs");

        }
    }

        //seconds_sleep(seconds);
}

void _ModInitMonitor()
{
    int ret = RET_OK;
    char *title = NULL;

    sprintf_s(title,lf_monitor_process);
    if (NULL == title) {
        /* fatal */
        exit(2);
    }
    setproctitle("%s", lf_monitor_process);

    monitorFunc();
    
}
/*select()*/
void seconds_sleep(unsigned seconds)
{
	struct timeval tv;
	tv.tv_sec = seconds;
	tv.tv_usec = 0;
	int err;
	do
    {
	   err = select(0,NULL,NULL,NULL,&tv);
	}while(err < 0 && errno == EINTR);
}
/*select()*/
void milliseconds_sleep(unsigned long mSec)
{
	struct timeval tv;
	tv.tv_sec = mSec/1000;
	tv.tv_usec = (mSec%1000)*1000;
	int err;
	do
    {
	   err = select(0,NULL,NULL,NULL,&tv);
	}while(err < 0 && errno == EINTR);
}
/*select()*/
void microseconds_sleep(unsigned long uSec)
{
	struct timeval tv;
	tv.tv_sec = uSec/1000000;
	tv.tv_usec = uSec%1000000;
	int err;
	do
    {
	    err = select(0,NULL,NULL,NULL,&tv);
	}while(err < 0 && errno == EINTR);
}

static void timer_signal_handler(int signo)
{
    ngx_event_timer_alarm = 1;

#if 1
    ngx_log_debug0(NGX_LOG_DEBUG_EVENT, ngx_cycle->log, 0, "timer signal");
#endif
}
/*setitimer()*/
int time_setitimer(int delay)
{
    struct sigaction  sa;
    struct itimerval  itv;

    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = timer_signal_handler;
    sigemptyset(&sa.sa_mask);

    if (sigaction(SIGALRM, &sa, NULL) == -1)
    {
        lf_log_std(LOG_LEVEL_ERR, "sigaction(SIGALRM) failed");
        return RET_ERR;
    }

    itv.it_interval.tv_sec = 0;
    itv.it_interval.tv_usec = 0;
    itv.it_value.tv_sec = delay / 1000;
    itv.it_value.tv_usec = (delay % 1000 ) * 1000;

    if (setitimer(ITIMER_REAL, &itv, NULL) == -1)
    {
        lf_log_std(LOG_LEVEL_ERR, "setitimer() failed");
        return RET_ERR;
    }

    return RET_OK;
}



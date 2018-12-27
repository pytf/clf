#ifndef _lf_TIME_H_
#define _lf_TIME_H_

#include <sys/time.h>

#define lf_tm_sec            tm_sec
#define lf_tm_min            tm_min
#define lf_tm_hour           tm_hour
#define lf_tm_mday           tm_mday
#define lf_tm_mon            tm_mon
#define lf_tm_year           tm_year
#define lf_tm_wday           tm_wday
#define lf_tm_isdst          tm_isdst

void lfGmTime(time_t s, struct tm *tm);
void timeNow(time_t* nTime);
void localTimeR(time_t s, struct tm *tm);
void timeString(char strTime[80]);

#define lf_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define lf_msleep(ms)        (void) usleep(ms * 1000)
#define lf_sleep(s)          (void) sleep(s)


#endif /* _lf_TIME_H_ */


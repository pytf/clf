
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_TIME_H_INCLUDED_
#define _lf_TIME_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


typedef lf_rbtree_key_t      lf_msec_t;
typedef lf_rbtree_key_int_t  lf_msec_int_t;

typedef struct tm             lf_tm_t;

#define lf_tm_sec            tm_sec
#define lf_tm_min            tm_min
#define lf_tm_hour           tm_hour
#define lf_tm_mday           tm_mday
#define lf_tm_mon            tm_mon
#define lf_tm_year           tm_year
#define lf_tm_wday           tm_wday
#define lf_tm_isdst          tm_isdst

#define lf_tm_sec_t          int
#define lf_tm_min_t          int
#define lf_tm_hour_t         int
#define lf_tm_mday_t         int
#define lf_tm_mon_t          int
#define lf_tm_year_t         int
#define lf_tm_wday_t         int


#if (lf_HAVE_GMTOFF)
#define lf_tm_gmtoff         tm_gmtoff
#define lf_tm_zone           tm_zone
#endif


#if (lf_SOLARIS)

#define lf_timezone(isdst) (- (isdst ? altzone : timezone) / 60)

#else

#define lf_timezone(isdst) (- (isdst ? timezone + 3600 : timezone) / 60)

#endif


void lf_timezone_update(void);
void lf_localtime(time_t s, lf_tm_t *tm);
void lf_libc_localtime(time_t s, struct tm *tm);
void lf_libc_gmtime(time_t s, struct tm *tm);

#define lf_gettimeofday(tp)  (void) gettimeofday(tp, NULL);
#define lf_msleep(ms)        (void) usleep(ms * 1000)
#define lf_sleep(s)          (void) sleep(s)


#endif /* _lf_TIME_H_INCLUDED_ */

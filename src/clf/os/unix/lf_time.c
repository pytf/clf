#include "lf_time.h"

/*
 * function : 把日期和时间转换为格林威治(GMT)时间
 * input : s ---
 * output : tm ---格林威治(GMT)时间
*/
void lfGmTime(time_t s, struct tm *tm)
{
    struct tm  *t;

    t = gmtime(&s);
    *tm = *t;
}

/*
 * function : get the current time
 * output : nTime ---current time 距1970年1月1日00:00点 UTC的秒数
*/
void timeNow(time_t* nTime)
{
    if (NULL == nTime)
    {
        return;
    }

    time(nTime);
}

/*
 * function : 将时间秒数转化为本地时间
 * input : s ---current time 距1970年1月1日00:00点 UTC的秒数
 * output : tm ---当前时区
*/
void localTimeR(time_t s, struct tm *tm)
{
    if (NULL == s || NULL == tm)
    {
        return NULL;
    }
    localtime_r(&s, tm);
}

/*
 * function : 获取当前时间并格式化输出(2018-09-19 08:59:07)
 * output : strTime
*/
void timeString(char strTime[80])
{
    time_t      s;
    struct tm  *t;

    timeNow(s);

    localTimeR(s, t);

    //strftime(buf, 4, "%H", t);
    strftime(strTime, 80, "[%Y-%m-%d] [%H:%M:%S]", t);
}


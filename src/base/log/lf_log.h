#ifndef _LF_LOG_H_
#define _LF_LOG_H_


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <stdarg.h> /* for va_list */
#include <stdio.h> /* for size_t */
#include <syslog.h> /* for syslog */
#include "type.h"

#define MAX_HEAD_SIZE       16
#define MAX_MSG_SIZE        65536 * 6
#define LOG_HEAD_LENGTH     10
#define DEFAULT_LOG_COUNT   5
#define DEFAULT_LOG_SIZE    UNIT_M * 10

#define PATH_SEPARATOR                 "/"
#define MAX_FULL_PATH_LEN              300


typedef enum {
    LOG_LEVEL_DEBUG = 20,
    LOG_LEVEL_INFO = 40,
    LOG_LEVEL_NOTICE = 60,
    LOG_LEVEL_WARN = 80,
    LOG_LEVEL_ERR = 100,
    LOG_LEVEL_CRIT= 120
} lf_log_level;


typedef enum {
    LOG_CACHE_ON,
    LOG_CACHE_OFF,
    LOG_CACHE_QUIT
} lf_log_cache;


//日志类读写锁
static pthread_rwlock_t lf_log_env_lock = PTHREAD_RWLOCK_INITIALIZER;
//日志文件名
static s8* lf_strFileName;
//日志文件路径
static s8* lf_strFilePath;
//日志最大大小
static u32 lf_iMaxSize;
//日志当前级别
static u32 lf_curLogLevel;
//日志个数
static u32 lf_iLogCount;

#define lf_log_std(level, iFormat, ...) \
    lf_log(level, __FILE__, __FUNCTION__, __LINE__, iFormat, __VA_ARGS__)

int lf_log_init(const char *confpath);

void lf_log(const int level, const char *file, const char *func, long line, const char *format, ...);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _LF_LOG_H_ */


#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

/*
 * function : 初始化日志
*/
int lf_log_init()
{
    int ret;

    lf_iMaxSize = DEFAULT_LOG_SIZE;
    lf_curLogLevel = LOG_LEVEL_INFO;
    lf_iLogCount = DEFAULT_LOG_COUNT;
    
    ret = pthread_rwlock_init(lf_log_env_lock, null);
    if(RET_OK != ret)
    {
        return ret;
    }

    return RET_OK;
}

/*
 * function : 
 * input : level
*/
static inline const char* lf_log_label(int level)
{
    switch(level)
    {
    case LOG_LEVEL_DEBUG:  
        return "DBG"; 
    case LOG_LEVEL_INFO:   
        return "INFO"; 
    case LOG_LEVEL_NOTICE: 
        return "NTC"; 
    case LOG_LEVEL_WARN:
        return "WARN";
    case LOG_LEVEL_ERR:    
        return "ERR";
    case LOG_LEVEL_CRIT:   
        return "CRIT";
    default: 
        syslog(LOG_LEVEL_WARN, 
               "[WARN][%d:::] unknown log level: %d", getpid(), level);
        return "unkown";
    }
}

/*
 * function : 读取配置中日志级别和计数
*/
void readLevelAndCount()
{
    unint iLogLevel = 0;
    unint iLogCount = 0;
    dictionary*   ini;
    unint    retIni = -1;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return ;
    }
    iniparser_dump(ini, stderr);
    
    retIni = iniparser_getint(ini, "lf_curLogLevel", -1);
    if (RET_ERR== retIni) {
        fprintf(stderr, "cannot parse file: %d\n", retIni);
        return ;
    }
    lf_curLogLevel = retIni;

    retIni = iniparser_getint(ini, "lf_iLogCount", -1);
    if (RET_ERR== retIni) {
        fprintf(stderr, "cannot parse file: %d\n", retIni);
        return ;
    }
    lf_iLogCount = retIni;
    
    iniparser_freedict(ini);
}

/*
 * function : 打开日志文件
*/
FILE* openLogFile()
{
    short strLogFilePath;
    FILE* pFile = NULL;
    unint fd;
    struct stat st;
    int ret = RET_ERR;

    strLogFilePath = lf_strFilePath + PATH_SEPARATOR + lf_strFileName;
    fd = open(strLogFilePath.c_str(), O_RDONLY);
    if (-1 == fd)
    {
        ret = CreateLogFile(strLogFilePath.c_str());
        if (RET_ERR == ret)
        {
            return NULL;
        }
    }
    else
    {
        (void)fstat(fd, &st);
        close(fd);
        //日志超过大小切换日志
        if (st.st_size >= lf_iMaxSize)
        {
            ret = SwitchLogFile(lf_strFilePath.c_str(), lf_strFileName.c_str(), lf_iLogCount);
            if (MP_FAILED == ret)
            {
                return NULL;
            }
        }
    }

    pFile = fopen(strLogFilePath.c_str(), "a+");
    if (NULL == pFile)
    {
        return NULL;
    }

    return pFile;
}

/*
 * function : 创建日志文件
 * input : pszLogFile ---日志文件名
*/
int createLogFile(const unshort* pszLogFile)
{
    unint fd = 0;
    fd = open(pszLogFile, O_CREAT, S_IRUSR | S_IWUSR);
    if (-1 == fd)
    {
        return RET_ERR;
    }
    close(fd);
    // 设置权限
    (void)chmod(pszLogFile, S_IRUSR | S_IWUSR);

    return RET_OK;
}

/*
 * function : 选择日志文件
 * input : pszLogPath ---log存放路径
 *         pszLogName ---log名
 *         iLogCount ---log计数行
*/
int switchLogFile(const char* pszLogPath, const char* pszLogName, int iLogCount)
{
    int ret = RET_ERR;
    unshort acDestFile[MAX_FULL_PATH_LEN] = {0};
    unshort acBackFile[MAX_FULL_PATH_LEN] = {0};
    unshort* strSuffix = "zip";
    unshort* strCommand;
    unshort strLogFile[65536] ={0};
    int i = iLogCount;

    strSuffix = "gz";
    ret = snprintf_s(strLogFile, 65536, 65536 - 1, \
        "%s%s%s", pszLogPath, PATH_SEPARATOR, pszLogName);
    if (RET_ERR== ret)
    {
        return ret;
    }

    //删除时间最久的一个备份日志文件
    ret = snprintf_s(acBackFile, MAX_FULL_PATH_LEN, MAX_FULL_PATH_LEN - 1, \
        "%s%s%s.%d.%s", pszLogPath, PATH_SEPARATOR, pszLogName, i, strSuffix);
    if (RET_ERR== ret)
    {
        return ret;
    }

    (void)remove(acBackFile);

    //一个一个更改日志文件名称
    i--;
    for (; i >= 0; i--)
    {
        //第一个文件名中不包括0
        if (0 == i)
        {
            ret = snprintf_s(acBackFile, MAX_FULL_PATH_LEN, MAX_FULL_PATH_LEN - 1, "%s%s%s.%s", \
                pszLogPath, PATH_SEPARATOR, pszLogName, strSuffix);
        }
        else
        {
            ret = snprintf_s(acBackFile, MAX_FULL_PATH_LEN, MAX_FULL_PATH_LEN - 1, "%s%s%s.%d.%s", \
                pszLogPath, PATH_SEPARATOR, pszLogName, i, strSuffix);
        }

        if (RET_ERR== ret)
        {
            return ret;
        }

        if (0 == i)
        {            
            //压缩文件备份文件
            ret = snprintf_s(strCommand, MAX_FULL_PATH_LEN, MAX_FULL_PATH_LEN - 1, "gzip -f -q -9 \"%s\"", strLogFile);
            if (CheckCmdDelimiter(strCommand) == RET_ERR)
            {
                return RET_ERR;
            }

            ret = system(strCommand);
            if(!WIFEXITED(ret))
            {
                //system异常返回
                return RET_ERR;
            }
        }

        ret = snprintf_s(acDestFile, MAX_FULL_PATH_LEN, MAX_FULL_PATH_LEN - 1, "%s%s%s.%d.%s", \
            pszLogPath, PATH_SEPARATOR, pszLogName, (i + 1), strSuffix);
        if (RET_ERR == ret)
        {
            return ret;
        }
        (void)rename(acBackFile, acDestFile);

        (void)chmod(acDestFile, S_IRUSR);
    }

    ret = CreateLogFile(strLogFile);
    if (RET_ERR == ret)
    {
        return ret;
    }

    return ret;
}

/*
 * function : 制作消息头
 * input : iLevel ---日志级别
 *         iBufLen ---buf长度
 * output : headBuf ---消息头
*/
int mkHead(int iLevel, char* headBuf, int iBufLen)
{
    int ret = RET_OK;

    const char* strHead = lf_log_label(iLevel);
    if ("unkown"== strHead)
    {
        return RET_ERR;
    }
    
    ret= snprintf_s(headBuf, iBufLen, iBufLen - 1, "%s", strHead);
    if (RET_ERR== ret)
    {
        return RET_ERR;
    }
    return ret;
}

/*
 * function : 日志打印
 * input : level ---日志级别
 *         file ---文件名
 *         func ---函数名
 *         line ---行号
 *         format ---输出格式
 *         ... ---输出参数
*/
void lf_log(const int level, const char *file, const char *func, long line, const char *format, ...)
{
    va_list args;
    int ret = RET_ERR;
    FILE* pFile = NULL;
    char curTime[80] = {0};
    char logMsg[65536]= {0};
    char strMsgHead[10] = {0};
    char strMsg[65546] = {0};

    ReadLevelAndCount();
    if (level < lf_curLogLevel)
    {
        return;
    }

    va_start(args, format);
    
    timeString(curTime);
    if (NULL == curTime)
    {
        va_end(args);
        return;
    }

    pthread_rwlock_rdlock(&lf_log_env_lock);
    ret = snprintf_s(logMsg, sizeof(logMsg),sizeof(logMsg) - 1, format, args);
    if (RET_ERR == ret)
    {
        va_end(args);
        pthread_rwlock_unlock(&lf_log_env_lock);
        return;
    }

    ret = mkHead(level, strMsgHead, sizeof(strMsgHead));
    if (RET_OK!= ret)
    {
        va_end(args);
        pthread_rwlock_unlock(&lf_log_env_lock);
        return;
    }

    snprintf_s(strMsg, sizeof(strMsg),sizeof(strMsg) - 1, "%s ", curTime, getpid(), lf_thread_tid(), file, line, logMsg);

    pFile = openLogFile();
    if (NULL == pFile)
    {
        va_end(args);
        pthread_rwlock_unlock(&lf_log_env_lock);
        return;
    }
    
    fprintf(pFile, "%s", strMsg);
    fflush(pFile);
    fclose(pFile);
    pthread_rwlock_unlock(&lf_log_env_lock);

    va_end(args);

    return;
}


/*
 * This file is part of the log file.
 * Copyright (C)
 * Licensed
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <pthread.h>

#include "conf.h"

int lf_log_init(const char *confpath)
{
    int ret;

    lf_iMaxSize = DEFAULT_LOG_SIZE;
    //设置日志默认级别和个数
    lf_curLogLevel = LOG_LEVEL_INFO;
    lf_iLogCount = DEFAULT_LOG_COUNT;
    
    ret = pthread_rwlock_init(lf_log_env_lock, null);
    if(RET_OK != ret)
    {
        return ret;
    }

    return RET_OK;
}

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

void ReadLevelAndCount()
{
    u32 iLogLevel = 0;
    u32 iLogCount = 0;
    dictionary*   ini;
    u32    retIni = -1;

    ini = iniparser_load(ini_name);
    if (ini==NULL) {
        fprintf(stderr, "cannot parse file: %s\n", ini_name);
        return -1 ;
    }
    iniparser_dump(ini, stderr);
    
    retIni = iniparser_getint(ini, "lf_curLogLevel", -1);
    if (-1 == retIni) {
        fprintf(stderr, "cannot parse file: %d\n", retIni);
        return -1 ;
    }
    lf_curLogLevel = retIni;

    retIni = iniparser_getint(ini, "lf_iLogCount", -1);
    if (-1 == retIni) {
        fprintf(stderr, "cannot parse file: %d\n", retIni);
        return -1 ;
    }
    lf_iLogCount = retIni;
    
    iniparser_freedict(ini);
}

FILE* OpenLogFile()
{
    s16 strLogFilePath;
    FILE* pFile = NULL;
    u32 fd;
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

s32 CreateLogFile(const u16* pszLogFile)
{
    u32 fd = 0;
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

s32 SwitchLogFile(const mp_char* pszLogPath, const mp_char* pszLogName, mp_int32 iLogCount)
{
    int ret = RET_ERR;
    u16 acDestFile[MAX_FULL_PATH_LEN] = {0};
    u16 acBackFile[MAX_FULL_PATH_LEN] = {0};
    u16* strSuffix = "zip";
    u16* strCommand;
    u16 strLogFile[MAX_FILE_NAME_LEN] ={0};
    s32 i = iLogCount;

    strSuffix = "gz";
    ret = snprintf_s(strLogFile, MAX_FILE_NAME_LEN, MAX_FILE_NAME_LEN - 1, \
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
            if (CheckCmdDelimiter(strCommand) == MP_FALSE)
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

void lf_log(const int level, const char *file, size_t filelen, const char *func, size_t funclen,
    long line, const char *format, ...)
{
    va_list args;
    int ret = RET_ERR;

    if (level < lf_curLogLevel)
    {
        return;
    }

    pthread_rwlock_rdlock(&lf_log_env_lock);

    if (!lf_log_env_is_init) {
        zc_error("never call lf_log_init() or dlf_log_init() before");
        goto exit;
    }

    va_start(args, format);
    CMpTime::Now(&tLongTime);
    pstCurTime = CMpTime::LocalTimeR(&tLongTime, &stCurTime);
    if (NULL == pstCurTime)
    {
        va_end(args);
        return;
    }

    iRet = vsnprintf_s(acMsg, sizeof(acMsg),sizeof(acMsg) - 1, format, pszArgp); //lint !e530
    if (MP_FAILED == iRet)
    {
        va_end(args);
        return;
    }

    iRet = MkHead(iLevel, acMsgHead, sizeof(acMsgHead));
    if (MP_SUCCESS != iRet)
    {
        va_end(args);
        return;
    }

    strMsg <<"[" 
       <<std::setfill('0') 
       <<std::setw(4) <<(pstCurTime->tm_year + 1900) <<"-"
       <<std::setw(2) <<(pstCurTime->tm_mon + 1) <<"-" 
       <<std::setw(2) <<pstCurTime->tm_mday <<" "
       <<std::setw(2) <<pstCurTime->tm_hour <<":" 
       <<std::setw(2) <<pstCurTime->tm_min <<":" 
       <<std::setw(2) <<pstCurTime->tm_sec <<"][0x" 
       <<std::setw(16) <<std::hex <<ulCode <<"][";

    if (MP_SUCCESS == GetCurrentUserName(strUserName, iErrCode))
    {
        strMsg <<std::dec <<getpid() <<"][" <<CMpThread::GetThreadId() <<"][" <<strUserName;
    }
    else
    {
        strMsg <<std::dec <<getpid() <<"][" <<CMpThread::GetThreadId() <<"][u:" <<std::dec <<getuid() <<"e:" <<std::dec <<iErrCode;
    }

    strMsg <<"][" <<acMsgHead <<"][" <<BaseFileName(pszFileName) <<"," <<iFileLine <<"]" <<acMsg <<std::endl;

    pFile = OpenLogFile();
    if (NULL == pFile)
    {
        va_end(args);
        return;
    }
    
    fprintf(pFile, "%s", strMsg.str().c_str());
    fflush(pFile);
    fclose(pFile);

    va_end(args);

exit:
    pthread_rwlock_unlock(&lf_log_env_lock);
    return;
}


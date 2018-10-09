
#ifndef __UTILS_H__
#define __UTILS_H__

#include "Types.h"
#include "Defines.h"
#include "common/Log.h"

#define LINUX_USERNAME_LEN 1024

#define HOST_OS_UNKNOWN         0
#define HOST_OS_WINDOWS         1
#define HOST_OS_REDHAT          2
#define HOST_OS_HP_UX           3
#define HOST_OS_SOLARIS         4
#define HOST_OS_AIX             5
#define HOST_OS_SUSE            6
#define HOST_OS_ORACLE_LINUX    7
#define HOST_OS_OTHER_LINUX     8

//不要直接使用signal注册信号
typedef mp_void (*signal_proc)(mp_int32);
mp_int32 SignalRegister(mp_int32 signo, signal_proc func);

mp_void DoSleep(mp_uint32 ms);
mp_bool CheckCmdDelimiter(mp_string& str);
mp_int32 GetOSError();
mp_char* GetOSStrErr(mp_int32 err, mp_char* buf, mp_size buf_len);
mp_int32 InitCommonModules(mp_char* pszFullBinPath);
mp_int32 GetHostName(mp_string& strHostName);

//动态库操作相关
#define DFLG_LOCAL   (RTLD_NOW | RTLD_LOCAL)
#define DFLG_GLOBAL  (RTLD_NOW | RTLD_GLOBAL)
mp_handle_t DlibOpen(const mp_char* pszLibName);
mp_handle_t DlibOpenEx(const mp_char* pszLibName, mp_bool bLocal);
mp_void DlibClose(mp_handle_t hDlib);
mp_void* DlibDlsym(mp_handle_t hDlib, const char *pszFname);
const mp_char* DlibError(mp_char* szMsg, mp_uint32 isz);
mp_int32 PackageLog(mp_string strLogName);
mp_int32 GetCurrentUserName(mp_string &strUserName, mp_ulong &iErrCode);
const mp_char* BaseFileName(const mp_char* pszFileName);
mp_void RemoveFullPathForLog(mp_string strCmd, mp_string &strLogCmd);

mp_int32 GetUidByUserName(mp_string strUserName, mp_int32 &uid, mp_int32 &gid);
mp_int32 ChownFile(mp_string strFileName, mp_int32 uid, mp_int32 gid);

//以下临时使用固定函数实现，后续开源软件选型后采用开源软件优化
mp_int32 CheckParamString(mp_string &paramValue, mp_int32 lenBeg, mp_int32 lenEnd, mp_string &strInclude, 
    mp_string &strExclude);
mp_int32 CheckParamString(mp_string &paramValue, mp_int32 lenBeg, mp_int32 lenEnd, mp_string &strPre);
mp_int32 CheckParamStringEnd(mp_string &paramValue, mp_int32 lenBeg, mp_int32 lenEnd, mp_string &strEnd);
mp_int32 CheckParamInteger32(mp_int32 paramValue, mp_int32 begValue, mp_int32 endValue, vector<mp_int32> &vecExclude);
mp_int32 CheckParamInteger64(mp_int64 paramValue, mp_int64 begValue, mp_int64 endValue, vector<mp_int64> &vecExclude);
mp_int32 CheckParamStringIsIP(mp_string &paramValue);
mp_int32 CheckPathString(mp_string &pathValue);
mp_int32 CheckPathString(mp_string &pathValue, mp_string strPre);
mp_int32 CheckFileSysMountParam(mp_string strDeviceName, mp_int32 volumeType, mp_string strMountPoint);
mp_int32 CheckFileSysFreezeParam(mp_string strDiskNames);
//获取OS类型
mp_void GetOSType(mp_int32 &iOSType);
//获取OS版本信息
mp_int32 GetOSVersion(mp_int32 iOSType, mp_string &strOSVersion);

#endif //__AGENT_UTILS_H__


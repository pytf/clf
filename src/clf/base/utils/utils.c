#include <signal.h>
#include <libgen.h>
#include <pwd.h>
#include <unistd.h>
#include <sys/types.h>
#include <sstream>
#include "Utils.h"
#include "Log.h"
#include "Path.h"
#include "UniqueId.h"
#include "ConfigXmlParse.h"
#include "CryptAlg.h"
#include "RootCaller.h"
#include "SystemExec.h"
#include "String.h"

thread_lock_t g_GetUidByUName_Mutex;
thread_lock_t g_DlibError_Mutex;
/*------------------------------------------------------------ 
Description  :˯�ߺ���
Input        : ms -- ʱ��
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
void DoSleep(mp_uint32 ms)
{
#ifdef WIN32
    Sleep(ms);
#else
    struct timeval stTimeOut;

    stTimeOut.tv_sec  = ms / 1000;
    stTimeOut.tv_usec = (ms % 1000) * 1000;
    (void)select(0, NULL, NULL, NULL, &stTimeOut);
#endif
}
/*------------------------------------------------------------ 
Description  :ע���ź�
Input        : signo -- �ź�
                  func -- ���źŰ󶨵Ľӿ�
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
u32 SignalRegister(u32 signo, signal_proc func)
{
#ifndef WIN32
    struct sigaction act;
    struct sigaction oact;

    (void)memset_s(&act, sizeof(struct sigaction), 0, sizeof(struct sigaction));
    if (0 != sigemptyset(&act.sa_mask))
    {
        return MP_FAILED;
    }
    act.sa_flags = 0;
    act.sa_handler = func;

    if (0 != sigaction(signo, &act, &oact))
    {
        return MP_FAILED;
    }
#endif

    return MP_SUCCESS;
}
/*------------------------------------------------------------ 
Description  :������������
Input        : str -- ����������
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
//����������ָ���("&&"��"||"��"&"��";")�򷵻�true�������򷵻�false
bool CheckCmdDelimiter(u16* str)
{
    size_t idx;

    idx = str.find(STR_SEMICOLON, 0);
    if (u16::npos != idx)
    {
        return MP_FALSE;
    }

    idx = str.find(STR_DOUBLE_VERTICAL_LINE, 0);
    if (u16::npos != idx)
    {
        return MP_FALSE;
    }

    idx = str.find(STR_ADDRESS, 0);
    if (u16::npos != idx)
    {
        return MP_FALSE;
    }

    idx = str.find(STR_DOUBLE_ADDRESS, 0);
    if (u16::npos != idx)
    {
        return MP_FALSE;
    }

    return MP_TRUE;
}
/*------------------------------------------------------------ 
Description  :��ȡϵͳ������
Input        : 
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
u32 GetOSError()
{
    return errno;
}
/*------------------------------------------------------------ 
Description  :��ȡϵͳ��������
Input        : 
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
mp_char* GetOSStrErr(u32 err, mp_char* buf, mp_size buf_len)
{
    //strerror_r(err, buf, buf_len);
    buf = strerror(err);
    buf[buf_len - 1] = 0;
    
    return buf;
}
/*------------------------------------------------------------ 
Description  :��lib
Input        : pszLibName -- lib��
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
mp_handle_t DlibOpen(const mp_char* pszLibName)
{
    return DlibOpenEx(pszLibName, MP_TRUE);
}
/*------------------------------------------------------------ 
Description  :��lib
Input        : pszLibName -- lib��
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
mp_handle_t DlibOpenEx(const mp_char* pszLibName, bool bLocal)
{
    u32 flag = bLocal? DFLG_LOCAL : DFLG_GLOBAL;
    //Coverity&Fortify��:FORTIFY.Process_Control
    //pszLibName���ó����ݶ��Ǿ���·��
    return dlopen(pszLibName, flag);
}
/*------------------------------------------------------------ 
Description  :�ر�lib
Input        : hDlib -- ��libʱ�ľ��
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
void DlibClose(mp_handle_t hDlib)
{
    if(0 == hDlib)
    {
        return;
    }

    dlclose(hDlib);
}
/*------------------------------------------------------------ 
Description  :ȡ�÷���pszFname�ĵ�ַ
Input        : hDlib -- ���
                  pszFname -- ������
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
void* DlibDlsym(mp_handle_t hDlib, const mp_char* pszFname)
{
    return dlsym(hDlib, pszFname);
}
/*------------------------------------------------------------ 
Description  :ȡ��lib�﷽��ִ�г�����Ϣ
Input        : 
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
const mp_char* DlibError(mp_char* szMsg, mp_uint32 isz)
{
    CThreadAutoLock cLock(&g_DlibError_Mutex);
    const mp_char* pszErr = dlerror();
    if(NULL == pszErr)
    {
        szMsg[0] = 0;
        return NULL;
    }
    
    u32 iRet = strncpy_s(szMsg, isz, pszErr, isz-1);
    if (EOK != iRet)
    {
        return NULL;
    }
    szMsg[isz - 1] = 0;
    return szMsg;
}
/*------------------------------------------------------------ 
Description  :��ʼ������ģ��
Input        : 
Output       : 
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
u32 InitCommonModules(mp_char* pszFullBinPath)
{
    u32 iRet = MP_SUCCESS;

    //��ʼ��Agent·��
    iRet = CPath::GetInstance().Init(pszFullBinPath);
    if (MP_SUCCESS != iRet)
    {
        printf("Init agent path failed.\n");
        return iRet;
    }

    //��ʼ�������ļ�ģ��
    iRet = CConfigXmlParser::GetInstance().Init(CPath::GetInstance().GetConfFilePath(AGENT_XML_CONF));
    if (MP_SUCCESS != iRet)
    {
        printf("Init conf file %s failed.\n", AGENT_XML_CONF);
        return iRet;
    }

    //��ʼ����־ģ��
    CLogger::GetInstance().Init(AGENT_LOG_NAME, CPath::GetInstance().GetLogPath());

    //���������ʼ��
    CUniqueID::GetInstance().Init();
    return MP_SUCCESS;
}
/*------------------------------------------------------------ 
Description  :��ȡ������
Input        : 
Output       : strHostName -- ������
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
u32 GetHostName(u16& strHostName)
{
    u32 iRet = MP_SUCCESS;
    mp_char szHostName[MAX_HOSTNAME_LEN] = {0};

    iRet = gethostname(szHostName, sizeof(szHostName));
    if (MP_SUCCESS != iRet)
    {
        iRet = GetOSError();
    }

    strHostName = szHostName;
    return iRet;
}

/*------------------------------------------------------------ 
Description  :��ȡ�û���
Input        : 
Output       : strUserName -- �û���
                  iErrCode    --   ������
Return       :
Create By    :
Modification : 
-------------------------------------------------------------*/ 
u32 GetCurrentUserName(u16 &strUserName, mp_ulong &iErrCode) 
{
    struct passwd pwd;
    struct passwd *result = NULL;
    mp_char *pbuf = NULL;
    u32 error;
    mp_size size;

    //��ʼ��buf�Ĵ�С,ͨ��ϵͳ�������л�ȡ
    size = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (MP_FALSE == size)
    {
        size = LINUX_USERNAME_LEN;
    }

    pbuf = (char *)malloc(size);
    if (NULL == pbuf) 
    {
        iErrCode = GetOSError();
        return MP_FAILED;
    }
    
    error = getpwuid_r(getuid(), &pwd, pbuf, size, &result);
    if (MP_FAILED == error || NULL == result)
    {
        iErrCode = GetOSError();
        free(pbuf);
        pbuf = NULL;
        return MP_FAILED;
    }
    strUserName = pwd.pw_name;
    
    free(pbuf);
    pbuf = NULL;

    return MP_SUCCESS;
}

#ifdef WIN32
u32 GetCurrentUserNameW(mp_wstring &strUserName, mp_ulong &iErrCode) 
{
    mp_ulong size = WINDOWS_USERNAME_LEN;   
    mp_wchar *pUsername = new mp_wchar[size];
    if (!GetUserNameW(pUsername, &size))
    {
        delete[] pUsername;
        iErrCode = GetOSError();
        // ��־ģ��ʹ�ã��޷���¼��־
        return MP_FAILED;
    }
    strUserName = pUsername;
    iErrCode = 0;
    delete[] pUsername;
    return MP_SUCCESS;
}

const mp_wchar* BaseFileNameW(const mp_wchar* pszFileName)
{
    const mp_wchar* p = pszFileName;
    mp_int64 len = wcslen(pszFileName);
    while (len > 0)
    {
        p = pszFileName + len;
        if (*p == L'\\')
            return &pszFileName[len + 1];

        len--;
    }

    return pszFileName;
}

const mp_char* BaseFileName(const mp_char* pszFileName)
{
    const mp_char* p = pszFileName;
    mp_int64 len = strlen(pszFileName);
    while (len > 0)
    {
        p = pszFileName + len;
        if (*p == '/')
        return &pszFileName[len + 1];

        len--;
    }

    return pszFileName;
}


/*---------------------------------------------------------------------------
Function Name: RemoveFullPathForLog
Description  : ȥ�������е�ȫ·��������־��ӡ�� 
    ��/usr/bin/ls filename,���ls filename
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
void RemoveFullPathForLog(u16 strCmd, u16 &strLogCmd)
{
    vector<u16> vecCmdParams;
    CMpString::StrSplit(vecCmdParams, strCmd, ' ');

    if (vecCmdParams.size() == 0)
    {
        strLogCmd = strCmd;
    }
    else
    {
        strLogCmd = BaseFileName(vecCmdParams[0].c_str());
        vector<u16>::iterator iter = vecCmdParams.begin();
        ++iter;
        for (; iter != vecCmdParams.end(); ++iter)
        {
            strLogCmd = strLogCmd + " " + *iter;
        }
    }
}

/*---------------------------------------------------------------------------
Function Name: PackageLog
Description  : �����־
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 PackageLog(u16 strLogName)
{
    //���ýű������־
    //������־��rootȨ�ޣ���Ҫ����Ȩ�޲ſ��Ե�������־���ƺ�׺�нű����
    COMMLOG(OS_LOG_INFO, OS_LOG_INFO, "Package Log Name is %s.", strLogName.c_str());
    //��־�ռ��ű�������־������Ʋ�����׺
    u16::size_type pos = strLogName.find(ZIP_SUFFIX);
    if (pos == u16::npos)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR, "strLogName is invalid.", strLogName.c_str());
    }
    u16 strLogNameWithoutSuffix = strLogName.substr(0, pos);

    ROOT_EXEC((u32)ROOT_COMMAND_SCRIPT_PACKAGELOG, strLogNameWithoutSuffix, NULL);
    return MP_SUCCESS;
}

#ifndef WIN32
/*---------------------------------------------------------------------------
Function Name: GetUidByUserName
Description  : �����û����ƻ�ȡUID��GID
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 GetUidByUserName(u16 strUserName, u32 &uid, u32 &gid)
{
    struct passwd *user;
    uid = -1;
    gid = -1;

    CThreadAutoLock cLock(&g_GetUidByUName_Mutex);
    setpwent();
    while((user = getpwent()) != 0)
    {
        u16 strUser = u16(user->pw_name);
        if (strUser.compare(strUserName) == 0)
        {
            uid = user->pw_uid;
            gid = user->pw_gid;
            break;
        }
    }
    endpwent();

    if (uid == -1)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Get uid of user(%s) failed.", strUserName.c_str());
        return MP_FAILED;
    }
    else
    {
        COMMLOG(OS_LOG_DEBUG, OS_LOG_DEBUG, "User(%s) info: uid=%d, gid=%d.", strUserName.c_str(), uid, gid);
        return MP_SUCCESS;
    }
}

/*---------------------------------------------------------------------------
Function Name: ChownFile
Description  : �����ļ����û�����Ȩ��
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 ChownFile(u16 strFileName, u32 uid, u32 gid)
{
    u32 iRet = MP_SUCCESS;

    iRet = chown(strFileName.c_str(), uid, gid);
    if (0 != iRet)
    {   
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Chmod file(%s) failed, errno[%d]: %s.", BaseFileName(strFileName.c_str()), 
            errno, strerror(errno));
        return MP_FAILED;
    }

    return MP_SUCCESS;
}

#endif


//������ʱʹ�ù̶�����ʵ�֣�������Դ���ѡ�ͺ���ÿ�Դ����Ż�
/*---------------------------------------------------------------------------
Function Name: CheckParamString
Description  : ���string���͵Ĳ���
Input        : paramValue -- �����ַ���
                 lenBeg        -- �ַ�����С����
                 lenEnd        -- �ַ�����󳤶�
                 strInclude   -- ��������ַ���
                 strExclude   --���ܰ����ַ���
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamString(u16 &paramValue, u32 lenBeg, u32 lenEnd, u16 &strInclude, 
    u16 &strExclude)
{
    //CodeDex�󱨣�Dead Code
    if(lenBeg == -1 && lenEnd == -1)
    {
        COMMLOG(OS_LOG_INFO, OS_LOG_INFO, "This string(%s) has no length restrictions.", paramValue.c_str());
    }
    // check length
    else if (paramValue.length() < lenBeg || paramValue.length() > lenEnd)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The len of string(%s) is not between %d and %d.", paramValue.c_str(), lenBeg, lenEnd);
        return MP_FAILED;
    }

    // check include and exclude
    for (u16::iterator iter = paramValue.begin(); iter != paramValue.end(); ++iter)
    {
        // check exclude
        if (strExclude.find(*iter) != string::npos)
        {   
            COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The string(%s) have exclude char %c.", paramValue.c_str(), *iter);
            return MP_FAILED; 
        }

        // check include
        if(!strInclude.empty())
        {
            if (strInclude.find(*iter) == string::npos)
            {   
                COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Char %c is not in include string(%s).", *iter, paramValue.c_str());
                return MP_FAILED; 
            }
        }
    }
    
    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckParamString
Description  : ���string���͵Ĳ���
Input        : paramValue -- �����ַ���
                 lenBeg        -- �ַ�����С����
                 lenEnd        -- �ַ�����󳤶�
                 strPre         -- �ַ���ǰ׺
Return       :
Call           :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamString(u16 &paramValue, u32 lenBeg, u32 lenEnd, u16 &strPre)
{
    if (paramValue.length() < lenBeg || paramValue.length() > lenEnd)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The len of string(%s) is not between %d and %d.", paramValue.c_str(), lenBeg, lenEnd);
        return MP_FAILED;
    }

    size_t idxPre = paramValue.find_first_of(strPre);
    if (idxPre != 0)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "String(%s) is not begin with %s.", paramValue.c_str(), strPre.c_str());
        return MP_FAILED;
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckParamString
Description  : ���string���͵Ĳ���
Input        : paramValue -- �����ַ���
                 lenBeg        -- �ַ�����С����
                 lenEnd        -- �ַ�����󳤶�
                 strPre         -- �ַ�����׺
Return       :
Call           :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamStringEnd(u16 &paramValue, u32 lenBeg, u32 lenEnd, u16 &strEnd)
{
    if (paramValue.length() < lenBeg || paramValue.length() > lenEnd)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The len of string(%s) is not between %d and %d.", paramValue.c_str(), lenBeg, lenEnd);
        return MP_FAILED;
    }

    size_t idxEnd = paramValue.rfind(strEnd);
    if (idxEnd != (paramValue.length() - strEnd.length()))
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "String(%s) is not end with %s.", paramValue.c_str(), strEnd.c_str());
        return MP_FAILED;
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckParamInteger32
Description  : ���int32���͵Ĳ���
Input        : paramValue       -- ��������
                 begValue            -- ������Сֵ
                 endValue            -- �������ֵ
                 vecExclude         -- ���ֲ��ܰ���ֵ
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamInteger32(u32 paramValue, u32 begValue, u32 endValue, vector<u32> &vecExclude)
{
    if (begValue != -1 && paramValue < begValue)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d < %d.", paramValue, begValue);
        return MP_FAILED;
    }
    
    if (endValue != -1 && paramValue > endValue)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d > %d.", paramValue, endValue);
        return MP_FAILED;
    }

    vector<u32>::iterator iter = vecExclude.begin();
    for (; iter != vecExclude.end(); ++iter)
    {
        if (paramValue == *iter)
        {
            COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d = %d.", paramValue, *iter);
            return MP_FAILED;
        }
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckParamInteger64
Description  : ���int64���͵Ĳ���
Input        : paramValue       -- ��������
                 begValue            -- ������Сֵ
                 endValue            -- �������ֵ
                 vecExclude         -- ���ֲ��ܰ���ֵ
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamInteger64(mp_int64 paramValue, mp_int64 begValue, mp_int64 endValue, vector<mp_int64> &vecExclude)
{
    if (begValue != -1 && paramValue < begValue)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d < %d.", paramValue, begValue);
        return MP_FAILED;
    }
    
    if (endValue != -1 && paramValue > endValue)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d > %d.", paramValue, endValue);
        return MP_FAILED;
    }

    vector<mp_int64>::iterator iter = vecExclude.begin();
    for (; iter != vecExclude.end(); ++iter)
    {
        if (paramValue == *iter)
        {
            COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "Check failed, %d = %d.", paramValue, *iter);
            return MP_FAILED;
        }
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckParamInteger
Description  : �������Ƿ���IP��ʽ
Input        : paramValue       -- ����IP�ַ���
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckParamStringIsIP(u16 &paramValue)
{

    u32 iIpnum1, iIpnum2, iIpnum3, iIpnum4;
    u32 iIpnum;
    iIpnum=sscanf_s(paramValue.c_str(),"%d.%d.%d.%d",&iIpnum1,&iIpnum2,&iIpnum3,&iIpnum4); 
    if(iIpnum==4&&(iIpnum1>=1&&iIpnum1<=233)&&(iIpnum2>=0&&iIpnum2<=255)&&(iIpnum3>=0&&iIpnum3<=255)&&(iIpnum4>=0&&iIpnum4<=255)) 
    { 
        return MP_SUCCESS;
    }
    else
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The string(%s) is not ip.", paramValue.c_str());
        return MP_FAILED;
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckPathString
Description  : ���·���ַ����Ƿ������󳤶�
Input        : pathValue       -- ·���ַ���
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckPathString(u16 &pathValue)
{
#ifdef WIN32
    u32 imaxpath = MAX_PATH;
#else
    u32 imaxpath = PATH_MAX;
#endif
    if(pathValue.length() >= imaxpath) 
    { 
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The path string(%s) len is not less than %d.", pathValue.c_str(), imaxpath);
        return MP_FAILED;
    }

    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckPathString
Description  : ���·���ַ����Ƿ������󳤶�
Input        : pathValue       -- ·���ַ���
               strPre          -- ǰ׺
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckPathString(u16 &pathValue, u16 strPre)
{
#ifdef WIN32
    u32 imaxpath = MAX_PATH;
#else
    u32 imaxpath = PATH_MAX;
#endif
    if(pathValue.length() >= imaxpath) 
    { 
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "The path string(%s) len is not less than %d.", pathValue.c_str(), imaxpath);
        return MP_FAILED;
    }

    size_t idxPre = pathValue.find_first_of(strPre);
    if (idxPre != 0)
    {
        COMMLOG(OS_LOG_ERROR, OS_LOG_ERROR, "String(%s) is not begin with %s.", pathValue.c_str(), strPre.c_str());
        return MP_FAILED;
    }

    u16 strInclude("");
    u16 strExclude("");
    size_t idxSep = pathValue.find_last_of("/");
    if(u16::npos == idxSep)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR,
          "The string %s does not contain / character.", pathValue.c_str());
        return MP_FAILED;
    }
    u16 strFileName = pathValue.substr(idxSep + 1);
    CHECK_FAIL_EX(CheckParamString(strFileName, 1, 254, strInclude, strExclude));
    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckFileSysMountParam
Description  : ����ļ�ϵͳ���ز���
Input        : pathValue       -- ·���ַ���
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckFileSysMountParam(u16 strDeviceName, u32 volumeType, u16 strMountPoint)
{
    //����У��
    u16 strFileName;
    u16 strPre("/\\");
    u32 lenBeg, lenEnd;
    size_t idxSep;
    vector<u32> vecExclude;

#ifdef WIN32
    lenBeg = lenEnd = 49;
#else
    lenBeg = 1;
    lenEnd = PATH_MAX - 1;
#endif    
    CHECK_FAIL_EX(CheckParamString(strDeviceName, lenBeg, lenEnd, strPre));
#ifndef WIN32
    idxSep = strDeviceName.find_last_of("/");
    if(u16::npos == idxSep)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR,
          "The string(%s) does not contain / character.", strDeviceName.c_str());
        return MP_FAILED;
    }
    strFileName = strDeviceName.substr(idxSep + 1);
    u16 strInclude("");
    u16 strExclude("");
    CHECK_FAIL_EX(CheckParamString(strFileName, 1, 254, strInclude, strExclude));
#endif

    CHECK_FAIL_EX(CheckParamInteger32(volumeType, 0, 4, vecExclude));

#ifdef WIN32
    lenBeg = 1;
    lenEnd = 1;
    u16 strInclude("BCDEFGHIJKLMNOPQRSTUVWXYZ");
    u16 strExclude;
    CHECK_FAIL_EX(CheckParamString(strMountPoint, lenBeg, lenEnd, strInclude, strExclude));
#else
    lenBeg = 1;
    lenEnd = PATH_MAX - 1;
    strPre = u16("/");
    CHECK_FAIL_EX(CheckParamString(strMountPoint, lenBeg, lenEnd, strPre));

    idxSep = strMountPoint.find_last_of("/");
    if(u16::npos == idxSep)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR,
          "The string(%s) does not contain / character.", strMountPoint.c_str());
        return MP_FAILED;
    }
    strFileName = strMountPoint.substr(idxSep + 1);
    strInclude = u16("");
    strExclude = u16("");
    if(!strFileName.empty())
    {
        CHECK_FAIL_EX(CheckParamString(strFileName, 1, 254, strInclude, strExclude));
    }
#endif
    return MP_SUCCESS;
}

/*---------------------------------------------------------------------------
Function Name: CheckFileSysFreezeParam
Description  : ����ļ�ϵͳ�������
Input        : pathValue       -- ·���ַ���
Return       :
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
u32 CheckFileSysFreezeParam(u16 strDiskNames)
{
    u32 lenBeg, lenEnd;
#ifdef WIN32
    lenBeg = 1;
    lenEnd = 1;
    u16 strInclude("BCDEFGHIJKLMNOPQRSTUVWXYZ");
    u16 strExclude;
    CHECK_FAIL_EX(CheckParamString(strDiskNames, lenBeg, lenEnd, strInclude, strExclude));
#else
    lenBeg = 1;
    lenEnd = PATH_MAX - 1;
    u16 strPre("/");
    CHECK_FAIL_EX(CheckParamString(strDiskNames, lenBeg, lenEnd, strPre));
#endif

#ifndef WIN32
    size_t idxSep = strDiskNames.find_last_of("/");
    if(u16::npos == idxSep)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR,
          "The string(%s) does not contain / character.", strDiskNames.c_str());
        return MP_FAILED;
    }
    u16 strFileName = strDiskNames.substr(idxSep + 1);
    u16 strInclude("");
    u16 strExclude("");
    CHECK_FAIL_EX(CheckParamString(strFileName, 1, 254, strInclude, strExclude));
#endif
    return MP_SUCCESS;
}


/*---------------------------------------------------------------------------
Function Name: GetOSType
Description  : ��ȡOS���ͣ���Ҫʹ��rootȨ��ִ��
Input        : OS_TYPE_E       -- ����ϵͳ���ͣ���ȡ��ͨ����ǰ�ֶη���
Return       : 
Call         :
Called by    :
Modification :
Others       :-------------------------------------------------------------*/
void GetOSType(u32 &iOSType)
{
    LOGGUARD("");
#ifdef WIN32
    iOSType = HOST_OS_WINDOWS;
    COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is windows.");
#elif defined(AIX)
    iOSType = HOST_OS_AIX;
    COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is AIX.");
#elif defined(HP_UX)
    iOSType = HOST_OS_HP_UX;
    COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is HP-UX.");
#elif defined(SOLARIS)
    iOSType = HOST_OS_SOLARIS;
    COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is Solaris.");
#elif defined(LINUX)
    //�����ж�˳���ܱ仯����Ϊoracle linux��Ҳ����redhat-release�ļ�
    if (MP_TRUE == CMpFile::FileExist("/etc/SuSE-release"))
    {
        iOSType = HOST_OS_SUSE;
        COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is SUSE.");
    }
    else if (MP_TRUE == CMpFile::FileExist("/etc/oracle-release"))
    {
        iOSType = HOST_OS_ORACLE_LINUX;
        COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is oracle linux.");
    }
    else if (MP_TRUE == CMpFile::FileExist("/etc/redhat-release"))
    {
        iOSType = HOST_OS_REDHAT;
        COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is redhat.");
    }
    else
    {
        iOSType = HOST_OS_OTHER_LINUX;
        COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "Host is other linux.");
    }
#else
    iOSType = HOST_OS_UNKNOWN;
    COMMLOG(OS_LOG_WARN, LOG_COMMON_WARN, "Canot not get OS type.");
#endif
}

//��ȡOS�汾��Ϣ
u32 GetOSVersion(u32 iOSType, u16 &strOSVersion)
{
    strOSVersion = "";
#ifdef LINUX
    // ʹ��cat����򵥴���
    u16 strExecCmd;
    vector<u16> vecResult;
    u32 iRet = MP_FAILED;
    
    if (HOST_OS_SUSE == iOSType)
    {
        strExecCmd = "cat /etc/SuSE-release | grep VERSION | awk  -F '=' '{print $2}' | sed 's/^[ \t]*//g'";
    }
    // redhat��oracle linux��ʹ��redhat-release��ȡ�汾��
    else if (HOST_OS_ORACLE_LINUX == iOSType || HOST_OS_REDHAT == iOSType)
    {
        strExecCmd = "cat /etc/redhat-release | awk -F '.' '{print $1}' | awk '{print $NF}'";
    }
    else
    {
        COMMLOG(OS_LOG_WARN, LOG_COMMON_WARN, "OS type is %d, not support to get os version.", iOSType);
        return MP_SUCCESS;
    }

    COMMLOG(OS_LOG_DEBUG, LOG_COMMON_DEBUG, "cmd '%s' will be excute.", strExecCmd.c_str());
    iRet = CSystemExec::ExecSystemWithEcho(strExecCmd, vecResult);
    if (MP_SUCCESS != iRet)
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR, "Excute getting os version failed, iRet %d.", iRet);
        return iRet;
    }

    // ִ��cat����ִ�н����ȡ�汾��
    if (vecResult.size() > 0)
    {
        strOSVersion = vecResult.front();
    }
    else
    {
        COMMLOG(OS_LOG_ERROR, LOG_COMMON_ERROR, "The result of getting os version is empty.");
        return MP_FAILED;
    }

    return MP_SUCCESS;
#else
    COMMLOG(OS_LOG_WARN, LOG_COMMON_WARN, "OS type is %d, not support to get os version.", iOSType);
    return MP_SUCCESS;
#endif
}



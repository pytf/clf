/*
�ṩ��־��¼�����ý������¼����������̹���������ȵ�
*/

#include "lf_main.h"

static void showHelpInfo();
static int coreModInit();
static int get_options(int argc, char *const *argv);

static unint       g_show_help;
static unint       g_show_version;
static unint       g_show_configure;

static unint       g_callBackCounter[MOD_ID_BUIT];

static MOD_TABLE_S g_modTable[] =
{
    {MOD_ID_CONF, "confModule", _ModInitConf},
    {MOD_ID_TASK, "taskModule", _ModInitTask},
    {MOD_ID_MSG, "msgModule", _ModInitMsg},
};

static MODULE_CB_TABLE_S g_modCbTable[] =
{
    {MOD_ID_TASK,     "task",     60,  _TASK_Shedule},/* ��Ϣģ������ں˲麯�� */
    {MOD_ID_MSG,     "msg",     60,  _MSG_Trig},
};

//1.ʱ��ȳ�ʼ��
//2.���������в���
//3.����Ϊ��̨����
//4.���벢��������
//5.����ģ���ʼ��
//6.����ģ���ʼ��
int main(int argc, char *const *argv)
{
    int ret = RET_OK;
    char *pBuf = "set daemon failed.";
    
    if (get_options(argc, argv) != RET_OK)
    {
        return RET_ERR;
    }

    if(g_show_help)
    {
        showHelpInfo();
    }

    ret = lf_log_init();
    if(RET_OK != ret)
    {
        return RET_ERR;
    }

    ret = lf_daemon();
    if(RET_OK != ret)
    {
        write(STDERR_FILENO, pBuf, strlen(pBuf));
    }

    time_init();

    ret = coreModInit();
    if(RET_OK != ret)
    {
        lf_log_std(LOG_LEVEL_ERR, "Init modules failed.");
        return RET_ERR;
    }

    //funcCallBack();

    _ModInitMonitor();
    BUG();

    return RET_OK;
}

static void showHelpInfo()
{
    char buf[] = 
        "Usage: nginx [-?hvVtTq] [-s signal] [-c filename] "
                             "[-p prefix] [-g directives]" LINEFEED
                             LINEFEED
                "Options:" LINEFEED
                "  -?,-h         : this help" LINEFEED
                "  -v            : show version and exit" LINEFEED
                "  -V            : show version and configure options then exit" //除了version外还可以显示操作系统和configure阶段等相关信息
                LINEFEED LINEFEED;
    write(STDERR_FILENO, buf, strlen(buf));
}

static int coreModInit()
{
    int  i;
    int num;
    MOD_TABLE_S *ptModule;

    for (i = 0; num; i++) {
        ptModule = &(g_modTable[i]);
        if (NULL != ptModule->pfunc)
        {
            if (RET_OK != ptModule->pfunc())
            {
                lf_log_std(LOG_LEVEL_ERR, "Module [%s] start failed", ptModule->acModName);
                return RET_ERR;
            }
        }

        lf_log_std(LOG_LEVEL_DEBUG, "Module [%s] start success", ptModule->acModName);
    }

    return RET_OK;
}

static int get_options(int argc, char *const *argv)
{
    unchar     *p;
    int   i = 0;
    char pBuf[1024] = {0};

    for (i = 1; i < argc; i++) {

        p = (unchar *) argv[i];

        if (*p++ != '-') {
            memset(pBuf, 0, sizeof(pBuf));
            snprintf_s(pBuf, sizeof(pBuf), sizeof(pBuf) - 1, "invalid option: \"%s\"", argv[i]);
            write(STDERR_FILENO, pBuf, strlen(pBuf));
            return RET_ERR;
        }

        while (*p) {

            switch (*p++) {

            case '?':
            case 'h':
                g_show_version = 1;
                g_show_help = 1;
                break;

            case 'v':
                g_show_version = 1;
                break;

            case 'V':
                g_show_version = 1;
                g_show_configure = 1;
                break;

            default:
                memset(pBuf, 0, sizeof(pBuf));
                snprintf_s(pBuf, sizeof(pBuf), sizeof(pBuf) - 1, "invalid option: \"%c\"", *(p - 1));
                write(STDERR_FILENO, pBuf, strlen(pBuf));
                return RET_ERR;
            }
        }
    }

    return RET_OK;
}

void funcCallBack()
{
    MODULE_CB_TABLE_S *pCb;
    unint             num, i;

    num = sizeof(g_modCbTable)/sizeof(MODULE_CB_TABLE_S);

    for(;;)
    {
        for (i = 0; i < num; i++)
        {
            pCb = &g_modCbTable[i];

            if (!pCb->pCBFunc || !pCb->time_out)
            {
                continue;
            }

            g_callBackCounter[i]++;
            if (g_callBackCounter[i] == pCb->time_out)
            {
                g_callBackCounter[i] = 0;
                pCb->pCBFunc();
                sleep(1);
            }
        }

        sleep(1);
    }
}


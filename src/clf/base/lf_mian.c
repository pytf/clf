#include "lf_main.h"

static void showHelpInfo();
static int coreModInit();
static int coreModInit();
static int get_options(int argc, char *const *argv);

static unint       g_show_help;
static unint       g_show_version;
static unint       g_show_configure;

static unint       g_callBackCounter[MOD_ID_BUIT];

static MOD_TABLE_S g_modTable[] =
{
    {MOD_ID_LOG, "logModule", _ModInitLog},
    {MOD_ID_CONF, "confModule", _ModInitConf},
    {MOD_ID_TASK, "taskModule", _ModInitTask},
};

static MODULE_CB_TABLE_S g_modCbTable[] =
{
    {MOD_ID_TASK,     "task",     60,  _TASK_Shedule},          /* 消息模块的周期核查函数 */
    {MOD_ID_MSG,     "msg",     60,  _MSG_Trig},                /* 打印消息管理模块的统计值 */
};

//1.时间等初始化
//2.读入命令行参数
//3.设置为后台进程
//4.读入并解析配置
//5.核心模块初始化
//6.所有模块初始化
int main(int argc, char *const *argv)
{
    int ret = RET_OK;
    char *pBuf = "set daemon failed.";
    
    if (get_options(argc, argv) != RET_OK) { //解析命令参数
        return RET_ERR;
    }

    if(g_show_help)
    {
        showHelpInfo();
    }

    ret = lf_daemon();
    if(RET_OK != ret)
    {
        write(STDERR_FILENO, pBuf, strlen(pBuf));
    }

    time_init(); //初始化环境的当前时间

    ret = coreModInit();
    if(RET_OK != ret)
    {
        lf_log_std(LOG_LEVEL_ERR, "Init modules failed.");
        return RET_ERR;
    }

    /*创建周期性任务执行*/
    //funcCallBack();

    /*启动进程监控*/
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
    int   i;

    for (i = 1; i < argc; i++) {

        p = (unchar *) argv[i];

        if (*p++ != '-') {
            lf_log_std(LOG_LEVEL_ERR, "invalid option: \"%s\"", argv[i]);
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
                lf_log_std(0, "invalid option: \"%c\"", *(p - 1));
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
                sleep(1);               /* 防止产生CPU峰值, 1秒只调用1个回调 */
            }
        }

        sleep(1);
    }
}


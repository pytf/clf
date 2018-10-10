#include "lf_main.h"

static MOD_TABLE_S g_modTable[] =
{
        {MOD_ID_LOG, "logModule", _ModInitLog},
        {MOD_ID_CONF, "confModule", _ModInitConf},
        {MOD_ID_TASK, "taskModule", _ModInitTask}
};

void showHelpInfo()
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

int coreModInit()
{
    int i;
    char *pName;
    int num = sizeof(g_modTable)/sizeof(MOD_TABLE_S);

    for(i = 0; i < num; i++)
    {
        pName = g_modTable[i].mod_name;

        if(NULL != g_modTable[i].pFunc)
        {
            if(RET_OK != g_modTable[i].pFunc())
            {
                lf_log_std(LOG_LEVEL_ERR, "Init module [%s] failed.", pName);
                return RET_ERR;
            }
        }

        lf_log_std(LOG_LEVEL_DEBUG, "Init module [%s] success.", pName);
    }

    return RET_OK;
}

//1.时间等初始化
//2.读入命令行参数
//3.设置为后台进程
//4.读入并解析配置
//5.核心模块初始化
//6.所有模块初始化
int main(int argc, char *argv[])
{
    int ret = RET_OK;
    char *pBuf = "set daemon failed.";
    
    if (get_options(argc, argv) != RET_OK) { //解析命令参数
        return RET_ERR;
    }

    ret = lf_daemon();
    if(RET_OK != ret)
    {
        write(STDERR_FILENO, pBuf, strlen(pBuf));
    }

    time_init(); //初始化环境的当前时间

    coreModInit();

    return RET_OK;
}

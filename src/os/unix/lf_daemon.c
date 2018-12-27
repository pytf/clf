
/*
 * 后台进程创建，将程序置于后台运行
 * daemon函数亦可设置守护进程，具体参考man手册
 * 1、创建子进程，关闭父进程
 * 2、setsid创建新会话
 * 3、设置umask
 * 4、重定向标准输入输出出错到/dev/null
 */


#include <lf_config.h>
#include <lf_core.h>


int lf_daemon()
{
    int  fd;

    switch (fork()) {
    case -1:
        lf_log_std(LOG_LEVEL_ERR, "fork() failed");
        return RET_ERR;
    //子进程
    case 0:
        break;
    //父进程
    default:
        exit(0);
    }
    //setsid创建新会话，脱离控制终端和进程组
    if (setsid() == -1) {
        lf_log_std(LOG_LEVEL_ERR, "setsid() failed");
        return RET_ERR;
    }
    //设置进程的文件权限掩码为0
    umask(0);
    
    fd = open("/dev/null", O_RDWR);
    if (fd == -1) {
        lf_log_std(LOG_LEVEL_ERR, "open(\"/dev/null\") failed");
        return RET_ERR;
    }
    //重定向标准输入到/dev/null
    if (dup2(fd, STDIN_FILENO) == -1) {
        lf_log_std(LOG_LEVEL_ERR, "dup2(STDIN) failed");
        return RET_ERR;
    }
    //重定向标准输出到/dev/null
    if (dup2(fd, STDOUT_FILENO) == -1) {
        lf_log_std(LOG_LEVEL_ERR, "dup2(STDOUT) failed");
        return RET_ERR;
    }

#if 0
    //重定向标准出错到/dev/null
    if (dup2(fd, STDERR_FILENO) == -1) {
        lf_log_std(LOG_LEVEL_ERR, "dup2(STDERR) failed");
        return RET_ERR;
    }
#endif

    if (fd > STDERR_FILENO) {
        if (close(fd) == -1) {
            lf_log_std(LOG_LEVEL_ERR, "close() failed");
            return RET_ERR;
        }
    }

    return RET_OK;
}


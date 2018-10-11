#ifndef __LF_MAIN_H__
#define __LF_MAIN_H__

#define mod_name_len 20
#define LINEFEED             "\x0a"
/* ´¥·¢½ø³Ì±ÀÀ£, ²úÉúcoredump raiseÏò½ø³Ì±¾Éí·¢ËÍÐÅºÅ*/
#define BUG()  \
    do { \
        raise(SIGILL); \
        exit(-1);      \
    } while (0)

typedef enum
{
    MOD_ID_MAIN = 0,
    MOD_ID_LOG  = 1,
    MOD_ID_CONF  = 2,
    MOD_ID_TASK  = 3,
    MOD_ID_MSG  = 4,
    MOD_ID_MONITOR = 5,
    MOD_ID_BUIT = 16
};

typedef struct MOD_TABLE
{
    int mod_id;
    char mod_name[mod_name_len];
    void *pFunc;
}MOD_TABLE_S;

typedef struct MOD_CB_TABLE
{
    int mod_id;
    char mod_name[mod_name_len];
    int time_out;
    void *pCBFunc;
}MODULE_CB_TABLE_S;

#endif /*__LF_MAIN_H__*/


#ifndef __LF_MAIN_H__
#define __LF_MAIN_H__

#define mod_name_len 20
#define LINEFEED             "\x0a"

typedef enum
{
    MOD_ID_MAIN = 0,
    MOD_ID_LOG,
    MOD_ID_CONF,
    MOD_ID_TASK,
    MOD_ID_MSG,
    MOD_ID_BUIT
};

typedef struct MOD_TABLE
{
    int mod_id;
    char mod_name[mod_name_len];
    void *pFunc;
}MOD_TABLE_S;

#endif

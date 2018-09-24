#ifndef _DEFINE_H_
#define _DEFINE_H_

//一些公用符号定义
#define NODE_COLON                        ":"            //字符串中的冒号
#define NODE_SEMICOLON                    ";"            //字符串中的分号

//分隔符
#define STR_COMMA                      ","            //字符串中的逗号
#define STR_SEMICOLON                  ";"            //字符串中的分号，Windows下已此为分隔符，避免与路径中的冒号冲突
#define STR_COLON                      ":"            //字符串中的冒号
#define STR_DASH                       "-"
#define STR_PLUS                       "+"
#define STR_VERTICAL_LINE              "|"
#define STR_DOUBLE_VERTICAL_LINE       "||"
#define STR_ADDRESS                    "&"
#define STR_DOUBLE_ADDRESS             "&&"
#define STR_SPACE                     " "
#define CHAR_COMMA                     ','
#define CHAR_SEMICOLON                 ';'
#define CHAR_COLON                     ':'
#define CHAR_VERTICAL_LINE             '|'
#define CHAR_SLASH                     '/'

#define CHECK_FAIL( Call )                                                                        \
{                                                                                                 \
    u32 iCheckFailRet = Call;                                                                \
    if (RET_ERR == iCheckFailRet)                                                               \
    {                                                                                             \
        lf_log_std(LOG_LEVEL_ERR, "Call %s failed, ret %d.", #Call, iCheckFailRet); \
        return RET_ERR;                                                                         \
    }                                                                                             \
}

#define CHECK_NOT_OK( Call )                                                                      \
{                                                                                                 \
    u32 iCheckNotOkRet = Call;                                                               \
    if (RET_OK != iCheckNotOkRet)                                                                    \
    {                                                                                             \
        lf_log_std(LOG_LEVEL_ERR, "Call %s failed, ret %d.", #Call, iCheckNotOkRet);\
        return RET_ERR;                                                                         \
    }                                                                                             \
}

#define CHECK_FAIL_NOLOG( Call )                                                                  \
{                                                                                                 \
    u32 iCheckFailRet = Call;                                                                \
    if (RET_ERR == iCheckFailRet)                                                               \
    {                                                                                             \
        return RET_ERR;                                                                         \
    }                                                                                             \
}

//将调用返回码返回
#define CHECK_FAIL_EX( Call )                                                                     \
{                                                                                                 \
    u32 iCheckNotOkRet = Call;                                                               \
    if (RET_OK != iCheckNotOkRet)                                                             \
    {                                                                                             \
        lf_log_std(LOG_LEVEL_ERR, "Call %s failed, ret %d.", #Call, iCheckNotOkRet);\
        return iCheckNotOkRet;                                                                    \
    }                                                                                             \
}

#endif

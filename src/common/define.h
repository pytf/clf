#ifndef _DEFINE_H_
#define _DEFINE_H_

//һЩ���÷��Ŷ���
#define NODE_COLON                        ":"            //�ַ����е�ð��
#define NODE_SEMICOLON                    ";"            //�ַ����еķֺ�

//�ָ���
#define STR_COMMA                      ","            //�ַ����еĶ���
#define STR_SEMICOLON                  ";"            //�ַ����еķֺţ�Windows���Ѵ�Ϊ�ָ�����������·���е�ð�ų�ͻ
#define STR_COLON                      ":"            //�ַ����е�ð��
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

//�����÷����뷵��
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

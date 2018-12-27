#ifndef _CONFIG_FILE_H_
#define _CONFIG_FILE_H_

#include "../common/type.h"

<<<<<<< HEAD
struct lf_modules{
=======
struct lf_core_modules{
    u32 cModuleID;
    s16 name;
    u32 (*initModule)();
}

#endif


/*
 * 
 * 
 */


#ifndef _lf_DLOPEN_H_INCLUDED_
#define _lf_DLOPEN_H_INCLUDED_


#include <lf_config.h>
#include <lf_core.h>


#define lf_dlopen(path)           dlopen((char *) path, RTLD_NOW | RTLD_GLOBAL)
#define lf_dlopen_n               "dlopen()"

#define lf_dlsym(handle, symbol)  dlsym(handle, symbol)
#define lf_dlsym_n                "dlsym()"

#define lf_dlclose(handle)        dlclose(handle)
#define lf_dlclose_n              "dlclose()"


#if (lf_HAVE_DLOPEN)
char *lf_dlerror(void);
#endif


#endif /* _lf_DLOPEN_H_INCLUDED_ */

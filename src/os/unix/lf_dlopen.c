
/*
 * 
 * 
 */


#include <lf_config.h>
#include <lf_core.h>


#if (lf_HAVE_DLOPEN)

char *
lf_dlerror(void)
{
    char  *err;

    err = (char *) dlerror();

    if (err == NULL) {
        return "";
    }

    return err;
}

#endif

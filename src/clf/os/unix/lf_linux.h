
/*
 * Copyright (C) Igor Sysoev
 * Copyright (C) Nginx, Inc.
 */


#ifndef _lf_LINUX_H_INCLUDED_
#define _lf_LINUX_H_INCLUDED_


lf_chain_t *lf_linux_sendfile_chain(lf_connection_t *c, lf_chain_t *in,
    off_t limit);


#endif /* _lf_LINUX_H_INCLUDED_ */

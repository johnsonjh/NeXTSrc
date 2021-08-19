#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = 	"@(#)rpccommondata.c	1.2 88/07/27 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

#include <rpc/rpc.h>
/*
 * This file should only contain common data (global data) that is exported
 * by public interfaces 
 */
/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  It
 * is padded so that new data can take the place of storage occupied by part
 * of it.
 */
struct opaque_auth _null_auth = { 0 };
#ifdef FD_SETSIZE
fd_set svc_fdset = { 0 };
#else
int svc_fds = 0;
#endif /* def FD_SETSIZE */
struct rpc_createerr rpc_createerr = { 0 };

/* global data padding, must NOT be static */
char _rpc_comdata_padding[196] = { 0 };

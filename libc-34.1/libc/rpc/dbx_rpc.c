#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = 	"@(#)dbx_rpc.c	1.2 88/07/27 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *  1.7 88/02/08 
 */


/*
 * This file is optionally brought in by including a
 * "psuedo-device dbx" line in the config file.  It is
 * compiled using the "-g" flag to generate structure
 * information which is used by dbx with the -k flag.
 */

#include <sys/param.h>
#include <sys/socket.h>
#include <sys/socketvar.h>
#include <netinet/in.h>

#include <rpc/types.h>
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/clnt.h>
#include <rpc/rpc_msg.h>
#include <rpc/svc.h>
#include <rpc/svc_auth.h> 
#include <rpc/auth_unix.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>

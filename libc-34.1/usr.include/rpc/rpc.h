/*	@(#)rpc.h	1.2 88/04/26 4.0NFSSRC SMI	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 *	@(#)rpc.h 1.9 88/02/08 SMI	  
 */


/*
 * rpc.h, Just includes the billions of rpc header files necessary to 
 * do remote procedure calling.
 */
#ifndef __RPC_HEADER__
#define __RPC_HEADER__

#ifdef KERNEL
#include "../rpc/types.h"	/* some typedefs */
#include "../netinet/in.h"

/* external data representation interfaces */
#include "../rpc/xdr.h"		/* generic (de)serializer */

/* Client side only authentication */
#include "../rpc/auth.h"	/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include "../rpc/clnt.h"	/* generic rpc stuff */

/* semi-private protocol headers */
#include "../rpc/rpc_msg.h"	/* protocol for rpc messages */
#include "../rpc/auth_unix.h"	/* protocol for unix style cred */
#include "../rpc/auth_des.h"	/* protocol for des style cred */

/* Server side only remote procedure callee */
#include "../rpc/svc.h"		/* service manager and multiplexer */
#include "../rpc/svc_auth.h"	/* service side authenticator */

#else

#include <rpc/types.h>		/* some typedefs */
#include <netinet/in.h>

/* external data representation interfaces */
#include <rpc/xdr.h>		/* generic (de)serializer */

/* Client side only authentication */
#include <rpc/auth.h>		/* generic authenticator (client side) */

/* Client side (mostly) remote procedure call */
#include <rpc/clnt.h>		/* generic rpc stuff */

/* semi-private protocol headers */
#include <rpc/rpc_msg.h>	/* protocol for rpc messages */
#include <rpc/auth_unix.h>	/* protocol for unix style cred */
#include <rpc/auth_des.h>	/* protocol for des style cred */

/* Server side only remote procedure callee */
#include <rpc/svc.h>		/* service manager and multiplexer */
#include <rpc/svc_auth.h>	/* service side authenticator */
#endif KERNEL

#endif /* ndef __RPC_HEADER__ */

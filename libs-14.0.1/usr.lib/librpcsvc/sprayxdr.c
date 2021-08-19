#ifndef lint
static char sccsid[] = 	"@(#)sprayxdr.c	1.3 88/05/08 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.7
 */

#include <rpc/rpc.h>
#ifdef NeXT_MOD
#include <sys/time.h>
#endif NeXT_MOD
#include <rpcsvc/spray.h>

xdr_sprayarr(xdrsp, arrp)
	XDR *xdrsp;
	struct sprayarr *arrp;
{
	if (!xdr_bytes(xdrsp, (char **) &arrp->data, &arrp->lnth, SPRAYMAX))
		return(0);
	return(1);
}

xdr_spraycumul(xdrsp, cumulp)
	XDR *xdrsp;
	struct spraycumul *cumulp;
{
	if (!xdr_u_int(xdrsp, &cumulp->counter))
		return(0);
	if (!xdr_timeval(xdrsp, &cumulp->clock))
		return(0);
	return(1);
}

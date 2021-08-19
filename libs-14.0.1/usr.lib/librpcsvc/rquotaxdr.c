#ifndef lint
static char sccsid[] = 	"@(#)rquotaxdr.c	1.2 88/05/08 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.7
 */

#include <stdio.h>
#include <rpc/rpc.h>
#ifdef NeXT_MOD
#include <ufs/quotas.h>
#else
#include <ufs/quota.h>
#endif NeXT_MOD
#include <rpcsvc/rquota.h>


bool_t
xdr_getquota_args(xdrs, gq_argsp)
	XDR *xdrs;
	struct getquota_args *gq_argsp;
{
	extern bool_t xdr_path();

	return (xdr_path(xdrs, &gq_argsp->gqa_pathp) &&
	    xdr_int(xdrs, &gq_argsp->gqa_uid));
}

struct xdr_discrim gqr_arms[2] = {
	{ (int)Q_OK, xdr_rquota },
	{ __dontcare__, NULL }
};

bool_t
xdr_getquota_rslt(xdrs, gq_rsltp)
	XDR *xdrs;
	struct getquota_rslt *gq_rsltp;
{

	return (xdr_union(xdrs,
	    (int *) &gq_rsltp->gqr_status, (char *) &gq_rsltp->gqr_rquota,
	    gqr_arms, (xdrproc_t) xdr_void));
}

bool_t
xdr_rquota(xdrs, rqp)
	XDR *xdrs;
	struct rquota *rqp;
{

	return (xdr_int(xdrs, &rqp->rq_bsize) &&
	    xdr_bool(xdrs, &rqp->rq_active) &&
	    xdr_u_long(xdrs, &rqp->rq_bhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_bsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curblocks) &&
	    xdr_u_long(xdrs, &rqp->rq_fhardlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_fsoftlimit) &&
	    xdr_u_long(xdrs, &rqp->rq_curfiles) &&
	    xdr_u_long(xdrs, &rqp->rq_btimeleft) &&
	    xdr_u_long(xdrs, &rqp->rq_ftimeleft) );
}

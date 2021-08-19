/* @(#)nlm_prot.c	1.3 87/06/23 3.2/4.3NFSSRC */
/*
 * nlm_prot.c
 * generated from rpcgen -c nlm_prot.x
 */

#include <rpc/rpc.h>
#include <rpcsvc/nlm_prot.h>


bool_t
xdr_nlm_stats(xdrs,objp)
	XDR *xdrs;
	nlm_stats *objp;
{
	if (! xdr_enum(xdrs, (enum_t *) objp)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_holder(xdrs,objp)
	XDR *xdrs;
	nlm_holder *objp;
{
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->svid)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->oh)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_testrply(xdrs,objp)
	XDR *xdrs;
	nlm_testrply *objp;
{
	static struct xdr_discrim choices[] = {
		{ (int) nlm_granted, xdr_void },
		{ (int) nlm_denied, xdr_nlm_holder },
		{ (int) nlm_denied_nolocks, xdr_void },
		{ (int) nlm_blocked, xdr_void },
		{ (int) nlm_denied_grace_period, xdr_void },
		{ __dontcare__, NULL }
	};

	if (! xdr_union(xdrs, (enum_t *) &objp->stat, (char *) &objp->nlm_testrply, choices, NULL)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_stat(xdrs,objp)
	XDR *xdrs;
	nlm_stat *objp;
{
	if (! xdr_nlm_stats(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_res(xdrs,objp)
	XDR *xdrs;
	nlm_res *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_stat(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_testres(xdrs,objp)
	XDR *xdrs;
	nlm_testres *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_testrply(xdrs, &objp->stat)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_lock(xdrs,objp)
	XDR *xdrs;
	nlm_lock *objp;
{
	if (! xdr_string(xdrs, &objp->caller_name, LM_MAXSTRLEN)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->fh)) {
		return(FALSE);
	}
	if (! xdr_netobj(xdrs, &objp->oh)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->svid)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_offset)) {
		return(FALSE);
	}
	if (! xdr_u_int(xdrs, &objp->l_len)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_lockargs(xdrs,objp)
	XDR *xdrs;
	nlm_lockargs *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->reclaim)) {
		return(FALSE);
	}
	if (! xdr_int(xdrs, &objp->state)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_cancargs(xdrs,objp)
	XDR *xdrs;
	nlm_cancargs *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->block)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_testargs(xdrs,objp)
	XDR *xdrs;
	nlm_testargs *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_bool(xdrs, &objp->exclusive)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}




bool_t
xdr_nlm_unlockargs(xdrs,objp)
	XDR *xdrs;
	nlm_unlockargs *objp;
{
	if (! xdr_netobj(xdrs, &objp->cookie)) {
		return(FALSE);
	}
	if (! xdr_nlm_lock(xdrs, &objp->lock)) {
		return(FALSE);
	}
	return(TRUE);
}



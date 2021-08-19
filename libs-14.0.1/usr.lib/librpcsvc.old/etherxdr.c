/* @(#)etherxdr.c	1.4 87/08/13 3.2/4.3NFSSRC */
#ifndef lint
static  char sccsid[] = "@(#)etherxdr.c 1.1 86/09/25 Copyright 1985, 1987 Sun Microsystems, Inc.";
#endif
/*
 *
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 *
 */

/*
 * Copyright (c) 1985, 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/time.h>
#include <rpcsvc/ether.h>

xdr_etherstat(xdrsp, ep)
	XDR *xdrsp;
	struct etherstat *ep;
{
	int i;
	
	if (!xdr_timeval(xdrsp, &ep->e_time))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bytes))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_packets))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bcast))
		return (0);
	for (i = 0; i < NBUCKETS; i++)
		if (!xdr_u_long(xdrsp, &ep->e_size[i]))
			return (0);
	for (i = 0; i < NPROTOS; i++)
		if (!xdr_u_long(xdrsp, &ep->e_proto[i]))
			return (0);
}


xdr_etheraddrs(xdrsp, ep)
	register XDR *xdrsp;
	register struct etheraddrs *ep;
{
	if (!xdr_timeval(xdrsp, &ep->e_time))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bytes))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_packets))
		return (0);
	if (!xdr_u_long(xdrsp, &ep->e_bcast))
		return (0);
	if (!xdr_reference(xdrsp, &ep->e_addrs,
	    HASHSIZE*sizeof(struct etherhmem *), xdr_etherhtable))
		return (0);
}

xdr_etherhtable(xdrs, hp)
	register XDR *xdrs;
	register struct etherhmem **hp;
{
	int i;
	
	for (i = 0; i < HASHSIZE; i++)
		if (!xdr_etherhmem(xdrs, &hp[i]))
			return (0);
}

xdr_etherhmem(xdrs, hp)
	register XDR *xdrs;
	register struct etherhmem **hp;
{
	/*
	 * more_elements is pre-computed in case the direction is
	 * XDR_ENCODE or XDR_FREE.  more_elements is overwritten by
	 * xdr_bool when the direction is XDR_DECODE.
	 */
	int more_elements;
	register int freeing = (xdrs->x_op == XDR_FREE);
	register struct etherhmem **nxt;

	for (;;) {
		more_elements = (*hp != NULL);
		if (! xdr_bool(xdrs, &more_elements))
			return (FALSE);
		if (! more_elements)
			return (TRUE);  /* we are done */
		/*
		 * the unfortunate side effect of non-recursion is that in
		 * the case of freeing we must remember the nxt object
		 * before we free the current object ...
		 */
		if (freeing)
			nxt = &((*hp)->h_nxt); 
		if (! xdr_reference(xdrs, hp, sizeof(struct etherhmem),
		    xdr_etherhbody))
			return (FALSE);
		hp = (freeing) ? nxt : &((*hp)->h_nxt);
	}
}

/* 
 * body of an etherhmem
 */
bool_t
xdr_etherhbody(xdrs, hp)
	XDR *xdrs;
	struct etherhmem *hp;
{
	if (!xdr_int(xdrs, &hp->h_addr))
		return FALSE;
	if (!xdr_u_long(xdrs, &hp->h_cnt))
		return FALSE;
	return(TRUE);
}

xdr_addrmask(xdrs, ap)
	XDR *xdrs;
	struct addrmask *ap;
{
	if (xdr_int(xdrs, &ap->a_addr) == 0)
		return 0;
	if (xdr_int(xdrs, &ap->a_mask) == 0)
		return 0;
	return (1);
}

/* 
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 */

/* @(#)rstatxdr.c	1.6 87/08/13 3.2/4.3NFSSRC */
#ifndef lint
static  char sccsid[] = "@(#)rstatxdr.c 1.1 86/09/25 Copyright 1984, 1987 Sun Microsystems, Inc.";
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

#include <rpc/rpc.h>
#include <rpcsvc/rstat.h>

#ifdef NeXT_MOD
#undef DK_NDRIVE
#define DK_NDRIVE 4
#endif NeXT_MOD

rstat(host, statp)
	char *host;
	struct statstime *statp;
{
	return (callrpc(host, RSTATPROG, RSTATVERS_TIME, RSTATPROC_STATS,
	    xdr_void, 0, xdr_statstime, statp));
}

havedisk(host)
	char *host;
{
	long have;
	
	if (callrpc(host, RSTATPROG, RSTATVERS_SWTCH, RSTATPROC_HAVEDISK,
	    xdr_void, 0, xdr_long,  &have) != 0)
		return (-1);
	else
		return (have);
}

xdr_stats(xdrs, statp)
	XDR *xdrs;
	struct stats *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	return (1);
}

xdr_statsswtch(xdrs, statp)		/* version 2 */
	XDR *xdrs;
	struct statsswtch *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	return (1);
}

xdr_statstime(xdrs, statp)		/* version 3 */
	XDR *xdrs;
	struct statstime *statp;
{
	int i;
	
	for (i = 0; i < CPUSTATES; i++)
		if (xdr_int(xdrs, &statp->cp_time[i]) == 0)
			return (0);
	for (i = 0; i < DK_NDRIVE; i++)
		if (xdr_int(xdrs, &statp->dk_xfer[i]) == 0)
			return (0);
	if (xdr_int(xdrs, &statp->v_pgpgin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pgpgout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpin) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_pswpout) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_intr) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ipackets) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_ierrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_oerrors) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->if_collisions) == 0)
		return (0);
	if (xdr_int(xdrs, &statp->v_swtch) == 0)
		return (0);
	for (i = 0; i < 3; i++)
		if (xdr_long(xdrs, &statp->avenrun[i]) == 0)
			return (0);
	if (xdr_timeval(xdrs, &statp->boottime) == 0)
		return (0);
	if (xdr_timeval(xdrs, &statp->curtime) == 0)
		return (0);
	/* 
	 * Many implementations of this protocol incorrectly left out
	 * if_opackets.  This value is included here for compatibility 
	 * with these buggy implementations.
	 */
	(void) xdr_int (xdrs, &statp->if_opackets);
	return (1);
}

xdr_timeval(xdrs, tvp)
	XDR *xdrs;
	struct timeval *tvp;
{
	if (xdr_long(xdrs, &tvp->tv_sec) == 0)
		return (0);
	if (xdr_long(xdrs, &tvp->tv_usec) == 0)
		return (0);
	return (1);
}

/* 
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 */

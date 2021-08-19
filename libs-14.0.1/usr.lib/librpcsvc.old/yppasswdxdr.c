/* @(#)yppasswdxdr.c	1.4 87/08/13 3.2/4.3NFSSRC */
#ifndef lint
static  char sccsid[] = "@(#)yppasswdxdr.c 1.1 86/09/25 Copyright 1985, 1987 Sun Microsystems, Inc.";
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
#include <pwd.h>
#include <rpcsvc/yppasswd.h>

yppasswd(oldpass, newpw)
	char *oldpass;
	struct passwd *newpw;
{
	int port, ok, ans;
	char domain[256];
	char *master;
	struct yppasswd yppasswd;
	
	yppasswd.oldpass = oldpass;
	yppasswd.newpw = *newpw;
	if (getdomainname(domain, sizeof(domain)) < 0)
		return(-1);
	if (yp_master(domain, "passwd.byname", &master) != 0)
		return (-1);
	port = getrpcport(master, YPPASSWDPROG, YPPASSWDPROC_UPDATE,
		IPPROTO_UDP);
	if (port == 0) {
		free(master);
		return (-1);
	}
	if (port >= IPPORT_RESERVED) {
		free(master);
		return (-1);
	}
	ans = callrpc(master, YPPASSWDPROG, YPPASSWDVERS,
	    YPPASSWDPROC_UPDATE, xdr_yppasswd, &yppasswd, xdr_int, &ok);
	free(master);
	if (ans != 0 || ok != 0)
		return (-1);
	else
		return (0);
}

xdr_yppasswd(xdrsp, pp)
	XDR *xdrsp;
	struct yppasswd *pp;
{
	if (xdr_wrapstring(xdrsp, &pp->oldpass) == 0)
		return (0);
	if (xdr_passwd(xdrsp, &pp->newpw) == 0)
		return (0);
	return (1);
}

xdr_passwd(xdrsp, pw)
	XDR *xdrsp;
	struct passwd *pw;
{
	if (xdr_wrapstring(xdrsp, &pw->pw_name) == 0)
		return (0);
	if (xdr_wrapstring(xdrsp, &pw->pw_passwd) == 0)
		return (0);
	if (xdr_int(xdrsp, &pw->pw_uid) == 0)
		return (0);
	if (xdr_int(xdrsp, &pw->pw_gid) == 0)
		return (0);
	if (xdr_wrapstring(xdrsp, &pw->pw_gecos) == 0)
		return (0);
	if (xdr_wrapstring(xdrsp, &pw->pw_dir) == 0)
		return (0);
	if (xdr_wrapstring(xdrsp, &pw->pw_shell) == 0)
		return (0);
}











/* 
 * NFSSRC 3.2/4.3 for the VAX*
 * Copyright (C) 1987 Sun Microsystems, Inc.
 * 
 * (*)VAX is a trademark of Digital Equipment Corporation
 */

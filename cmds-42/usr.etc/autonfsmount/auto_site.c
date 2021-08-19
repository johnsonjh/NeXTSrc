#ifndef lint
static char sccsid[] = 	"@(#)auto_site.c	1.2 88/05/10 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>

extern char mydomain[];

getsite(addr, site)
	struct in_addr addr;
	char *site;
{
	static int lastnet = 0;
	static char lastsite[64];
	int net;
	register u_long iaddr;
	register char *caddr;
	struct netent *n;
	char *val;
	int vallen;
	int reason;

	net = inet_netof(addr);
	if (net == lastnet && lastnet) {
		strcpy(site, lastsite);
		return;
	}
	n = getnetbyaddr(net, AF_INET);
	if (n == NULL) {
		iaddr = ntohl(addr.s_addr);
		caddr = (char *)&addr;
		if (IN_CLASSC(iaddr)) 
			sprintf(site, "%d.%d.%d", caddr[0], caddr[1], caddr[2]);
		else if (IN_CLASSB(iaddr))
			sprintf(site, "%d.%d", caddr[0], caddr[1]);
		else
			sprintf(site, "%d", caddr[0]);
	} else
		strcpy(site, n->n_name);
	reason = yp_match(mydomain, "site.bynet", site, strlen(site), &val,
			&vallen);
	if (reason) { 	/* site has net name or number */
		strcpy(lastsite, site);
		lastnet = net;
		return;
	}
	strncpy(lastsite, val, vallen);
	lastsite[vallen] = '\0';
	free(val);
	lastnet = net;
	strcpy(site, lastsite);
}

getoptsbysites(from, to, opts)
	char *from, *to, *opts;
{
	char key[140];
	int keylen;
	char *val;
	char *p, *last;
	int vallen;
	int reason;

	sprintf(key, "%s:%s", from, to);
	keylen = strlen(key);
	reason = yp_match(mydomain, "mountopts.bysitepair",
	    key, keylen, &val, &vallen);
	if (reason == 0)
		goto gotit;
        else if (reason != YPERR_KEY)
                goto nodata;

	sprintf(key, "%s:%s", to, from);
	keylen = strlen(key);
	if (yp_match(mydomain, "mountopts.bysitepair",
	    key, keylen, &val, &vallen) == 0)
		goto gotit;

	sprintf(key, "?:%s", to);
	keylen = strlen(key);
	if (yp_match(mydomain, "mountopts.bysitepair",
	    key, keylen, &val, &vallen) == 0)
		goto gotit;

	sprintf(key, "%s:?", from);
	keylen = strlen(key);
	if (yp_match(mydomain, "mountopts.bysitepair",
	    key, keylen, &val, &vallen) == 0)
		goto gotit;

	sprintf(key, "?:?");
	keylen = strlen(key);
	if (yp_match(mydomain, "mountopts.bysitepair",
	    key, keylen, &val, &vallen) == 0)
		goto gotit;

nodata:	/* no data */
	opts[0] = '\0';
	return;

gotit:
	last = val + vallen;
	for (p = val; p < last; p++)
		if (!isspace(*p) && *p != '-')
			break;
	strncpy(opts, p, last - p);
	opts[last - p] = '\0';
	free(val);
}


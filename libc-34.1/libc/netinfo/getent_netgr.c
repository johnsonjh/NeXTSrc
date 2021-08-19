/*
 * Netgroup lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include "lu_utils.h"

#define FIX(x) ((x == NULL) ? NULL : &(x))

extern int _old_innetgr();
extern int _old_getnetgrent();
extern void _old_setnetgrent();
extern void _old_endnetgrent();

static lookup_state netgroup_state = LOOKUP_CACHE;
static char *netgroup_data;
static unsigned netgroup_datalen;
static int netgroup_nentries;
static XDR netgroup_xdr;

static int
lu_innetgr(
	   char *group,
	   char *host,
	   char *user,
	   char *domain
	   )
{
	unsigned datalen;
	XDR xdr;
	char namebuf[4*_LU_MAXLUSTRLEN + 3*BYTES_PER_XDR_UNIT];
	static int proc = -1;
	int size;
	int res;
	_lu_innetgr_args args;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "innetgr", &proc) != KERN_SUCCESS) {
			return (0);
		}
	}
	args.group = group;
	args.host = FIX(host);
	args.user = FIX(user);
	args.domain = FIX(domain);
	xdrmem_create(&xdr, namebuf, sizeof(namebuf), XDR_ENCODE);
	if (!xdr__lu_innetgr_args(&xdr, &args)) {
		return (0);
	}
	size = xdr_getpos(&xdr) / BYTES_PER_XDR_UNIT;
	xdr_destroy(&xdr);

	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)namebuf, size, lookup_buf, 
			&datalen) != KERN_SUCCESS) {
		return (0);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	if (!xdr_int(&xdr, &res)) {
		return (0);
	}
	return (res);
}

static void
lu_endnetgrent()
{
	if (netgroup_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)netgroup_data, 
			      netgroup_datalen);
		netgroup_data = NULL;
	}
}

static void
lu_setnetgrent(char *group)
{
	static int proc = -1;

	if (netgroup_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)netgroup_data, 
			      netgroup_datalen);
		netgroup_data = NULL;
	}
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getnetgrent", &proc) !=
		    KERN_SUCCESS) {
			return;
		}
	}
	if (_lookup_all(_lu_port, proc, NULL, 0, &netgroup_data, 
			&netgroup_datalen) != KERN_SUCCESS) {
		netgroup_data = NULL;
		return;
	}
	xdrmem_create(&netgroup_xdr, netgroup_data, 
		      netgroup_datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	if (!xdr_int(&netgroup_xdr, &netgroup_nentries)) {
		lu_endnetgrent();
	}
}


int
lu_getnetgrent(
	       char **machine,
	       char **user,
	       char **domain
	       )
{
	static _lu_netgrent ng;

	if (netgroup_nentries == 0) {
		return (0);
	}
	xdr_free(xdr__lu_netgrent, &ng);
	bzero(&ng, sizeof(ng));
	if (!xdr__lu_netgrent(&netgroup_xdr, &ng)) {
		return (0);
	}
	netgroup_nentries--;
	*machine = ng.ng_host;
	*user = ng.ng_user;
	*domain = ng.ng_domain;
	return (1);
}

int 
innetgr(
	char *group,
	char *host,
	char *user,
	char *domain
	)
{
	if (_lu_running()) {
		return (lu_innetgr(group, host, user, domain));
	} else {
		return (_old_innetgr(group, host, user, domain));
	}
}

int
getnetgrent(
	    char **host,
	    char **user,
	    char **domain
	    )
{
	if (_lu_running() && netgroup_state == LOOKUP_CACHE) {
		return (lu_getnetgrent(host, user, domain));
	} else {
		return (_old_getnetgrent(host, user, domain));
	}

}


void
setnetgrent(
	    char *group
	    )
{
	if (_lu_running()) {
		lu_setnetgrent(group);
		netgroup_state = LOOKUP_CACHE;
	} else {
		_old_setnetgrent(group);
		netgroup_state = LOOKUP_FILE;
	}
}


void
endnetgrent(
	    void
	    )
{
	lu_endnetgrent();
	_old_endnetgrent();
	netgroup_state = LOOKUP_FILE;
}


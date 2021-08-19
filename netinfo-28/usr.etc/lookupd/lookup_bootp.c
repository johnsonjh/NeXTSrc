/*
 * bootp lookup implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <sys/message.h>
#include <sys/socket.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include "_lu_types.h"
#include "lookup.h"
#include "copy_mem.h"
#include "clib.h"
#include "clib_internal.h"

#define NOTNULL(x) ((x) == NULL ? "" : (x))

static _lu_bootp_ent *
fixbootp(
	 struct ether_addr *enaddr,
	 char *name,
	 struct in_addr *ipaddr,
	 char *bootfile
	 )
{
	static _lu_bootp_ent bootp;

	bootp.bootp_name = NOTNULL(name);
	bootp.bootp_bootfile = NOTNULL(bootfile);
	bootp.bootp_ipaddr = ipaddr->s_addr;
	bcopy(enaddr, &bootp.bootp_enaddr, sizeof(*enaddr));
	return (&bootp);
}

int 
lookup_bootp_getbyether(
			int inlen,
			char *indata,
			int *outlen,
			char **outdata
			)
{
	XDR inxdr;
	XDR outxdr;
	int stat = 0;
	struct ether_addr enaddr;
	struct in_addr ipaddr;
	char *name;
	char *bootfile;
	_lu_bootp_ent_ptr bootp;
	
	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	name = NULL;
	if (!xdr_opaque(&inxdr, &enaddr, sizeof(enaddr))) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	if (bootp_getbyether(&enaddr, &name, &ipaddr, &bootfile)) {
		bootp = fixbootp(&enaddr, name, &ipaddr, bootfile);
	} else {
		bootp = NULL;
	}
	if (xdr__lu_bootp_ent_ptr(&outxdr, &bootp)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_bootp_getbyip(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		     )
{
	XDR inxdr;
	XDR outxdr;
	int stat = 0;
	struct ether_addr enaddr;
	struct in_addr ipaddr;
	char *name;
	char *bootfile;
	_lu_bootp_ent_ptr bootp;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	name = NULL;
	if (!xdr_u_long(&inxdr, &ipaddr.s_addr)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	if (bootp_getbyip(&enaddr, &name, &ipaddr, &bootfile)) {
		bootp = fixbootp(&enaddr, name, &ipaddr, bootfile);
	} else {
		bootp = NULL;
	}
	if (xdr__lu_bootp_ent_ptr(&outxdr, &bootp)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

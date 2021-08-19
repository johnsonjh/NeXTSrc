/*
 * Bootp lookup - netinfo only
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include "lu_utils.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netinet/if_ether.h>

static int 
lu_bootp_getbyether(
		   struct ether_addr *enaddr,
		   char **name,
		   struct in_addr *ipaddr,
		   char **bootfile
		   )
{
	unsigned datalen;
	XDR xdr;
	static _lu_bootp_ent_ptr bp;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "bootp_getbyether",
				 &proc) != KERN_SUCCESS) {
			return (0);
		}
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)enaddr, 
			((sizeof(*enaddr) + sizeof(unit) - 1) / sizeof(unit)), 
			lookup_buf, 
			&datalen) != KERN_SUCCESS) {
		return (0);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	xdr_free(xdr__lu_bootp_ent_ptr, &bp);
	if (!xdr__lu_bootp_ent_ptr(&xdr, &bp) || bp == NULL) {
		return (0);
	}
	*name = bp->bootp_name;
	*bootfile = bp->bootp_bootfile;
	ipaddr->s_addr = bp->bootp_ipaddr;
	return (1);
}

static int 
lu_bootp_getbyip(
		 struct ether_addr *enaddr,
		 char **name,
		 struct in_addr *ipaddr,
		 char **bootfile
		 )
{
	unsigned datalen;
	XDR xdr;
	static _lu_bootp_ent_ptr bp;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "bootp_getbyip",
				 &proc) != KERN_SUCCESS) {
			return (0);
		}
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)ipaddr, 
			((sizeof(*ipaddr) + sizeof(unit) - 1) / sizeof(unit)),
			lookup_buf, 
			&datalen) != KERN_SUCCESS) {
		return (0);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	xdr_free(xdr__lu_bootp_ent_ptr, &bp);
	if (!xdr__lu_bootp_ent_ptr(&xdr, &bp) || bp == NULL) {
		return (0);
	}
	*name = bp->bootp_name;
	*bootfile = bp->bootp_bootfile;
	bcopy(bp->bootp_enaddr, enaddr, sizeof(*enaddr));
	return (1);
}


int
bootp_getbyether(
	      struct ether_addr *enaddr,
	      char **name,
	      struct in_addr *ipaddr,
	      char **bootfile
	      )
{
	if (_lu_running()) {
		return (lu_bootp_getbyether(enaddr, name, ipaddr, bootfile));
	} else {
		return (0);
	}
}

int
bootp_getbyip(
	      struct ether_addr *enaddr,
	      char **name,
	      struct in_addr *ipaddr,
	      char **bootfile
	      )
{
	if (_lu_running()) {
		return (lu_bootp_getbyip(enaddr, name, ipaddr, bootfile));
	} else {
		return (0);
	}
}


/*
 * network lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <netdb.h>
#include "lu_utils.h"
#include <sys/socket.h>

extern struct netent *_old_getnetbyaddr();
extern struct netent *_old_getnetbyname();
extern struct netent *_old_getnetent();
extern void _old_setnetent();
extern void _old_endnetent();

static lookup_state n_state = LOOKUP_CACHE;
static struct netent global_n = { 0 };
static char *n_data;
static unsigned n_datalen;
static int n_nentries;
static XDR n_xdr;

static void
convert_n(
	  _lu_netent *lu_n
	  )
{
	char **aliases;
	int i;

	if (global_n.n_name != NULL) {
		free(global_n.n_name);
	}
	aliases = global_n.n_aliases;
	if (aliases != NULL) {
		while (*aliases != NULL) {
			free(*aliases++);
		}
		free(global_n.n_aliases);
	}
	global_n.n_name = lu_n->n_names.n_names_val[0];
	global_n.n_aliases = lu_n->n_names.n_names_val;
	for (i = 0; i < lu_n->n_names.n_names_len - 1; i++) {
		global_n.n_aliases[i] = global_n.n_aliases[i + 1];
	}
	global_n.n_aliases[lu_n->n_names.n_names_len - 1] = 0;
	global_n.n_addrtype = AF_INET;
	global_n.n_net = lu_n->n_net;
	bzero(lu_n, sizeof(*lu_n));
}



static struct netent *
lu_getnetbyaddr(
		long addr,
		int type
		)
{
	unsigned datalen;
	_lu_netent_ptr lu_n;
	XDR xdr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (type != AF_INET) {
		return (NULL);
	}
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getnetbyaddr", &proc) !=
		    KERN_SUCCESS) {
			return (NULL);
		}
	}
	addr = htonl(addr);
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)&addr, 1, lookup_buf, 
			&datalen) !=
	    KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_n = NULL;
	if (!xdr__lu_netent_ptr(&xdr, &lu_n) || lu_n == NULL) {
		return (NULL);
	}
	convert_n(lu_n);
	xdr_free(xdr__lu_netent_ptr, &lu_n);
	return (&global_n);
}

static struct netent *
lu_getnetbyname(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_netent_ptr lu_n;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getnetbyname", &proc) != 
		    KERN_SUCCESS) {
			return (NULL);
		}
	}
	xdrmem_create(&outxdr, namebuf, sizeof(namebuf), XDR_ENCODE);
	if (!xdr__lu_string(&outxdr, &name)) {
		return (NULL);
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)namebuf,
			xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT, 
			lookup_buf, &datalen) != KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&inxdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_n = NULL;
	if (!xdr__lu_netent_ptr(&inxdr, &lu_n) || lu_n == NULL) {
		return (NULL);
	}
	convert_n(lu_n);
	xdr_free(xdr__lu_netent_ptr, &lu_n);
	return (&global_n);
}

static void
lu_setnetent()
{
	if (n_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)n_data, n_datalen);
		n_data = NULL;
	}
}

static void
lu_endnetent()
{
	if (n_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)n_data, n_datalen);
		n_data = NULL;
	}
}

static struct netent *
lu_getnetent()
{
	static int proc = -1;
	_lu_netent lu_n;

	if (n_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getnetent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &n_data, 
				&n_datalen) != KERN_SUCCESS) {
			n_data = NULL;
			return (NULL);
		}
		xdrmem_create(&n_xdr, n_data, 
			      n_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&n_xdr, &n_nentries)) {
			lu_endnetent();
			return (NULL);
		}
	}
	if (n_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_n, sizeof(lu_n));
	if (!xdr__lu_netent(&n_xdr, &lu_n)) {
		return (NULL);
	}
	n_nentries--;
	convert_n(&lu_n);
	xdr_free(xdr__lu_netent, &lu_n);
	return (&global_n);
}

struct netent *
getnetbyaddr(
	     long addr,
	     int type
	     )
{
	LOOKUP2(lu_getnetbyaddr, _old_getnetbyaddr, addr, type, struct netent);
}

struct netent *
getnetbyname(
	      char *name
	      )
{
	LOOKUP1(lu_getnetbyname, _old_getnetbyname,  name, struct netent);
}

struct netent *
getnetent(
	   void
	   )
{
	GETENT(lu_getnetent, _old_getnetent, &n_state, struct netent);
}


void
setnetent(
	  int stayopen
	  )
{
	SETSTATE(lu_setnetent, _old_setnetent, &n_state, stayopen);
}


void
endnetent(
	   void
	   )
{
	UNSETSTATE(lu_endnetent, _old_endnetent, &n_state);
}

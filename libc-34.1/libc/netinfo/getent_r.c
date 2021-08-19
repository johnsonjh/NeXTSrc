/*
 * RPC lookup
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

extern struct rpcent *_old_getrpcbynumber();
extern struct rpcent *_old_getrpcbyname();
extern struct rpcent *_old_getrpcent();
extern void _old_setrpcent();
extern void _old_endrpcent();
extern void _old_setrpcfile();

static lookup_state r_state = LOOKUP_CACHE;
static struct rpcent global_r = { 0 };
static char *r_data;
static unsigned r_datalen;
static int r_nentries;
static XDR r_xdr;

static void
convert_r(
	  _lu_rpcent *lu_r
	  )
{
	char **aliases;
	int i;

	if (global_r.r_name != NULL) {
		free(global_r.r_name);
	}
	aliases = global_r.r_aliases;
	if (aliases != NULL) {
		while (*aliases != NULL) {
			free(*aliases++);
		}
		free(global_r.r_aliases);
	}
	global_r.r_name = lu_r->r_names.r_names_val[0];
	global_r.r_aliases = lu_r->r_names.r_names_val;
	for (i = 0; i < lu_r->r_names.r_names_len - 1; i++) {
		global_r.r_aliases[i] = global_r.r_aliases[i + 1];
	}
	global_r.r_aliases[lu_r->r_names.r_names_len - 1] = 0;
	global_r.r_number = lu_r->r_number;
	bzero(lu_r, sizeof(*lu_r));
}



static struct rpcent *
lu_getrpcbynumber(long number)
{
	unsigned datalen;
	_lu_rpcent_ptr lu_r;
	XDR xdr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getrpcbynumber", &proc) != 
		    KERN_SUCCESS) {
			return (NULL);
		}
	}
	number = htonl(number);
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)&number, 1, lookup_buf, 
			&datalen) !=
	    KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_r = NULL;
	if (!xdr__lu_rpcent_ptr(&xdr, &lu_r) || lu_r == NULL) {
		return (NULL);
	}
	convert_r(lu_r);
	xdr_free(xdr__lu_rpcent_ptr, &lu_r);
	return (&global_r);
}

static struct rpcent *
lu_getrpcbyname(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_rpcent_ptr lu_r;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getrpcbyname", &proc) !=
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
	lu_r = NULL;
	if (!xdr__lu_rpcent_ptr(&inxdr, &lu_r) || lu_r == NULL) {
		return (NULL);
	}
	convert_r(lu_r);
	xdr_free(xdr__lu_rpcent_ptr, &lu_r);
	return (&global_r);
}

static void
lu_setrpcent()
{
	if (r_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)r_data, r_datalen);
		r_data = NULL;
	}
}

static void
lu_endrpcent()
{
	if (r_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)r_data, r_datalen);
		r_data = NULL;
	}
}

static struct rpcent *
lu_getrpcent()
{
	static int proc = -1;
	_lu_rpcent lu_r;

	if (r_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getrpcent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &r_data, 
				&r_datalen) != KERN_SUCCESS) {
			r_data = NULL;
			return (NULL);
		}
		xdrmem_create(&r_xdr, r_data, 
			      r_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&r_xdr, &r_nentries)) {
			lu_endrpcent();
			return (NULL);
		}
	}
	if (r_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_r, sizeof(lu_r));
	if (!xdr__lu_rpcent(&r_xdr, &lu_r)) {
		return (NULL);
	}
	r_nentries--;
	convert_r(&lu_r);
	xdr_free(xdr__lu_rpcent, &lu_r);
	return (&global_r);
}

struct rpcent *
getrpcbynumber(
	       long number
	       )
{
	LOOKUP1(lu_getrpcbynumber, _old_getrpcbynumber, number, struct rpcent);
}

struct rpcent *
getrpcbyname(
	      char *name
	      )
{
	LOOKUP1(lu_getrpcbyname, _old_getrpcbyname,  name, struct rpcent);
}

struct rpcent *
getrpcent(
	   void
	   )
{
	GETENT(lu_getrpcent, _old_getrpcent, &r_state, struct rpcent);
}


void
setrpcent(
	  int stayopen
	  )
{
	SETSTATE(lu_setrpcent, _old_setrpcent, &r_state, stayopen);
}


void
endrpcent(
	   void
	   )
{
	UNSETSTATE(lu_endrpcent, _old_endrpcent, &r_state);
}



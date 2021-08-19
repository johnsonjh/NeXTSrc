/*
 * Protocol lookup
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

extern struct protoent *_old_getprotobynumber();
extern struct protoent *_old_getprotobyname();
extern struct protoent *_old_getprotoent();
extern void _old_setprotoent();
extern void _old_endprotoent();

static lookup_state p_state = LOOKUP_CACHE;
static struct protoent global_p = { 0 };
static char *p_data;
static unsigned p_datalen;
static int p_nentries;
static XDR p_xdr;

static void
convert_p(
	  _lu_protoent *lu_p
	  )
{
	char **aliases;
	int i;

	if (global_p.p_name != NULL) {
		free(global_p.p_name);
	}
	aliases = global_p.p_aliases;
	if (aliases != NULL) {
		while (*aliases != NULL) {
			free(*aliases++);
		}
		free(global_p.p_aliases);
	}
	global_p.p_name = lu_p->p_names.p_names_val[0];
	global_p.p_aliases = lu_p->p_names.p_names_val;
	for (i = 0; i < lu_p->p_names.p_names_len - 1; i++) {
		global_p.p_aliases[i] = global_p.p_aliases[i + 1];
	}
	global_p.p_aliases[lu_p->p_names.p_names_len - 1] = 0;
	global_p.p_proto = lu_p->p_proto;
	bzero(lu_p, sizeof(*lu_p));
}



static struct protoent *
lu_getprotobynumber(long number)
{
	unsigned datalen;
	_lu_protoent_ptr lu_p;
	XDR xdr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getprotobynumber", &proc) !=
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
	lu_p = NULL;
	if (!xdr__lu_protoent_ptr(&xdr, &lu_p) || lu_p == NULL) {
		return (NULL);
	}
	convert_p(lu_p);
	xdr_free(xdr__lu_protoent_ptr, &lu_p);
	return (&global_p);
}

static struct protoent *
lu_getprotobyname(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_protoent_ptr lu_p;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getprotobyname", &proc) !=
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
	lu_p = NULL;
	if (!xdr__lu_protoent_ptr(&inxdr, &lu_p) || lu_p == NULL) {
		return (NULL);
	}
	convert_p(lu_p);
	xdr_free(xdr__lu_protoent_ptr, &lu_p);
	return (&global_p);
}

static void
lu_setprotoent()
{
	if (p_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)p_data, p_datalen);
		p_data = NULL;
	}
}

static void
lu_endprotoent()
{
	if (p_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)p_data, p_datalen);
		p_data = NULL;
	}
}

static struct protoent *
lu_getprotoent()
{
	static int proc = -1;
	_lu_protoent lu_p;

	if (p_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getprotoent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &p_data, 
				&p_datalen) != KERN_SUCCESS) {
			p_data = NULL;
			return (NULL);
		}
		xdrmem_create(&p_xdr, p_data, 
			      p_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&p_xdr, &p_nentries)) {
			lu_endprotoent();
			return (NULL);
		}
	}
	if (p_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_p, sizeof(lu_p));
	if (!xdr__lu_protoent(&p_xdr, &lu_p)) {
		return (NULL);
	}
	p_nentries--;
	convert_p(&lu_p);
	xdr_free(xdr__lu_protoent, &lu_p);
	return (&global_p);
}

struct protoent *
getprotobynumber(
		 int number
		 )
{
	LOOKUP1(lu_getprotobynumber, _old_getprotobynumber,  number, struct protoent);
}

struct protoent *
getprotobyname(
	      char *name
	      )
{
	LOOKUP1(lu_getprotobyname, _old_getprotobyname,  name, struct protoent);
}

struct protoent *
getprotoent(
	   void
	   )
{
	GETENT(lu_getprotoent, _old_getprotoent, &p_state, struct protoent);
}


void
setprotoent(
	    int stayopen
	    )
{
	SETSTATE(lu_setprotoent, _old_setprotoent, &p_state, stayopen);
}


void
endprotoent(
	   void
	   )
{
	UNSETSTATE(lu_endprotoent, _old_endprotoent, &p_state);
}



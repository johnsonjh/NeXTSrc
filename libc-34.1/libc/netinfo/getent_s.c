/*
 * Services file lookup
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

extern struct servent *_old_getservbyport();
extern struct servent *_old_getservbyname();
extern struct servent *_old_getservent();
extern void _old_setservent();
extern void _old_endservent();
extern void _old_setservfile();

static lookup_state s_state = LOOKUP_CACHE;
static struct servent global_s = { 0 };
static char *s_data;
static unsigned s_datalen;
static int s_nentries;
static XDR s_xdr;

static void
convert_s(
	  _lu_servent *lu_s
	  )
{
	char **aliases;
	int i;

	if (global_s.s_name != NULL) {
		free(global_s.s_name);
	}
	aliases = global_s.s_aliases;
	if (aliases != NULL) {
		while (*aliases != NULL) {
			free(*aliases++);
		}
		free(global_s.s_aliases);
	}
	global_s.s_name = lu_s->s_names.s_names_val[0];
	global_s.s_aliases = lu_s->s_names.s_names_val;
	for (i = 0; i < lu_s->s_names.s_names_len - 1; i++) {
		global_s.s_aliases[i] = global_s.s_aliases[i + 1];
	}
	global_s.s_aliases[lu_s->s_names.s_names_len - 1] = 0;
	global_s.s_proto = lu_s->s_proto;
	global_s.s_port = lu_s->s_port;
	bzero(lu_s, sizeof(*lu_s));
}



static struct servent *
lu_getservbyport(
		 int port,
		 char *proto
		 )
{
	unsigned datalen;
	_lu_servent_ptr lu_s;
	XDR xdr;
	static int proc = -1;
	char output_buf[_LU_MAXLUSTRLEN + 3 * BYTES_PER_XDR_UNIT];
	unit lookup_buf[MAX_INLINE_UNITS];
	XDR outxdr;

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getservbyport", &proc) !=
		    KERN_SUCCESS) {
			return (NULL);
		}
	}
	xdrmem_create(&outxdr, output_buf, sizeof(output_buf), XDR_ENCODE);
	if (!xdr_int(&outxdr, &port) ||
	    !xdr__lu_string(&outxdr, &proto)) {
		return (NULL);
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)output_buf, 
		       xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT, 
		       lookup_buf, &datalen) != KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_s = NULL;
	if (!xdr__lu_servent_ptr(&xdr, &lu_s) || lu_s == NULL) {
		return (NULL);
	}
	convert_s(lu_s);
	xdr_free(xdr__lu_servent_ptr, &lu_s);
	return (&global_s);
}

static struct servent *
lu_getservbyname(
		 char *name,
		 char *proto
		 )
{
	unsigned datalen;
	unit lookup_buf[MAX_INLINE_UNITS];
	char output_buf[2 * (_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT)];
	XDR outxdr;
	XDR inxdr;
	_lu_servent_ptr lu_s;
	static int proc = -1;

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getservbyname", &proc) != 
		    KERN_SUCCESS) {
		    return (NULL);
		}
	}
	xdrmem_create(&outxdr, output_buf, sizeof(output_buf), XDR_ENCODE);
	if (!xdr__lu_string(&outxdr, &name) ||
	    !xdr__lu_string(&outxdr, &proto)) {
		return (NULL);
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)output_buf,
			xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT, 
			lookup_buf, &datalen) != KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&inxdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_s = NULL;
	if (!xdr__lu_servent_ptr(&inxdr, &lu_s) || lu_s == NULL) {
		return (NULL);
	}
	convert_s(lu_s);
	xdr_free(xdr__lu_servent_ptr, &lu_s);
	return (&global_s);
}

static void
lu_setservent()
{
	if (s_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)s_data, s_datalen);
		s_data = NULL;
	}
}

static void
lu_endservent()
{
	if (s_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)s_data, s_datalen);
		s_data = NULL;
	}
}

static struct servent *
lu_getservent()
{
	static int proc = -1;
	_lu_servent lu_s;

	if (s_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getservent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &s_data, 
				&s_datalen) != KERN_SUCCESS) {
			s_data = NULL;
			return (NULL);
		}
		xdrmem_create(&s_xdr, s_data, 
			      s_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&s_xdr, &s_nentries)) {
			lu_endservent();
			return (NULL);
		}
	}
	if (s_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_s, sizeof(lu_s));
	if (!xdr__lu_servent(&s_xdr, &lu_s)) {
		return (NULL);
	}
	s_nentries--;
	convert_s(&lu_s);
	xdr_free(xdr__lu_servent, &lu_s);
	return (&global_s);
}

struct servent *
getservbyport(
	      int port,
	      char *proto
	      )
{
	LOOKUP2(lu_getservbyport, _old_getservbyport, port, proto, struct servent);
}

struct servent *
getservbyname(
	      char *name,
	      char *proto
	      )
{
	LOOKUP2(lu_getservbyname, _old_getservbyname,  name, proto, struct servent);
}

struct servent *
getservent(
	   void
	   )
{
	GETENT(lu_getservent, _old_getservent, &s_state, struct servent);
}


void
setservent(
	   int stayopen
	   )
{
	SETSTATE(lu_setservent, _old_setservent, &s_state, stayopen);
}


void
endservent(
	   void
	   )
{
	UNSETSTATE(lu_endservent, _old_endservent, &s_state);
}



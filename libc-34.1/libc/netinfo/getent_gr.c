/*
 * Unix group lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <grp.h>
#include "lu_utils.h"

extern struct group *_old_getgrgid();
extern struct group *_old_getgrnam();
extern struct group *_old_getgrent();
extern void _old_setgrent();
extern void _old_endgrent();
extern void _old_setgrfile();

static lookup_state gr_state = LOOKUP_CACHE;
static struct group global_gr = { 0 };
static char *gr_data;
static unsigned gr_datalen;
static int gr_nentries;
static XDR gr_xdr;

static void
convert_gr(
	   _lu_group *lu_gr
	   )
{
	char **mem;

	if (global_gr.gr_name != NULL) {
		free(global_gr.gr_name);
	}
	if (global_gr.gr_passwd != NULL) {
		free(global_gr.gr_passwd);
	}
	mem = global_gr.gr_mem;
	if (mem != NULL) {
		while (*mem != NULL) {
			free(*mem++);
		}
		free(global_gr.gr_mem);
	}
	global_gr.gr_name = lu_gr->gr_name;
	global_gr.gr_passwd = lu_gr->gr_passwd;
	global_gr.gr_gid = lu_gr->gr_gid;
	global_gr.gr_mem = (char **)realloc(lu_gr->gr_mem.gr_mem_val,
					    ((lu_gr->gr_mem.gr_mem_len + 1) * 
					     sizeof(char *)));
	global_gr.gr_mem[lu_gr->gr_mem.gr_mem_len] = NULL;

	bzero(lu_gr, sizeof(*lu_gr));
}


static struct group *
lu_getgrgid(int gid)
{
	unsigned datalen;
	_lu_group_ptr lu_gr;
	XDR xdr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getgrgid", &proc) != KERN_SUCCESS) {
			return (NULL);
		}
	}
	gid = htonl(gid);
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)&gid, 1, lookup_buf, 
			&datalen) != KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_gr = NULL;
	if (!xdr__lu_group_ptr(&xdr, &lu_gr) || lu_gr == NULL) {
		return (NULL);
	}
	convert_gr(lu_gr);
	xdr_free(xdr__lu_group_ptr, &lu_gr);
	return (&global_gr);
}

static struct group *
lu_getgrnam(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_group_ptr lu_gr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getgrnam", &proc) != KERN_SUCCESS) {
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
	lu_gr = NULL;
	if (!xdr__lu_group_ptr(&inxdr, &lu_gr) || lu_gr == NULL) {
		return (NULL);
	}
	convert_gr(lu_gr);
	xdr_free(xdr__lu_group_ptr, &lu_gr);
	return (&global_gr);
}

static void
lu_setgrent()
{
	if (gr_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)gr_data, gr_datalen);
		gr_data = NULL;
	}
}

static void
lu_endgrent()
{
	if (gr_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)gr_data, gr_datalen);
		gr_data = NULL;
	}
}

static struct group *
lu_getgrent()
{
	static int proc = -1;
	_lu_group lu_gr;

	if (gr_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getgrent", &proc) != 
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &gr_data, 
			       &gr_datalen) != KERN_SUCCESS) {
			gr_data = NULL;
			return (NULL);
		}
		xdrmem_create(&gr_xdr, gr_data, 
			      gr_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&gr_xdr, &gr_nentries)) {
			lu_endgrent();
			return (NULL);
		}
	}
	if (gr_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_gr, sizeof(lu_gr));
	if (!xdr__lu_group(&gr_xdr, &lu_gr)) {
		return (NULL);
	}
	gr_nentries--;
	convert_gr(&lu_gr);
	xdr_free(xdr__lu_group, &lu_gr);
	return (&global_gr);
}

struct group *
getgrgid(
	 int gid
	 )
{
	LOOKUP1(lu_getgrgid, _old_getgrgid,  gid, struct group);
}

struct group *
getgrnam(
	 char *name
	 )
{
	LOOKUP1(lu_getgrnam, _old_getgrnam,  name, struct group);
}

struct group *
getgrent(
	 void
	 )
{
	GETENT(lu_getgrent, _old_getgrent, &gr_state, struct group);
}


void
setgrent(
	 void
	 )
{
	SETSTATE(lu_setgrent, _old_setgrent, &gr_state, 0);
}


void
endgrent(
	 void
	 )
{
	UNSETSTATE(lu_endgrent, _old_endgrent, &gr_state);
}

void
setgrfile(
	  char *fname
	  )
{
	/*
	 * This makes no sense with non-file lookup
	 */
	_old_setgrfile(fname);
}


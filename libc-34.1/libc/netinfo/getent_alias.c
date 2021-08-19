/*
 * Alias lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <aliasdb.h>
#include "lu_utils.h"

extern struct aliasent *_old_alias_getbyname();
extern struct aliasent *_old_alias_getent();
extern void _old_alias_setent();
extern void _old_alias_endent();

static lookup_state alias_state = LOOKUP_CACHE;
static struct aliasent global_aliasent;
static char *alias_data;
static unsigned alias_datalen;
static int alias_nentries;
static XDR alias_xdr;


static void 
freeold(void)
{
	int i;

	if (global_aliasent.alias_name != NULL) {
		free(global_aliasent.alias_name);
		global_aliasent.alias_name = NULL;
	}
	if (global_aliasent.alias_members != NULL) {
		for (i = 0; i < global_aliasent.alias_members_len; i++) {
			free(global_aliasent.alias_members[i]);
		}
	}
	global_aliasent.alias_members = NULL;
}

static void
convert_aliasent(
		 _lu_aliasent *lu_aliasent
		 )
{
	freeold();
	global_aliasent.alias_name = lu_aliasent->alias_name;
	(global_aliasent.alias_members_len = 
	 lu_aliasent->alias_members.alias_members_len);
	(global_aliasent.alias_members = 
	 lu_aliasent->alias_members.alias_members_val);
	global_aliasent.alias_local = lu_aliasent->alias_local;
	bzero(lu_aliasent, sizeof(*lu_aliasent));
}


static struct aliasent *
lu_alias_getbyname(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	unit lookup_buf[MAX_INLINE_UNITS];
	XDR outxdr;
	XDR inxdr;
	_lu_aliasent_ptr lu_aliasent;
	static int proc = -1;

	if (proc < 0) {
		if (_lookup_link(_lu_port, "alias_getbyname", 
				 &proc) != KERN_SUCCESS) {
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
	lu_aliasent = NULL;
	if (!xdr__lu_aliasent_ptr(&inxdr, &lu_aliasent) || 
	    lu_aliasent == NULL) {
		return (NULL);
	}
	convert_aliasent(lu_aliasent);
	xdr_free(xdr__lu_aliasent_ptr, &lu_aliasent);
	return (&global_aliasent);
}

static void
lu_alias_setent(int ignored)
{
	if (alias_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)alias_data, 
			      alias_datalen);
		alias_data = NULL;
		freeold();
	}
}

static void
lu_alias_endent(void)
{
	if (alias_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)alias_data, 
			      alias_datalen);
		alias_data = NULL;
	}
}

static struct aliasent *
lu_alias_getent(void)
{
	static int proc = -1;
	_lu_aliasent lu_aliasent;

	if (alias_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "alias_getent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &alias_data, 
				&alias_datalen) != KERN_SUCCESS) {
			alias_data = NULL;
			return (NULL);
		}
		xdrmem_create(&alias_xdr, alias_data, 
			      alias_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&alias_xdr, &alias_nentries)) {
			lu_alias_endent();
			return (NULL);
		}
	}
	if (alias_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_aliasent, sizeof(lu_aliasent));
	if (!xdr__lu_aliasent(&alias_xdr, &lu_aliasent)) {
		return (NULL);
	}
	alias_nentries--;
	convert_aliasent(&lu_aliasent);
	xdr_free(xdr__lu_aliasent, &lu_aliasent);
	return (&global_aliasent);
}

struct aliasent *
alias_getbyname(
		char *name
		)
{
	LOOKUP1(lu_alias_getbyname, _old_alias_getbyname, name, aliasent);
}

struct aliasent *
alias_getent(
	     void
	     )
{
	GETENT(lu_alias_getent, _old_alias_getent, &alias_state, aliasent);
}


void
alias_setent(
	     void
	     )
{
	SETSTATE(lu_alias_setent, _old_alias_setent, &alias_state, 0);
}


void
alias_endent(
	     void
	     )
{
	UNSETSTATE(lu_alias_endent, _old_alias_endent, &alias_state);
}



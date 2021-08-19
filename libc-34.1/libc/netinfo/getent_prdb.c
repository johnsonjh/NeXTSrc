/*
 * Printer lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <printerdb.h>
#include "lu_utils.h"

extern struct prdb_ent *_old_prdb_get();
extern struct prdb_ent *_old_prdb_getbyname();
extern void _old_prdb_set();
extern void _old_prdb_end();

static lookup_state prdb_state = LOOKUP_CACHE;
static struct prdb_ent global_prdb = { 0 };
static char *prdb_data = 0;
static unsigned prdb_datalen = 0;
static int prdb_nentries;
static XDR prdb_xdr = { 0 };

static void
freeprop(prdb_property prop)
{
	if (prop.pp_key != NULL) {
		free(prop.pp_key);
	}
	if (prop.pp_value != NULL) {
		free(prop.pp_value);
	}
}


static void 
freeold(void)
{
	char **names;
	int i;

	names = global_prdb.pe_name;
	if (names != NULL) {
		while (*names) {
			free(*names++);
		}
		free(global_prdb.pe_name);
	}
	global_prdb.pe_name = NULL;
	for (i = 0; i < global_prdb.pe_nprops; i++) {
		freeprop(global_prdb.pe_prop[i]);
	}
	if (global_prdb.pe_prop != NULL) {
		free(global_prdb.pe_prop);
	}
	global_prdb.pe_prop = NULL;
	global_prdb.pe_nprops = 0;
}


static void
convert_prdb(
	   _lu_prdb_ent *lu_prdb
	   )
{
	freeold();

	global_prdb.pe_name = (char **)realloc(lu_prdb->pe_names.pe_names_val,
					       (lu_prdb->pe_names.pe_names_len
						+ 1) * sizeof(char *));
	global_prdb.pe_name[lu_prdb->pe_names.pe_names_len] = NULL;
	global_prdb.pe_prop = (prdb_property *)lu_prdb->pe_props.pe_props_val;
	global_prdb.pe_nprops = lu_prdb->pe_props.pe_props_len;
	bzero(lu_prdb, sizeof(*lu_prdb));
}


static void
lu_prdb_set()
{
	if (prdb_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)prdb_data, 
			      prdb_datalen);
		prdb_data = NULL;
	}
}

static void
lu_prdb_end()
{
	if (prdb_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)prdb_data, 
			      prdb_datalen);
		prdb_data = NULL;
		freeold();
	}
}

static struct prdb_ent *
lu_prdb_getbyname(const char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_prdb_ent_ptr lu_prdb;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "prdb_getbyname", &proc) !=
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
	lu_prdb = NULL;
	if (!xdr__lu_prdb_ent_ptr(&inxdr, &lu_prdb) || lu_prdb == NULL) {
		return (NULL);
	}
	convert_prdb(lu_prdb);
	xdr_free(xdr__lu_prdb_ent_ptr, &lu_prdb);
	return (&global_prdb);
}

static prdb_ent *
lu_prdb_get()
{
	static int proc = -1;
	_lu_prdb_ent lu_prdb;

	if (prdb_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "prdb_get", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &prdb_data, 
			       &prdb_datalen) != KERN_SUCCESS) {
			prdb_data = NULL;
			return (NULL);
		}
		xdrmem_create(&prdb_xdr, prdb_data, 
			      prdb_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&prdb_xdr, &prdb_nentries)) {
			lu_prdb_end();
			return (NULL);
		}
	}
	if (prdb_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_prdb, sizeof(lu_prdb));
	if (!xdr__lu_prdb_ent(&prdb_xdr, &lu_prdb)) {
		return (NULL);
	}
	prdb_nentries--;
	convert_prdb(&lu_prdb);
	xdr_free(xdr__lu_prdb_ent, &lu_prdb);
	return (&global_prdb);
}

const prdb_ent *
prdb_getbyname(
	       const char *name
	       )
{
	LOOKUP1(lu_prdb_getbyname, _old_prdb_getbyname, name, prdb_ent);
}


const prdb_ent *
prdb_get(void)
{
	GETENT(lu_prdb_get, _old_prdb_get, &prdb_state, prdb_ent);
}


void
prdb_set(
	 const char *domain
	 )
{
	SETSTATE(lu_prdb_set, _old_prdb_set, &prdb_state, domain);
}


void
prdb_end(void)
{
	UNSETSTATE(lu_prdb_end, _old_prdb_end, &prdb_state);
}

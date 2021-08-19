/*
 * prdb (printer lookup) implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <sys/message.h>
#include <sys/socket.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include "lookup.h"
#include "copy_mem.h"
#include "clib.h"
#include "clib_internal.h"

struct _lu_prdb_ent *
fixprdb(
	const struct prdb_ent *prdb
	)
{
	static struct _lu_prdb_ent newprdb;
	int i;

	if (prdb == NULL) {
		return (NULL);
	}
	for (i = 0; i < _LU_MAXPRNAMES && prdb->pe_name[i]; i++) {
	}
	newprdb.pe_names.pe_names_len = i;
	newprdb.pe_names.pe_names_val = prdb->pe_name;
	newprdb.pe_props.pe_props_len = prdb->pe_nprops;
	newprdb.pe_props.pe_props_val = (_lu_prdb_property *)prdb->pe_prop;
	return (&newprdb);
}


int 
lookup_prdb_getbyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		      )
{
	struct _lu_prdb_ent *prdb;
	char *name;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	name = NULL;
	if (!xdr__lu_string(&inxdr, &name)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	prdb = fixprdb(prdb_getbyname(name));
	free(name);
	if (xdr__lu_prdb_ent_ptr(&outxdr, &prdb)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int
lookup_prdb_get(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_prdb_ent *prdb;
	XDR xdr;
	int count;
	int size;
	static char *target;
	static int target_offset;

	if (target != NULL) {
		/*
		 * Never cache
		 */
		vm_deallocate(task_self(), (vm_address_t)target, 
			      target_offset);
		target = NULL;
	}
	if (vm_allocate(task_self(), (vm_address_t *)&target, vm_page_size, 
			TRUE) != KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	prdb_set(NULL);
	count = 0;
	while (prdb = fixprdb(prdb_get())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_prdb_ent(&xdr, prdb)) {
			debug("serialization error");
			break;
		}
		size = xdr_getpos(&xdr);
		if (!copy_mem(lookup_buf, &target, target_offset, size)) {
			break;
		}
		target_offset += size;
		count++;
	}
	xdr_destroy(&xdr);
	prdb_end();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



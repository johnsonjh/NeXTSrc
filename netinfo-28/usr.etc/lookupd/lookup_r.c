/*
 * getrpcent implementation
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

struct _lu_rpcent *
fixr(
     struct rpcent *r
     )
{
	static char *r_names[_LU_MAXPNAMES];
	static struct _lu_rpcent newr = { { 0, r_names }};
	int i;
	
	if (r == NULL) {
		return (NULL);
	}
	newr.r_names.r_names_val[0] = r->r_name;
	for (i = 1; i < _LU_MAXSNAMES && r->r_aliases[i - 1]; i++) {
		newr.r_names.r_names_val[i] = r->r_aliases[i - 1];
	}
	newr.r_names.r_names_len = i;
	newr.r_number = r->r_number;
	return (&newr);
}


int 
lookup_getrpcbyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		)
{
	struct _lu_rpcent *r;
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
	r = fixr(getrpcbyname(name));
	free(name);
	if (xdr__lu_rpcent_ptr(&outxdr, &r)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getrpcbynumber(
			int inlen,
			char *indata,
			int *outlen,
			char **outdata
			)
{
	struct _lu_rpcent *r;
	int number;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	if (!xdr_int(&inxdr, &number)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	r = fixr(getrpcbynumber(number));
	if (xdr__lu_rpcent_ptr(&outxdr, &r)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getrpcent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_rpcent *r;
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
			TRUE) !=
	    KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	setrpcent(1);
	count = 0;
	while (r = fixr(getrpcent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_rpcent(&xdr, r)) {
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
	endrpcent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



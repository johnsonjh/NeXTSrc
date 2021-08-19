/*
 * netgroup implementation
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

extern bool_t xdr_free(xdrproc_t, void *);

#define FIX(x) ((x) == NULL ? NULL : *(x))

int 
lookup_innetgr(
	       int inlen,
	       char *indata,
	       int *outlen,
	       char **outdata
	       )
{
	XDR inxdr;
	XDR outxdr;
	int stat = 0;
	_lu_innetgr_args args;
	int res;

	bzero(&args, sizeof(args));
	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	if (!xdr__lu_innetgr_args(&inxdr, &args)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	res = innetgr(args.group, FIX(args.host), FIX(args.user), 
		      FIX(args.domain));
	if (xdr_int(&outxdr, &res)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_free(xdr__lu_innetgr_args, &args);
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getnetgrent(
		   int inlen,
		   char *indata,
		   int *outlen,
		   char **outdata
		   )
{
	struct _lu_netgrent ng;
	XDR xdr;
	int count;
	int size;
	static char *target;
	static int target_offset;
	char *group;
	XDR inxdr;

	if (target != NULL) {
		/*
		 * Never cache
		 */
		vm_deallocate(task_self(), (vm_address_t)target, 
			      target_offset);
		target = NULL;
	}

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	group = NULL;
	if (!xdr__lu_string(&inxdr, &group)) {
		return (0);
	}
	xdr_destroy(&inxdr);
	
	if (vm_allocate(task_self(), (vm_address_t *)&target, vm_page_size, 
			TRUE) !=  KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	setnetgrent(group);
	count = 0;
	while (getnetgrent(&ng.ng_host, &ng.ng_user, &ng.ng_domain)) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_netgrent(&xdr, &ng)) {
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
	endnetgrent();
	free(group);
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}

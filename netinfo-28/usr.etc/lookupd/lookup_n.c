/*
 * getnetent implementation
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

struct _lu_netent *
fixn(
     struct netent *n
     )
{
	static char *n_names[_LU_MAXNNAMES];
	static struct _lu_netent newn = { { 0, n_names }};
	int i;
	
	if (n == NULL) {
		return (NULL);
	}
	newn.n_names.n_names_val[0] = n->n_name;
	for (i = 1; i < _LU_MAXNNAMES && n->n_aliases[i - 1]; i++) {
		newn.n_names.n_names_val[i] = n->n_aliases[i - 1];
	}
	newn.n_names.n_names_len = i;
	newn.n_net = n->n_net;
	return (&newn);
}


int 
lookup_getnetbyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		)
{
	struct _lu_netent *n;
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
	n = fixn(getnetbyname(name));
	free(name);
	if (xdr__lu_netent_ptr(&outxdr, &n)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getnetbyaddr(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		     )
{
	struct _lu_netent *n;
	unsigned long addr;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	if (!xdr_int(&inxdr, &addr)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	n = fixn(getnetbyaddr(addr, AF_INET));
	if (xdr__lu_netent_ptr(&outxdr, &n)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getnetent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_netent *n;
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
	setnetent(1);
	count = 0;
	while (n = fixn(getnetent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_netent(&xdr, n)) {
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
	endnetent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



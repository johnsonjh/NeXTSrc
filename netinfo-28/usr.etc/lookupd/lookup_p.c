/*
 * getprotent implementation
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


struct _lu_protoent *
fixp(
     struct protoent *p
     )
{
	static char *p_names[_LU_MAXPNAMES];
	static struct _lu_protoent newp = { { 0, p_names }};
	int i;
	
	if (p == NULL) {
		return (NULL);
	}
	newp.p_names.p_names_val[0] = p->p_name;
	for (i = 1; i < _LU_MAXSNAMES && p->p_aliases[i - 1]; i++) {
		newp.p_names.p_names_val[i] = p->p_aliases[i - 1];
	}
	newp.p_names.p_names_len = i;
	newp.p_proto = p->p_proto;
	return (&newp);
}


int 
lookup_getprotobyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		)
{
	struct _lu_protoent *p;
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
	p = fixp(getprotobyname(name));
	free(name);
	if (xdr__lu_protoent_ptr(&outxdr, &p)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getprotobynumber(
			int inlen,
			char *indata,
			int *outlen,
			char **outdata
			)
{
	struct _lu_protoent *p;
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
	p = fixp(getprotobynumber(number));
	if (xdr__lu_protoent_ptr(&outxdr, &p)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getprotoent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_protoent *p;
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
	setprotoent(1);
	count = 0;
	while (p = fixp(getprotoent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_protoent(&xdr, p)) {
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
	endprotoent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



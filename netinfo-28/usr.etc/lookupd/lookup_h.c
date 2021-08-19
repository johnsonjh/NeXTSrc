/*
 * gethostent implementation
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

struct _lu_hostent *
fixh(
     struct hostent *h
     )
{
	static char *h_names[_LU_MAXHNAMES];
	static unsigned long h_addrs[_LU_MAXADDRS];
	static struct _lu_hostent newh = { { 0, h_names }, { 0, h_addrs }};
	int i;
	
	if (h == NULL) {
		return (NULL);
	}
	newh.h_names.h_names_val[0] = h->h_name;
	for (i = 1; i < _LU_MAXHNAMES && h->h_aliases[i - 1]; i++) {
		newh.h_names.h_names_val[i] = h->h_aliases[i - 1];
	}
	newh.h_names.h_names_len = i;
	for (i = 0; i < _LU_MAXADDRS && h->h_addr_list[i]; i++) {
		bcopy(h->h_addr_list[i], &newh.h_addrs.h_addrs_val[i], 
		      sizeof(newh.h_addrs.h_addrs_val[i]));
	}
	newh.h_addrs.h_addrs_len = i;
	return (&newh);
}


int 
lookup_gethostbyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		)
{
	struct _lu_hostent *h;
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
	h = fixh(gethostbyname(name));
	free(name);
	if (xdr__lu_hostent_ptr(&outxdr, &h) &&
	    xdr_int(&outxdr, &h_errno)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_gethostbyaddr(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		     )
{
	struct _lu_hostent *h;
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
	h = fixh(gethostbyaddr((char *)&addr, sizeof(addr), AF_INET));
	if (xdr__lu_hostent_ptr(&outxdr, &h) &&
	    xdr_int(&outxdr, &h_errno)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_gethostent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_hostent *h;
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
	sethostent(1);
	count = 0;
	while (h = fixh(gethostent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_hostent(&xdr, h)) {
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
	endhostent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}


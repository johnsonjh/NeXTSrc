/*
 * getservent implementation
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

struct _lu_servent *
fixs(
     struct servent *s
     )
{
	static char *s_names[_LU_MAXSNAMES];
	static struct _lu_servent news = { { 0, s_names }};
	int i;
	
	if (s == NULL) {
		return (NULL);
	}
	news.s_names.s_names_val[0] = s->s_name;
	for (i = 1; i < _LU_MAXSNAMES && s->s_aliases[i - 1]; i++) {
		news.s_names.s_names_val[i] = s->s_aliases[i - 1];
	}
	news.s_names.s_names_len = i;
	news.s_port = s->s_port;
	news.s_proto = s->s_proto;
	return (&news);
}


int 
lookup_getservbyname(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		)
{
	struct _lu_servent *s;
	char *name;
	char *proto;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	name = NULL;
	proto = NULL;
	if (!xdr__lu_string(&inxdr, &name) ||
	    !xdr__lu_string(&inxdr, &proto)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	s = fixs(getservbyname(name, proto));
	free(name);
	free(proto);
	if (xdr__lu_servent_ptr(&outxdr, &s)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getservbyport(
		     int inlen,
		     char *indata,
		     int *outlen,
		     char **outdata
		     )
{
	struct _lu_servent *s;
	int port;
	char *proto;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	proto = NULL;
	if (!xdr_int(&inxdr, &port) ||
	    !xdr__lu_string(&inxdr, &proto)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	s = fixs(getservbyport(port, proto));
	free(proto);
	if (xdr__lu_servent_ptr(&outxdr, &s)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getservent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_servent *s;
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
			TRUE) !=  KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	setservent(1);
	count = 0;
	while (s = fixs(getservent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_servent(&xdr, s)) {
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
	endservent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



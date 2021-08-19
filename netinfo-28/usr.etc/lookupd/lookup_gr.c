/*
 * getgrent implementation
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

#define NONULL(x) (((x) == NULL) ? "" : (x))

int
grmemlen(
	 char **grmem
	 )
{
	int i;
	
	for (i = 0; grmem[i] != NULL; i++) {
	}
	return (i);
}

struct _lu_group *
fixgr(
      struct group *gr
      )
{
	static struct _lu_group newgr;

	if (gr == NULL) {
		return (NULL);
	}
	newgr.gr_name = NONULL(gr->gr_name);
	newgr.gr_passwd = NONULL(gr->gr_passwd);
	newgr.gr_gid = gr->gr_gid;
	newgr.gr_mem.gr_mem_len = grmemlen(gr->gr_mem);
	newgr.gr_mem.gr_mem_val = gr->gr_mem;
	return (&newgr);
}


int 
lookup_getgrnam(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_group *gr;
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
	gr = fixgr(getgrnam(name));
	free(name);
	if (xdr__lu_group_ptr(&outxdr, &gr)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int 
lookup_getgrgid(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_group *gr;
	int gid;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	if (!xdr_int(&inxdr, &gid)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	gr = fixgr(getgrgid(gid));
	if (xdr__lu_group_ptr(&outxdr, &gr)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getgrent(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_group *gr;
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
	if (vm_allocate(task_self(), (vm_address_t *)&target, 
			vm_page_size, TRUE) != KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	setgrent();
	count = 0;
	while (gr = fixgr(getgrent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_group(&xdr, gr)) {
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
	endgrent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}


/*
 * alias lookup implementation
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

static const int target_init = 0;
#define TARGET_INIT ((char *)&target_init)
static char *target = TARGET_INIT;
static int target_offset = sizeof(int);

struct _lu_aliasent *
fixalias(
	 struct aliasent *alias
	 )
{
	static struct _lu_aliasent newalias;

	if (alias == NULL) {
		return (NULL);
	}
	newalias.alias_name = alias->alias_name;
	newalias.alias_members.alias_members_len = alias->alias_members_len;
	newalias.alias_members.alias_members_val = alias->alias_members;
	newalias.alias_local = alias->alias_local;
	return (&newalias);
}


int 
lookup_alias_getbyname(
		       int inlen,
		       char *indata,
		       int *outlen,
		       char **outdata
		       )
{
	struct _lu_aliasent *alias;
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
	alias = fixalias(alias_getbyname(name));
	free(name);
	if (xdr__lu_aliasent_ptr(&outxdr, &alias)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

int
lookup_alias_getent(
		    int inlen,
		    char *indata,
		    int *outlen,
		    char **outdata
		    )
{
	struct _lu_aliasent *alias;
	XDR xdr;
	int count;
	int size;

	if (target != NULL) {
		if (outdata != NULL) {
			/*
			 * Use cached entry
			 */
			*outdata = target;
			*outlen = target_offset;
			return (1);
		}
		/*
		 * Flush cache
		 */
		target = NULL;
	}
	if (outdata == NULL) {
		/*
		 * Was call to invalidate cache: return error
		 */
		target = NULL;
		return (0);
	}
	if (vm_allocate(task_self(), (vm_address_t *)&target, vm_page_size, 
			TRUE) != KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	alias_setent();
	count = 0;
	while (alias = fixalias(alias_getent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_aliasent(&xdr, alias)) {
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
	alias_endent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}

int
lookup_alias_setent(
		    int inlen,
		    char *indata,
		    int *outlen,
		    char **outdata
		    )
{
	if (target != TARGET_INIT) {
		vm_deallocate(task_self(), (vm_address_t)target, 
			      target_offset);
	}
	target = indata;
	target_offset = inlen;
	*outlen = 0;
	*outdata = NULL;
	return (1);
}


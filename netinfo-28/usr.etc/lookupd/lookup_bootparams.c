/*
 * bootparams implementation
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

int 
lookup_bootparams_getbyname(
			    int inlen,
			    char *indata,
			    int *outlen,
			    char **outdata
			    )
{
	char *name;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;
	_lu_bootparams_ent bootparams_storage;
	_lu_bootparams_ent_ptr bootparams;
	int len;
	char **values;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	name = NULL;
	if (!xdr__lu_string(&inxdr, &name)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	len = bootparams_getbyname(name, &values);
	if (len < 0) {
		bootparams = NULL;
	} else {
		bootparams = &bootparams_storage;
		(bootparams->bootparams_keyvalues.bootparams_keyvalues_len = 
		 len);
		(bootparams->bootparams_keyvalues.bootparams_keyvalues_val = 
		 values);
	}
	free(name);
	if (xdr__lu_bootparams_ent_ptr(&outxdr, &bootparams)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

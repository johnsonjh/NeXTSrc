/*
 * Bootparams lookup - netinfo only
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include "lu_utils.h"

static int 
lu_bootparams_getbyname(char *name, char ***values)
{
	unsigned datalen;
	XDR xdr;
	int size;
	static _lu_bootparams_ent_ptr bp;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "bootparams_getbyname",
				 &proc) != KERN_SUCCESS) {
			return (-1);
		}
	}
	xdrmem_create(&xdr, namebuf, sizeof(namebuf), XDR_ENCODE);
	if (!xdr__lu_string(&xdr, &name)) {
		return (-1);
	}
	size = xdr_getpos(&xdr);
	xdr_destroy(&xdr);

	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)namebuf, size, lookup_buf, 
			&datalen) != KERN_SUCCESS) {
		return (-1);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	xdr_free(xdr__lu_bootparams_ent_ptr, &bp);
	if (!xdr__lu_bootparams_ent_ptr(&xdr, &bp) || bp == NULL) {
		return (-1);
	}
	*values = bp->bootparams_keyvalues.bootparams_keyvalues_val;
	return (bp->bootparams_keyvalues.bootparams_keyvalues_len);
}


int
bootparams_getbyname(
		     char *name,
		     char ***values
		     )
{
	if (_lu_running()) {
		return (lu_bootparams_getbyname(name, values));
	} else {
		return (-1);
	}
}


/*
 * getmntent implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <sys/message.h>
#include <sys/socket.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include <stdio.h>
#include "_lu_types.h"
#include "lookup.h"
#include "copy_mem.h"
#include "clib.h"
#include "clib_internal.h"
#include "checksum.h"

struct _lu_mntent *
fixmnt(
     struct mntent *mnt
     )
{
	static struct _lu_mntent newmnt;
	
	if (mnt == NULL) {
		return (NULL);
	}
	newmnt.mnt_fsname = mnt->mnt_fsname;
	newmnt.mnt_dir = mnt->mnt_dir;
	newmnt.mnt_type = mnt->mnt_type;
	newmnt.mnt_opts = mnt->mnt_opts;
	newmnt.mnt_freq = mnt->mnt_freq;
	newmnt.mnt_passno = mnt->mnt_passno;
	return (&newmnt);
}


int
lookup_getmntent(
		  int inlen,
		  char *indata,
		  int *outlen,
		  char **outdata
		)
{
	struct _lu_mntent *mnt;
	XDR xdr;
	int count;
	int size;
	static char *target;
	static int target_offset;
	static checksum startsum;
	checksum finsum;

	FILE *f;

	if (target != NULL) {
		if (checksum_valid(startsum)) {
			ni_getchecksums(&finsum.len, &finsum.val);
			if (checksum_eq(startsum, finsum)) {
				/*
				 * Use cached entry
				 */
				checksum_free(&finsum);
				*outdata = target;
				*outlen = target_offset;
				return (1);
			} 
			checksum_free(&finsum);
		}

		/*
		 * Invalidate cache
		 */
		checksum_free(&startsum);
		vm_deallocate(task_self(), (vm_address_t)target, 
			      target_offset);
		target = NULL;
	}
	

		
	f = setmntent(NULL, "r");
	if (f == NULL) {
		return (0);
	}
	if (vm_allocate(task_self(), (vm_address_t *)&target, vm_page_size, 
			TRUE) != KERN_SUCCESS) {
		target = NULL;
		return (0);
	}
	target_offset = BYTES_PER_XDR_UNIT;
	xdrmem_create(&xdr, lookup_buf, sizeof(lookup_buf), XDR_ENCODE);
	count = 0;
	if (!checksum_makes_no_sense()) {
		ni_getchecksums(&startsum.len, &startsum.val);
	}
	while (mnt = fixmnt(getmntent(f))) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_mntent(&xdr, mnt)) {
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
	if (!checksum_makes_no_sense() && checksum_valid(startsum)) {
		ni_getchecksums(&finsum.len, &finsum.val);
		if (!checksum_eq(startsum, finsum)) {
			checksum_free(&startsum);
		}
		checksum_free(&finsum);
	}
	xdr_destroy(&xdr);
	endmntent(f);
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}



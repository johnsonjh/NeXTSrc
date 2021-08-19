/*
 * mount lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <mntent.h>
#include "lu_utils.h"

#define LU_FILE ((FILE *)-1)

extern struct mntent *_old_getmntent();
extern FILE *_old_setmntent();
extern int _old_endmntent();
extern int _old_addmntent();

static struct mntent global_mnt = { 0 };
static char *mnt_data = 0;
static unsigned mnt_datalen = 0;
static int mnt_nentries = 0;
static XDR mnt_xdr = { 0 };

static void
convert_mnt(
	   _lu_mntent *lu_mnt
	   )
{
	if (global_mnt.mnt_fsname != NULL) {
		free(global_mnt.mnt_fsname);
	}
	if (global_mnt.mnt_dir != NULL) {
		free(global_mnt.mnt_dir);
	}
	if (global_mnt.mnt_type != NULL) {
		free(global_mnt.mnt_type);
	}
	if (global_mnt.mnt_opts != NULL) {
		free(global_mnt.mnt_opts);
	}
	global_mnt.mnt_fsname = lu_mnt->mnt_fsname;
	global_mnt.mnt_dir = lu_mnt->mnt_dir;
	global_mnt.mnt_type = lu_mnt->mnt_type;
	global_mnt.mnt_opts = lu_mnt->mnt_opts;
	global_mnt.mnt_freq = lu_mnt->mnt_freq;
	global_mnt.mnt_passno = lu_mnt->mnt_passno;
	bzero(lu_mnt, sizeof(*lu_mnt));
}


static FILE *
lu_setmntent()
{
	if (mnt_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)mnt_data, 
			      mnt_datalen);
		mnt_data = NULL;
	}
	return (LU_FILE);
}

static int
lu_endmntent()
{
	if (mnt_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)mnt_data, 
			      mnt_datalen);
		mnt_data = NULL;
	}
	return (1);
}

static struct mntent *
lu_getmntent()
{
	static int proc = -1;
	_lu_mntent lu_mnt;

	if (mnt_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getmntent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &mnt_data, 
				&mnt_datalen) != KERN_SUCCESS) {
			mnt_data = NULL;
			return (NULL);
		}
		xdrmem_create(&mnt_xdr, mnt_data, 
			      mnt_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&mnt_xdr, &mnt_nentries)) {
			lu_endmntent();
			return (NULL);
		}
	}
	if (mnt_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_mnt, sizeof(lu_mnt));
	if (!xdr__lu_mntent(&mnt_xdr, &lu_mnt)) {
		return (NULL);
	}
	mnt_nentries--;
	convert_mnt(&lu_mnt);
	xdr_free(xdr__lu_mntent, &lu_mnt);
	return (&global_mnt);
}

struct mntent *
getmntent(
	  FILE *f
	  )
{
	if (f == LU_FILE) {
		return (lu_getmntent());
	} else {
		return (_old_getmntent(f));
	}
}

int
addmntent(
	  FILE *f,
	  struct mntent *mnt
	  )
{
	if (f == LU_FILE) {
		return (0);
	}
	return (_old_addmntent(f, mnt));
}


FILE *
setmntent(
	  char *fname,
	  char *type
	  )
{
	if (fname == NULL && _lu_running()) {
		return (lu_setmntent());
	} else {
		return (_old_setmntent(fname == NULL ? MNTTAB : fname, type));
	}
}


int
endmntent(
	  FILE *f
	  )
{
	if (f == LU_FILE) {
		return (lu_endmntent());
	} else {
		return (_old_endmntent(f));
	}
}

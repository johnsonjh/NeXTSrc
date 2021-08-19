/*
 * getpwent implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <sys/message.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include "lookup.h"
#include "copy_mem.h"
#include "clib.h"
#include "clib_internal.h"
#include "checksum.h"

#define NONULL(x) (((x) == NULL) ? "" : (x))

static const int target_init = 0;
#define TARGET_INIT ((char *)&target_init)
static char *target = TARGET_INIT;
static int target_offset = 0;

struct _lu_passwd *
fixpw(
      struct passwd *pw
      )
{
	static struct _lu_passwd newpw;

	if (pw == NULL) {
		return (NULL);
	}
	newpw.pw_name = NONULL(pw->pw_name);
	newpw.pw_passwd = NONULL(pw->pw_passwd);
	newpw.pw_uid = pw->pw_uid;
	newpw.pw_gid = pw->pw_gid;
	newpw.pw_gecos = NONULL(pw->pw_gecos);
	newpw.pw_dir = NONULL(pw->pw_dir);
	newpw.pw_shell = NONULL(pw->pw_shell);
	return (&newpw);
}


int 
lookup_getpwnam(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_passwd *pw;
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
	pw = fixpw(getpwnam(name));
	free(name);
	if (xdr__lu_passwd_ptr(&outxdr, &pw)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}

#ifdef notdef
int
lookup_putpwpasswd(
		   int inlen,
		   char *indata,
		   int *outlen,
		   char **outdata
		   )
{
	_lu_string login;
	_lu_string old_passwd;
	_lu_string new_passwd;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;
	int res;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	login = NULL;
	old_passwd = NULL;
	new_passwd = NULL;
	if (!xdr__lu_string(&inxdr, &login) ||
	    !xdr__lu_string(&inxdr, &old_passwd) ||
	    !xdr__lu_string(&inxdr, &new_passwd)) {
		return (stat);
	}
	xdr_destroy(&inxdr);
	
	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	res = _ni_putpwpasswd(login, old_passwd, new_passwd);
	if (xdr_int(&outxdr, &res)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	free(login);
	free(old_passwd);
	free(new_passwd);
	return (stat);
}
#endif

int 
lookup_getpwuid(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_passwd *pw;
	int uid;
	XDR inxdr;
	XDR outxdr;
	int stat = 0;

	xdrmem_create(&inxdr, indata, inlen, XDR_DECODE);
	if (!xdr_int(&inxdr, &uid)) {
		return (stat);
	}
	xdr_destroy(&inxdr);

	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	pw = fixpw(getpwuid(uid));
	if (xdr__lu_passwd_ptr(&outxdr, &pw)) {
		*outlen = xdr_getpos(&outxdr);
		stat = 1;
	}
	xdr_destroy(&outxdr);
	return (stat);
}


int
lookup_getpwent(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	struct _lu_passwd *pw;
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
	setpwent();
	count = 0;
	while (pw = fixpw(getpwent())) {
		xdr_setpos(&xdr, 0);
		if (!xdr__lu_passwd(&xdr, pw)) {
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
	endpwent();
	*(int *)target = htonl(count);
	*outlen = target_offset;
	*outdata = target;
	return (1);
}

int
lookup_setpwent(
		int inlen,
		char *indata,
		int *outlen,
		char **outdata
		)
{
	checksum_mark();
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


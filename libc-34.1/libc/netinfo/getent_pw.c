/*
 * user information (passwd) lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <pwd.h>
#include "lu_utils.h"

extern struct passwd *_old_getpwuid();
extern struct passwd *_old_getpwnam();
extern struct passwd *_old_getpwent();
extern void _old_setpwent();
extern void _old_endpwent();
extern void _old_setpwfile();

static lookup_state pw_state = LOOKUP_CACHE;
static struct passwd global_pw = { 0 };
static char *pw_data;
static unsigned pw_datalen;
static int pw_nentries;
static XDR pw_xdr;

static void
convert_pw(
	   _lu_passwd *lu_pw
	   )
{
	if (global_pw.pw_name != NULL) {
		free(global_pw.pw_name);
	}
	if (global_pw.pw_passwd != NULL) {
		free(global_pw.pw_passwd);
	}
	if (global_pw.pw_gecos != NULL) {
		free(global_pw.pw_gecos);
	}
	if (global_pw.pw_dir != NULL) {
		free(global_pw.pw_dir);
	}
	if (global_pw.pw_shell != NULL) {
		free(global_pw.pw_shell);
	}

	global_pw.pw_name = lu_pw->pw_name;
	global_pw.pw_passwd = lu_pw->pw_passwd;
	global_pw.pw_uid = lu_pw->pw_uid;
	global_pw.pw_gid = lu_pw->pw_gid;
	global_pw.pw_gecos = lu_pw->pw_gecos;
	global_pw.pw_dir = lu_pw->pw_dir;
	global_pw.pw_shell = lu_pw->pw_shell;
	bzero(lu_pw, sizeof(*lu_pw));
}


static struct passwd *
lu_getpwuid(int uid)
{
	unsigned datalen;
	_lu_passwd_ptr lu_pw;
	XDR xdr;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "getpwuid", &proc) != KERN_SUCCESS) {
			return (NULL);
		}
	}
	uid = htonl(uid);
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)&uid, 1, lookup_buf, 
			&datalen) != 
	    KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_pw = NULL;
	if (!xdr__lu_passwd_ptr(&xdr, &lu_pw) || lu_pw == NULL) {
		return (NULL);
	}
	convert_pw(lu_pw);
	xdr_free(xdr__lu_passwd_ptr, &lu_pw);
	return (&global_pw);
}

static struct passwd *
lu_getpwnam(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_passwd_ptr lu_pw;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "getpwnam", &proc) != KERN_SUCCESS) {
			return (NULL);
		}
	}
	xdrmem_create(&outxdr, namebuf, sizeof(namebuf), XDR_ENCODE);
	if (!xdr__lu_string(&outxdr, &name)) {
		return (NULL);
	}
	
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)namebuf,
		       xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT, 
		       lookup_buf, &datalen) != KERN_SUCCESS) {
		return (NULL);
	}
	xdrmem_create(&inxdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_pw = NULL;
	if (!xdr__lu_passwd_ptr(&inxdr, &lu_pw) || lu_pw == NULL) {
		return (NULL);
	}
	convert_pw(lu_pw);
	xdr_free(xdr__lu_passwd_ptr, &lu_pw);
	return (&global_pw);
}

#ifdef notdef
static int
lu_putpwpasswd(
	       char *login,
	       char *old_passwd,
	       char *new_passwd
	       )
{
	unsigned datalen;
	int changed;
	XDR xdr;
	static int proc = -1;
	char output_buf[3 * (_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT)];
	unit lookup_buf[MAX_INLINE_UNITS];
	XDR outxdr;
	
	if (proc < 0) {
		if (_lookup_link(_lu_port, "putpwpasswd", 
				 &proc) != KERN_SUCCESS) {
			return (0);
		}
	}
	xdrmem_create(&outxdr, output_buf, sizeof(output_buf),
		      XDR_ENCODE);
	if (!xdr__lu_string(&outxdr, &login) ||
	    !xdr__lu_string(&outxdr, &old_passwd) ||
	    !xdr__lu_string(&outxdr, &new_passwd)) {
		return (0);
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, output_buf,
			xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT,
			lookup_buf, &datalen) != KERN_SUCCESS) {
		return (0);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	if (!xdr_int(&xdr, &changed)) {
		return (0);
	}
	return (changed);
}
#endif

static void
lu_setpwent()
{
	if (pw_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)pw_data, pw_datalen);
		pw_data = NULL;
	}
}

static void
lu_endpwent()
{
	if (pw_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)pw_data, pw_datalen);
		pw_data = NULL;
	}
}

static struct passwd *
lu_getpwent()
{
	static int proc = -1;
	_lu_passwd lu_pw;

	if (pw_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "getpwent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &pw_data, 
				&pw_datalen) != KERN_SUCCESS) {
			pw_data = NULL;
			return (NULL);
		}
		xdrmem_create(&pw_xdr, pw_data, 
			      pw_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&pw_xdr, &pw_nentries)) {
			lu_endpwent();
			return (NULL);
		}
	}
	if (pw_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_pw, sizeof(lu_pw));
	if (!xdr__lu_passwd(&pw_xdr, &lu_pw)) {
		return (NULL);
	}
	pw_nentries--;
	convert_pw(&lu_pw);
	xdr_free(xdr__lu_passwd, &lu_pw);
	return (&global_pw);
}

struct passwd *
getpwuid(
	 int uid
	 )
{
	LOOKUP1(lu_getpwuid, _old_getpwuid,  uid, struct passwd);
}

struct passwd *
getpwnam(
	 char *name
	 )
{
	LOOKUP1(lu_getpwnam, _old_getpwnam,  name, struct passwd);
}

#ifdef notdef
/*
 * putpwpasswd() is not supported with anything other than netinfo
 * right now
 */
#define _old_passwd(name, oldpass, newpass) 0
int
putpwpasswd(
	    char *login,
	    char *old_passwd, /* cleartext */
	    char *new_passwd /* encrypted */
	    )
{
	int res;

	if (_lu_running()) {
		res = lu_putpwpasswd(login, old_passwd, new_passwd);
	} else {
		res = _old_passwd(login, old_passwd, new_passwd);
	}
	return (res);
}
#endif

struct passwd *
getpwent(
	 void
	 )
{
	GETENT(lu_getpwent, _old_getpwent, &pw_state, struct passwd);
}


void
setpwent(
	 void
	 )
{
	SETSTATE(lu_setpwent, _old_setpwent, &pw_state, 0);
}


void
endpwent(
	 void
	 )
{
	UNSETSTATE(lu_endpwent, _old_endpwent, &pw_state);
}

void
setpwfile(
	  char *fname
	  )
{
	/*
	 * This makes no sense with non-file lookup
	 */
	_old_setpwfile(fname);
}


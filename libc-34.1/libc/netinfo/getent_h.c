/*
 * host lookup
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <mach.h>
#include <stdio.h>
#include "lookup.h"
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "_lu_types.h"
#include <netdb.h>
#include "lu_utils.h"
#include <sys/socket.h>

extern struct hostent *_old_gethostbyaddr();
extern struct hostent *_old_gethostbyname();
extern struct hostent *_old_gethostent();
extern void _old_sethostent();
extern void _old_endhostent();
extern void _old_sethostfile();

extern int h_errno;

static lookup_state h_state = LOOKUP_CACHE;
/*
 * The static return value from get*ent functions
 */
static struct hostent global_h = { 0 };
/*
 * A copy to use when freeing in case user mucks with return value
 */
static struct hostent global_h_copy = { 0 };

static unsigned long *global_addrlist = 0;
static char *h_data;
static unsigned h_datalen;
static int h_nentries;
static XDR h_xdr;

static void
convert_h(
	  _lu_hostent *lu_h
	  )
{
	char **aliases;
	int i;

	/*
	 * First, free up anything from last call
	 */
	if (global_h_copy.h_name != NULL) {
		free(global_h_copy.h_name);
	}
	aliases = global_h_copy.h_aliases;
	if (aliases != NULL) {
		while (*aliases != NULL) {
			free(*aliases++);
		}
		free(global_h_copy.h_aliases);
	}
	if (global_h_copy.h_addr_list != NULL) {
		free(global_h_copy.h_addr_list);
		free(global_addrlist);
	}

	/*
	 * Copy hostname
	 */
	global_h.h_name = lu_h->h_names.h_names_val[0];

	/*
	 * Copy aliases, reusing malloced array returned from lookupd
	 */
	global_h.h_aliases = lu_h->h_names.h_names_val;
	for (i = 0; i < lu_h->h_names.h_names_len - 1; i++) {
		global_h.h_aliases[i] = global_h.h_aliases[i + 1];
	}
	global_h.h_aliases[lu_h->h_names.h_names_len - 1] = 0;

	global_h.h_addrtype = AF_INET;
	global_h.h_length = sizeof(long);

	/*
	 * Must malloc array to store pointers to addresses,
	 * then copy pointers to addresses into the array
	 */
	global_h.h_addr_list = (char **)malloc((lu_h->h_addrs.h_addrs_len + 1)
					       * sizeof(long));
	for (i = 0; i < lu_h->h_addrs.h_addrs_len; i++) {
		global_h.h_addr_list[i] = (char *)&lu_h->h_addrs.h_addrs_val[i];
	}
	global_h.h_addr_list[lu_h->h_addrs.h_addrs_len] = NULL;

	/*
	 * Remember area where addresses are store to free later
	 */
	global_addrlist = lu_h->h_addrs.h_addrs_val;

	/*
	 * Zero out structure returned by lookupd to avoid XDR freeing
	 */
	bzero(lu_h, sizeof(*lu_h));

	/*
	 * Make copy for use later
	 */
	global_h_copy = global_h;
}



static struct hostent *
lu_gethostbyaddr(
		 char *addr,
		 int len,
		 int type
		 )
{
	unsigned datalen;
	_lu_hostent_ptr lu_h;
	XDR xdr;
	long address;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];
	
	if (len != sizeof(long) ||
	    type != AF_INET) {
		h_errno = HOST_NOT_FOUND;
		return (NULL);
	}
	if (proc < 0) {
		if (_lookup_link(_lu_port, "gethostbyaddr", &proc) !=
		    KERN_SUCCESS) {
			h_errno = HOST_NOT_FOUND;
			return (NULL);
		}
	}
	bcopy(addr, &address, sizeof(address));
	address = htonl(address);
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)&address, 1, lookup_buf, 
			&datalen) !=
	    KERN_SUCCESS) {
		h_errno = HOST_NOT_FOUND;
		return (NULL);
	}
	xdrmem_create(&xdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_h = NULL;
	h_errno = HOST_NOT_FOUND;
	if (!xdr__lu_hostent_ptr(&xdr, &lu_h) || 
	    !xdr_int(&xdr, &h_errno) || lu_h == NULL) {
		return (NULL);
	}
	convert_h(lu_h);
	xdr_free(xdr__lu_hostent_ptr, &lu_h);
	return (&global_h);
}

static struct hostent *
lu_gethostbyname(char *name)
{
	unsigned datalen;
	char namebuf[_LU_MAXLUSTRLEN + BYTES_PER_XDR_UNIT];
	XDR outxdr;
	XDR inxdr;
	_lu_hostent_ptr lu_h;
	static int proc = -1;
	unit lookup_buf[MAX_INLINE_UNITS];

	if (proc < 0) {
		if (_lookup_link(_lu_port, "gethostbyname", &proc) !=
		    KERN_SUCCESS) {
			h_errno = HOST_NOT_FOUND;
			return (NULL);
		}
	}
	xdrmem_create(&outxdr, namebuf, sizeof(namebuf), XDR_ENCODE);
	if (!xdr__lu_string(&outxdr, &name)) {
		h_errno = HOST_NOT_FOUND;
		return (NULL);
	}
	datalen = MAX_INLINE_UNITS;
	if (_lookup_one(_lu_port, proc, (unit *)namebuf,
			xdr_getpos(&outxdr) / BYTES_PER_XDR_UNIT, 
			lookup_buf, &datalen) != KERN_SUCCESS) {
		h_errno = HOST_NOT_FOUND;
		return (NULL);
	}
	xdrmem_create(&inxdr, lookup_buf, datalen * BYTES_PER_XDR_UNIT,
		      XDR_DECODE);
	lu_h = NULL;
	h_errno = HOST_NOT_FOUND;
	if (!xdr__lu_hostent_ptr(&inxdr, &lu_h) || 
	    !xdr_int(&inxdr, &h_errno) || lu_h == NULL) {
		return (NULL);
	}
	convert_h(lu_h);
	xdr_free(xdr__lu_hostent_ptr, &lu_h);
	return (&global_h);
}

static void
lu_sethostent()
{
	if (h_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)h_data, h_datalen);
		h_data = NULL;
	}
}

static void
lu_endhostent()
{
	if (h_data != NULL) {
		vm_deallocate(task_self(), (vm_address_t)h_data, h_datalen);
		h_data = NULL;
	}
}

static struct hostent *
lu_gethostent()
{
	static int proc = -1;
	_lu_hostent lu_h;

	if (h_data == NULL) {
		if (proc < 0) {
			if (_lookup_link(_lu_port, "gethostent", &proc) !=
			    KERN_SUCCESS) {
				return (NULL);
			}
		}
		if (_lookup_all(_lu_port, proc, NULL, 0, &h_data, 
				&h_datalen) != KERN_SUCCESS) {
			h_data = NULL;
			return (NULL);
		}
		xdrmem_create(&h_xdr, h_data, 
			      h_datalen * BYTES_PER_XDR_UNIT,
			      XDR_DECODE);
		if (!xdr_int(&h_xdr, &h_nentries)) {
			lu_endhostent();
			return (NULL);
		}
	}
	if (h_nentries == 0) {
		return (NULL);
	}
	bzero(&lu_h, sizeof(lu_h));
	if (!xdr__lu_hostent(&h_xdr, &lu_h)) {
		return (NULL);
	}
	h_nentries--;
	convert_h(&lu_h);
	xdr_free(xdr__lu_hostent, &lu_h);
	return (&global_h);
}

struct hostent *
gethostbyaddr(
	      char *addr,
	      int len,
	      int type
	      )
{
	struct hostent *res;

	if (_lu_running()) {
		res = lu_gethostbyaddr(addr, len, type);
	} else {
		res = _old_gethostbyaddr(addr, len, type);
	}
	return (res);
}

struct hostent *
gethostbyname(
	      char *name
	      )
{
	LOOKUP1(lu_gethostbyname, _old_gethostbyname,  name, struct hostent);
}

struct hostent *
gethostent(
	   void
	   )
{
	GETENT(lu_gethostent, _old_gethostent, &h_state, struct hostent);
}


void
sethostent(
	   int stayopen
	   )
{
	SETSTATE(lu_sethostent, _old_sethostent, &h_state, stayopen);
}


void
endhostent(
	   void
	   )
{
	UNSETSTATE(lu_endhostent, _old_endhostent, &h_state);
}

void
sethostfile(
	    char *fname
	    )
{
	/*
	 * This makes no sense with non-file lookup
	 */
	_old_sethostfile(fname);
}


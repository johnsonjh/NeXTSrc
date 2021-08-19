/*
 * lookupd dispatcher
 * Copyright (C) 1989 by NeXT, Inc.
 */
 
#include <mach.h>
#include <sys/message.h>
#include <rpc/types.h>
#include <rpc/xdr.h>
#include "lookup.h"
#include "clib.h"

typedef int lookup_proc(unsigned, char *, unsigned *, char **);

lookup_proc lookup_getpwuid, lookup_getpwnam, lookup_getpwent, lookup_setpwent;
#ifdef notdef
lookup_proc lookup_putpwpasswd;
#endif
lookup_proc lookup_getgrgid, lookup_getgrnam, lookup_getgrent;
lookup_proc lookup_gethostbyname, lookup_gethostbyaddr, lookup_gethostent;
lookup_proc lookup_getnetbyname, lookup_getnetbyaddr, lookup_getnetent;
lookup_proc lookup_getservbyname, lookup_getservbyport, lookup_getservent;
lookup_proc lookup_getprotobyname, lookup_getprotobynumber, 
		lookup_getprotoent;
lookup_proc lookup_getrpcbyname, lookup_getrpcbynumber, lookup_getrpcent;
lookup_proc lookup_getmntent;
lookup_proc lookup_prdb_getbyname;
lookup_proc lookup_prdb_get;
lookup_proc lookup_bootparams_getbyname;
lookup_proc lookup_bootp_getbyip;
lookup_proc lookup_bootp_getbyether;
lookup_proc lookup_alias_getent;
lookup_proc lookup_alias_setent;
lookup_proc lookup_alias_getbyname;
lookup_proc lookup_innetgr, lookup_getnetgrent;
lookup_proc lookup_getstatistics;

typedef struct lookup_node {
	char *name;
	lookup_proc *proc;
	int islong;
	unsigned ncalls;
	unsigned totaltime;
} lookup_node;

static lookup_node _lookup_links[] = { 
	{ "getpwent", 		&lookup_getpwent, 		1 },
	{ "getpwuid", 		&lookup_getpwuid, 		0 },
	{ "getpwnam", 		&lookup_getpwnam, 		0 },
#ifdef notdef
	{ "putpwpasswd",	&lookup_putpwpasswd,		0 },
#endif
	{ "setpwent",		&lookup_setpwent,		1 },
	{ "getgrent", 		&lookup_getgrent, 		1 },
	{ "getgrgid", 		&lookup_getgrgid, 		0 },
	{ "getgrnam",		&lookup_getgrnam, 		0 },
	{ "gethostent", 	&lookup_gethostent, 		1 },
	{ "gethostbyname", 	&lookup_gethostbyname,		0 },
	{ "gethostbyaddr", 	&lookup_gethostbyaddr,		0 },
	{ "getnetent", 		&lookup_getnetent, 		1 },
	{ "getnetbyname", 	&lookup_getnetbyname,		0 },
	{ "getnetbyaddr", 	&lookup_getnetbyaddr,		0 },
	{ "getservent", 	&lookup_getservent, 		1 },
	{ "getservbyname", 	&lookup_getservbyname,		0 },
	{ "getservbyport", 	&lookup_getservbyport,		0 },
	{ "getprotoent", 	&lookup_getprotoent, 		1 },
	{ "getprotobyname", 	&lookup_getprotobyname,		0 },
	{ "getprotobynumber", 	&lookup_getprotobynumber,	0 },
	{ "getrpcent", 		&lookup_getrpcent, 		1 },
	{ "getrpcbyname", 	&lookup_getrpcbyname,		0 },
	{ "getrpcbynumber", 	&lookup_getrpcbynumber,		0 },
	{ "getmntent",		&lookup_getmntent,		1 },
	{ "prdb_get",		&lookup_prdb_get,		1 },
	{ "prdb_getbyname",	&lookup_prdb_getbyname,		0 },
	{ "bootparams_getbyname",&lookup_bootparams_getbyname,	0 },
	{ "bootp_getbyip",	&lookup_bootp_getbyip,		0 },
	{ "bootp_getbyether",	&lookup_bootp_getbyether,	0 },
	{ "alias_getbyname",	&lookup_alias_getbyname,        0 },
	{ "alias_getent",       &lookup_alias_getent,           1 },
	{ "alias_setent",	&lookup_alias_setent,		1 },
	{ "innetgr",		&lookup_innetgr,		0 },
	{ "getnetgrent",	&lookup_getnetgrent,		1 },
	{ "_getstatistics",	&lookup_getstatistics,		0 },
};
#define LOOKUP_NPROCS  (sizeof(_lookup_links)/sizeof(_lookup_links[0]))


int 
lookup_getstatistics(
		     unsigned inlen, 
		     char *indata, 
		     unsigned *outlen, 
		     char **outdata
		     )
{
	XDR outxdr;
	unsigned len;
	unsigned i;
	int stat;
	extern bool_t xdr_u_int(XDR *, unsigned *);
	extern bool_t xdr__lu_string(XDR *, char **);

	xdrmem_create(&outxdr, *outdata, MAX_INLINE_DATA, XDR_ENCODE);
	len = LOOKUP_NPROCS;
	stat = 0;
	if (xdr_u_int(&outxdr, &len)) {
		stat = 1;
		for (i = 0; i < len; i++) {
			if (!xdr__lu_string(&outxdr, &_lookup_links[i].name) ||
			    !xdr_u_int(&outxdr, &_lookup_links[i].ncalls) ||
			    !xdr_u_int(&outxdr, 
					  &_lookup_links[i].totaltime)) {
				stat = 0;
				break;
			}
		}
		if (stat) {
			*outlen = xdr_getpos(&outxdr);
		}
	}
	xdr_destroy(&outxdr);
	return (stat);
}


static void
time_record(
	    int procno,
	    int elapsed
	    )
{
	_lookup_links[procno].ncalls++;
	_lookup_links[procno].totaltime += elapsed;
}


static unsigned
time_msec(
	  void
	  )
{
	static struct timeval base;
	static int initialized;
	struct timeval now;

	if (!initialized) {
		gettimeofday(&base, NULL);
		initialized = 1;
	}
	gettimeofday(&now, NULL);
	if (now.tv_usec < base.tv_usec) {
		return (((now.tv_sec - 1) - base.tv_sec) * 1000 +
			((now.tv_usec + 1000000) - base.tv_usec) / 1000); 
	} else {
		return ((now.tv_sec - base.tv_sec) * 1000 +
			(now.tv_usec - base.tv_usec) / 1000); 
	}
}

kern_return_t
__lookup_link(
	    port_t server,
	    lookup_name name,
	    int *procno
	    )
{
	int i;

	for (i = 0; i < LOOKUP_NPROCS; i++) {
		if (strcmp(name, _lookup_links[i].name) == 0) {
			*procno = i;
			return (KERN_SUCCESS);
		}
	}
	return (KERN_FAILURE);
}

kern_return_t
__lookup_all(
	   port_t server,
	   int procno,
	   inline_data indata,
	   unsigned inlen,
	   ooline_data *outdata,
	   unsigned *outlen
	   )
{
	unsigned time;

	if (procno < 0 || procno >= LOOKUP_NPROCS) {
		return (KERN_FAILURE);
	}
	if (!_lookup_links[procno].islong) {
		return (KERN_FAILURE);
	}
	inlen *= BYTES_PER_XDR_UNIT;
	time = time_msec();
	if (!_lookup_links[procno].proc(inlen, (char *)indata, outlen, 
					(char **)outdata)) {
		return (KERN_FAILURE);
	}
	time_record(procno, time_msec() - time);
	*outlen /= BYTES_PER_XDR_UNIT;
	return (KERN_SUCCESS);
}

kern_return_t
__lookup_one(
	   port_t server,
	   int procno,
	   inline_data indata,
	   unsigned inlen,
	   inline_data outdata,
	   unsigned *outlen
	   )
{
	char *datap = (char *)outdata;
	unsigned time;

	if (procno < 0 || procno >= LOOKUP_NPROCS) {
		return (KERN_FAILURE);
	}
	if (_lookup_links[procno].islong) {
		return (KERN_FAILURE);
	}
	inlen *= BYTES_PER_XDR_UNIT;
	time = time_msec();
	if (!_lookup_links[procno].proc(inlen, (char *)indata, 
					outlen, &datap)) {
		return (KERN_FAILURE);
	}
	time_record(procno, time_msec() - time);
	*outlen /= BYTES_PER_XDR_UNIT;
	return (KERN_SUCCESS);
}

void
dealloc(
	ooline_data data,
	unsigned len
	)
{
	(void)vm_deallocate(task_self(), (vm_address_t)data, len * UNIT_SIZE);
}

kern_return_t
__lookup_ooall(
	       port_t server,
	       int procno,
	       ooline_data indata,
	       unsigned inlen,
	       ooline_data *outdata,
	       unsigned *outlen
	       )
{
	unsigned time;

	if (procno < 0 || procno >= LOOKUP_NPROCS) {
		dealloc(indata, inlen);
		return (KERN_FAILURE);
	}

	if (!_lookup_links[procno].islong) {
		dealloc(indata, inlen);
		return (KERN_FAILURE);
	}
	inlen *= BYTES_PER_XDR_UNIT;
	time = time_msec();
	if (!_lookup_links[procno].proc(inlen, (char *)indata, outlen, 
					(char **)outdata)) {
		dealloc(indata, inlen);
		return (KERN_FAILURE);
	}
	time_record(procno, time_msec() - time);
	*outlen /= BYTES_PER_XDR_UNIT;
	return (KERN_SUCCESS);
}


/*
 * /etc/bootptab format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);
static void putline(ni_proplist *);

void
dump_bootp(void)
{
	ni_proplist props;

	printf("/private/tftpboot:/\n"
	       "mach\n"
	       "%%%%\n");
	NI_INIT(&props);
	while (loadprops(&props)) {
		putline(&props);
		ni_proplist_free(&props);
	}
}

static ni_namelist *
getval(ni_proplist *props, char *name)
{
	int i;

	for (i = 0; i < props->nipl_len; i++) {
		if (ni_name_match(props->nipl_val[i].nip_name, name)) {
			if (props->nipl_val[i].nip_val.ninl_len == 0) {
				return (NULL);
			}
			return (&props->nipl_val[i].nip_val);
		}
	}
	return (NULL);

}

static void
putline(ni_proplist *props)
{
	ni_namelist *bf;
	ni_namelist *en;
	ni_namelist *ip;
	ni_namelist *name;

	bf = getval(props, "bootfile");
	if (bf == NULL || bf->ninl_len == 0) {
		return;
	}
	name = getval(props, "name");
	if (name == NULL || name->ninl_len == 0) {
		printf("#unknown\n");
		ni_namelist_free(bf);
		return;
	}
	en = getval(props, "en_address");
	if (en == NULL || en->ninl_len == 0) {
		printf("#%-16s 1 unknown ethernet address\n", 
		       name->ninl_val[0]);
		ni_namelist_free(bf);
		ni_namelist_free(name);
		return;
	}
	ip = getval(props, "ip_address");
	if (ip == NULL || ip->ninl_len == 0) {
		printf("#%-16s 1 %-14s unknown ip address\n", 
		       name->ninl_val[0],
		       en->ninl_val[0]);
		ni_namelist_free(bf);
		ni_namelist_free(name);
		ni_namelist_free(en);
		return;
	}
	printf("%-16s 1  %-14s  %-15s  %s\n", 
	       name->ninl_val[0],
	       en->ninl_val[0],
	       ip->ninl_val[0],
	       bf->ninl_val[0]);
	ni_namelist_free(bf);
	ni_namelist_free(name);
	ni_namelist_free(en);
	ni_namelist_free(ip);
}

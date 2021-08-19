/*
 * /etc/services format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static void putline(ni_proplist *);

void
dump_services(void)
{
	ni_proplist props;

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
	int i;
	ni_namelist *names;
	ni_namelist *port;
	ni_namelist *proto;
	int j;


	names = getval(props, "name");
	port = getval(props, "port");
	proto = getval(props, "protocol");
	if (names == NULL || port == NULL || proto == NULL ||
	    names->ninl_len == 0 || port->ninl_len == 0 || 
	    proto->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	for (j = 0; j < proto->ninl_len; j++) {
		printf("%s\t", names->ninl_val[0]);
		printf("%s/", port->ninl_val[0]);
		printf("%s\t", proto->ninl_val[j]);
		for (i = 1; i < names->ninl_len; i++) {
			printf("%s%s", 
			       names->ninl_val[i],
			       (i + 1 == names->ninl_len) ? "" : "\t");
		}
		printf("\n");
	}
}

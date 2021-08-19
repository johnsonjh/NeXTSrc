/*
 * /etc/networks format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static void putline(ni_proplist *);

void
dump_networks(void)
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
	ni_namelist *address;

	names = getval(props, "name");
	address = getval(props, "address");
	if (names == NULL || address == NULL || 
	    names->ninl_len == 0 || address->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	printf("%s\t", names->ninl_val[0]);
	printf("%s\t", address->ninl_val[0]);
	for (i = 1; i < names->ninl_len; i++) {
		printf("%s%s", 
		       names->ninl_val[i],
		       (i + 1 == names->ninl_len) ? "" : "\t");
	}
	printf("\n");
}

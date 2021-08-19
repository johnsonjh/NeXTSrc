/*
 * /etc/hosts format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static void putline(ni_proplist *);

void
dump_machines(void)
{
	ni_proplist props;

	NI_INIT(&props);
	while (loadprops(&props)) {
		putline(&props);
		ni_proplist_free(&props);
	}
}

static void
printlist(ni_namelist *nl)
{
	int i;

	for (i = 0; i < nl->ninl_len; i++) {
		printf("%s ", nl->ninl_val[i]);
	}
}


static void
printval(ni_proplist *props, char *name)
{
	int i;

	for (i = 0; i < props->nipl_len; i++) {
		if (ni_name_match(props->nipl_val[i].nip_name, name)) {
			if (props->nipl_val[i].nip_val.ninl_len == 0) {
				return;
			}
			printlist(&props->nipl_val[i].nip_val);
			return;
		}
	}
}

static void
putline(ni_proplist *props)
{
	
	printval(props, "ip_address");
	printf("\t");
	printval(props, "name");
	printf("\n");
}



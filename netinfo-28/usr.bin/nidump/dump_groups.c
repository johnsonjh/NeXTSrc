/*
 * /etc/group format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static char *field_name[] = {
	"name",
	"passwd",
	"gid",
	"users",
};
#define NGROUP_FIELDS (sizeof(field_name)/sizeof(field_name[0]))


static void putline(ni_proplist *);

void
dump_groups(void)
{
	ni_proplist props;

	NI_INIT(&props);
	while (loadprops(&props)) {
		putline(&props);
		ni_proplist_free(&props);
	}
}

static void
printit(ni_namelist *nl)
{
	ni_index j;

	for (j = 0; j < nl->ninl_len; j++) {
		printf("%s%s", nl->ninl_val[j],
		       j + 1 == nl->ninl_len ? "" : ",");
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
			printit(&props->nipl_val[i].nip_val);
		}
	}
	return;
}


static void
putline(ni_proplist *props)
{
	int i;

	for (i = 0; i < NGROUP_FIELDS; i++) {
		printval(props, field_name[i]);
		printf((i + 1 == NGROUP_FIELDS) ? "" : ":");
	}
	printf("\n");
}

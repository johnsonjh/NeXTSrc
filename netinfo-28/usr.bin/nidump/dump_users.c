/*
 * /etc/passwd format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static char *field_name[] = {
	"name",
	"passwd",
	"uid",
	"gid",
	"realname",
	"home",
	"shell"
};
#define NPASSWD_FIELDS (sizeof(field_name)/sizeof(field_name[0]))

static void putline(ni_proplist *);

void
dump_users(void)
{
	ni_proplist props;

	NI_INIT(&props);
	while (loadprops(&props)) {
		putline(&props);
		ni_proplist_free(&props);
	}
}

static char *
getval(ni_proplist *props, char *name)
{
	int i;

	for (i = 0; i < props->nipl_len; i++) {
		if (ni_name_match(props->nipl_val[i].nip_name, name)) {
			if (props->nipl_val[i].nip_val.ninl_len == 0) {
				return ("");
			}
			return(props->nipl_val[i].nip_val.ninl_val[0]);
		}
	}
	return ("");
}


static void
putline(ni_proplist *props)
{
	int i;

	for (i = 0; i < NPASSWD_FIELDS; i++) {
		printf("%s%s", getval(props, field_name[i]),
		       (i + 1 == NPASSWD_FIELDS) ? "" : ":");
	}
	printf("\n");
}

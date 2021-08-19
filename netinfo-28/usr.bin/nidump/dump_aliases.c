/*
 * /usr/lib/aliases format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

#define TABSIZE 8
#define MAXLEN 70

static void putline(ni_proplist *);

void
dump_aliases(void)
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
	ni_namelist *name;
	ni_namelist *members;
	ni_index i;
	int len;
	int inclen;

	name = getval(props, "name");
	if (name == NULL || name->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	printf("%s: ", name->ninl_val[0]);
	len = strlen(name->ninl_val[0]) + 2;
	members = getval(props, "members");
	if (members == NULL || name->ninl_len == 0) {
		printf("\n");
		return;
	}
	for (i = 0; i < members->ninl_len; i++) {
		inclen = strlen(members->ninl_val[i]);
		if (len + inclen > MAXLEN) {
			printf("\n\t");
			len = TABSIZE + inclen;
		} else {
			len += inclen;
		}
		printf("%s", members->ninl_val[i]);
		if (i + 1 < members->ninl_len) {
			printf(",");
			len += 1;
		}
	}
	printf("\n");
}


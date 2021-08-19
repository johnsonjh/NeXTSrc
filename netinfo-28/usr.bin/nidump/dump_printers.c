/*
 * /etc/printcap format dumper
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
dump_printers(void)
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

static int
lenof(ni_property prop)
{
	int len;

	len = strlen(prop.nip_name);
	if (prop.nip_val.ninl_len > 0) {
		if (*prop.nip_val.ninl_val[0] == '#') {
			len += strlen(prop.nip_val.ninl_val[0]);
		} else {
			len += strlen(prop.nip_val.ninl_val[0]) + 1;
		}
	}
	return (len);
}

static void
putline(ni_proplist *props)
{
	ni_namelist *name;
	ni_index i;
	int len;
	int inclen;

	name = getval(props, "name");
	if (name == NULL || name->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	for (i = 0; i < name->ninl_len; i++) {
		printf("%s", name->ninl_val[i]);
		if (i == name->ninl_len - 1) {
			printf(":");
		} else {
			printf("|");
		}
	}
	printf(" \\\n\t:");
	len = TABSIZE;
	for (i = 0; i < props->nipl_len; i++) {
		if (ni_name_match(props->nipl_val[i].nip_name, "name")) {
			continue;
		}
		inclen = lenof(props->nipl_val[i]);
		if (len + inclen > MAXLEN) {
			printf(" \\\n\t:");
			len = TABSIZE + inclen;
		} else {
			len += inclen;
		}
		printf("%s", props->nipl_val[i].nip_name);
		if (props->nipl_val[i].nip_val.ninl_len > 0) {
			if (*props->nipl_val[i].nip_val.ninl_val[0] == '#') {
				printf("%s", 
				       props->nipl_val[i].nip_val.ninl_val[0]);
			} else {
				printf("=%s", 
				       props->nipl_val[i].nip_val.ninl_val[0]);
			}
		}
		printf(":");
	}
	printf("\n");
}


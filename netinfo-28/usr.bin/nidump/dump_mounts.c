/*
 * /etc/fstab format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern int loadprops(ni_proplist *);

static void putline(ni_proplist *);

void
dump_mounts(void)
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
	ni_namelist *dir;
	ni_namelist *field;
	ni_index i;

	name = getval(props, "name");
	dir = getval(props, "dir");
	if (name == NULL || dir == NULL || name->ninl_len == 0 ||
	    dir->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	printf("%s %s ", name->ninl_val[0], dir->ninl_val[0]);
	if ((field = getval(props, "type")) && field->ninl_len > 0) {
		printf("%s ", field->ninl_val[0]);
	} else {
		printf("nfs ");
	}
	if ((field = getval(props, "opts")) && field->ninl_len > 0) {
		for (i = 0; i < field->ninl_len; i++) {
			printf("%s", field->ninl_val[i]);
			if (i + 1 < field->ninl_len) {
				printf(",");
			} else {
				printf(" ");
			}
		}
	} else {
		printf("rw ");
	}
	if ((field = getval(props, "freq")) && field->ninl_len > 0) {
		printf("%s ", field->ninl_val[0]);
	} else {
		printf("0 ");
	}
	if ((field = getval(props, "passno")) && field->ninl_len > 0) {
		printf("%s ", field->ninl_val[0]);
	} else {
		printf("0 ");
	}
	printf("\n");
}

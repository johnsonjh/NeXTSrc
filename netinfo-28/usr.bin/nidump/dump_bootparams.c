/*
 * /etc/bootparams format dumper
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

#define TABSIZE 8
#define MAXLEN 70

extern int loadprops(ni_proplist *);

static void putline(ni_proplist *);

void
dump_bootparams(void)
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
	ni_namelist *bp;
	ni_namelist *name;
	ni_index i;
	int len;
	int inclen;

	bp = getval(props, "bootparams");
	if (bp == NULL) {
		return;
	}
	name = getval(props, "name");
	if (name == NULL || bp == NULL ||
	    name->ninl_len == 0 || bp->ninl_len == 0) {
		printf("#unknown\n");
		return;
	}
	printf("%s ", name->ninl_val[0]);
	len = strlen(name->ninl_val[0]) + 1;
	for (i = 0; i < bp->ninl_len; i++) {
		inclen = strlen(bp->ninl_val[i]) + 1;
		if (inclen + len > MAXLEN) {
			printf("\\\n\t");
			len = TABSIZE + inclen;
		} else {
			len += inclen;
		}
		printf("%s ", bp->ninl_val[i]);
	}
	printf("\n");
}

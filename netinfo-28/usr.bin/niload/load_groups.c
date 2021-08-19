/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

static char *field_name[] = {
	"name",
	"passwd",
	"gid",
	"users",
};
#define NGROUP_FIELDS (sizeof(field_name)/sizeof(field_name[0]))

extern void loadprops(ni_proplist *, ni_namelist);
static int getname(char **, ni_name *);
static int getnamelist(char **, ni_namelist *);

void
load_groups(void)
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_namelist deletions;
	ni_name name;
	int i;

	NI_INIT(&deletions);
	while (gets(line) != NULL) {
		if (*line == '+' || *line == '-' || *line == '#') {
			/*
			 * YP crud: ignore
			 */
			continue;
		}
		s = line;
		NI_INIT(&props);
		for (i = 0; i < NGROUP_FIELDS - 1; i++) {
			NI_INIT(&prop);
			if (getname(&s, &name)) {
				prop.nip_name = ni_name_dup(field_name[i]);
				ni_namelist_insert(&prop.nip_val, name, 
						NI_INDEX_NULL);
				ni_name_free(&name);
				ni_proplist_insert(&props, prop, NI_INDEX_NULL);
				ni_prop_free(&prop);
			} else {
				ni_namelist_insert(&deletions,
						   field_name[i],
						   NI_INDEX_NULL);
			}
		}
		NI_INIT(&prop);
		if (getnamelist(&s, &prop.nip_val)) {
			prop.nip_name = ni_name_dup(field_name[i]);
			ni_proplist_insert(&props, prop, NI_INDEX_NULL);
			ni_prop_free(&prop);
		}
		loadprops(&props, deletions);
		ni_proplist_free(&props);
		ni_namelist_free(&deletions);
	}
}

static int
getname(
	char **where,
	ni_name *name
	)
{
	char *p;
	char *s;

	s = *where;
	if (*s == 0) {
		return (0);
	}
	p = index(s, ':');
	if (p == NULL) {
		for (p = s; *p; p++) {
		}
	} else {
		*p++ = 0;
	}
	*where = p;
	*name = ni_name_dup(s);
	return (1);
}


static int
getnamelist(
	    char **where,
	    ni_namelist *nl
	    )
{
	char *p;
	char *s;
	ni_name name;
	int done;

	s = *where;
	if (*s == 0) {
		return (0);
	}
	for (done = 0; !done; ) {
		p = index(s, ',');
		if (p == NULL) {
			for (p = s; *p; p++) {
			}
			done++;
		} else {
			*p++ = 0;
		}
		*where = p;
		name = ni_name_dup(s);
		ni_namelist_insert(nl, name, NI_INDEX_NULL);
		ni_name_free(&name);
		s = *where;
	}
	return (1);
}



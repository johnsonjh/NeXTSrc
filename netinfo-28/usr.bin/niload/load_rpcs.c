/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern void fatal(char *);
extern void loadprops(ni_proplist *, ni_namelist);
static int commentline(char *);
static int getname(char **, ni_name *);

static ni_namelist null_deletions;

void
load_rpcs(void)
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_name name;
	ni_name addr;

	while (gets(line) != NULL) {
		if (commentline(line)) {
			continue;
		}
		s = line;
		NI_INIT(&props) ;
		NI_INIT(&prop) ;
		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		if (!getname(&s, &addr)) {
			fatal("corrupted input");
		}
		prop.nip_name = ni_name_dup("number");
		ni_namelist_insert(&prop.nip_val, addr, NI_INDEX_NULL);
		ni_name_free(&addr);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);

		NI_INIT(&prop) ;
		prop.nip_name = ni_name_dup("name");
		do {
			ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
			ni_name_free(&name);
		} while (getname(&s, &name));
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);
		loadprops(&props, null_deletions);
		ni_proplist_free(&props);
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
	char *q;

	s = *where;
	while (*s == ' ' || *s == '\t') {
		s++;
	}
	if (*s == 0 || *s == '#') {
		return (0);
	}
	p = index(s, ' ');
	q = index(s, '\t');
	if (p == NULL || (q != NULL && p > q)) {
		p = q;
	}
	if (p == NULL) {
		for (p = s; *p != 0 && *p != '#'; p++) {
		}
		*p = 0;
	} else {
		*p++ = 0;
	}
	*where = p;
	*name = ni_name_dup(s);
	return (1);
}

static int
commentline(
	    char *line
	    )
{
	while (*line == ' ' || *line == '\t') {
		line++;
	}
	return (*line == 0 || *line == '#');
}

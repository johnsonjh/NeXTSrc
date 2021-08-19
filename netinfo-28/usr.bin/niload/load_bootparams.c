/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"


extern void fatal(char *);
extern void loadprops(ni_proplist *, ni_namelist);
static int getname(char **, ni_name *);
static char *getline(void);

static ni_namelist null_deletions;

void
load_bootparams(void)
{
	char *line;
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_name name;
	
	while ((line = getline()) != NULL) {
		s = line;
		NI_INIT(&props);
		NI_INIT(&prop);
		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		prop.nip_name = ni_name_dup("name");
		ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
		ni_name_free(&name);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);
		
		NI_INIT(&prop);
		prop.nip_name = ni_name_dup("bootparams");
		while (getname(&s, &name)) {
			ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
		}
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		loadprops(&props, null_deletions);
		ni_proplist_free(&props);
		ni_name_free(&line);
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

static char *
getline(
	void
	)
{
	char line[BUFSIZ];
	char *res = NULL;
	int more = 1;
	int len;
	int inclen;


	len = 0;
	while (more && gets(line)) {
		if (*line == '#') {
			continue;
		}
		inclen = strlen(line);
		if (res == NULL) {
			res = malloc(inclen + 1);
		} else {
			res = realloc(res, len + inclen + 1);
		}
		if (line[inclen - 1] == '\\') {
			line[inclen - 1] = 0;
			inclen--;
		} else {
			more = 0;
		}
		bcopy(line, res + len, inclen);
		len += inclen;
		res[len] = 0;
	}
	return (res);
}

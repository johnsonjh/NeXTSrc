/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

static char *field_name[] = {
	"name",
	"dir",
	"type",
	"opts",
	"freq",
	"passno"
};
#define NMOUNT_FIELDS (sizeof(field_name)/sizeof(field_name[0]))

#define standardval(key, val) 	\
	((ni_name_match(key, "type") && ni_name_match(val, "nfs")) || \
	 (ni_name_match(key, "opts") && ni_name_match(val, "rw")) || \
	 (ni_name_match(key, "freq") && ni_name_match(val, "0")) || \
	 (ni_name_match(key, "passno") && ni_name_match(val, "0"))) 




extern void fatal(char *);
extern void loadprops(ni_proplist *, ni_namelist);
static int commentline(char *);
static int getname(char **, ni_name *);

void
load_mounts(void)
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_name name;
	ni_namelist deletions;
	int i;
	char *p;
	char *end;

	while (gets(line) != NULL) {
		if (commentline(line)) {
			continue;
		}
		s = line;
		NI_INIT(&props);
		NI_INIT(&deletions);
		for (i = 0; i < NMOUNT_FIELDS; i++) {
			NI_INIT(&prop);
			prop.nip_name = ni_name_dup(field_name[i]);
			if (!getname(&s, &name)) {
				fatal("corrupted input");
			}
			if (standardval(field_name[i], name)) {
				ni_namelist_insert(&deletions, name, 
						   NI_INDEX_NULL);
				ni_name_free(&name);
				continue;
			}
			if (ni_name_match(field_name[i], "opts")) {
				for (p = name; end = index(p, ','); p = end) {
					*end++ = 0;
					ni_namelist_insert(&prop.nip_val,
							p, NI_INDEX_NULL);
				} 
				ni_namelist_insert(&prop.nip_val, p, 
						NI_INDEX_NULL);
			}  else {
				ni_namelist_insert(&prop.nip_val, name, 
						NI_INDEX_NULL);
			}
			ni_proplist_insert(&props, prop, NI_INDEX_NULL);
			ni_prop_free(&prop);
			ni_name_free(&name);
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

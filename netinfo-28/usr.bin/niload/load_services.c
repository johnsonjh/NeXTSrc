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

typedef struct seennode *seenlist;
typedef struct seennode {
	ni_proplist props;
	seenlist next;
} seennode;

int merge(ni_proplist *, ni_proplist *);

static ni_namelist null_deletions;

void
load_services(void)
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_name name;
	ni_name addr;
	ni_name prot;
	char *p;
	seenlist seen = NULL;
	seenlist *sp;
	
	while (gets(line) != NULL) {
		if (commentline(line)) {
			continue;
		}
		s = line;
		NI_INIT(&props);
		NI_INIT(&prop);
		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		p = index(s, '/');
		if (p == NULL) {
			fatal("corrupted input");
		}
		*p = ' ';
		if (!getname(&s, &addr)) {
			fatal("corrupted input");
		}
		prop.nip_name = ni_name_dup("port");
		ni_namelist_insert(&prop.nip_val, addr, NI_INDEX_NULL);
		ni_name_free(&addr);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);

		NI_INIT(&prop);
		if (!getname(&s, &prot)) {
			fatal("corrupted input");
		}
		prop.nip_name = ni_name_dup("protocol");
		ni_namelist_insert(&prop.nip_val, prot, NI_INDEX_NULL);
		ni_name_free(&prot);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);

		NI_INIT(&prop);
		prop.nip_name = ni_name_dup("name");
		do {
			ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
			ni_name_free(&name);
		} while (getname(&s, &name));
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);
		for (sp = &seen; (*sp) != NULL; sp = &(*sp)->next) {
			if (merge(&props, &(*sp)->props)) {
				ni_proplist_free(&props);
				break;
			}
		}
		if (*sp == NULL) {
			*sp = (void *)malloc(sizeof(**sp));
			(*sp)->props = props;
			(*sp)->next = NULL;
		}
	}
	while (seen != NULL) {
		loadprops(&seen->props, null_deletions);
		seen = seen->next;
	}
}

ni_index
proplist_index(
	       ni_proplist *props,
	       ni_name name
	       )
{
	int i;

	for (i = 0; i < props->nipl_len; i++) {
		if (ni_name_match(props->nipl_val[i].nip_name, name)) {
			return (i);
		}
	}
	return (NI_INDEX_NULL);
}

/*
 * If the ports are the same and the names are the same, then
 * merge the two protocols into one single list.
 */
int
merge(
      ni_proplist *props,
      ni_proplist *seen
      )
{
	ni_index pi1;
	ni_index pi2;

	pi1 = proplist_index(props, "port");
	pi2 = proplist_index(seen, "port");
	if (props->nipl_val[pi1].nip_val.ninl_len == 0 ||
	    seen->nipl_val[pi2].nip_val.ninl_len == 0 ||
	    !ni_name_match(props->nipl_val[pi1].nip_val.ninl_val[0],
			seen->nipl_val[pi2].nip_val.ninl_val[0])) {
		return (0);
	}

	pi1 = proplist_index(props, "name");
	pi2 = proplist_index(seen, "name");
	if (props->nipl_val[pi1].nip_val.ninl_len == 0 ||
	    seen->nipl_val[pi2].nip_val.ninl_len == 0 ||
	    !ni_name_match(props->nipl_val[pi1].nip_val.ninl_val[0],
			seen->nipl_val[pi2].nip_val.ninl_val[0])) {
		return (0);
	}

	pi1 = proplist_index(seen, "protocol");
	pi2 = proplist_index(props, "protocol");
	ni_namelist_insert(&seen->nipl_val[pi1].nip_val,
			props->nipl_val[pi2].nip_val.ninl_val[0], NI_INDEX_NULL);
	return (1);
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

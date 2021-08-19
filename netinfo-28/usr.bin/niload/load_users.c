/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern void loadprops(ni_proplist *, ni_namelist);

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

static int getname(char **, ni_name *, int);
static void malformed(char *);

void
load_users(
	   void
	   )
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_property editprop = { "_writers_passwd" };
	ni_namelist deletions;
	ni_name name;
	int i;
	int ignore;

	NI_INIT(&editprop.nip_val);
	while (gets(line) != NULL) {
		if (*line == '+' || *line == '-' || *line == '#') {
			/*
			 * YP crud or comment: ignore
			 */
			continue;
		}
		ignore = 0;
		s = line;
		NI_INIT(&props);
		NI_INIT(&deletions);
		for (i = 0; !ignore && i < NPASSWD_FIELDS; i++) {
			NI_INIT(&prop);
			switch (getname(&s, &name, i == NPASSWD_FIELDS - 1)) {
			case -1:
				/*
				 * A malformed entry
				 */
				malformed(line);
				ignore++;
				break;
			case 0:
				if (ni_name_match(field_name[i], "passwd")) {
					prop.nip_name = "passwd";
					ni_proplist_insert(&props, prop,
							   NI_INDEX_NULL);
					prop.nip_name = NULL;
				} else if (ni_name_match(field_name[i], 
							 "uid") ||
					   ni_name_match(field_name[i], 
							 "gid") ||
					   ni_name_match(field_name[i], 
							 "name")) {
					/*
					 * It makes no sense to load
					 * an entry with a NULL username,
					 * a uid or a gid.
					 */
					malformed(line);
					ignore++;
					break;
				} else {
					ni_namelist_insert(&deletions, 
							   field_name[i],
							   NI_INDEX_NULL);
				}
				break;
			default:
				prop.nip_name = ni_name_dup(field_name[i]);
				if (ni_name_match(field_name[i], "name")) {
					ni_namelist_free(&editprop.nip_val);
					ni_namelist_insert(&editprop.nip_val,
							   name, 
							   NI_INDEX_NULL);
				}
				ni_namelist_insert(&prop.nip_val, name,
						NI_INDEX_NULL);
				ni_name_free(&name);
				ni_proplist_insert(&props, prop,
						NI_INDEX_NULL);
				ni_prop_free(&prop);
				break;
			}
		}
		if (!ignore) {
			ni_proplist_insert(&props, editprop, NI_INDEX_NULL);
			loadprops(&props, deletions);
		}
		ni_proplist_free(&props);
		ni_namelist_free(&deletions);
	}
}

static int
getname(
	char **where,
	ni_name *name,
	int wantlast
	)
{
	char *p;
	char *s;
	char *save;
	char savechar;
	int islast;

	islast = 0;
	s = *where;
	if (*s == 0) {
		return (0);
	}
	p = index(s, ':');
	if (p == NULL) {
		for (p = s; *p; p++) {
		}
		islast++;
		save = p;
		savechar = 0;
	} else {
		save = p;
		savechar = ':';
		*p++ = 0;
	}
	*where = p;
	if (p - s > 1) {
		*name = ni_name_dup(s);
		*save = savechar;
		return ((!wantlast && islast) ? -1 : 1);
	} else {
		*save = savechar;
		return ((!wantlast && islast) ? -1 : 0);
	}
}


static void
malformed(
	  char *line
	  )
{
	fprintf(stderr, "malformed line: <%s>\n", line);
}

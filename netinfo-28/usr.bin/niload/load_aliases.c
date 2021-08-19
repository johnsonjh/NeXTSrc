/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <ctype.h>
#include "clib.h"

#define ungetchar(c) ungetc(c, stdin)

extern void loadprops(ni_proplist *, ni_namelist);

static char *getline(void);

static ni_namelist null_deletions;
static ni_namelist checkforupcase(ni_name);

void
load_aliases(
	     void
	     )
{
	ni_proplist props;
	ni_property prop;
	char *line;
	char *end;
	char *p;
	char *where;

	while (line = getline()) {
		NI_INIT(&props) ;
		prop.nip_name = "name";
		NI_INIT(&prop.nip_val) ;
		where = line;
		end = index(where, ':');
		if (end == NULL) {
			continue;
		}
		*end++ = 0;  
		while (*end == ' ' || *end == '\t') {
			*end++ = 0;
		}
		prop.nip_val = checkforupcase(where);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_namelist_free(&prop.nip_val);
		where = end;
		NI_INIT(&prop) ;
		prop.nip_name = ni_name_dup("members");
		while (where != NULL) {
		  	if (*where == '"') {
			  	end = index(where + 1, '"');
				if (end != NULL) {
					end = index(end + 1, ',');
				}
			} else {
				end = index(where, ',');
			}
			if (end != NULL) {
				 


			  	p = end - 1;
				while (*p == ' ' || *p == '\t') {
					p--;
				}
				*++p = 0;	 
				end++;  
				 


				while (*end == ' ' || *end == '\t') {
					end++;
				}
			}
			ni_namelist_insert(&prop.nip_val, where, NI_INDEX_NULL);
			where = end;
		}
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);
		loadprops(&props, null_deletions);
		ni_proplist_free(&props);
		free(line);
	}
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
	int c;

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
		c = getchar();
		ungetchar(c);
		if (c != ' ' && c != '\t') {
			more = 0;
		}
		bcopy(line, res + len, inclen);
		len += inclen;
		res[len] = 0;
	}
	return (res);
}


int
hasupcase(
	  ni_name name
	  )
{
	char *p;
	int changed = 0;

	for (p = name; *p; p++) {
		if (isupper(*p)) {
			*p = tolower(*p);
			changed++;
		}
	}
	return (changed);
}

static ni_namelist
checkforupcase(
	       ni_name name
	       )
{
	ni_namelist nl;
	ni_name tmp;

	NI_INIT(&nl);
	ni_namelist_insert(&nl, name, NI_INDEX_NULL);
	tmp = ni_name_dup(name);
	if (hasupcase(tmp)) {
		ni_namelist_insert(&nl, tmp, NI_INDEX_NULL);
	}
	ni_name_free(&tmp);
	return (nl);
}

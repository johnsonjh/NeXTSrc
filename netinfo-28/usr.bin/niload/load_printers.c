/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include "clib.h"

extern void fatal(char *);
extern void loadprops(ni_proplist *, ni_namelist);
static char *getline(void);
static int emptyfield(char *);

static void deletions_init(ni_namelist *);
static void deletions_delete(ni_namelist *, ni_name);

void
load_printers(
	      void
	      )
{
	ni_proplist props;
	ni_property prop;
	char *line;
	char *end;
	char *where;
	char *p;
	char *hash;
	char *equal;
	ni_namelist deletions;

	while (line = getline()) {
		if (*line == 0) {
			continue;
		}
		NI_INIT(&props) ;
		prop.nip_name = "name";
		NI_INIT(&prop.nip_val) ;
		where = line;
		end = index(where, ':');
		if (end == NULL) {
			continue;
		}
		*end++ = 0;
		for (;;) {
			p = index(where, '|');
			if (p != NULL && (end == NULL || p < end)) {
				*p++ = 0;
				ni_namelist_insert(&prop.nip_val, where, 
						NI_INDEX_NULL);
				where = p;
			} else {
				ni_namelist_insert(&prop.nip_val, where,
						NI_INDEX_NULL);
				break;
			}
		}
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_namelist_free(&prop.nip_val);
		where = end;
		deletions_init(&deletions);
		while (where != NULL) {
			end = index(where, ':');
			if (end != NULL) {
				*end++ = 0;
			}
			hash = index(where, '#');
			equal = index(where, '=');
			if (hash != NULL && (end == NULL || hash < end)) {
				*hash = 0;
				prop.nip_name = ni_name_dup(where);
				*hash = '#';
				prop.nip_val.ninl_val = &hash;
				prop.nip_val.ninl_len = 1;
				ni_proplist_insert(&props, prop, 
						   NI_INDEX_NULL);
				deletions_delete(&deletions, prop.nip_name);
				ni_name_free(&prop.nip_name);
			} else if (equal != NULL && (end == NULL || 
						     equal < end)) {
				*equal++ = 0;
				prop.nip_name = where;
				prop.nip_val.ninl_len = 1;
				prop.nip_val.ninl_val = &equal;
				ni_proplist_insert(&props, prop, 
						   NI_INDEX_NULL);
				deletions_delete(&deletions, prop.nip_name);
			} else if (!emptyfield(where)) {
				prop.nip_name = where;
				prop.nip_val.ninl_len = 0;
				ni_proplist_insert(&props, prop, 
						   NI_INDEX_NULL);
				deletions_delete(&deletions, prop.nip_name);
			}
			where = end;
		}
		loadprops(&props, deletions);
		ni_proplist_free(&props);
		ni_namelist_free(&deletions);
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

static int
emptyfield(
	   char *line
	   )
{
	while (*line) {
		if (*line != ' ' && *line != '\t') {
			return (0);
		}
		line++;
	}
	return (1);
}


static char *printcap_keys[] = {
	"af",	"br",	"cf",	"df",
	"fc",	"ff",	"fo",	"fs",
	"gf",	"hl",	"ic",	"if",
	"lf",	"lo",	"lp",	"mx",
	"nd",	"nf",	"of",	"pc",
	"pl",	"pw",	"px",	"py",
	"rf",	"rg",	"rm",	"rp",
	"rs",	"rw",	"sb",	"sc",
	"sd",	"sf",	"sh",	"st",
	"tf",	"tr",	"vf",	"xc",
	"xs",   "mf",	"re",	"ty",
};
#define NPRINTCAP_KEYS (sizeof(printcap_keys)/sizeof(printcap_keys[0]))	

const ni_namelist printcap_keylist = {
	NPRINTCAP_KEYS,
	printcap_keys
};


static void
deletions_init(
	       ni_namelist *deletions
	       )
{
	*deletions = ni_namelist_dup(printcap_keylist);
}

static void
deletions_delete(
		ni_namelist *deletions,
		ni_name what
		)
{
	ni_index i;

	i = ni_namelist_match(*deletions, what);
	if (i != NI_INDEX_NULL) {
		ni_namelist_delete(deletions, i);
	}
}

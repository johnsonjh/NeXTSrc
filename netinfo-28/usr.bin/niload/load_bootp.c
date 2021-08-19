/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include "clib.h"

extern struct ether_addr *ether_aton(char *);
extern char *ether_ntoa(struct ether_addr *);

extern void fatal(char *);
extern void loadprops(ni_proplist *, ni_namelist);
static int commentline(char *);
static int getname(char **, ni_name *);

void
load_bootp(void)
{
	char line[BUFSIZ];
	char *s;
	ni_proplist props;
	ni_property prop;
	ni_name name;
	ni_namelist deletions;
	struct ether_addr *en;

	while (gets(line) != NULL) {
		if (commentline(line)) {
			continue;
		}
		s = line;
		NI_INIT(&props);
		NI_INIT(&deletions);
		NI_INIT(&prop);
		prop.nip_name = ni_name_dup("name");
		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
		ni_name_free(&name);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);

		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		if (atoi(name) != 1) {
			ni_name_free(&name);
			fprintf(stderr, "%s: htype != 1\n", 
				props.nipl_val[0].nip_val.ninl_val[0]);
			ni_proplist_free(&props);
			continue;
		}
		ni_name_free(&name);

		if (!getname(&s, &name)) {
			fatal("corrupted input");
		}
		en = ether_aton(name);
		ni_name_free(&name);
		if (en == NULL) {
			fprintf(stderr, "%s: invalid ethernet address\n",
				props.nipl_val[0].nip_val.ninl_val[0]);
			ni_proplist_free(&props);
			continue;
		}
		ni_name_free(&name);
		name = ni_name_dup(ether_ntoa(en));
		NI_INIT(&prop);
		prop.nip_name = ni_name_dup("en_address");
		ni_namelist_insert(&prop.nip_val, name, NI_INDEX_NULL);
		ni_name_free(&name);
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		ni_prop_free(&prop);

		if (getname(&s, &name)) {
			ni_name_free(&name);

			if (getname(&s, &name)) {
				NI_INIT(&prop);
				prop.nip_name = ni_name_dup("bootfile");
				ni_namelist_insert(&prop.nip_val, 
						   name, NI_INDEX_NULL);
					   
				ni_name_free(&name);
				ni_proplist_insert(&props, prop,
						   NI_INDEX_NULL);
				ni_prop_free(&prop);
			} else {
				ni_namelist_insert(&deletions, "bootfile",
						   NI_INDEX_NULL);
			}
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

typedef enum bootp_state {
	BOOTP_WAITSECONDSECTION,
	BOOTP_WAITEOF
} bootp_state;
	
static int
commentline(
	    char *line
	    )
{
	static bootp_state state = BOOTP_WAITSECONDSECTION;
	char *p;

	for (p = line; *p == ' ' || *p == '\t'; p++) {
	}
	if (*p == 0 || *p == '#') {
		return (1);
	}
	if (state == BOOTP_WAITEOF) {
		return (0);
	}
	if (line[0] == '%' && line[1] == '%') {
		state = BOOTP_WAITEOF;
	}
	return (1);
}


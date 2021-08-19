/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#define SUN_RPC 1
#include <netinfo/ni.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/socket.h>
#include <ctype.h>
#include "clib.h"

const char ROOTUSER[] = "root";

typedef void niu_doit(void *, ni_id *, int, char **);
niu_doit niu_createprop;
niu_doit niu_destroyprop;
niu_doit niu_read;
niu_doit niu_list;
niu_doit niu_create;
niu_doit niu_destroy;
niu_doit niu_stats;

struct argstuffstruct {
	char *which;
	niu_doit (*doit);
	int nargs;
	int lock;
	char *message;
} argstuff[] = {
	"-createprop",  niu_createprop,  1, 1, "propkey propval1 propval2 ...",
	"-destroyprop", niu_destroyprop, 1, 1, "propkey",
	"-read",        niu_read, 	 0, 0, "",
	"-list",        niu_list, 	 0, 0, "[propkey]",
	"-create",      niu_create, 	 0, 1, "",
	"-destroy",     niu_destroy, 	 0, 1, "",
#ifdef notdef
	"-stats", niu_stats, 0, "",
#endif
};
#define ARGSTUFF_SIZE (sizeof(argstuff)/sizeof(argstuff[0]))

void usage(char *);
void *niu_open(char *, char *, ni_id *, int, int);
char *lastcomponent(char *);
char *unescape(char *);

void
main(
     int argc,
     char **argv
     )
{
	char *domain;
	char *myname;
	char *path;
	void *ni;
	int i;
	niu_doit (*doit);
	ni_id id;
	int tagged;
	char *password = NULL;
	int lock;
	ni_status status;
	int getpasswd = 0;

	myname = *argv++;
	argc--;
	doit = NULL;
	tagged = 0;
	lock = 0;
	for (; argc > 0 && **argv == '-'; argv++, argc--) {
		for (i = 0; i < ARGSTUFF_SIZE; i++) {
			if (strcmp(argstuff[i].which, *argv) == 0) {
				if (doit != NULL ||
				    (argc - 3 < argstuff[i].nargs)) {
					usage(myname);
				}
				doit = argstuff[i].doit;
				lock = argstuff[i].lock;
				break;
			}
		}
		if (i == ARGSTUFF_SIZE) {
			if (strcmp(*argv, "-t") == 0) {
				tagged++;
			} else if (strcmp(*argv, "-p") == 0) {
				getpasswd++;
			} else {
				usage(myname);
			}
		}
	}
	if (doit == NULL) {
		usage(myname);
	}
	if (getpasswd) {
		password = getpass("Password: ");
	}
	domain = argv[0];
	path = argv[1];
	argv += 2;
	argc -= 2;
	if (doit == niu_create) {
		char *p;

		p = lastcomponent(path);
		if (p - 1 == path) {
			path = "/";
		} else {
			/*
			 * Remove separator
			 */
			p[-1] = 0;
		}
		p = unescape(p);
		argv[0] = p;
	}
	ni = niu_open(domain, path, &id, tagged, lock);
	if (ni == NULL) {
		fprintf(stderr, "%s: can't open %s:%s\n", myname, domain, 
			path);
		exit(1);
	}
	if (password != NULL) {
		status = ni_setuser(ni, ROOTUSER);
		if (status != NI_OK) {
			fprintf(stderr, "cannot find user '%s': %s\n",
				ROOTUSER, ni_error(status));
			exit(1);
		}
		status = ni_setpassword(ni, password);
		if (status != NI_OK) {
			fprintf(stderr, "cannot set password: %s\n",
				ni_error(status));
			exit(1);

		}
	}
	doit(ni, &id, argc, argv);
	exit(0);
}

void
usage(
      char *myname
      )
{
	int i;

	fprintf(stderr, "usage: \n");
	for (i = 0; i < ARGSTUFF_SIZE; i++) {
		fprintf(stderr, "\t%s %s [-t] [-p] <domain> <path> %s\n",
			myname,
			argstuff[i].which,
			argstuff[i].message);
	}
	exit(1);
}

int
getaddr(
	char *host,
	struct sockaddr_in *sin
	)
{
	struct hostent *h;
	
	sin->sin_family = AF_INET;
	sin->sin_port = 0;
	bzero(sin->sin_zero, sizeof(sin->sin_zero));
	sin->sin_addr.s_addr = inet_addr(host);
	if (sin->sin_addr.s_addr != -1) {
		return (1);
	}
	h = gethostbyname(host);
	if (h != NULL) {
		bcopy(h->h_addr, &sin->sin_addr, sizeof(sin->sin_addr));
		return (1);
	}
	return (0);
}

void *
niu_tagopen(
	    char *host,
	    char *tag
	    )
{
	struct sockaddr_in sin;

	if (!getaddr(host, &sin)) {
		return (NULL);
	}
	return (ni_connect(&sin, tag));
}


void *
niu_open(
	 char *domain, 
	 char *path, 
	 ni_id *id,
	 int tagged,
	 int lock
	)
{
	void *ni;
	char *p;
	ni_fancyopenargs args;

	if (!tagged) {
		args.rtimeout = 0;
		args.wtimeout = 0;
		args.abort = 1;
		args.needwrite = lock;
		if (ni_fancyopen(NULL, domain, &ni, &args) != NI_OK) {
			return (NULL);
		}
	} else {
		p = rindex(domain, '/');
		if (p == NULL) {
			return (NULL);
		}
		*p = 0;
		ni = niu_tagopen(domain, p + 1);
		*p = '/';
	}
	if (ni == NULL) {
		return (NULL);
	}
	for (p = path; isdigit(*p); p++) {
	}
	if (*p == 0) {
		/*
		 * They are giving an ID instead of a path
		 */
		id->nii_object = atoi(path);
	} else {
		/*
		 * It's a real path
		 */
		if (ni_pathsearch(ni, id, path) != NI_OK) {
			return (NULL);
		}
	}
	return (ni);
}

void 
niu_createprop(
	       void *ni,
	       ni_id *id,
	       int argc,
	       char **argv
	       )
{
	ni_property prop;
	ni_status status;
	ni_namelist nl;
	ni_index which;

	status = ni_self(ni, id);
	if (status != NI_OK) {
		fprintf(stderr, "cannot refresh: %s\n", ni_error(status));
		exit(1);
	}
	NI_INIT(&prop);
	prop.nip_name = argv[0];
	argv++;
	argc--;
	while (argc > 0) {
		ni_namelist_insert(&prop.nip_val, *argv, NI_INDEX_NULL);
		argv++;
		argc--;
	}
	status = ni_listprops(ni, id, &nl);
	if (status != NI_OK) {
		fprintf(stderr, "cannot list props: %s\n", ni_error(status));
		exit(1);
	}
	which = ni_namelist_match(nl, prop.nip_name);
	if (which == NI_INDEX_NULL) {
		/*
		 * If no property, create new one
		 */
		status = ni_createprop(ni, id, prop, NI_INDEX_NULL);
	} else {
		/*
		 * If one already there, overwrite it 
		 */
		status = ni_writeprop(ni, id, which, prop.nip_val);
	}
	if (status != NI_OK) {
		fprintf(stderr, "cannot create: %s\n", ni_error(status));
		exit(1);
	}
	ni_namelist_free(&prop.nip_val);
}

void 
niu_destroyprop(
		void *ni,
		ni_id *id,
		int argc,
		char **argv
		)
{
	ni_status status;
	ni_namelist nl;
	ni_index i;

	status = ni_listprops(ni, id, &nl);
	if (status != NI_OK) {
		fprintf(stderr, "cannot list props: %s\n", ni_error(status));
		exit(1);
	}
	for (i = 0; i < nl.ninl_len; i++) {
		if (ni_name_match(nl.ninl_val[i], argv[0])) {
			break;
		}
	}
	if (i >= nl.ninl_len) {
		fprintf(stderr, "%s not found\n", argv[0]);
		exit(1);
	}
	status = ni_destroyprop(ni, id, i);
	if (status != NI_OK) {
		fprintf(stderr, "cannot destroyprop: %s\n", ni_error(status));
		exit(1);
	}
}

void
ni_proplist_print(
		  ni_proplist pl
		  )
{
	ni_index i;
	ni_index j;
	ni_namelist *nl;

	for (i = 0; i < pl.nipl_len; i++) {
		printf("%s: ", pl.nipl_val[i].nip_name);
		nl = &pl.nipl_val[i].nip_val;
		for (j = 0; j < nl->ninl_len; j++) {
			printf("%s ", nl->ninl_val[j]);
		}
		printf("\n");
	}
}

void 
niu_read(
	 void *ni,
	 ni_id *id,
	 int argc,
	 char **argv
	 )
{
	ni_status status;
	ni_proplist pl;

	status = ni_read(ni, id, &pl);
	if (status != NI_OK) {
		fprintf(stderr, "cannot read props: %s\n", ni_error(status));
		exit(1);
	}
	ni_proplist_print(pl);
	ni_proplist_free(&pl);
}

void 
niu_list(
	 void *ni,
	 ni_id *id,
	 int argc,
	 char **argv
	 )
{
	ni_entrylist entries;
	ni_index i;
	ni_index j;
	ni_status status;
	ni_namelist *nl;

	status = ni_list(ni, id, argc > 0 ? argv[0] : "name", &entries);
	if (status != NI_OK) {
		fprintf(stderr, "cannot list: %s\n", ni_error(status));
		exit(1);
	}
	for (i = 0; i < entries.niel_len; i++) {
		printf("%-8d ", entries.niel_val[i].id);
		if (entries.niel_val[i].names != NULL) {
			nl = entries.niel_val[i].names;
			for (j = 0; j < nl->ninl_len; j++) {
				printf("%s ", nl->ninl_val[j]);
			}
		}
		printf("\n");
	}
	ni_entrylist_free(&entries);
}

void 
niu_create(
	   void *ni,
	   ni_id *id,
	   int argc,
	   char **argv
	   )
{
	ni_status status;
	ni_id nid;
	ni_proplist pl;
	ni_property prop;

	status = ni_self(ni, id);
	if (status != NI_OK) {
		fprintf(stderr, "cannot refresh: %s\n", ni_error(status));
		exit(1);
	}
	NI_INIT(&pl);
	NI_INIT(&prop);
	prop.nip_name = "name";
	ni_namelist_insert(&prop.nip_val, argv[0], NI_INDEX_NULL);
	ni_proplist_insert(&pl, prop, NI_INDEX_NULL);
	status = ni_create(ni, id, pl, &nid, NI_INDEX_NULL);
	if (status != NI_OK) {
		fprintf(stderr, "cannot create: %s\n", ni_error(status));
		exit(1);
	}
}

void 
niu_destroy(
	    void *ni,
	    ni_id *id,
	    int argc,
	    char **argv
	    )
{
	ni_status status;
	ni_id pid;

	status = ni_parent(ni, id, &pid.nii_object);
	if (status != NI_OK) {
		fprintf(stderr, "cannot refresh: %s\n", ni_error(status));
		exit(1);
	}
	status = ni_self(ni, &pid);
	if (status != NI_OK) {
		fprintf(stderr, "cannot refresh: %s\n", ni_error(status));
		exit(1);
	}
	status = ni_destroy(ni, &pid, *id);
	if (status != NI_OK) {
		fprintf(stderr, "cannot destroy: %s\n", ni_error(status));
		exit(1);
	}
}

void
niu_stats(
	  void *ni,
	  ni_id *id,
	  int argc,
	  char **argv
	  )
{
	ni_proplist pl;
	ni_status status;

	status = ni_statistics(ni, &pl);
	if (status != NI_OK) {
		fprintf(stderr, "cannot get stats: %s\n", ni_error(status));
		exit(1);
	}
	ni_proplist_print(pl);
}



char *
lastcomponent(
	      char *path
	      )
{
	char *last;
	int escaping;
	char *p;

	last = NULL;
	escaping = 0;
	for (p = path; *p; p++) {
		if (escaping) {
			escaping = 0;
		} else {
			switch (*p) {
			case '\\':
				escaping = 1;
				break;
			case '/':
				last = p + 1;
				break;
			}
		}
	}
	if (last == NULL) {
		return (path);
	}
	return (last);
}

char *
unescape(
	 char *path
	 )
{
	char *p;
	char *s;
	int escaping;

	escaping = 0;
	s = path;
	for (p = path; *p; p++) {
		if (*p == '\\') {
			escaping = 1;
		} else {
			if (escaping) {
				escaping = 0;
			} 
			*s++ = *p;
		}
	}
	*s = 0;
	return (path);
}

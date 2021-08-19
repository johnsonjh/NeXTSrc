/*
 * nidump main
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <netinfo/ni.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "clib.h"

typedef void fvoid2void(void);
typedef fvoid2void *pfvoid2void;

pfvoid2void getconverter(char *, ni_name *);
void fatal(ni_status);

void usage(char *);

void *ni_tagopen(char *);

char *myname;

void *ni;
ni_idlist children;
ni_index which;

void
main(
     int argc,
     char **argv
     )
{
	ni_status status;
	ni_id root;
	ni_id maproot;
	ni_idlist ids;
	ni_name name;
	ni_name val;
	pfvoid2void doit;
	int tagged = 0;

	myname = argv[0];
	argc--;
	argv++;
	for (; argc > 0 && **argv == '-'; argv++, argc--) {
		if (strcmp(*argv, "-t") == 0) {
			tagged++;
		} else {
			usage(myname);
		}
	}
	if (argc != 2) {
		usage(myname);
	}
	doit = getconverter(argv[0], &val);
	if (doit == NULL) {
		usage(myname);
	}
	argc--;
	argv++;
	
	if (tagged) {
		ni = ni_tagopen(argv[0]);
		if (ni == NULL) {
			fprintf(stderr, 
				"cannot connect to tagged domain: %s\n",
				argv[0]);
			exit(1);
		}
		status = NI_OK;
	} else {
		status = ni_open(NULL, argv[0], &ni);
	}
	if (status != NI_OK) {
		fprintf(stderr, "cannot connect to netinfo server: %s\n",
			ni_error(status));
		exit(1);
	}
	status = ni_root(ni, &root);
	if (status != NI_OK) {
		fatal(status);
	}
	name = ni_name_dup("name");
	status = ni_lookup(ni, &root, name, val, &ids);
	if (status == NI_NODIR) {
		fprintf(stderr, "Directory '/%s' does not exist.\n", val);
		exit(1);
	}
	if (status != NI_OK) {
		fatal(status);
	}
	switch (ids.niil_len) {
	case 0:
		fprintf(stderr, "Directory '/%s' is empty.\n", val);
		exit(1);
	case 1:
		maproot.nii_object = ids.niil_val[0];
		status = ni_children(ni, &maproot, &children);
		if (status != NI_OK) {
			fatal(status);
		}
		break;
	default:
		printf("more than one database named %s, picking first",
		       argv[1]);
 		maproot.nii_object = ids.niil_val[0];
		status = ni_children(ni, &maproot, &children);
		if (status != NI_OK) {
			fatal(status);
		}
		break;
	}
	which = 0;
	(*doit)();
	exit(0);
}

int 
loadprops(
	  ni_proplist *props
	  )
{
	ni_id id;

	while (which < children.niil_len) {
		id.nii_object = children.niil_val[which];
		if (ni_read(ni, &id, props) == NI_OK) {
			which++;
			return (1);
		}
		which++;
	}
	return (0);
}

typedef struct convertmap {
	char *format;
	char *dirname;
	pfvoid2void converter;
} convertmap;


extern fvoid2void dump_users;
extern fvoid2void dump_groups;
extern fvoid2void dump_machines;
extern fvoid2void dump_networks;
extern fvoid2void dump_protocols;
extern fvoid2void dump_services;
extern fvoid2void dump_rpcs;
extern fvoid2void dump_printers;
extern fvoid2void dump_mounts;
extern fvoid2void dump_aliases;
extern fvoid2void dump_bootparams;
extern fvoid2void dump_bootp;


convertmap converters[] = {
	{ "aliases",	"aliases",	dump_aliases },
	{ "bootptab",	"machines",	dump_bootp },
	{ "bootparams",	"machines",	dump_bootparams },
	{ "fstab", 	"mounts", 	dump_mounts },
	{ "group",	"groups",   	dump_groups },
	{ "hosts",	"machines", 	dump_machines },
	{ "networks",	"networks", 	dump_networks },
	{ "passwd", 	"users", 	dump_users },
	{ "printcap", 	"printers",	dump_printers },
	{ "protocols",  "protocols", 	dump_protocols },
	{ "rpc", 	"rpcs", 	dump_rpcs },
	{ "services",   "services",	dump_services },
	{ NULL, NULL, NULL }
};


pfvoid2void
getconverter(char *format, char **dirname)
{
	convertmap *map;

	for (map = &converters[0]; map->format != NULL; map++) {
		if (strcmp(map->format, format) == 0) {
			*dirname = map->dirname;
			return (map->converter);
		}
	}
	return (NULL);
}


void
error(char *msg)
{
	fprintf(stderr, "%s: %s\n", myname, msg);
	exit(1);
}

void
fatal(ni_status status)
{
	fprintf(stderr, "%s: fatal error: %s\n", myname, ni_error(status));
	exit(1);
}
void
usage(char *myname)
{
	convertmap *map;

	fprintf(stderr, "usage: %s [-t] <format> <domain>\n", myname);
	fprintf(stderr, "<format> must be one of the following:\n");
	for (map = &converters[0]; map->format != NULL; map++) {
		fprintf(stderr, "\t%s\n", map->format);
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
ni_hosttagopen(
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
ni_tagopen(
	   char *host_tag
	   )
{
	char *p;
	void *ni;

	p = rindex(host_tag, '/');
	if (p == NULL) {
		fprintf(stderr, "no slash character (/) in tagged domain\n");
		return (NULL);
	}
	*p = 0;
	ni = ni_hosttagopen(host_tag, p + 1);
	*p = '/';
	return (ni);
}

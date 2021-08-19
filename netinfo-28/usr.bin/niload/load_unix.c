/*
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#define SUN_RPC 1
#include <netinfo/ni.h>
#include <sys/socket.h>
#include "clib.h"

/* 
 * Set long timeout, writes can take a while when master is 
 * busy transferring database to a clone.
 */
#define LONGTIMEOUT 60*60	/* one hour timeout */

#ifdef notdef
const char LOAD_SERVICES_MAGIC[] = "Just the address is important";
#endif
const char ROOTUSER[] = "root";
const char NI_NAME_NAME[] = "name";
const char NI_NAME_PROTOCOL[] = "protocol";
#ifdef notdef
const char NI_NAME_SERVICES[] = "services";
#endif

typedef void fvoid2void(void);
typedef fvoid2void *pfvoid2void;

pfvoid2void getconverter(char *, ni_name *);
void usage(char *);

ni_status replace(void *, ni_id *, ni_proplist *, ni_namelist, ni_index);
void usage(char *);
void ni_fatal(ni_status);
void deletion_failed(ni_status, ni_entry);
void update_entries(ni_namelist);

void *ni;
ni_id maproot;
char *uniq = (char *)NI_NAME_NAME;
int verbose = 0;
char *myname;
int deletions = 0;
ni_entrylist entries;

void
main(
     int argc,
     char **argv
     )
{
	ni_status status;
	ni_id root;
	ni_name name;
	ni_name val;
	ni_idlist ids;
	ni_property prop;
	ni_proplist props;
	char *dir;
	char *domain;
	pfvoid2void doit;
	char *password = NULL;
	ni_index i;
	ni_id id;
	int setuniq = 0;
	ni_fancyopenargs args;

	myname = *argv;
	argc--; argv++;
	while (argc > 0 && **argv == '-') {
		if (strcmp(*argv, "-u") == 0) {
			argc--; argv++;
			if (argc == 0) {
				usage(myname);
			}
			setuniq++;
			uniq = *argv;
		} else if (strcmp(*argv, "-v") == 0) {
			verbose++;
		} else if (strcmp(*argv, "-p") == 0) {
			password = getpass("Password: ");
		} else if (strcmp(*argv, "-d") == 0) {
			deletions++;
		} else {
			usage(myname);
		}
		argc--; argv++;
	}
	if (argc != 2) {
		usage(myname);
	}
	domain = argv[1];
	doit = getconverter(argv[0], &dir);
	if (doit == NULL) {
		usage(myname);
	}
	args.rtimeout = LONGTIMEOUT;
	args.wtimeout = LONGTIMEOUT;
	args.abort = 1;
	args.needwrite = 1;
	status = ni_fancyopen(NULL, domain, &ni, &args);
	if (status != NI_OK) {
		fprintf(stderr, "cannot connect to netinfo server: %s\n",
			ni_error(status));
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
	status = ni_root(ni, &root);
	if (status != NI_OK) {
		ni_fatal(status);
	}
	name = ni_name_dup(NI_NAME_NAME);
	val = ni_name_dup(dir);
	status = ni_lookup(ni, &root, name, val, &ids);
	if (status != NI_OK && status != NI_NODIR) {
		ni_fatal(status);
	}
	if (status == NI_NODIR) {
		prop.nip_name = name;
		prop.nip_val.ninl_len = 0;
		prop.nip_val.ninl_val = NULL;
		ni_namelist_insert(&prop.nip_val, val, NI_INDEX_NULL);
		props.nipl_len = 0;
		props.nipl_val = NULL;
		ni_proplist_insert(&props, prop, NI_INDEX_NULL);
		status = ni_create(ni, &root, props, &maproot, NI_INDEX_NULL);
		if (status != NI_OK) {
			ni_fatal(status);
		}
	} else {
		if (ids.niil_len > 1) {
			fprintf(stderr,
				"warning: more than one directory named %s",
				val);
		}
		maproot.nii_object = ids.niil_val[0];
		status = ni_self(ni, &maproot);
		if (status != NI_OK) {
			ni_fatal(status);
		}
	}
#ifdef notdef
	/*
	 * Services file is special: we do not uniq by name by default
	 */
	if (!setuniq && ni_name_match(dir, NI_NAME_SERVICES)) {
		uniq = (ni_name)LOAD_SERVICES_MAGIC;
	}
#endif
	if (verbose) {
		setbuf(stdout, NULL);
	}
	if (deletions) {
		status = ni_list(ni, &maproot, uniq, &entries);
		if (status != NI_OK) {
			ni_fatal(status);
		}
	}
	(*doit)();
	if (deletions) {
		for (i = 0; i < entries.niel_len; i++) {
			id.nii_object = entries.niel_val[i].id;
			status = ni_self(ni, &id);
			if (status != NI_OK) {
				deletion_failed(status, entries.niel_val[i]);
				continue;
			}
			status = ni_destroy(ni, &maproot, id);
			if (status != NI_OK) {
				deletion_failed(status, entries.niel_val[i]);
			} else {
				if (verbose) {
					printf("-");
				}
			}
		}
	}
	if (verbose) {
		printf("\n");
	}
	exit(0);
}


int
hasprop(
	ni_name_const name,
	ni_property prop
	)
{
	return (ni_name_match(name, prop.nip_name) &&
		prop.nip_val.ninl_len > 0);
}

int
eq_nl(
      ni_namelist nl1,
      ni_namelist nl2
      )
{
	ni_index i;

	if (nl1.ninl_len != nl2.ninl_len) {
		return (0);
	}
	for (i = 0; i < nl1.ninl_len; i++) {
		if  (!ni_name_match(nl1.ninl_val[i],
				 nl2.ninl_val[i])) {
			return (0);
		}
	}
	return (1);
}

#ifdef notdef
ni_status
services_replace(
		 void *ni, 
		 ni_id *maproot,  
		 ni_proplist *props, 
		 ni_namelist deletions,
		 ni_index whname,
		 ni_index whprot
		 )
{
	ni_idlist nidlist;
	ni_idlist pidlist;
	ni_status status;
	ni_id id;
	ni_proplist oldprops;
	ni_index i;
	ni_index j;
	ni_index eq;

	status = ni_lookup(ni, maproot, props->nipl_val[whname].nip_name,
			   props->nipl_val[whname].nip_val.ninl_val[0], 
			   &nidlist);
	if (status != NI_OK) {
		return (status);
	}
	status = ni_lookup(ni, maproot, props->nipl_val[whprot].nip_name,
			   props->nipl_val[whprot].nip_val.ninl_val[0], 
			   &pidlist);
	if (status != NI_OK) {
		return (status);
	}
	id.nii_object = NI_INDEX_NULL;
	for (i = 0; i < nidlist.niil_len; i++) {
		for (j = 0; j < pidlist.niil_len; j++) {
			if (nidlist.niil_val[i] == pidlist.niil_val[j]) {
				id.nii_object = nidlist.niil_val[i];
				goto done;
			}
		}
	}
done:
	ni_idlist_free(&nidlist);
	ni_idlist_free(&pidlist);
	if (id.nii_object == NI_INDEX_NULL) {
		return (NI_FAILED);
	}
	status = ni_read(ni, &id, &oldprops);
	if (status != NI_OK) {
		return (status);
	}
	eq = 0;
	for (i = 0; i < props->nipl_len; i++) {
		for (j = 0; j < oldprops.nipl_len; j++) {
			if (ni_name_match(oldprops.nipl_val[j].nip_name,
				       props->nipl_val[i].nip_name)) {
				if (eq_nl(oldprops.nipl_val[j].nip_val,
					  props->nipl_val[i].nip_val)) {
					eq++;
				}
				ni_proplist_delete(&oldprops, j);
				break;
			}
		}
	}
	if (eq == props->nipl_len) {
		/*
		 * All the new properties have equals in the old properties.
		 * No need to update
		 */
		ni_proplist_free(&oldprops);
		return (NI_OK);
	}
	for (i = 0; i < oldprops.nipl_len; i++) {
		if (ni_namelist_match(deletions, 
				      oldprops.nipl_val[i].nip_name) ==
		    NI_INDEX_NULL) {
			ni_proplist_insert(props, oldprops.nipl_val[i], 
					   NI_INDEX_NULL);
		}
	}
	ni_proplist_free(&oldprops);
	status = ni_write(ni, &id, *props);
	return (status);
}


void
services_loadprops(
		   ni_proplist *props,
		   ni_namelist delprops
		   )
{
	int done = 0;
	ni_status status = NI_FAILED;
	ni_index i = 0;
	ni_id newid;
	ni_index whname = NI_INDEX_NULL;
	ni_index whprot = NI_INDEX_NULL;

	for (i = 0; i < props->nipl_len; i++) {
		if (hasprop(NI_NAME_NAME, props->nipl_val[i])) {
			whname = i;
		}
		if (hasprop(NI_NAME_PROTOCOL, props->nipl_val[i])) {
			whprot = i;
		}
		if (whname != NI_INDEX_NULL && whprot != NI_INDEX_NULL) {
			if ((status = services_replace(ni, &maproot, 
						       props, delprops, 
						       whname,
						       whprot)) == NI_OK) {
				done++;
			}
			break;
		}
	}
	if (done) {
		if (deletions) {
			update_entries(props->nipl_val[i].nip_val);
		}
	} else {
		status = ni_create(ni, &maproot, *props, 
				   &newid, NI_INDEX_NULL);
	}
	if (status != NI_OK) {
		ni_fatal(status);
	}
	if (verbose) {
		printf("+");
	}
}
#endif

void
loadprops(
	  ni_proplist *props,
	  ni_namelist delprops
	  )
{
	int done;
	ni_status status = NI_FAILED;
	ni_index i = 0;
	ni_id newid;

#ifdef notdef	
	if (uniq == LOAD_SERVICES_MAGIC) {
		services_loadprops(props, delprops);
		return;
	} 
#endif
	done = 0;
	if (uniq != NULL) {
		for (i = 0; i < props->nipl_len; i++) {
			if (hasprop(uniq, props->nipl_val[i])) {
				if ((status = replace(ni, &maproot, 
						      props, delprops, i
						      )) == NI_OK) {
					done++;
				}
				break;
			}
		}
	} 
	if (done) {
		if (deletions) {
			update_entries(props->nipl_val[i].nip_val);
		}
	} else {
		status = ni_create(ni, &maproot, *props, 
				   &newid, NI_INDEX_NULL);
	}
	if (status != NI_OK) {
		ni_fatal(status);
	}
	if (verbose) {
		printf("+");
	}
}

ni_status
replace(
	void *ni, 
	ni_id *maproot,  
	ni_proplist *props, 
	ni_namelist deletions,
	ni_index which
	)
{
	ni_idlist idlist;
	ni_status status;
	ni_id id;
	ni_proplist oldprops;
	ni_index i;
	ni_index j;
	ni_index eq;

	status = ni_lookup(ni, maproot, props->nipl_val[which].nip_name,
			   props->nipl_val[which].nip_val.ninl_val[0], 
			   &idlist);
	if (status != NI_OK) {
		return (status);
	}
	id.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);
	status = ni_read(ni, &id, &oldprops);
	if (status != NI_OK) {
		return (status);
	}
	eq = 0;
	for (i = 0; i < props->nipl_len; i++) {
		for (j = 0; j < oldprops.nipl_len; j++) {
			if (ni_name_match(oldprops.nipl_val[j].nip_name,
				       props->nipl_val[i].nip_name)) {
				if (eq_nl(oldprops.nipl_val[j].nip_val,
					  props->nipl_val[i].nip_val)) {
					eq++;
				}
				ni_proplist_delete(&oldprops, j);
				break;
			}
		}
	}
	if (eq == props->nipl_len) {
		/*
		 * All the new properties have equals in the old properties.
		 * No need to update
		 */
		ni_proplist_free(&oldprops);
		return (NI_OK);
	}
	for (i = 0; i < oldprops.nipl_len; i++) {
		if (ni_namelist_match(deletions, 
				      oldprops.nipl_val[i].nip_name) ==
		    NI_INDEX_NULL) {
			ni_proplist_insert(props, oldprops.nipl_val[i], 
					   NI_INDEX_NULL);
		}
	}
	ni_proplist_free(&oldprops);
	status = ni_write(ni, &id, *props);
	return (status);
}

void
fatal(char *msg)
{
	fprintf(stderr, "%s: fatal error: %s\n", myname, msg);
	exit(1);
}

void
ni_fatal(ni_status status)
{
	fprintf(stderr, "%s: fatal error: %s\n", myname, ni_error(status));
	exit(1);
}

typedef struct convertmap {
	char *format;
	ni_name dirname;
	pfvoid2void converter;
} convertmap;


extern fvoid2void load_users;
extern fvoid2void load_machines;
extern fvoid2void load_networks;
extern fvoid2void load_rpcs;
extern fvoid2void load_services;
extern fvoid2void load_groups;
extern fvoid2void load_protocols;
extern fvoid2void load_printers;
extern fvoid2void load_mounts;
extern fvoid2void load_aliases;
extern fvoid2void load_bootparams;
extern fvoid2void load_bootp;


convertmap converters[] = {
	{ "aliases", 	"aliases", 	load_aliases },
	{ "bootparams", "machines",     load_bootparams },
	{ "bootptab",	"machines",	load_bootp },
	{ "fstab", 	"mounts", 	load_mounts },
	{ "group",	"groups",   	load_groups },
	{ "hosts",	"machines", 	load_machines },
	{ "networks",	"networks", 	load_networks },
	{ "passwd", 	"users", 	load_users },
	{ "printcap", 	"printers",	load_printers },
	{ "protocols",  "protocols", 	load_protocols },
	{ "rpc", 	"rpcs", 	load_rpcs },
	{ "services",   "services",	load_services },
	{ NULL, NULL, NULL }
};


pfvoid2void
getconverter(char *format, ni_name *dirname)
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
usage(
      char *myname
      )
{
	convertmap *map;
	
	fprintf(stderr, 
		"usage: %s [-v] [-d] [-p] [-u <name>] <format> <domain>\n", 
		myname);
	fprintf(stderr, "<format> must be one of the following:\n");
	for (map = &converters[0]; map->format != NULL; map++) {
		fprintf(stderr, "\t%s\n", map->format);
	}
	exit(1);
}


void 
deletion_failed(
		ni_status status, 
		ni_entry entry
		)
{
	if (entry.names != NULL && entry.names->ninl_len > 0) {
		fprintf(stderr, "deletion for %s failed: %s\n",
			entry.names->ninl_val[0], ni_error(status));
	} else {
		fprintf(stderr, "deletion for ID %d failed: %s\n",
			entry.id, ni_error(status));
	}
}

void
update_entries(
	       ni_namelist names
	       )
{
	ni_index i;
	ni_index j;
	ni_namelist *nl;
	ni_name name;

	for (i = 0; i < entries.niel_len; i++) {
		nl = entries.niel_val[i].names;
		if (nl == NULL) {
			continue;
		}
		for (j = 0; j < names.ninl_len; j++) {
			name = names.ninl_val[j];
			if (ni_namelist_match(*nl, name) != NI_INDEX_NULL) {
				ni_namelist_free(nl);
				entries.niel_len--;
				(entries.niel_val[i] = 
				 entries.niel_val[entries.niel_len]);
				i--;
				break;
			}
		}
	}
}


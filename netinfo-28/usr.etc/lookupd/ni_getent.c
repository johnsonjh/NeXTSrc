/*
 * C-library lookup routines which may be overridden by NetInfo.
 * Copyright (C) 1989 by NeXT, Inc.
 *
 */

/*
 * This module contains many of the get*ent routines in the C library.
 *
 * Search for: 		to get:
 * GENERAL_STUFF	generally useful stuff 
 * PASSWD_STUFF		passwd specific stuff
 * GROUP_STUFF		group specific stuff
 * HOST_STUFF		hosts specific stuff
 * SERVICES_STUFF	services specific stuff
 * RPC_STUFF		rpc specific stuff
 * PROTOCOLS_STUFF	protocols specific stuff
 * NETWORKS_STUFF	networks specific stuff
 * PRINTER_STUFF	printer information
 * MOUNT_STUFF		mount information
 * BOOTP_STUFF		bootp information
 * BOOTPARAMS_STUFF	bootparams information
 * ALIAS_STUFF		alias information
 * NETGROUP_STUFF	netgroup information
 */

/*
 * GENERAL_STUFF
 */
#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <netdb.h>
#include <aliasdb.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <netinfo/ni.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/if_ether.h>
#include <ctype.h>
#include "clib.h"
#include "mm.h"

#define MAX_INT_SIZE (3*sizeof(int) + 1) /* portable, though liberal */

#define ID_NOBODY -2
#define MAXADDRLEN 32   /* Max ascii length of address */

#define UNINIT ((void *)(-1))
#define CANT_USE_QUICKPROPS ((void *)-1)

static const ni_name NAME_NAME = "name";
static const ni_name NAME_USERS = "users";
static const ni_name NAME_GID = "gid";

static const char STR_NULL[] = "";
static const char *STRLST_NULL[] = { NULL };

typedef struct ni_global_state_node {
	ni_id root_id;
	void *ni;
} ni_global_state_node;

typedef struct ni_global_state {
	ni_index len;
	ni_global_state_node *val;
} ni_global_state;

static struct ni_global_state nichain = { NI_INDEX_NULL, UNINIT };

/*
 * The idea behind the quicklist was to get all of the information in
 * a single RPC call. While this may be easier on the network, it is
 * not very good for the server because it ties it up. We are turning
 * this feature off here by not defining USEQUICKLIST. It may disappear
 * from the server too.
 */
#define NENTRIES(state)       ((state).havequicklist ? \
			       (state).entries.nipll_len : \
			       (state).children.niil_len)

typedef struct ni_local_state_node {
	ni_id dir;
	ni_idlist children;
	ni_proplist_list entries;
	int havequicklist; /* not sure if this is a good idea */
	int havequicklook; /* this BS is only for 0.9 compatibility */
	ni_index which;
} ni_local_state_node;

typedef struct ni_local_state {
	ni_index which;
	ni_index len;
	ni_local_state_node *val;
} ni_local_state;

static char *
itoa(
     char *str,
     int num
     )
{
	char buf[MAX_INT_SIZE + 1];
	char *p = buf;
	int sign = 0;
	char *s;
	
	if (num < 0) {
		sign++;
		num = -num;
	}
	while (num > 0) {
		*p++ = '0' + (num % 10);
		num /= 10;
	}
	if (p == buf) {
		*p++ = '0';
	}
	
	/* reverse */
	s = str;
	if (sign) {
		*s++ = '-';
	}
	for (p--; p >= buf; p--) {
		*s++ = *p;
	}
	*s = 0;
	return (str);
}

int
_ni_running(
	  void
	  )
{
	void *local_ni;
	ni_id root_id;
	ni_index i;
	
	if (nichain.val != UNINIT) {
		return (nichain.val != NULL);
	}
	
	local_ni = ni_new(NULL, ".");
	if (local_ni == NULL) {
		return (0);
	}
	if (ni_root(local_ni, &root_id) != NI_OK) {
		ni_free(local_ni);
		nichain.val = NULL;
		return (0);
	}
	if (nichain.val != UNINIT) {
		for (i = 0; i < nichain.len; i++) {
			ni_free(nichain.val[i].ni);
		}
		MM_FREE(nichain.val);
	}
	MM_ALLOC(nichain.val);
	nichain.val[0].ni = local_ni;
	nichain.val[0].root_id = root_id;
	nichain.len = 1;
	return (1);
}



static ni_index
getchecksum(
	    void *ni
	    )
{
	ni_status status;
	ni_proplist pl;
	ni_index sum;
	ni_namelist nl;
	ni_index i;
	static const ni_name NAME_CHECKSUM = "checksum";

	/*
	 * Set to random for case of failure: random checksums will
	 * almost never match one another.
	 */
	sum = random();
	status = ni_statistics(ni, &pl);
	
	if (status != NI_OK) {
		return (sum);
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_CHECKSUM)) {
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len > 0) {
				sum = atoi(nl.ninl_val[0]);
			}
			break;
		}
	}
	ni_proplist_free(&pl);
	return (sum);
}


void
ni_getchecksums(
		unsigned *lenp,
		unsigned **sumsp
		)
{
	unsigned len;
	unsigned *sums;
	void *ni;
	ni_id root;
	
	len = 0;
	sums = NULL;
	if (!_ni_running()) {
		*lenp = len;
		*sumsp = sums;
		return;
	}
	while (len < nichain.len) {
		MM_GROW_ARRAY(sums, len);
		sums[len] = getchecksum(nichain.val[len].ni);
		len++;
		
		if (len + 1 < nichain.len) {
			continue;
		}

		/*
		 * Need to grow it
		 */
		ni = ni_new(nichain.val[nichain.len - 1].ni, "..");
		if (ni == NULL) {
			continue;
		}
		if (ni_root(ni, &root) == NI_OK) {
			MM_GROW_ARRAY(nichain.val, nichain.len);
			nichain.val[nichain.len].ni = ni;
			nichain.val[nichain.len].root_id = root;
			nichain.len++;
		} else {
			ni_free(ni);
		}
	}
	*lenp = len;
	*sumsp = sums;
}

static void
name_get(
	 ni_namelist *nl,
	 char **val
	 )
{
	if (nl->ninl_len > 0) {
		*val = ni_name_dup(nl->ninl_val[0]);
	}
}

static void
int_get(
	ni_namelist *nl,
	int *ival
	)
{
	int val;
	int sign;
	ni_name nm;
	
	val = 0;
	sign = 1;
	if (nl->ninl_len > 0) {
		for (nm = nl->ninl_val[0]; *nm; nm++) {
			if (val == 0 && *nm == '-') {
				sign = -sign;
			} else if (isdigit(*nm)) {
				val *= 10;
				val += *nm - '0';
			} else {
				return;
			}
		}
	}
	*ival = sign * val;
}

static void
namelist_get(
	     ni_namelist *nl,
	     char ***val
	     )
{
	ni_index i;
	
	*val = (char **)malloc(sizeof(char *) * (nl->ninl_len + 1));
	for (i = 0; i < nl->ninl_len; i++) {
		(*val)[i] = ni_name_dup(nl->ninl_val[i]);
	}
	(*val)[nl->ninl_len] = NULL;
}

static void
freelist(
	 char **l
	 )
{
	int i;
	
	for (i = 0; l[i] != NULL; i++) {
		free(l[i]);
	}
	free(l);
}

static void
addr_get(
	 ni_namelist *nl,
	 char ***l
	 )
{
	u_long addr;
	ni_index i;
	
	*l = (char **)malloc(sizeof(char *) * (nl->ninl_len + 1));
	(*l)[nl->ninl_len] = NULL;
	for (i = 0; i < nl->ninl_len; i++) {
		(*l)[i] = (char *)malloc(sizeof(u_long));
		addr = inet_addr(nl->ninl_val[i]);
		bcopy(&addr, (*l)[i], sizeof(addr));
	}
}

static void
sorry(
      char *func
      )
{
	(void)fprintf(stderr, "sorry, %s not supported with NetInfo (yet)\n", 
		      func);
}


static int
do_init(
	const ni_name name,
	ni_local_state *state
	)
{
	ni_idlist match;
	
	if (!_ni_running()) {
		return (0);
	}
	if (state->val != NULL) {
		return (1);
	}
	if (ni_lookup(nichain.val[0].ni, &nichain.val[0].root_id, 
		      NAME_NAME, name, &match) != NI_OK) {
		return (0);
	}
	state->len = 1;
	state->which = 0;
	MM_ALLOC(state->val);
	state->val[0].dir.nii_object = match.niil_val[0];
	state->val[0].which = NI_INDEX_NULL;
	state->val[0].havequicklook = 1;
	ni_idlist_free(&match);
	return (1);
}



static void
do_end(
       ni_local_state *state
       )
{
}

static void
do_set(
       ni_local_state *state
       )
{
	ni_index i;
	
	for (i = 0; i < state->len; i++) {
		if (state->val[i].which != NI_INDEX_NULL) {
			ni_idlist_free(&state->val[i].children);
			if (state->val[i].havequicklist) {
				ni_proplist_list_free(&state->val[i].entries);
			}
			state->val[i].which = NI_INDEX_NULL;
#ifdef USEQUICKLIST
			state->val[i].havequicklist = 1;
#else
			state->val[i].havequicklist = 0;
#endif
		}
	}
	state->which = 0;
}

static int
do_more(
	const ni_name_const name,
	ni_local_state *state,
	ni_index which
	)
{
	void *ni;
	ni_idlist match;
	ni_id root;
	
	if (nichain.len <= which + 1) {
		/*
		 * Need to grow it
		 */
		ni = ni_new(nichain.val[nichain.len - 1].ni, "..");
		if (ni == NULL) {
			return (0);
		}
		if (ni_root(ni, &root) != NI_OK) {
			ni_free(ni);
			return (0);
		}
		MM_GROW_ARRAY(nichain.val, nichain.len);
		nichain.val[nichain.len].ni = ni;
		nichain.val[nichain.len].root_id = root;
		nichain.len++;
	} else {
		ni = nichain.val[which + 1].ni;
		root = nichain.val[which + 1].root_id;
	}
	if (which + 1 >= state->len) {
		if (ni_lookup(ni, &root, NAME_NAME, name, &match) != NI_OK) {
			return (0);
		}
		MM_GROW_ARRAY(state->val, state->len);
		state->val[state->len].dir.nii_object = match.niil_val[0];
		state->val[state->len].which = NI_INDEX_NULL;
		state->val[state->len].havequicklook = 1;
		ni_idlist_free(&match);
		state->len++;
	}
	if (state->val[which + 1].which != NI_INDEX_NULL) {
		state->val[which + 1].which = NI_INDEX_NULL;
		ni_idlist_free(&state->val[which + 1].children);
		if (state->val[which + 1].havequicklist) {
			ni_proplist_list_free(&state->val[which + 1].entries);
		} 
	}
	return (1);
}

static void *
do_get(
       const ni_name_const name,
       ni_local_state *state,
       const ni_name_const prop_name,
       const ni_name_const prop_val,
       void *(*readit)(),
       void *arg
       )
{
	ni_idlist match;
	void *res = CANT_USE_QUICKPROPS;
	ni_id id;
	ni_status status = NI_FAILED;
	ni_index i;
	void *ni;
	ni_proplist props;
	
	if (state->val[0].which == NI_INDEX_NULL) {
		do_set(state);
	}
	for (i = 0; i < state->len; i++) {
		ni = nichain.val[i].ni;
		id = state->val[i].dir;
		if (state->val[i].havequicklook) {
			status = ni_lookupread(ni, &id, prop_name, 
					       prop_val, &props);
			if (status == NI_FAILED) {
				state->val[i].havequicklook = 0;
			}
		}
		do {
			if (!state->val[i].havequicklook) {
				status = ni_lookup(ni, &id, prop_name, 
						   prop_val, &match);
			}
			if (status != NI_OK && status != NI_NODIR) {
				return (NULL);
			}
			if (status == NI_OK) {
				if (state->val[i].havequicklook) {
					res = (*readit)(ni, NULL, arg, 
							&props);
					ni_proplist_free(&props);
				} else {
					res = (*readit)(ni, &match, arg, 
							NULL);
					ni_idlist_free(&match);
				}
				if (res != NULL) {
					if (res != CANT_USE_QUICKPROPS) {
						return (res);
					}
					state->val[i].havequicklook = 0;
				}
			}
		} while (res == CANT_USE_QUICKPROPS && status == NI_OK);
		if (!do_more(name, state, i)) {
			return (NULL);
		}
	}
	return (NULL);
}

static void *
do_getent(
	  ni_name name,
	  ni_local_state *state,
	  void *(*readit)(),
	  void *arg
	  )
{
	ni_idlist match;
	ni_local_state_node *lsn;


	if (state->val[0].which == NI_INDEX_NULL) {
		do_set(state);
#ifdef USEQUICKLIST
		if (ni_listall(nichain.val[state->which].ni,
			       &state->val[state->which].dir,
			       &state->val[state->which].entries) == NI_OK) {
			state->val[state->which].havequicklist = 1;
		} else 
#endif
		{
			state->val[state->which].havequicklist = 0;
		}
		if (ni_children(nichain.val[state->which].ni,	
				&state->val[state->which].dir,
				&state->val[state->which].children) == NI_OK) {
			state->val[state->which].which = 0;
		}
	}
	while (NENTRIES(state->val[state->which]) <=
	       state->val[state->which].which) {
		if (!do_more(name, state, state->which)) {
			return (NULL);
		}
		state->which++;
#ifdef USEQUICKLIST
		if (ni_listall(nichain.val[state->which].ni,
			       &state->val[state->which].dir,
			       &state->val[state->which].entries) == NI_OK) {
			state->val[state->which].havequicklist = 1;
		} else 
#endif
		{
			state->val[state->which].havequicklist = 0;
		}
		if (ni_children(nichain.val[state->which].ni,
				&state->val[state->which].dir,
				&state->val[state->which].children) 
		    != NI_OK) {
			state->val[state->which].children.niil_len = 0;
			state->val[state->which].children.niil_val = NULL;
		}
		state->val[state->which].which = 0;
	}
	lsn = &state->val[state->which];
	match.niil_val = &lsn->children.niil_val[lsn->which++];
	match.niil_len = 1;
	return ((*readit)(nichain.val[state->which].ni, &match, arg,
			  (lsn->havequicklist ?
			   &lsn->entries.nipll_val[lsn->which - 1] : NULL)));
}

#ifdef notdef
static int
do_putent(
	  ni_local_state *state,
	  int (*writeit)(),
	  void *arg,
	  char *login,
	  char *passwd
	  )
{
	ni_local_state_node *lsn;
	void *ni;
	
	lsn = &state->val[state->which];
	ni = nichain.val[state->which].ni;
	if (passwd != NULL) {
		if (ni_setpassword(ni, passwd) != NI_OK) {
			return (0);
		}
		if (ni_setuser(ni, login) != NI_OK) {
			return (0);
		}
	}
	return ((*writeit)(ni, lsn->children.niil_val[lsn->which - 1], arg));
}
#endif

/*
 * PASSWD_STUFF
 */
static const ni_name NAME_PASSWD = "passwd";
static const ni_name NAME_UID = "uid";
static const ni_name NAME_REALNAME = "realname";
static const ni_name NAME_HOME = "home";
static const ni_name NAME_SHELL = "shell";

static void *pw_read(void *, ni_idlist *, void *, ni_proplist *);
#ifdef notdef
static int pw_writepasswd(void *, ni_index, ni_name);
#endif
#define pw_init() do_init(NAME_USERS, &pw_state)

static ni_local_state pw_state = { NI_INDEX_NULL };
static struct passwd pw = { 
	(char *)STR_NULL, (char *)STR_NULL, ID_NOBODY, ID_NOBODY,
	0, (char *)STR_NULL, (char *)STR_NULL, (char *)STR_NULL, 
	(char *)STR_NULL 
};


struct passwd *
_ni_getpwuid(
	     int uid
	     )
{
	char uidbuf[MAX_INT_SIZE + 1];
	ni_name uidnm;
	
	if (!pw_init()) {
		return (NULL);
	}
	
	uidnm = itoa(uidbuf, uid);
	return (do_get(NAME_USERS, &pw_state, NAME_UID, 
		       uidnm, pw_read, NULL));
}

struct passwd *
_ni_getpwnam(
	     char *name
	     )
{
	if (!pw_init()) {
		return (NULL);
	}
	return (do_get(NAME_USERS, &pw_state, NAME_NAME,
		       name, pw_read, NULL));
}

struct passwd *
_ni_getpwent(
	     void
	     )
{
	if (!pw_init()) {
		return (NULL);
	}
	return (do_getent(NAME_USERS, &pw_state, pw_read, NULL));
}


void
_ni_setpwent(
	     void
	     )
{
	if (!pw_init()) {
		return;
	}
	do_set(&pw_state);
}


void
_ni_endpwent(
	     void
	     )
{
	if (!pw_init()) {
		return;
	}
	do_end(&pw_state);
}

#ifdef notdef
int
_ni_putpwpasswd(
		char *login,
		char *old_passwd, /* cleartext */
		char *new_passwd /* encrypted */
		)
{
	int res;
	struct passwd *pw;
	
	if (!pw_init()) {
		return (0);
	}
	_ni_setpwent();
	res = 0;
	while (pw = _ni_getpwent()) {
		if (strcmp(pw->pw_name, login) == 0) {
			res = do_putent(&pw_state, pw_writepasswd, 
					new_passwd, login, old_passwd);
			break;
		}
	}
	_ni_endpwent();
	return (res);
}
#endif

static void *
pw_read(
	void *ni,
	ni_idlist *match,
	void *unused,
	ni_proplist *quickprops
	)
{
	ni_proplist pl;
	ni_index i;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (pw.pw_name != (char *)STR_NULL) {
		free(pw.pw_name);
		pw.pw_name = (char *)STR_NULL;
	}
	if (pw.pw_passwd != (char *)STR_NULL) {
		free(pw.pw_passwd);
		pw.pw_passwd = (char *)STR_NULL;
	}
	pw.pw_uid = ID_NOBODY;
	pw.pw_gid = ID_NOBODY;
	if (pw.pw_gecos != (char *)STR_NULL) {
		free(pw.pw_gecos);
		pw.pw_gecos = (char *)STR_NULL;
	}
	if (pw.pw_dir != (char *)STR_NULL) {
		free(pw.pw_dir);
		pw.pw_dir = (char *)STR_NULL;
	}
	if (pw.pw_shell != (char *)STR_NULL) {
		free(pw.pw_shell);
		pw.pw_shell = (char *)STR_NULL;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_REALNAME)) {
			name_get(&pl.nipl_val[i].nip_val, &pw.pw_gecos);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_PASSWD)) {
			name_get(&pl.nipl_val[i].nip_val, &pw.pw_passwd);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &pw.pw_name);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_SHELL)) {
			name_get(&pl.nipl_val[i].nip_val, &pw.pw_shell);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_HOME)) {
			name_get(&pl.nipl_val[i].nip_val, &pw.pw_dir);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_UID)) {
			int_get(&pl.nipl_val[i].nip_val, &pw.pw_uid);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_GID)) {
			int_get(&pl.nipl_val[i].nip_val, &pw.pw_gid);
		}
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&pw);
}

#ifdef notdef
static int
pw_writepasswd(
	       void *ni,
	       ni_index which,
	       ni_name newpw
	       )
{
	ni_proplist pl;
	ni_index i;
	ni_index prop_index;
	ni_property prop;
	ni_id id;
	ni_status stat;
	
	id.nii_object = which;
	if (ni_read(ni, &id, &pl) != NI_OK) {
		return (0);
	}
	prop_index = NI_INDEX_NULL;
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_PASSWD)) {
			prop_index = i;
			break;
		}
	}
	if (prop_index == NI_INDEX_NULL) {
		prop.nip_name = NAME_PASSWD;
		prop.nip_val.ninl_len = 1;
		prop.nip_val.ninl_val = &newpw;
		stat = ni_createprop(ni, &id, prop, NI_INDEX_NULL);
	} else {
		if (pl.nipl_val[i].nip_val.ninl_len == 0) {
			stat = ni_createname(ni, &id, prop_index, newpw, 0);
		} else {
			stat = ni_writename(ni, &id, prop_index, 0, newpw);
		}
	}
	ni_proplist_free(&pl);
	return (stat == NI_OK);
}
#endif

/*
 * GROUP_STUFF
 */
static const ni_name NAME_GROUPS = "groups";

static void *gr_read(void *, ni_idlist *, void *, ni_proplist *);
#define gr_init() do_init(NAME_GROUPS, &gr_state)

static ni_local_state gr_state = { NI_INDEX_NULL };

struct group *
_ni_getgrgid(
	     int gid
	     )
{
	char gidbuf[MAX_INT_SIZE + 1];
	ni_name gidnm;
	
	if (!gr_init()) {
		return (NULL);
	}
	
	gidnm = itoa(gidbuf, gid);
	
	return (do_get(NAME_GROUPS, &gr_state, NAME_GID, gidnm, gr_read,
		       NULL));
}

struct group *
_ni_getgrnam(
	     char *name
	     )
{
	ni_name grname;
	
	if (!gr_init()) {
		return (NULL);
	}
	grname = name;
	return (do_get(NAME_GROUPS, &gr_state, NAME_NAME, grname, 
		       gr_read, NULL));
}

struct group *
_ni_getgrent(
	     void
	     )
{
	if (!gr_init()) {
		return (NULL);
	}
	return (do_getent(NAME_GROUPS, &gr_state, gr_read, NULL));
	
}

void
_ni_setgrent(
	     void
	     )
{
	if (!gr_init()) {
		return;
	}
	do_set(&gr_state);
}


void
_ni_endgrent(
	     void
	     )
{
	if (!gr_init()) {
		return;
	}
	do_end(&gr_state);
}


static void *
gr_read(
	void *ni,
	ni_idlist *match,
	void *ununsed,
	ni_proplist *quickprops
	)
{
	static struct group gr = { 
		(char *)STR_NULL, (char *)STR_NULL, ID_NOBODY, (char **)STRLST_NULL 
	};
	
	ni_proplist pl;
	ni_index i;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (gr.gr_name != (char *)STR_NULL) {
		free(gr.gr_name);
		gr.gr_name = (char *)STR_NULL;
	}
	if (gr.gr_passwd != (char *)STR_NULL) {
		free(gr.gr_passwd);
		gr.gr_passwd = (char *)STR_NULL;
	}
	gr.gr_gid = ID_NOBODY;
	if (gr.gr_mem != (char **)&STRLST_NULL) {
		freelist(gr.gr_mem);
		gr.gr_mem = (char **)&STRLST_NULL;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &gr.gr_name);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_PASSWD)) {
			name_get(&pl.nipl_val[i].nip_val, &gr.gr_passwd);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_USERS)) {
			namelist_get(&pl.nipl_val[i].nip_val, &gr.gr_mem);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_GID)) {
			int_get(&pl.nipl_val[i].nip_val, &gr.gr_gid);
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&gr);
}


/*
 * HOST_STUFF
 */
static const ni_name NAME_MACHINES = "machines";
static const ni_name NAME_IP_ADDRESS = "ip_address";

extern int h_errno;
static void *h_read(void *, ni_idlist *, void *, ni_proplist *);
#define h_init() do_init(NAME_MACHINES, &h_state)

static ni_local_state h_state = { NI_INDEX_NULL };

struct hostent *
_ni_gethostbyname(
		  char *name
		  )
{
	ni_name hnm;
	
	if (!h_init()) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	
	hnm = name;
	h_errno = HOST_NOT_FOUND;
	return (do_get(NAME_MACHINES, &h_state, NAME_NAME, hnm, h_read, 
		       NULL));
}

struct hostent *
_ni_gethostbyaddr(
		  char *addr,
		  int len,
		  int type
		  )
{
	ni_name nm;
	struct in_addr in;
	
	if (!h_init()) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	if (len != sizeof(u_long) || type != AF_INET) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	bcopy(addr, &in.s_addr, sizeof(in.s_addr));
	nm = inet_ntoa(in);
	h_errno = HOST_NOT_FOUND;
	return (do_get(NAME_MACHINES, &h_state, NAME_IP_ADDRESS, nm, 
		       h_read, NULL));
}

struct hostent *
_ni_gethostent(
	       void
	       )
{
	if (!h_init()) {
		h_errno = NO_RECOVERY;
		return (NULL);
	}
	h_errno = NO_RECOVERY;
	return (do_getent(NAME_MACHINES, &h_state, h_read, NULL));
}

void
_ni_sethostent(
	       int stayopen
	       )
{
	if (!h_init()) {
		return;
	}
	do_set(&h_state);
}

void
_ni_endhostent(
	       void
	       )
{
	if (!h_init()) {
		return;
	}
	do_end(&h_state);
}

static void *
h_read(
       void *ni,
       ni_idlist *match,
       void *unused,
       ni_proplist *quickprops
       )
{
	static struct hostent h = { 
		(char *)STR_NULL, 
		(char **)STRLST_NULL, 
		AF_INET, 
		sizeof(u_long), 
		(char **)STRLST_NULL
	};
	
	ni_proplist pl;
	ni_index i;
	ni_namelist nl;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			h_errno = TRY_AGAIN;
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (h.h_name != (char *)STR_NULL) {
		free(h.h_name);
		h.h_name = (char *)STR_NULL;
	}
	if (h.h_aliases != (char **)STRLST_NULL) {
		freelist(h.h_aliases);
		h.h_aliases = (char **)STRLST_NULL;
	}
	if (h.h_addr_list != (char **)STRLST_NULL) {
		freelist(h.h_addr_list);
		h.h_addr_list = (char **)STRLST_NULL;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &h.h_name);
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len  > 0) {
				nl.ninl_len--;
				nl.ninl_val++;
				namelist_get(&nl, &h.h_aliases);
			}
			
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_IP_ADDRESS)) {
			addr_get(&pl.nipl_val[i].nip_val, &h.h_addr_list);
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	h_errno = 0;
	return (&h);
}


/*
 * SERVICE_STUFF
 */
static const ni_name NAME_SERVICES = "services";
static const ni_name NAME_PROTOCOL = "protocol";
static const ni_name NAME_PORT = "port";

static void *s_read(void *, ni_idlist *, char *, ni_proplist *);
#define s_init() do_init(NAME_SERVICES, &s_state)

static ni_local_state s_state = { NI_INDEX_NULL };

struct servent *
_ni_getservbyname(
		  char *name,
		  char *proto
		  )
{
	ni_name snm;
	
	if (!s_init()) {
		return (NULL);
	}
	
	snm = name;
	return (do_get(NAME_SERVICES, &s_state, NAME_NAME, snm, s_read, 
		       proto));
}

struct servent *
_ni_getservbyport(
		  int port,
		  char *proto
		  )
{
	ni_name nm;
	char buf[MAXADDRLEN + 1];
	
	if (!s_init()) {
		return (NULL);
	}
	nm = itoa(buf, port);
	return (do_get(NAME_SERVICES, &s_state, NAME_PORT, nm, 
		       s_read, proto));
}

struct servent *
_ni_getservent(
	       void
	       )
{
	struct servent *s;
	
	if (!s_init()) {
		return (NULL);
	}

	/*
	 * Is there some data from a previous call which we can use?
	 */
	s = s_read(NULL, NULL, NULL, NULL);
	if (s != NULL) {
		return (s);
	}
	return (do_getent(NAME_SERVICES, &s_state, s_read, NULL));
}

void
_ni_setservent(
	       int stayopen
	       )
{
	if (!s_init()) {
		return;
	}
	do_set(&s_state);
}

void
_ni_endservent(
	       void
	       )
{
	if (!s_init()) {
		return;
	}
	do_end(&s_state);
}

static void *
s_read(
       void *ni,
       ni_idlist *il,
       char *proto,
       ni_proplist *quickprops
       )
{
	static struct servent s = { 
		(char *)STR_NULL, 
		(char **)STRLST_NULL, 
		0,
		(char *)STR_NULL
	};
	
	ni_proplist pl;
	ni_index i;
	ni_index j;
	ni_index k;
	ni_id id;
	ni_namelist nl;
	static ni_namelist protolist;

	if (ni == NULL) {
	  	if (protolist.ninl_len == 0) {
			return (NULL);
		}
		free(s.s_proto);
		s.s_proto = ni_name_dup(protolist.ninl_val[0]);
		ni_namelist_delete(&protolist, 0);
		return (&s);
	}
	if (il == NULL) {
		return (CANT_USE_QUICKPROPS);
	}
	ni_namelist_free(&protolist);
	for (j = 0; j < il->niil_len; j++) {
		id.nii_object = il->niil_val[j];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			continue;
		}
		if (s.s_name != (char *)STR_NULL) {
			free(s.s_name);
			s.s_name = (char *)STR_NULL;
		}
		if (s.s_aliases != (char **)STRLST_NULL) {
			freelist(s.s_aliases);
			s.s_aliases = (char **)STRLST_NULL;
		}
		if (s.s_proto != (char *)STR_NULL) {
			free(s.s_proto);
			s.s_proto = (char *)STR_NULL;
		}
		for (i = 0; i < pl.nipl_len; i++) {
			if (ni_name_match(pl.nipl_val[i].nip_name, 
					  NAME_NAME)) {
				name_get(&pl.nipl_val[i].nip_val, &s.s_name);
				nl = pl.nipl_val[i].nip_val;
				if (nl.ninl_len  > 0) {
					nl.ninl_len--;
					nl.ninl_val++;
					namelist_get(&nl, &s.s_aliases);
				}
			} else if (ni_name_match(pl.nipl_val[i].nip_name, 
						 NAME_PORT)) {
				int_get(&pl.nipl_val[i].nip_val, &s.s_port);
			} else if (ni_name_match(pl.nipl_val[i].nip_name, 
						 NAME_PROTOCOL)) {
				nl = pl.nipl_val[i].nip_val;
				if (proto == NULL) {
					s.s_proto = ni_name_dup(nl.ninl_val[0]);
					protolist = ni_namelist_dup(nl);
					ni_namelist_delete(&protolist, 0);
				} else {
					bzero(&protolist, sizeof(protolist));
					for (k = 0; k < nl.ninl_len; k++) {
						if (ni_name_match(nl.ninl_val[k], 
								  proto)) {
							(s.s_proto = 
							 ni_name_dup(proto));
							break;
						}
					}
				}
			}
		}
		ni_proplist_free(&pl);
		if (s.s_proto != (char *)STR_NULL) {
			return (&s);
		}
	}
	return (NULL);
}


/*
 * RPC_STUFF
 */
static const ni_name NAME_RPCS = "rpcs";
static const ni_name NAME_NUMBER = "number";

static void  *r_read(void *, ni_idlist *, void *, ni_proplist *);
#define r_init() do_init(NAME_RPCS, &r_state)

static ni_local_state r_state = { NI_INDEX_NULL };

struct rpcent *
_ni_getrpcbyname(
		 char *name
		 )
{
	ni_name nm;
	
	if (!r_init()) {
		return (NULL);
	}
	
	nm = name;
	return (do_get(NAME_RPCS, &r_state, NAME_NAME, nm, r_read, NULL));
}

struct rpcent *
_ni_getrpcbynumber(
		   u_long number
		   )
{
	ni_name nm;
	char buf[MAXADDRLEN + 1];
	
	if (!r_init()) {
		return (NULL);
	}
	
	nm = itoa(buf, number);
	return (do_get(NAME_RPCS, &r_state, NAME_NUMBER, nm, r_read, 
		       NULL));
}

struct rpcent *
_ni_getrpcent(
	      void
	      )
{
	if (!r_init()) {
		return (NULL);
	}
	return (do_getent(NAME_RPCS, &r_state, r_read, NULL));
}

void
_ni_setrpcent(
	      int stayopen
	      )
{
	if (!r_init()) {
		return;
	}
	do_set(&r_state);
}

void
_ni_endrpcent(
	      void
	      )
{
	if (!r_init()) {
		return;
	}
	do_end(&r_state);
}

static void *
r_read(
       void *ni,
       ni_idlist *match,
       void *unused,
       ni_proplist *quickprops
       )
{
	static struct rpcent r = { 
		(char *)STR_NULL, (char **)STRLST_NULL, 0
	};
	
	ni_proplist pl;
	ni_index i;
	ni_namelist nl;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (r.r_name != (char *)STR_NULL) {
		free(r.r_name);
		r.r_name = (char *)STR_NULL;
	}
	if (r.r_aliases != (char **)STRLST_NULL) {
		freelist(r.r_aliases);
		r.r_aliases = (char **)STRLST_NULL;
	}
	r.r_number = 0;
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &r.r_name);
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len  > 0) {
				nl.ninl_len--;
				nl.ninl_val++;
				namelist_get(&nl, &r.r_aliases);
			}
			
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_NUMBER)) {
			int_get(&pl.nipl_val[i].nip_val, &r.r_number);
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&r);
}

/*
 * PROTOCOLS_STUFF
 */
static const ni_name NAME_PROTOCOLS = "protocols";

static void *p_read(void *, ni_idlist *, void *, ni_proplist *);
#define p_init() do_init(NAME_PROTOCOLS, &p_state)
static ni_local_state p_state = { NI_INDEX_NULL };

struct protoent *
_ni_getprotobyname(
		   char *name
		   )
{
	ni_name nm;
	
	if (!p_init()) {
		return (NULL);
	}
	
	nm = name;
	return (do_get(NAME_PROTOCOLS, &p_state, NAME_NAME, nm, p_read, 
		       NULL));
}

struct protoent *
_ni_getprotobynumber(
		     u_long number
		     )
{
	ni_name nm;
	char buf[MAXADDRLEN + 1];
	
	if (!p_init()) {
		return (NULL);
	}
	
	nm = itoa(buf, number);
	return (do_get(NAME_PROTOCOLS, &p_state, NAME_NUMBER, nm, 
		       p_read, NULL));
}

struct protoent *
_ni_getprotoent(
		void
		)
{
	if (!p_init()) {
		return (NULL);
	}
	return (do_getent(NAME_PROTOCOLS, &p_state, p_read, NULL));
}

void
_ni_setprotoent(
		int stayopen
		)
{
	if (!p_init()) {
		return;
	}
	do_set(&p_state);
}


void
_ni_endprotoent(
		void
		)
{
	if (!p_init()) {
		return;
	}
	do_end(&p_state);
}

static void *
p_read(
       void *ni,
       ni_idlist *match,
       void *unused,
       ni_proplist *quickprops

       )
{
	static struct protoent p = { 
		(char *)STR_NULL, (char **)STRLST_NULL, 0
	};
	
	ni_proplist pl;
	ni_index i;
	ni_namelist nl;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (p.p_name != (char *)STR_NULL) {
		free(p.p_name);
		p.p_name = (char *)STR_NULL;
	}
	if (p.p_aliases != (char **)STRLST_NULL) {
		freelist(p.p_aliases);
		p.p_aliases = (char **)STRLST_NULL;
	}
	p.p_proto = -1;
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &p.p_name);
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len  > 0) {
				nl.ninl_len--;
				nl.ninl_val++;
				namelist_get(&nl, &p.p_aliases);
			}
			
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_NUMBER)) {
			int_get(&pl.nipl_val[i].nip_val, &p.p_proto);
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&p);
}

/*
 * NETWORKS_STUFF
 */
static const ni_name NAME_NETWORKS = "networks";
static const ni_name NAME_ADDRESS = "address";

extern char *nettoa(unsigned);
static void *n_read(void *, ni_idlist *, void *, ni_proplist *);
#define n_init() do_init(NAME_NETWORKS, &n_state)

static ni_local_state n_state = { NI_INDEX_NULL };

struct netent *
_ni_getnetbyname(
		 char *name
		 )
{
	ni_name nm;
	
	if (!n_init()) {
		return (NULL);
	}
	
	nm = name;
	
	return (do_get(NAME_NETWORKS, &n_state, NAME_NAME, nm, n_read, NULL));
}

struct netent *
_ni_getnetbyaddr(
		 long addr,
		 int type
		 )
{
	ni_name nm;
	
	if (!n_init()) {
		return (NULL);
	}
	
	if (type != AF_INET) {
		return (NULL);
	}
	
	nm= nettoa((unsigned)addr);
	
	return (do_get(NAME_NETWORKS, &n_state, NAME_ADDRESS, nm, 
		       n_read, NULL));
}

struct netent *
_ni_getnetent(
	      void
	      )
{
	if (!n_init()) {
		return (NULL);
	}
	return (do_getent(NAME_NETWORKS, &n_state, n_read, NULL));
}

void
_ni_setnetent(
	      int stayopen
	      )
{
	if (!n_init()) {
		return;
	}
	do_set(&n_state);
}


void
_ni_endnetent(
	      void
	      )
{
	if (!n_init()) {
		return;
	}
	do_end(&n_state);
}

static void *
n_read(
       void *ni,
       ni_idlist *match,
       void *unused,
       ni_proplist *quickprops
       )
{
	static struct netent n = { 
		(char *)STR_NULL, (char **)STRLST_NULL, AF_INET, 0
	};
	
	ni_proplist pl;
	ni_index i;
	ni_namelist nl;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (n.n_name != (char *)STR_NULL) {
		free(n.n_name);
		n.n_name = (char *)STR_NULL;
	}
	if (n.n_aliases != (char **)STRLST_NULL) {
		freelist(n.n_aliases);
		n.n_aliases = (char **)STRLST_NULL;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &n.n_name);
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len  > 0) {
				nl.ninl_len--;
				nl.ninl_val++;
				namelist_get(&nl, &n.n_aliases);
			}
			
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_ADDRESS)) {
			nl = pl.nipl_val[i].nip_val;
			if (nl.ninl_len > 0) {
				n.n_net = inet_network(nl.ninl_val[0]);
			}
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&n);
}

/*
 * MOUNTS_STUFF
 */
#include <mntent.h>

static const ni_name NAME_MOUNTS = "mounts";

static const char STR_NFS[] = "nfs";
static const char STR_RW[] = "rw";

static const ni_name NAME_DIR = "dir";
static const ni_name NAME_TYPE = "type";
static const ni_name NAME_OPTS = "opts";
static const ni_name NAME_FREQ = "freq";
static const ni_name NAME_PASSNO = "passno";

static void *mnt_read(void *, ni_idlist *, void *, ni_proplist *);
#define mnt_init() do_init(NAME_MOUNTS, &mnt_state)

static ni_local_state mnt_state = { NI_INDEX_NULL };

struct mntent *
_ni_getmntent(
	      FILE *f
	      )
{
	return (do_getent(NAME_MOUNTS, &mnt_state, mnt_read, NULL));
}

int
_ni_addmntent(
	      FILE *f,
	      struct mntent *mnt
	      )
{
	sorry("addmntent");
	return (0);
}

FILE *
_ni_setmntent(
	      char *file,
	      char *type
	      )
{
	if (file != NULL || !mnt_init()) {
		return (NULL);
	}
	do_set(&mnt_state);
	return (stderr); /* just because the interface requires a FILE ptr */
}


int
_ni_endmntent(
	      FILE *f
	      )
{
	do_end(&mnt_state);
	return (1);
}


static ni_name
commafy(
	ni_namelist nl
	)
{
	ni_index i;
	int len;
	int inclen;
	ni_name list;
	
	len = 0;
	for (i = 0; i < nl.ninl_len; i++) {
		len += (strlen(nl.ninl_val[i]) + 1);
	}
	list = malloc(len);
	len = 0;
	list[0] = 0;
	for (i = 0; i < nl.ninl_len; i++) {
		inclen = strlen(nl.ninl_val[i]);
		bcopy(nl.ninl_val[i], list + len, inclen);
		len += inclen;
		list[len] = ',';
		len++;
	}
	if (len > 0) {
		list[len - 1] = 0;	/* erase last comma */
	}
	return (list);
}

static void *
mnt_read(
	 void *ni,
	 ni_idlist *match,
	 void *unused,
	 ni_proplist *quickprops
	 )
{
	static struct mntent mnt = { 
		(char *)STR_NULL,
		(char *)STR_NULL,
		(char *)STR_NFS,
		(char *)STR_RW,
		0,
		0
	};
	
	ni_proplist pl;
	ni_index i;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (mnt.mnt_fsname != (char *)STR_NULL) {
		free(mnt.mnt_fsname);
		mnt.mnt_fsname = (char *)STR_NULL;
	}
	if (mnt.mnt_dir != (char *)STR_NULL) {
		free(mnt.mnt_dir);
		mnt.mnt_dir = (char *)STR_NULL;
	}
	if (mnt.mnt_type != (char *)STR_NFS) {
		free(mnt.mnt_type);
		mnt.mnt_type = (char *)STR_NFS;
	}
	if (mnt.mnt_opts != (char *)STR_RW) {
		free(mnt.mnt_opts);
		mnt.mnt_opts = (char *)STR_RW;
	}
	mnt.mnt_freq = 0;
	mnt.mnt_passno = 0;
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			name_get(&pl.nipl_val[i].nip_val, &mnt.mnt_fsname);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_DIR)) {
			name_get(&pl.nipl_val[i].nip_val, &mnt.mnt_dir);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_TYPE)) {
			name_get(&pl.nipl_val[i].nip_val, &mnt.mnt_type);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_OPTS)) {
			mnt.mnt_opts = commafy(pl.nipl_val[i].nip_val);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_FREQ)) {
			int_get(&pl.nipl_val[i].nip_val, &mnt.mnt_freq);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_PASSNO)) {
			int_get(&pl.nipl_val[i].nip_val, &mnt.mnt_passno);
		}
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&mnt);
}

/*
 * PRINTERS_STUFF
 */
#include <printerdb.h>

static ni_name NAME_PRINTERS = "printers";

static void *prdb_read(void *, ni_idlist *, void *, ni_proplist *);
#define prdb_init() do_init(NAME_PRINTERS, &prdb_state)

static ni_local_state prdb_state = { NI_INDEX_NULL };
static prdb_ent pe;

void
_ni_prdb_set(
	     char *domain
	     )
{
	if (!prdb_init()) {
		return;
	} else {
		do_set(&prdb_state);
	}
}

prdb_ent *
_ni_prdb_get(
	     void
	     )
{
	if (!prdb_init()) {
		return (NULL);
	}
	return (do_getent(NAME_PRINTERS, &prdb_state,
			  prdb_read, NULL));
}


prdb_ent *
_ni_prdb_getbyname(
		   char *name
		   )
{
	if (!prdb_init()) {
		return (NULL);
	}
	return (do_get(NAME_PRINTERS, &prdb_state, NAME_NAME,
		       name, prdb_read, NULL));
}


void
_prdb_free_ent(
	       prdb_ent *ent
	       )
{
	ni_index i;
	
	if (ent->pe_name != NULL) {
		for (i = 0; ent->pe_name[i]; i++) {
			if (ent->pe_name[i] != (char *)STR_NULL) {
				ni_name_free(&ent->pe_name[i]);
			}
		}
		MM_FREE_ARRAY(ent->pe_name, i + 1);
		ent->pe_name = NULL;
	}
	for (i = 0; i < ent->pe_nprops; i++) {
		ni_name_free(&ent->pe_prop[i].pp_key);
		if (ent->pe_prop[i].pp_value != (char *)STR_NULL) {
			ni_name_free(&ent->pe_prop[i].pp_value);
		}
	}
	MM_FREE_ARRAY(ent->pe_prop, ent->pe_nprops);
	ent->pe_prop = NULL;
	ent->pe_nprops = 0;
}

void
_ni_prdb_end(
	     void
	     )
{
	if (!prdb_init()) {
		return;
	} else {
		do_end(&prdb_state);
	}
}


static void *
prdb_read(
	  void *ni,
	  ni_idlist *match,
	  void *unused,
	  ni_proplist *quickprops
	  )
{
	ni_proplist pl;
	ni_index i;
	ni_index j;
	ni_id id;
	ni_property prop;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	_prdb_free_ent(&pe);
	MM_ALLOC_ARRAY(pe.pe_prop, pl.nipl_len);
	pe.pe_nprops = 0;
	pe.pe_name = NULL;
	for (i = 0; i < pl.nipl_len; i++) {
		prop = pl.nipl_val[i];
		if (ni_name_match(prop.nip_name, NAME_NAME) && 
		    pe.pe_name == NULL) {
			MM_ALLOC_ARRAY(pe.pe_name, prop.nip_val.ninl_len + 1);
			for (j = 0; j < prop.nip_val.ninl_len; j++) {
				pe.pe_name[j] = ni_name_dup(prop.nip_val.ninl_val[j]);
			}
			pe.pe_name[prop.nip_val.ninl_len] = NULL;
		} else {
			pe.pe_prop[pe.pe_nprops].pp_key = ni_name_dup(prop.nip_name);
			if (prop.nip_val.ninl_len > 0) {
				(pe.pe_prop[pe.pe_nprops].pp_value = 
				 ni_name_dup(prop.nip_val.ninl_val[0]));
			} else {
				pe.pe_prop[pe.pe_nprops].pp_value = (char *)STR_NULL;
			}
			pe.pe_nprops++;
		}
	}
	if (pe.pe_name == NULL) {
		MM_ALLOC_ARRAY(pe.pe_name, 2);
		pe.pe_name[0] = (char *)STR_NULL;
		pe.pe_name[1] = NULL;
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return ((void *)&pe);
}

/*
 * BOOTP STUFF
 */
#define bootp_init() do_init(NAME_MACHINES, &bootp_state)
static const char NAME_EN_ADDRESS[] = "en_address";
static const char NAME_BOOTFILE[] = "bootfile";
static ni_local_state bootp_state = { NI_INDEX_NULL };
static void *bootp_read(void *, ni_idlist *, void *, ni_proplist *);


extern struct ether_addr ether_aton(char *);
extern char *ether_ntoa(struct ether_addr *);

typedef struct bootp_stuff {
	char *name;
	struct ether_addr en_addr;
	struct in_addr ip_addr;
	char *bootfile;
} bootp_stuff;
	
	
static struct bootp_stuff stuff;


/*
 * Converts ethernet minimal form (X:X:X:X:X:X) to
 * maximal form (XX:XX:XX:XX:XX:XX)
 */
static char *
maxaddrof(
	  char *minaddr
	  )
{
	static char maxaddr[18];
	int i;
	char *mx;
	char *mn;

	mn = minaddr;
	mx = maxaddr;
	for (i = 0; i < 6; i++) {
		if (mn[1] == ':' || mn[1] == 0) {
			mx[0] = '0';
			mx[1] = mn[0];
			mx[2] = mn[1];
			mn += 2;
		} else {
			mx[0] = mn[0];
			mx[1] = mn[1];
			mx[2] = mn[2];
			mn += 3;
		}
		mx += 3;
	}
	return (maxaddr);
}

int
_ni_bootp_getbyether(
		     struct ether_addr *en,
		     char **name,
		     struct in_addr *ip,
		     char **bootfile
		     )
{
	char *minaddr;	/* minimal form - X:X:X:X:X:X */
	char *maxaddr;	/* maximal form - XX:XX:XX:XX:XX:XX */

	if (!bootp_init()) {
		return (0);
	}
	minaddr = ether_ntoa(en);
	maxaddr = maxaddrof(minaddr);
	if (do_get(NAME_MACHINES, &bootp_state, NAME_EN_ADDRESS,
		   minaddr, bootp_read, &stuff) == NULL &&
	    do_get(NAME_MACHINES, &bootp_state, NAME_EN_ADDRESS,
		   maxaddr, bootp_read, &stuff) == NULL) {
		return (0);
	}
	*ip = stuff.ip_addr;
	*name = stuff.name;
	*bootfile = stuff.bootfile;
	return (1);
}

int
_ni_bootp_getbyip(
		  struct in_addr *ip,
		  char **name,
		  struct ether_addr *en,
		  char **bootfile
		  )
{
	if (!bootp_init()) {
		return (0);
	}
	if (do_get(NAME_MACHINES, &bootp_state, NAME_IP_ADDRESS,
		   inet_ntoa(*ip), bootp_read, &stuff) == NULL) {
		return (0);
	}
	*ip = stuff.ip_addr;
	*name = stuff.name;
	*bootfile = stuff.bootfile;
	return (1);
}

static void *
bootp_read(
	   void *ni,
	   ni_idlist *match,
	   void *unused,
	   ni_proplist *quickprops
	   )
{
	ni_proplist pl;
	ni_property prop;
	ni_index i;
	ni_id id;
	int gotether; /* do not reply unless we have a ether address */

	if (stuff.name != NULL) {
		ni_name_free(&stuff.name);
	}
	if (stuff.bootfile != NULL) {
		ni_name_free(&stuff.bootfile);
	}
	bzero(&stuff, sizeof(stuff));
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	gotether = 0;
	for (i = 0; i < pl.nipl_len; i++) {
		prop = pl.nipl_val[i];
		if (ni_name_match(prop.nip_name, NAME_NAME) &&
		    prop.nip_val.ninl_len > 0) {
			stuff.name = ni_name_dup(prop.nip_val.ninl_val[0]);
		} else if (ni_name_match(prop.nip_name, 
					 NAME_IP_ADDRESS) &&
			   prop.nip_val.ninl_len > 0) {
			(stuff.ip_addr.s_addr = 
			 inet_addr(prop.nip_val.ninl_val[0]));
		} else if (ni_name_match(prop.nip_name, 
					 NAME_EN_ADDRESS) &&
			   prop.nip_val.ninl_len > 0) {
			gotether++;
			stuff.en_addr = ether_aton(prop.nip_val.ninl_val[0]);
		} else if (ni_name_match(prop.nip_name, 
					 NAME_BOOTFILE) &&
			   prop.nip_val.ninl_len > 0) {
			stuff.bootfile = ni_name_dup(prop.nip_val.ninl_val[0]);
		}
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (gotether ? &stuff : NULL);
}

/*
 * BOOTPARAMS_STUFF
 */
#define bootparams_init() do_init(NAME_MACHINES, &bp_state)
static const char NAME_BOOTPARAMS[] = "bootparams";
static ni_local_state bp_state = { NI_INDEX_NULL };
static void *bp_read(void *, ni_idlist *, void *, ni_proplist *);

int
_ni_bootparams_getbyname(
			 char *client,
			 char ***values
			 )
{
	static ni_namelist nl;

	if (!bootparams_init()) {
		return (0);
	}
	if (nl.ninl_len > 0) {
		ni_namelist_free(&nl);
	}
	if (do_get(NAME_MACHINES, &bp_state, NAME_NAME,
		   client, bp_read, &nl) != NULL) {
		*values = nl.ninl_val;
		return (nl.ninl_len);
	} else {
		return (-1);
	}
}

static void *
bp_read(
	void *ni,
	ni_idlist *match,
	void *arg,
	ni_proplist *quickprops
	)
{
	ni_proplist pl;
	ni_index i;
	ni_id id;

	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_BOOTPARAMS)) {
			(*((ni_namelist *)arg) =
			 ni_namelist_dup(pl.nipl_val[i].nip_val));
			if (quickprops == NULL) {
				ni_proplist_free(&pl);
			}
			return (arg);
		}
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (NULL);
}


/*
 * ALIAS_STUFF
 */
static const ni_name NAME_ALIASES = "aliases";
static const ni_name NAME_MEMBERS = "members";

static void *alias_read(void *, ni_idlist *, void *, ni_proplist *);
#define alias_init() do_init(NAME_ALIASES, &alias_state)

static ni_local_state alias_state = { NI_INDEX_NULL };

aliasent *
_ni_alias_getbyname(
		    char *name
		    )
{
	if (!alias_init()) {
		return (NULL);
	}
	return (do_get(NAME_ALIASES, &alias_state, NAME_NAME, name, 
		       alias_read, NULL));
}

aliasent *
_ni_alias_getent(
		 void
		 )
{
	if (!alias_init()) {
		return (NULL);
	}
	return (do_getent(NAME_ALIASES, &alias_state, alias_read, NULL));
}

void
_ni_alias_setent(
		 void
		 )
{
	if (!alias_init()) {
		return;
	}
	do_set(&alias_state);
}

void
_ni_alias_endent(
		 void
		 )
{
	if (!alias_init()) {
		return;
	}
	do_end(&alias_state);
}

static void *
alias_read(
	   void *ni,
	   ni_idlist *match,
	   void *unused,
	   ni_proplist *quickprops
       )
{
	static aliasent alias = {
		(char *)STR_NULL, 
		0,
		(char **)STRLST_NULL,
	};
	
	ni_proplist pl;
	ni_index i;
	ni_namelist nl;
	ni_id id;
	
	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	if (alias.alias_name != (char *)STR_NULL) {
		free(alias.alias_name);
		alias.alias_name = (char *)STR_NULL;
	}
	for (i = 0; i < alias.alias_members_len; i++) {
		free(alias.alias_members[i]);
	}
	alias.alias_members_len = 0;
	if (alias.alias_members != (char **)STRLST_NULL) {
		free(alias.alias_members);
		alias.alias_members = (char **)STRLST_NULL;
	}
	alias.alias_local = 0;
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NAME)) {
			alias.alias_local = (ni == nichain.val[0].ni);
			name_get(&pl.nipl_val[i].nip_val, &alias.alias_name);
		} else if (ni_name_match(pl.nipl_val[i].nip_name, 
					 NAME_MEMBERS)) {
			nl = ni_namelist_dup(pl.nipl_val[i].nip_val);
			alias.alias_members_len = nl.ninl_len;
			alias.alias_members = nl.ninl_val;
		} 
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (&alias);
}


/*
 * NETGROUP_STUFF
 */
#define netgroup_init() do_init(NAME_MACHINES, &ng_state)
static const char NAME_NETGROUPS[] = "netgroups";
static ni_local_state ng_state = { NI_INDEX_NULL };
static void *ng_read(void *, ni_idlist *, void *, ni_proplist *);

void
_ni_setnetgrent(
		char *group
		)
{
}

int
_ni_getnetgrent(
		char **machinep,
		char **userp,
		char **domainp
		)
{
	return (0);
}

void
_ni_endnetgrent(void)
{
}

int
_ni_innetgr(
	    char *group,
	    char *host,
	    char *user,
	    char *domain
	    )
{
	if (!netgroup_init()) {
		return (0);
	}
	if (host == NULL) {
		return (0);
	}
	if (do_get(NAME_MACHINES, &ng_state, NAME_NAME,
		   host, ng_read, group) != NULL) {
		return (1);
	} else {
		return (0);
	}
}


static void *
ng_read(
	void *ni,
	ni_idlist *match,
	void *arg,
	ni_proplist *quickprops
	)
{
	ni_proplist pl;
	ni_index i;
	ni_id id;
	char *group = (char *)arg;

	if (quickprops == NULL) {
		id.nii_object = match->niil_val[0];
		if (ni_read(ni, &id, &pl) != NI_OK) {
			return (NULL);
		}
	} else {
		pl = *quickprops;
	}
	for (i = 0; i < pl.nipl_len; i++) {
		if (ni_name_match(pl.nipl_val[i].nip_name, NAME_NETGROUPS)) {
			if (ni_namelist_match(pl.nipl_val[i].nip_val, 
					      group) != NI_INDEX_NULL) {
				if (quickprops == NULL) {
					ni_proplist_free(&pl);
				}
				return (arg);
			}
		}
	}
	if (quickprops == NULL) {
		ni_proplist_free(&pl);
	}
	return (NULL);
}

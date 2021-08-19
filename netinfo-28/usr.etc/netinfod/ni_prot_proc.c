/*
 * Server-side procedure implementation
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include "ni_server.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <stdio.h>
#include "clib.h"
#include "mm.h"
#include "system.h"
#include "ni_globals.h"
#include "checksum.h"
#include "notify.h"
#include "ni_dir.h"
#include "socket_lock.h"
#include "alert.h"

#define MAXINTSTRSIZE sizeof("4294967296") /* size of largest integer */

#define NI_SEPARATOR '/'		/* separator for netinfo values */

/*
 * These definitions are used when doing the search for the parent. When
 * a timeout occurs because we are unable to find the parent server, we
 * sleep for 30 seconds, checking once a second to see if the user decided
 * to abort the search. After 30 seconds, the search is continued again.
 */
#define PARENT_NINTERVALS 30 	
#define PARENT_SLEEPTIME 1	
 
/*
 * How long to wait when many-casting for a parent server
 */
#define MANY_CAST_TIMEOUT 30

/*
 * How long to wait when pinging a parent server to see if it is still
 * alive. Ping once every two seconds, ten seconds total.
 */
#define PING_TIMEOUT  10
#define PING_TRIES    5		/* number of retries for above */

extern bool_t readall();
static int ni_ping(u_long, ni_name);

/*
 * Is this call an update from the master?
 * XXX: the method used is to look for a privileged port from the master
 * server. There is no way to distinguish this from a call from a root
 * user on the master (versus the master server process).
 */
static int
isupdate(
	 struct svc_req *req
	 )
{
	struct sockaddr_in *sin = svc_getcaller(req->rq_xprt);

	/*
	 * XXX: Master could have multiple addresses: should check all of
	 * them.
	 */
	return (sin->sin_addr.s_addr == master_addr && 
		sin->sin_port < IPPORT_RESERVED);
}

/*
 * Authenticate a NetInfo call. Only required for write operations.
 * NetInfo uses passwords for authentications, but does not send them
 * in the clear. Instead, a trivial authentication system is used to
 * defeat packet browsers.
 *
 * UNIX-style RPC authentication is used, with a gross hack that the encrypted
 * password is placed in the machine-name field.
 *
 * XXX: design a better authentication system (non-trivial!)
 */
static ni_status
authenticate(
	     void *ni,
	     struct svc_req *req
	     )
{
	struct sockaddr_in *sin = svc_getcaller(req->rq_xprt);
	struct authunix_parms *aup;
	char *p;
	ni_namelist nl;
	ni_status status;
	ni_id id;
	ni_idlist idl;
	char uidstr[MAXINTSTRSIZE];

	/*
	 * Root on the local machine can do anything
	 */
	if (sys_ismyaddress(sin->sin_addr.s_addr) && 
	    sin->sin_port < IPPORT_RESERVED) {
		ni_setuser(ni, ACCESS_USER_SUPER);
		return (NI_OK);
	}
	if (req->rq_cred.oa_flavor != AUTH_UNIX) {
		ni_setuser(ni, NULL);
		return (NI_OK);
	}
	aup = (struct authunix_parms *)req->rq_clntcred;

	status = ni_root(ni, &id);
	if (status != NI_OK) {
		return (status);
	}
	status = ni_lookup(ni, &id, NAME_NAME, NAME_USERS, &idl);
	if (status != NI_OK) {
		return (status == NI_NODIR ? NI_NOUSER : status);
	}
	id.nii_object = idl.niil_val[0];
	ni_idlist_free(&idl);
	sprintf(uidstr, "%d", aup->aup_uid);
	status = ni_lookup(ni, &id, NAME_UID, uidstr, &idl);
	if (status != NI_OK) {
		return (status == NI_NODIR ? NI_NOUSER : status);
	}
	id.nii_object = idl.niil_val[0];
	ni_idlist_free(&idl);
	status = ni_lookupprop(ni, &id, NAME_PASSWD, &nl);
	if (status != NI_OK && status != NI_NOPROP) {
		return (status);
	}
	if (status == NI_OK && nl.ninl_len > 0) {
		/*
		 * Our trivial encryption scheme just inverts the bits
		 */
		for (p = aup->aup_machname; *p; p++) {
			*p = ~(*p);
		}
		if (strcmp(nl.ninl_val[0], crypt(aup->aup_machname,
						 nl.ninl_val[0])) != 0) {
			ni_namelist_free(&nl);
			return (NI_AUTHERROR);
		}
		ni_namelist_free(&nl);
	}
	status = ni_lookupprop(ni, &id, NAME_NAME, &nl);
	if (status != NI_OK) {
		return (status == NI_NOPROP ? NI_NOUSER : status);
	}
	if (nl.ninl_len == 0) {
		return (NI_NOUSER);
	}
	ni_setuser(ni, nl.ninl_val[0]);
	ni_namelist_free(&nl);
	return (NI_OK);
}

/*
 * Validate that a write-call is allowed. A write call is allowed
 * to the master if the user is correctly authenticated and permission
 * is allowed. To the clone, the write is only allowed if it comes
 * from the master (an update).
 */
static ni_status
validate_write(
	       struct svc_req *req
	       )
{
	ni_status status;

	status = NI_OK;
	if (i_am_clone) {
		if (!isupdate(req)) {
			status = NI_RDONLY;
		} else {
			ni_setuser(db_ni, ACCESS_USER_SUPER);
		}
	} else {
		status = authenticate(db_ni, req);
	}
	return (status);
}

/*
 * The NetInfo PING procedure
 */
void *
_ni_ping_2(
	   void *arg,
	   struct svc_req *req
	   )
{
	return ((void *)~0);
}

/*
 * The NetInfo statistics procedure. Only the checksum is returned as
 * of now, but the format is flexible enough to allow other items to
 * be returned if future implementations choose to do so.
 * 
 * TODO: return more statistics to improve performance.
 */
ni_proplist *
_ni_statistics_2(
		 void *arg,
		 struct svc_req *req
		 )
{
	static char buf[MAXINTSTRSIZE];
	static ni_name val = buf;
	static ni_property prop = { "checksum", { 1, &val }};
	static ni_proplist res = { 1, &prop };

	if (req == NULL) {
		return (NULL);
	}
	sprintf(buf, "%u", db_checksum);
	return (&res);
}

/*
 * The NetInfo ROOT procedure
 */
ni_id_res *
_ni_root_2(
	   void *arg,
	   struct svc_req *req
	   )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = ni_root(db_ni, &res.ni_id_res_u.id);
	return (&res);
}

/*
 * The NetInfo SELF procedure
 */
ni_id_res *
_ni_self_2(
	   ni_id *arg,
	   struct svc_req *req
	   )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.ni_id_res_u.id = *arg;
	res.status = ni_self(db_ni, &res.ni_id_res_u.id);
	return (&res);
}

/*
 * The NetInfo PARENT procedure
 */
ni_parent_res * 
_ni_parent_2(
	     ni_id *arg,
	     struct svc_req *req
	     )
{
	static ni_parent_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.ni_parent_res_u.stuff.self_id = *arg;
	res.status = ni_parent(db_ni, &res.ni_parent_res_u.stuff.self_id,
			       &res.ni_parent_res_u.stuff.object_id);
	return (&res);
}

/*
 * The NetInfo CHILDREN procedure
 */
ni_children_res *
_ni_children_2(
	       ni_id *arg,
	       struct svc_req *req
	       )
{
	static ni_children_res res;

	ni_idlist_free(&res.ni_children_res_u.stuff.children);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_children_res_u.stuff.self_id = *arg;
	res.status = ni_children(db_ni, &res.ni_children_res_u.stuff.self_id, 
				 &res.ni_children_res_u.stuff.children);
	return (&res);
}

/*
 * The NetInfo CREATE procedure
 */
ni_create_res *
_ni_create_2(
	     ni_create_args *arg,
	     struct svc_req *req
	     )
{
	static ni_create_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	if (arg->target_id != NULL) {
		res.ni_create_res_u.stuff.id = *arg->target_id;
	} else {
		res.ni_create_res_u.stuff.id.nii_object = NI_INDEX_NULL;
	}
	res.ni_create_res_u.stuff.self_id = arg->id;
	res.status = ni_create(db_ni, &res.ni_create_res_u.stuff.self_id, 
			       arg->props, &res.ni_create_res_u.stuff.id,
			       arg->where);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			if (arg->target_id == NULL) {
				MM_ALLOC(arg->target_id);
				*arg->target_id = res.ni_create_res_u.stuff.id;
			}
			notify_clients(_NI_CREATE, arg);
		}
		checksum_inc(&db_checksum, 
			     res.ni_create_res_u.stuff.self_id);
		checksum_add(&db_checksum, res.ni_create_res_u.stuff.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo DESTROY procedure
 */
ni_id_res *
_ni_destroy_2(
	      ni_destroy_args *arg,
	      struct svc_req *req
	      )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->parent_id;
	res.status = ni_destroy(db_ni, &res.ni_id_res_u.id, arg->self_id);
	if (res.status == NI_OK) {
		/* 
		 * Must compute checksum first, because notify_clients
		 * destroys argument
		 */
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
		checksum_rem(&db_checksum, arg->self_id);
		if (!i_am_clone) {
			notify_clients(_NI_DESTROY, arg);
		}
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo READ procedure
 */
ni_proplist_res *
_ni_read_2(
	   ni_id *arg,
	   struct svc_req *req
	   )
{
	static ni_proplist_res res;

#if !ENABLE_CACHE
	ni_proplist_free(&res.ni_proplist_res_u.stuff.props);
#endif
	if (req == NULL) {
		return (NULL);
	}
	res.ni_proplist_res_u.stuff.id = *arg;
	res.status = ni_read(db_ni, &res.ni_proplist_res_u.stuff.id, 
			     &res.ni_proplist_res_u.stuff.props);
	return (&res);
}

/*
 * The NetInfo WRITE procedure
 */
ni_id_res *
_ni_write_2(
	    ni_proplist_stuff *arg,
	    struct svc_req *req
	    )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_write(db_ni, &res.ni_id_res_u.id, arg->props);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_WRITE, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo LOOKUP procedure
 */
ni_lookup_res *
_ni_lookup_2(
	     ni_lookup_args *arg,
	     struct svc_req *req
	     )
{
	static ni_lookup_res res;

	ni_idlist_free(&res.ni_lookup_res_u.stuff.idlist);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_lookup_res_u.stuff.self_id = arg->id;
	res.status = ni_lookup(db_ni, &res.ni_lookup_res_u.stuff.self_id,
			       arg->key, arg->value, 
			       &res.ni_lookup_res_u.stuff.idlist);
	return (&res);
}

/*
 * The NetInfo LOOKUPREAD procedure
 */
ni_proplist_res *
_ni_lookupread_2(
		 ni_lookup_args *arg,
		 struct svc_req *req
		 )
{
	static ni_proplist_res res;

#if !ENABLE_CACHE
	ni_proplist_free(&res.ni_proplist_res_u.stuff.props);
#endif
	if (req == NULL) {
		return (NULL);
	}
	res.ni_proplist_res_u.stuff.id = arg->id;
	res.status = ni_lookupread(db_ni, 
				   &res.ni_proplist_res_u.stuff.id,
				   arg->key, arg->value, 
				   &res.ni_proplist_res_u.stuff.props);
	return (&res);
}

/*
 * The NetInfo LIST procedure
 */
ni_list_res *
_ni_list_2(
	   ni_name_args *arg,
	   struct svc_req *req
	   )
{
	static ni_list_res res;

	if (req == NULL) {
		ni_list_const_free(db_ni);
		return (NULL);
	}
	res.ni_list_res_u.stuff.self_id = arg->id;
	res.status = ni_list_const(db_ni, &res.ni_list_res_u.stuff.self_id,
				   arg->name, 
				   &res.ni_list_res_u.stuff.entries);
	return (&res);
}

/*
 * WARNING: this function is dangerous and may be removed in future
 * implementations of the protocol.
 * While it is easier on the network, it eats up too much time on the
 * server and interrupts service for others.
 * 
 * PLEASE DO NOT CALL IT!!!
 */
ni_listall_res *
_ni_listall_2(
	      ni_id *id,
	      struct svc_req *req
	      )
{
	static ni_listall_res res;

	ni_proplist_list_free(&res.ni_listall_res_u.stuff.entries);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_listall_res_u.stuff.self_id = *id;
	res.status = ni_listall(db_ni, &res.ni_listall_res_u.stuff.self_id,
			     &res.ni_listall_res_u.stuff.entries);
	return (&res);
}

/*
 * The NetInfo READPROP procedure
 */
ni_namelist_res *
_ni_readprop_2(
	       ni_prop_args *arg,
	       struct svc_req *req
	       )
{
	static ni_namelist_res res;

	ni_namelist_free(&res.ni_namelist_res_u.stuff.values);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_namelist_res_u.stuff.self_id = arg->id;
	res.status = ni_readprop(db_ni, &res.ni_namelist_res_u.stuff.self_id,
				 arg->prop_index,
				 &res.ni_namelist_res_u.stuff.values);
	return (&res);
}

/*
 * The NetInfo WRITEPROP procedure
 */
ni_id_res *
_ni_writeprop_2(
		ni_writeprop_args *arg,
		struct svc_req *req
		)
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_writeprop(db_ni, &res.ni_id_res_u.id,
				  arg->prop_index,
				  arg->values);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_WRITEPROP, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo LISTPROPS procedure
 */
ni_namelist_res *
_ni_listprops_2(
		ni_id *arg,
		struct svc_req *req
		)
{
	static ni_namelist_res res;

	ni_namelist_free(&res.ni_namelist_res_u.stuff.values);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_namelist_res_u.stuff.self_id = *arg;
	res.status = ni_listprops(db_ni, &res.ni_namelist_res_u.stuff.self_id, 
				  &res.ni_namelist_res_u.stuff.values);
	return (&res);
}
	

/*
 * The NetInfo CREATEPROP procedure
 */	
ni_id_res *
_ni_createprop_2(
		 ni_createprop_args *arg,
		 struct svc_req *req
		 )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_createprop(db_ni, &res.ni_id_res_u.id, arg->prop, arg->where);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_CREATEPROP, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo DESTROYPROP procedure
 */
ni_id_res *
_ni_destroyprop_2(
		  ni_prop_args *arg,
		  struct svc_req *req
		  )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_destroyprop(db_ni, &res.ni_id_res_u.id, arg->prop_index);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_DESTROYPROP, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo RENAMEPROP procedure
 */
ni_id_res *
_ni_renameprop_2(
		 ni_propname_args *arg,
		 struct svc_req *req
		 )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_renameprop(db_ni, &res.ni_id_res_u.id, 
				   arg->prop_index, arg->name);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_RENAMEPROP, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
		
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo CREATENAME procedure
 */
ni_id_res *
_ni_createname_2(
		 ni_createname_args *arg,
		 struct svc_req *req
		 )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_createname(db_ni, &res.ni_id_res_u.id, arg->prop_index, arg->name,
				   arg->where);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_CREATENAME, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo WRITENAME procedure
 */
ni_id_res *
_ni_writename_2(
		ni_writename_args *arg,
		struct svc_req *req
		)
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_writename(db_ni, &res.ni_id_res_u.id, 
				    arg->prop_index, arg->name_index,
				    arg->name);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_WRITENAME, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * The NetInfo READNAME procedure 
 */
ni_readname_res *
_ni_readname_2(
	       ni_nameindex_args *arg,
	       struct svc_req *req
		)
{
	static ni_readname_res res;

	ni_name_free(&res.ni_readname_res_u.stuff.name);
	if (req == NULL) {
		return (NULL);
	}
	res.ni_readname_res_u.stuff.id = arg->id;
	res.status = ni_readname(db_ni, &res.ni_readname_res_u.stuff.id, 
				 arg->prop_index, arg->name_index,
				 &res.ni_readname_res_u.stuff.name);
	return (&res);
}

/*
 * The NetInfo DESTROYNAME procedure
 */
ni_id_res *
_ni_destroyname_2(
		  ni_nameindex_args *arg,
		  struct svc_req *req
		  )
{
	static ni_id_res res;

	if (req == NULL) {
		return (NULL);
	}
	res.status = validate_write(req);
	if (res.status != NI_OK) {
		return (&res);
	}
	res.ni_id_res_u.id = arg->id;
	res.status = ni_destroyname(db_ni, &res.ni_id_res_u.id, 
				    arg->prop_index, arg->name_index);
	if (res.status == NI_OK) {
		if (!i_am_clone) {
			notify_clients(_NI_DESTROYNAME, arg);
		}
		checksum_inc(&db_checksum, res.ni_id_res_u.id);
	} else if (i_am_clone) {
		dir_clonecheck();
	}
	return (&res);
}

/*
 * Given a NetInfo "serves" value (contains a slash character), does
 * the given tag match the field on the right of the value?
 */
static int
tag_match(
	  ni_name slashtag,
	  ni_name tag
	  )
{
	ni_name sep;
	int len;

	sep = index(slashtag, NI_SEPARATOR);
	if (sep == NULL) {
		return (0);
	}
	if (!ni_name_match(sep + 1, tag)) {
		return (0);
	}

	/*
	 * Ignore special values "." and ".."
	 */
	len = sep - slashtag;
	if (ni_name_match_seg(NAME_DOT, slashtag, len) ||
	    ni_name_match_seg(NAME_DOTDOT, slashtag, len)) {
		return (0);
	}

	return (1);
}

/*
 * The NetInfo BIND procedure.
 *
 * Only reply if served to avoid creating unnecessary net traffic. This
 * is implemented by returned ~NULL if served, NULL otherwise (an rpcgen
 * convention).
 */
void *
_ni_bind_2(
	   ni_binding *binding,
	   struct svc_req *req
	   )
{
	ni_id id;
	ni_idlist ids;
	ni_namelist nl;
	ni_index i;
	ni_name address;
	struct in_addr inaddr;
	ni_status status;

	if (req == NULL) {
		return (NULL);
	}
	if (db_ni == NULL) {
		return (NULL);
	}
	if (ni_root(db_ni, &id) != NI_OK) {
		return (NULL);
	}
	if (ni_lookup(db_ni, &id, NAME_NAME, NAME_MACHINES, 
		      &ids) != NI_OK) {
		return (NULL);
	}
	id.nii_object = ids.niil_val[0];
	ni_idlist_free(&ids);
	inaddr.s_addr = binding->addr;
	address = inet_ntoa(inaddr);
	status = ni_lookup(db_ni, &id, NAME_IP_ADDRESS, address, &ids);
	if (status != NI_OK) {
		return (NULL);
	}
	id.nii_object = ids.niil_val[0];
	ni_idlist_free(&ids);
	status = ni_lookupprop(db_ni, &id, NAME_SERVES, &nl);
	if (status != NI_OK) {
		return (NULL);
	}
	for (i = 0; i < nl.ninl_len; i++) {
		if (tag_match(nl.ninl_val[i], binding->tag)) {
			ni_namelist_free(&nl);
			return ((void *)~0);
		}
	}
	ni_namelist_free(&nl);
	return (NULL);
}

/*
 * Data structure used to hold arguments and results needed for calling
 * the BIND procedure which is broadcast and receives many replies.
 */
typedef struct ni_rparent_stuff {
	nibind_bind_args *bindings; /* arguements to BIND */
	ni_rparent_res *res;	    /* result from BIND */
} ni_rparent_stuff;

/*
 * Catches a BIND reply
 */
static bool_t
catch(
      ni_rparent_stuff *stuff,
      struct sockaddr_in *raddr,
      int which
      )
{
	ni_name_free(&stuff->res->ni_rparent_res_u.binding.tag);
	(stuff->res->ni_rparent_res_u.binding.tag =
	 ni_name_dup(stuff->bindings[which].server_tag));
	stuff->res->ni_rparent_res_u.binding.addr = raddr->sin_addr.s_addr;
	stuff->res->status = NI_OK;
	return (TRUE);
}

/*
 * Determine if this entry serves ".." (i.e. it has a serves property of
 * which one of the values looks like ../SOMETAG.
 */       
static unsigned
servesdotdot(
	     ni_entry entry,
	     ni_name *tag
	     )
{
	ni_name name;
	ni_name sep;
	unsigned addr;
	ni_namelist nl;
	ni_index i;
	ni_id id;

	if (entry.names == NULL) {
		return (0);
	}
	id.nii_object = entry.id;
	for (i = 0; i < entry.names->ninl_len; i++) {
		name = entry.names->ninl_val[i];
		sep = index(name, NI_SEPARATOR);
		if (sep == NULL) {
			continue;
		}

		if (!ni_name_match_seg(NAME_DOTDOT, name, sep - name)) {
			continue;
		}
		if (ni_lookupprop(db_ni, &id, NAME_IP_ADDRESS, &nl) != NI_OK) {
			continue;
		}
		if (nl.ninl_len == 0) {
			continue;
		}
		addr = inet_addr(nl.ninl_val[0]);
		ni_namelist_free(&nl);
		*tag = ni_name_dup(sep + 1);
		return (addr);
	}
	return (0);
}

/*
 * Find the addresses for the parent servers and call the BIND procedure
 * on each of them. Return 0 if no parent servers are wired into the local
 * database, 1 otherwise.
 */
static int
hardwired(
	  void *ni,
	  ni_rparent_res *res
	  )
{
	ni_name tag;
	ni_id id;
	ni_idlist ids;
	ni_entrylist entries;
	ni_index i;
	unsigned long addr;
	ni_rparent_stuff stuff;
	ni_status status;
	ni_name server_tag;
	struct in_addr *addrs;
	unsigned naddrs;
	unsigned long myaddr;

	if (ni_root(ni, &id) != NI_OK) {
		return (0);
	}
	if (ni_lookup(ni, &id, NAME_NAME, NAME_MACHINES, &ids) != NI_OK) {
		return (0);
	}
	id.nii_object = ids.niil_val[0];
	ni_idlist_free(&ids);
	status = ni_list_const(ni, &id, NAME_SERVES, &entries);
	if (status != NI_OK) {
		return (0);
	}
	tag = ni_tagname(ni);

	addrs = NULL;
	naddrs = 0;
	stuff.bindings = NULL;
	myaddr = sys_address();
	for (i = 0; i < entries.niel_len; i++) {
		addr = servesdotdot(entries.niel_val[i], &server_tag);
		if (addr != 0) {
			MM_GROW_ARRAY(addrs, naddrs);
			addrs[naddrs].s_addr = addr;
			MM_GROW_ARRAY(stuff.bindings, naddrs);
			stuff.bindings[naddrs].client_tag = tag;
			stuff.bindings[naddrs].client_addr = myaddr;
			stuff.bindings[naddrs].server_tag = server_tag;
			naddrs++;
		}
	}
	if (naddrs == 0) {
		ni_name_free(&tag);
		ni_list_const_free(ni);
		return (0);
	}

	/*
	 * Found the addresses and committed to manycasting now
	 */
	stuff.res = res;

	if (_many_cast_args(naddrs,
			   addrs, NIBIND_PROG, NIBIND_VERS, NIBIND_BIND,
			   xdr_nibind_bind_args, stuff.bindings, 
			   sizeof(nibind_bind_args),
			   xdr_void, &stuff, 
			   catch, MANY_CAST_TIMEOUT) != RPC_SUCCESS) {
		alert_open();
		res->status = NI_NORESPONSE;
	} else {
		alert_close();
	}
	ni_name_free(&tag);
	MM_FREE_ARRAY(addrs, naddrs);
	for (i = 0; i < naddrs; i++) {
		ni_name_free(&stuff.bindings[i].server_tag);
	}
	MM_FREE_ARRAY(stuff.bindings, naddrs);
	ni_list_const_free(ni);
	return (1);
}

/*
 * The NetInfo RPARENT procedure
 */
ni_rparent_res *
_ni_rparent_2(
	      void *arg,
	      struct svc_req *req
	      )
{
	static ni_rparent_res res = { NI_FAILED };
	
	if (req == NULL) {
		return (NULL);
	}
	/*
	 * If standalone (i.e. no network attached), then stop here
	 */
	if (alert_aborted() || sys_standalone()) {
		res.status = NI_NETROOT;
		return (&res);
	}

	/*
	 * If already have the result, return it.
	 */
	if (res.status == NI_OK) {
		if (ni_ping(res.ni_rparent_res_u.binding.addr,
			    res.ni_rparent_res_u.binding.tag)) {
			return (&res);
		}
	}

	/*
	 * If there are hard-wired addresses, use them.
	 */
	if (hardwired(db_ni, &res)) {
		return (&res);
	}

	/*
	 * Otherwise, we've hit the network root
	 */
	res.status = NI_NETROOT;
	return (&res);
}

/*
 * Called at startup: try to locate a parent server, allowing for user
 * to abort search if timeouts occur.
 */
void
waitforparent(void)
{
	ni_rparent_res *res;
	int i;

	alert_enable(1);
	for (;;) {
		res = _ni_rparent_2(NULL, (struct svc_req *)(~0));
		if (res->status != NI_NORESPONSE) {
			alert_enable(0);
			return;
		}
		for (i = 0; i < PARENT_NINTERVALS; i++) {
			if (alert_aborted()) {
				alert_enable(0);
				return;
			}
			sleep(PARENT_SLEEPTIME);
		}
	}
}		   

/*
 * The NetInfo CRASHED procedure
 * If master, do nothing. If clone, check that our database is up to date.
 */
void *
_ni_crashed_2(
	      unsigned *checksum,
	      struct svc_req *req
	      )
{
	if (req == NULL) {
		return (NULL);
	}
	if (i_am_clone) {
		if (*checksum != db_checksum) {
			dir_clonecheck();
		}
	}
	return ((void *)~0);
}

/*
 * The NetInfo READALL procedure
 *
 * XXX: doing a readall takes a long time. We should really
 * fork a thread to do this and disable writes until it is done.
 */
ni_readall_res *
_ni_readall_2(
	      unsigned *checksum,
	      struct svc_req *req
	      )
{
	static ni_readall_res res;
	int didit;

	if (req == NULL) {
		return (NULL);
	}
	if (i_am_clone) {
		res.status = NI_NOTMASTER;
		return (&res);
	}
	if (*checksum == db_checksum) {
		res.status = NI_OK;
		res.ni_readall_res_u.stuff.checksum = 0;
		res.ni_readall_res_u.stuff.list = NULL;
		return (&res);
	}
	socket_lock();
	didit = svc_sendreply(req->rq_xprt, readall, db_ni);
	socket_unlock();
	if (!didit) {
		sys_errmsg("can't readall");
	}
	return (NULL);
}

/*
 * The NetInfo RESYNC procedure
 */
ni_status *
_ni_resync_2(
	     void *arg,
	     struct svc_req *req
	     )
{
	static ni_status status;

	if (req == NULL) {
		return (NULL);
	}
	if (i_am_clone) {
		dir_clonecheck();
	} else {
		notify_resync();
	} 
	status = NI_OK;
	return (&status);
}


/*
 * Ping the server at the given address/tag
 */
static int
ni_ping(
	u_long address,
	ni_name tag
	)
{
	struct sockaddr_in sin;
	struct timeval tv;
	enum clnt_stat stat;
	int sock;
	nibind_getregister_res res;
	CLIENT *cl;

	sin.sin_family = AF_INET;
	sin.sin_port = 0;
	sin.sin_addr.s_addr = address;
	bzero(sin.sin_zero, sizeof(sin.sin_zero));
	sock = socket_open(&sin, NIBIND_PROG, NIBIND_VERS);
	if (sock < 0) {
		return (0);
	}
	tv.tv_usec = 0;
	tv.tv_sec = PING_TIMEOUT / PING_TRIES;
	cl = clntudp_create(&sin, NIBIND_PROG, NIBIND_VERS, 
			    tv, &sock);
	if (cl == NULL) {
		(void)socket_close(sock);
		return (0);
	}
	tv.tv_sec = PING_TIMEOUT;
	stat = clnt_call(cl, NIBIND_GETREGISTER, 
			 xdr_ni_name, &tag, xdr_nibind_getregister_res,
			 &res, tv);
	clnt_destroy(cl);
	(void)socket_close(sock);
	return (stat == RPC_SUCCESS && res.status == NI_OK);
}


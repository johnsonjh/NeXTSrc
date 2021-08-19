/*
 * Notification thread routines.
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * The notification thread runs only on the master. It notifies
 * clone servers about changes to the database or resynchronization
 * requests from the master.
 */
#include "ni_server.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <cthreads.h>
#include <stdio.h>
#include "system.h"
#include "ni_globals.h"
#include "mm.h"
#include "getstuff.h"
#include "notify.h"
#include "clib.h"
#include "socket_lock.h"

#define NI_SEPARATOR '/' /* Separator used in NetInfo property values */

#define NOTIFY_TIMEOUT 60 /* Time to wait before giving up on clone */


/*
 * The notification thread knows about the clone servers through
 * this data structure.
 */
typedef struct client_node *clone_list;
struct client_node {
	ni_name name;		/* host name of clone */
	unsigned long addr;	/* IP address of clone */
	ni_name tag;		/* Which database served by clone (by tag) */
	clone_list next;	/* next clone */
};

/*
 * This union encodes all possible arguments involved in NetInfo write
 * calls.
 */
typedef union write_any_args {
	ni_create_args create_args;
	ni_destroy_args destroy_args;
	ni_proplist_stuff write_args;
	ni_createprop_args createprop_args;
	ni_prop_args destroyprop_args;
	ni_propname_args renameprop_args;
	ni_writeprop_args writeprop_args;
	ni_createname_args createname_args;
	ni_nameindex_args destroy_name_args;
	ni_writename_args writename_args;
} write_any_args;

/*
 * This union encodes all possible results from NetInfo write calls.
 */
typedef union write_any_res {
	ni_create_res create_res;
	ni_id_res destroy_res;
	ni_id_res write_res;
	ni_id_res createprop_res;
	ni_id_res destroyprop_res;
	ni_id_res renameprop_res;
	ni_id_res writeprop_res;
	ni_id_res createname_res;
	ni_id_res destroyname_res;
	ni_id_res writename_res;
} write_any_res;


/*
 * We store the XDR routines associated with each NetInfo write call
 * in a table with this data structure for each entry.
 */
typedef struct xdr_table_entry {
	unsigned proc;		/* procedure number of write call */
	unsigned insize;	/* size of input arguments */
	xdrproc_t xdr_in;	/* xdr routine for input arguments */
	unsigned outsize;	/* size of output results */
	xdrproc_t xdr_out;	/* xdr routine for output results */
} xdr_table_entry;

/*
 * Macro used to initialize the table
 */
#define DOIT(proc, arg_type, res_type) \
	{ proc, sizeof(arg_type), xdr_##arg_type, \
	  sizeof(res_type), xdr_##res_type }


/*
 * The table itself
 */
static const xdr_table_entry xdr_table[] = {
	DOIT(_NI_CREATE, ni_create_args, ni_create_res),
	DOIT(_NI_DESTROY, ni_destroy_args, ni_id_res),
	DOIT(_NI_WRITE, ni_proplist_stuff, ni_id_res),
	DOIT(_NI_CREATEPROP, ni_createprop_args, ni_id_res),
	DOIT(_NI_DESTROYPROP, ni_prop_args, ni_id_res),
	DOIT(_NI_RENAMEPROP, ni_propname_args, ni_id_res),
	DOIT(_NI_WRITEPROP, ni_writeprop_args, ni_id_res),
	DOIT(_NI_CREATENAME, ni_createname_args, ni_id_res),
	DOIT(_NI_WRITEPROP, ni_writeprop_args, ni_id_res),
	DOIT(_NI_CREATENAME, ni_createname_args, ni_id_res),
	DOIT(_NI_DESTROYNAME, ni_nameindex_args, ni_id_res),
	DOIT(_NI_WRITENAME, ni_writename_args, ni_id_res),
	DOIT(_NI_RESYNC, void, ni_status),
};
#undef DOIT	
#define XDR_TABLE_SIZE (sizeof(xdr_table)/sizeof(xdr_table[0]))	


static const xdr_table_entry *xdr_table_lookup(unsigned);

/*
 * The list of notifications to be sent out
 */
typedef struct notify_node *notify_list;
struct notify_node {
	unsigned proc;		/* the procedure to execute on the clone */
	write_any_args args;	/* the arguments to supply */
	clone_list newlist;	/* new list of clones, if proc is RESYNC */
	notify_list next;	/* next item on list */
};


/*
 * Arguments to the notify() thread
 */
typedef struct notify_args {
       clone_list list;		/* list of clone servers */
       unsigned checksum;	/* checksum of master database */
} notify_args;


static void notify(notify_args *);


/*
 * The queue of notifications to be sent
 */
static volatile notify_list notifications;

/*
 * For locking the queue 
 */
static mutex_t notify_mutex;
static condition_t notify_condition;


static void doit(clone_list, unsigned, write_any_args *);

/*
 * Destroys a client handle with locks
 */
static void
clnt_destroy_lock(
		  CLIENT *cl,
		  int sock
		  )
{
	socket_lock();
	clnt_destroy(cl);
	(void)close(sock);
	socket_unlock();
}

/*
 * Frees up a clone list
 */
static void
freeclonelist(
	      clone_list clist
	      )
{
	clone_list l;

	while (clist != NULL) {
		l = clist;
		clist = clist->next;
		ni_name_free(&l->name);
		ni_name_free(&l->tag);
		MM_FREE(l);
	}
		
}

/*
 * Does this entry serve the "." domain?
 * Return the name and tag of the server if it does serve "."
 */
static unsigned
servesdot(
	  ni_entry entry,
	  ni_name *name,
	  ni_name *tag
	  )
{
	ni_name sep;
	ni_name slashname;	
	unsigned addr;
	ni_namelist nl;
	ni_index i;
	ni_id id;
	
	if (entry.names == NULL) {
		return (0);
	}
	id.nii_object = entry.id;
	for (i = 0; i < entry.names->ninl_len; i++) {
		slashname = entry.names->ninl_val[i];
		sep = index(slashname, NI_SEPARATOR);
		if (sep == NULL) {
			continue;
		}

		if (!ni_name_match_seg(NAME_DOT, slashname, sep - slashname)) {
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

		if (ni_lookupprop(db_ni, &id, NAME_NAME, &nl) != NI_OK) {
			*name = NULL;
		} else {
			if (nl.ninl_len > 0) {
				*name = ni_name_dup(nl.ninl_val[0]);
			}
			ni_namelist_free(&nl);
		}
		return (addr);
	}
	return (0);
}

/*
 * Reads the list of clone servers from the database
 */
static clone_list
getclonelist()
{
	ni_id id;
	ni_index i;
	unsigned addr;
	clone_list new;
	clone_list clist;
	ni_name name;
	ni_name tag;
	ni_idlist idlist;
	ni_entrylist entries;

	if (ni_root(db_ni, &id) != NI_OK) {
		return (NULL);
	}
	if (ni_lookup(db_ni, &id, NAME_NAME, NAME_MACHINES, &idlist) !=
	    NI_OK) {
		return (NULL);
	}
	id.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);
	if (ni_list_const(db_ni, &id, NAME_SERVES, &entries) != NI_OK) {
		return (NULL);
	}
	clist = NULL;
	for (i = 0; i < entries.niel_len; i++) {
		addr = servesdot(entries.niel_val[i], &name, &tag);
		if (addr == 0) {
			continue;
		}
		/*
		 * Don't notify self
		 */
		if (sys_ismyaddress(addr)) {
			ni_name_free(&name);
			ni_name_free(&tag);
			continue;
		}
		MM_ALLOC(new);
		new->name = name;
		new->addr = addr;
		new->tag = tag;
		new->next = clist;
		clist = new;
	}
	ni_list_const_free(db_ni);
	return (clist);
}


/*
 * Is there a notifier thread running?
 */
static int have_notifier;


/*
 * Starts up a notification thread. Is smart enough not to start one
 * if the clone list is empty.
 */
int
notify_start(
	     void
	     )
{
	notify_args *args;
	clone_list clist;

	clist = getclonelist();
	if (clist == NULL) {
		/*
		 * No need for notifier
		 */
		return (0);
	}

	MM_ALLOC(args);
	args->list = clist;
	args->checksum = db_checksum;
	notify_mutex = mutex_alloc();
	notify_condition = condition_alloc();
	cthread_detach(cthread_fork((cthread_fn_t)notify, (any_t)args));
	have_notifier++;
	return (1);
}


/*
 * Send out a resync notification. This procedure is special-cased
 * because the master updates its list of clone servers when it gets
 * a resync call.
 */
void
notify_resync(
	      void
	      )
{
	notify_args *args;
	clone_list clist;

	clist = getclonelist();
	if (!have_notifier) {
		if (clist != NULL) {
			MM_ALLOC(args);
			args->list = clist;
			args->checksum = db_checksum;
			notify_mutex = mutex_alloc();
			notify_condition = condition_alloc();
			have_notifier++;
			cthread_detach(cthread_fork((cthread_fn_t)notify, 
						    (any_t)args));
		}
	} else {
		notify_clients(_NI_RESYNC, clist);
	}
}

/*
 * The notification thread
 */
static void
notify(
       notify_args *args
       )
{
	clone_list l;
	int proc;
	write_any_args procargs;
	notify_list save;
	struct timeval tv;
	CLIENT *cl;
	int sock;
	struct sockaddr_in sin;
	nibind_getregister_res res;

	tv.tv_sec = NOTIFY_TIMEOUT;
	tv.tv_usec = 0;
	sin.sin_family = AF_INET;
	MM_ZERO(sin.sin_zero);

	/*
	 * First, tell everybody that you've crashed
	 */
	for (l = args->list; l != NULL; l = l->next) {
		sin.sin_port = 0;
		sin.sin_addr.s_addr = l->addr;
		sock = socket_connect(&sin, NIBIND_PROG, NIBIND_VERS);
		if (sock < 0) {
			continue;
		}
		cl = clnttcp_create(&sin, NIBIND_PROG, NIBIND_VERS, 
					 &sock, 0, 0);
		if (cl == NULL) {
			socket_close(sock);
			continue;
		}
		if (clnt_call(cl, NIBIND_GETREGISTER,
			      xdr_ni_name, &l->tag,
			      xdr_nibind_getregister_res, &res,
				      tv) != RPC_SUCCESS)  {
			clnt_destroy_lock(cl, sock);
			continue;
		}
		clnt_destroy_lock(cl, sock);
		if (res.status != NI_OK) {
			continue;
		}
		sin.sin_port = res.nibind_getregister_res_u.addrs.tcp_port;
		sock = socket_connect(&sin, NI_PROG, NI_VERS);
		if (sock < 0) {
			continue;
		}
		cl = clnttcp_create(&sin, NI_PROG, NI_VERS, &sock, 0, 0);
		if (cl == NULL) {
			socket_close(sock);
			continue;
		}
		(void)clnt_call(cl, _NI_CRASHED, xdr_u_int,
				&args->checksum,
				xdr_void, NULL, tv);
		clnt_destroy_lock(cl, sock);
	}

	/*
	 * Now, should wait for things to be added to the
	 * update queue and send them off to its clients.
	 */

	mutex_lock(notify_mutex);
	for (;;) {
		condition_wait(notify_condition, notify_mutex);
		while (notifications != NULL) {
			proc = notifications->proc;
			procargs = notifications->args;
			if (proc == _NI_RESYNC) {
				freeclonelist(args->list);
				args->list = notifications->newlist;
			}
			save = notifications;
			notifications = notifications->next;
			MM_FREE(save);

			mutex_unlock(notify_mutex);
			doit(args->list, proc, &procargs);
			mutex_lock(notify_mutex);
		}
	}
	mutex_unlock(notify_mutex);
}

/*
 * Notify the clone servers of a change to the database.
 * 	proc = procedure to execute on clone
 *	args = arguments to procedure
 * XXX: procedure is a misnomer - should be notify_clones
 */
void
notify_clients(
	       unsigned proc,
	       void *args
	       )
{
	notify_list *l;
	const xdr_table_entry *ent;

	if (!have_notifier) {
		if (!notify_start()) {
			return;
		}
	}

	ent = xdr_table_lookup(proc);
	if (ent == NULL) {
		return;
	}
	mutex_lock(notify_mutex);
	for (l = (notify_list *)&notifications; *l != NULL; l = &(*l)->next) {
	}
	MM_ALLOC(*l);
	(*l)->proc = proc;
	if (proc == _NI_RESYNC) {
		(*l)->newlist = (clone_list)args;
	} else {
		bcopy(args, &(*l)->args, ent->insize);
		bzero(args, ent->insize);
	}
	(*l)->next = NULL;
	condition_signal(notify_condition);
	mutex_unlock(notify_mutex);
}



/*
 * Tries to execute the procedure on each of the clone servers
 */
static void
doit(
     clone_list list,
     unsigned proc,
     write_any_args *args
     )
{
	write_any_res res;
	const xdr_table_entry *ent;
	int sock;
	struct sockaddr_in sin;
	struct timeval tv;
	CLIENT *cl;
	clone_list l;
	nibind_getregister_res gres;

	ent = xdr_table_lookup(proc);
	if (ent == NULL) {
		return; /* should never happen */
	}
	tv.tv_sec = NOTIFY_TIMEOUT;
	tv.tv_usec = 0;
	sin.sin_family = AF_INET;
	MM_ZERO(sin.sin_zero);
	for (l = list; l != NULL; l = l->next) {
		sin.sin_port = 0;
		sin.sin_addr.s_addr = l->addr;
		sock = socket_connect(&sin, NIBIND_PROG, NIBIND_VERS);
		if (sock < 0) {
			continue;
		}
		cl = clnttcp_create(&sin, NIBIND_PROG, NIBIND_VERS, 
					 &sock,  0, 0);
		if (cl == NULL) {
			socket_close(sock);
			continue;
		}
		if (clnt_call(cl, NIBIND_GETREGISTER,
			      xdr_ni_name, &l->tag,
			      xdr_nibind_getregister_res, &gres,
				      tv) != RPC_SUCCESS)  {
			clnt_destroy_lock(cl, sock);
			continue;
		}
		clnt_destroy_lock(cl, sock);
		if (gres.status != NI_OK) {
			continue;
		}
		sin.sin_port = gres.nibind_getregister_res_u.addrs.tcp_port;
		sock = socket_connect(&sin, NI_PROG, NI_VERS);
		if (sock < 0) {
			continue;
		}
		cl = clnttcp_create(&sin, NI_PROG, NI_VERS, &sock, 0, 0);
		if (cl == NULL) {
			socket_close(sock);
			continue;
		}
		bzero(&res, ent->outsize);
		(void)clnt_call(cl, proc, ent->xdr_in, args,
				ent->xdr_out, &res, tv);
		clnt_destroy_lock(cl, sock);
	}	
	xdr_free(ent->xdr_in, args);
}

/*
 * Looks up the XDR information for the given procedure
 */
static const xdr_table_entry *
xdr_table_lookup(
		 unsigned proc
		 )
{
	int i;

	for (i = 0; i < XDR_TABLE_SIZE; i++) {
		if (xdr_table[i].proc == proc) {
			return (&xdr_table[i]);
		}
	}
	return (NULL);
}

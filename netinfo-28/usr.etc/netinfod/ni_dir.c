/*
 * NetInfo directory (XXX.nidb) handling and database transfer
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * There are three kinds of NetInfo directories. 
 * .nidb extension: A directory in use.
 * .temp extension: Temporary directory being loaded from master server.
 * .move extension: Old directory moved away to make room for new one.
 *
 * The transactional implications on startup are the following:
 * 	1. A ".temp" directory is assumed to be in a partial state of 
 *	   creation and cannot be used UNLESS there is also a ".move"
 *	   directory.
 *
 *	2. A ".move" directory is assumed to be in a partial state of deletion
 *	   and cannot be used. If there is a ".temp" directory, it should
 *	   be renamed ".nidb" and used.
 */
#include "ni_server.h"
#include <sys/param.h>
#include <sys/dir.h>
#include <sys/socket.h>
#include <sys/fcntl.h>
#include <sys/time.h>
#include <cthreads.h>
#include <stdio.h>
#include "ni_globals.h"
#include "ni_file.h"
#include "mm.h"
#include "checksum.h"
#include "getstuff.h"
#include "clib.h"
#include "system.h"
#include "event.h"
#include "socket_lock.h"

#undef NFS_RM_BUG /* hasn't been fixed, just assume no NFS databases */
#define READALL_TIMEOUT 60

static unsigned given_checksum;	/* checksum given by master */
static void *new_ni;		/* NetInfo handle for database just received */

/*
 * Destroy a database directory
 */
static int
dir_destroy(
	     char *dir
	     )
{
	char path1[MAXPATHLEN + 1];
#ifdef NFS_RM_BUG
	char path2[MAXPATHLEN + 1];
#endif
	DIR *dp;
	struct direct *d;

	socket_lock();
	dp = opendir(dir);
	socket_unlock();
	if (dp == NULL) {
		return (0);
	}
	while (d = readdir(dp)) {
		sprintf(path1, "%s/%.*s", dir, d->d_namlen, d->d_name);
#ifdef NFS_RM_BUG
		sprintf(path2, "./%.*s.tmp", d->d_namlen, d->d_name);
		/*
		 * rename, then unlink in case NFS leaves tmp files behind
		 * (.nfs* files, that is).
		 */
		if (rename(path1, path2) != 0 ||
		    unlink(path2) != 0) {
			/* ignore error: rmdir will catch ENOTEMPTY */
		}
#else
		(void)unlink(path1);
#endif
	}
	socket_lock();
	closedir(dp);
	socket_unlock();
	return (rmdir(dir));
}

static const char NI_SUFFIX_CUR[] = ".nidb";
static const char NI_SUFFIX_MOVED[] = ".move";
static const char NI_SUFFIX_TMP[] = ".temp";

/*
 * Returns the three kinds of directory names used by NetInfo
 */
void
dir_getnames(
	     ni_name orig,
	     ni_name *target,
	     ni_name *moved,
	     ni_name *tmp
	     )
{
	if (target != NULL) {
		*target = malloc(strlen(orig) + strlen(NI_SUFFIX_CUR) + 1);
		sprintf(*target, "%s%s", orig, NI_SUFFIX_CUR);
	}
	if (moved != NULL) {
		*moved = malloc(strlen(orig) + strlen(NI_SUFFIX_MOVED) + 1);
		sprintf(*moved, "%s%s", orig, NI_SUFFIX_MOVED);
	}
	if (tmp != NULL) {
		*tmp = malloc(strlen(orig) + strlen(NI_SUFFIX_TMP) + 1);
		sprintf(*tmp, "%s%s", orig, NI_SUFFIX_TMP);
	}
}

/*
 * Switcharoo of directories
 *
 * new database is built in ".temp".
 * old database is moved to ".move".
 * ".temp" is renamed to ".nidb".
 * ".move" is destroyed.
 */
static int
dir_switch(
	    void *ni
	    )
{
	int res;
	char *target;
	char *moved;
	char *tmp;
	ni_name tag;

	tag = ni_tagname(ni);
	ni_free(ni); /* to close all files */
	dir_getnames(tag, &target, &moved, &tmp);
	ni_name_free(&tag);
	res = 1;
	if (access(target, F_OK) == 0) {
		res = rename(target, moved) == 0;
	}
	if (res) {
		res = (rename(tmp, target) == 0 &&
		       dir_destroy(moved) == 0);
	}
	ni_name_free(&target);
	ni_name_free(&moved);
	ni_name_free(&tmp);
	return (res);
}

/*
 * Check to see if anything needs cleaned up (like maybe we crashed
 * while a switch was going on).
 *
 * If ".temp" exists but ".move" does not then destroy ".temp".
 *
 * If ".temp" and ".move" exist, then rename ".temp" to ".nidb" 
 * and destroy ".move". 
 *
 * If ".move" exists and ".temp" does not, then destroy ".move".
 *
 */
void
dir_cleanup(
	     char *domain
	     )
{
	ni_name target;
	ni_name tmp;
	ni_name moved;

	dir_getnames(domain, &target, &moved, &tmp);
	if (access(tmp, F_OK) == 0) {
		if (access(moved, F_OK) == 0) {
			if (rename(tmp, target) == 0) {
				dir_destroy(moved);
			}
		} else {
			dir_destroy(tmp);
		}
	} else if (access(moved, F_OK) == 0) {
		dir_destroy(moved);
	}
	ni_name_free(&target);
	ni_name_free(&tmp);
	ni_name_free(&moved);
}

/*
 * Log a failure for the given ID
 */
static void
log_failure(ni_index id, char *message)
{
	sys_errmsg("transfer failed at %s: id=%d", message, id);
}

/*
 * XDR routine to write out a new database.
 */
static bool_t
xdr_writeall(
	     XDR *xdr,
	     char *dir
	     )
{
	ni_status status;
	int more;
	void *fh;
	ni_object object;
	ni_index highest_id;
	
	if (mkdir(dir, 0755) < 0) {
		log_failure(-1, "mkdir");
		return (FALSE);
	}
	status = file_init(dir, &fh);
	if (status != NI_OK) {
		log_failure(-1, "file_init");
		return (FALSE);
	}
	if (!xdr_ni_status(xdr, &status)) {
		log_failure(-1, "status");
		file_free(fh);
		return (FALSE);
	}
	if (status != NI_OK) {
		log_failure(-1, "bad status");
		file_free(fh);
		return (FALSE);
	}
	if (!xdr_u_int(xdr, &given_checksum)) {
		log_failure(-1, "no checksum");
		file_free(fh);
		return (FALSE);
	}
	if (given_checksum == 0) {
		/*
		 * Database is already up to date
		 */
		file_free(fh);
		return (FALSE);
	}
	if (!xdr_u_int(xdr, &highest_id)) {
		log_failure(-1, "no highest_id");
		file_free(fh);
		return (FALSE);
	}
	have_transferred++;
	for (;;) {
		if (!xdr_bool(xdr, &more)) {
			log_failure(object.nio_id.nii_object, "no more");
			file_free(fh);
			return (FALSE);
		}
		if (!more) {
			file_free(fh);
			return (TRUE);
		}
		MM_ZERO(&object);
		if (!xdr_ni_object(xdr, &object)) {
			log_failure(object.nio_id.nii_object, "no object");
			file_free(fh);
			return (FALSE);
		}
		if (file_forcewrite(fh, &object, highest_id) != NI_OK) {
			log_failure(object.nio_id.nii_object, "forcewrite");
			file_free(fh);
			return (FALSE);
		}
		(void)xdr_free(xdr_ni_object, &object);
	}
}

/*
 * Reads a new database from the master and writes it out. Can be
 * short-circuited if it is detected that the master's copy is no
 * different than our current version (via checksums).
 */
static int
dir_transfer(
	     struct sockaddr_in *sin,
	     char *tag,
	     char *dir,
	     unsigned checksum
	      )
{
	int status;
	int sock;
	CLIENT *cl;
	struct timeval tv;
	nibind_getregister_res res;

	sock = socket_connect(sin, NIBIND_PROG, NIBIND_VERS);
	if (sock < 0) {
		return (0);
	}
	cl = clnttcp_create(sin, NIBIND_PROG, NIBIND_VERS, &sock, 0, 0);
	if (cl == NULL) {
		socket_close(sock);
		return (0);
	}
	tv.tv_sec = READALL_TIMEOUT;
	tv.tv_usec = 0;
	if (clnt_call(cl, NIBIND_GETREGISTER, xdr_ni_name, &tag,
		      xdr_nibind_getregister_res, &res, tv) != RPC_SUCCESS) {
		clnt_destroy(cl);
		socket_close(sock);
		return (0);
	}
	clnt_destroy(cl);
	socket_close(sock);
	if (res.status != NI_OK) {
		return (0);
	}
	sin->sin_port = res.nibind_getregister_res_u.addrs.tcp_port;
	sock = socket_connect(sin, NI_PROG, NI_VERS);
	if (sock < 0) {
		return (0);
	}
	cl = clnttcp_create(sin, NI_PROG, NI_VERS, &sock, 0, 0);
	if (cl == NULL) {
		socket_close(sock);
		return (0);
	}
	status = clnt_call(cl, _NI_READALL, xdr_u_int, &checksum,
			   xdr_writeall, dir, tv);
	clnt_destroy(cl);
	socket_close(sock);
	return (status == RPC_SUCCESS);
}

/*
 * Information needed by transfer thread
 */
typedef struct transfer_info {
	struct sockaddr_in sin;
	unsigned checksum;
	ni_name tag;
} transfer_info;

/*
 * For locking
 */
static mutex_t transfer_mutex;
static volatile int transfer_inprogress;

/*
 * The transfer thread: Let the server continue serving while the transfer
 * is going on.
 */
static void
transfer(
	 transfer_info *info
	 )
{
	ni_name tmp;
	ni_name tag;
	ni_status status;

	tag = ni_tagname(db_ni);
	dir_getnames(tag, NULL, NULL, &tmp);
	if (dir_transfer(&info->sin, info->tag, tmp, info->checksum)) {
		/*
		 * Make sure everything is sunc to disk. 
 		 * XXX: sync() just starts the process - no way to
		 * know when everything is actually sunc. We hope
		 * the database initialization takes longer than
		 * a filsystem sync.
		 */
		sync();

		/*
		 * Init the new database to see if it transferred
		 * correctly.
		 */
		status = ni_init(tmp, &new_ni);
		if (status != NI_OK) {
			new_ni = NULL;
			sys_errmsg("cannot init new database");
		} else {
			/*
			 * Successfully initialized
			 */
			event_post();
		}
	} else {
		new_ni = NULL;
		dir_destroy(tmp);
	}
	ni_name_free(&tag);
	ni_name_free(&tmp);
	MM_FREE(info);

	if (new_ni == NULL) {
		/*
		 * If we failed the transfer, unlock now. Otherwise
		 * do not unlock - wait for the main event loop to do
		 * it.
		 */
		mutex_lock(transfer_mutex);
		transfer_inprogress = 0;
		mutex_unlock(transfer_mutex);
	}

	cthread_exit(0);
}

/*
 * Callback routine to switch to the new database. The svc_run() event
 * checker will call us back.
 */
static void
cb_switch(
	  void
	  )
{
	ni_name dbname;
	ni_name tag;

	/*
	 * OK, it seems to work. Let's commit to it now.
	 */
	tag = ni_tagname(db_ni);
	dir_getnames(tag, &dbname, NULL, NULL);
	ni_name_free(&tag);
	if (dir_switch(db_ni)) {
		db_ni = new_ni;
		ni_renamedir(db_ni, dbname);
		ni_name_free(&dbname);
		db_checksum = given_checksum;
	} else {
		ni_free(new_ni);
		sys_panic("couldn't switch directories");
	}
	mutex_lock(transfer_mutex);
	transfer_inprogress = 0;
	mutex_unlock(transfer_mutex);
}


/*
 * For clone, check to see if we are out of date wrt the master.
 */
void
dir_clonecheck(
	       void
	       )
{
	transfer_info *info;
	ni_name tag;

	if (have_transferred) {
		/*
		 * Do not transfer if already have done so in last
		 * cleanup period.
		 */
		return;
	}
	if (transfer_mutex == NULL) {
		transfer_mutex = mutex_alloc();
	}
	/*
	 * Do not check if already transferring
	 */
	mutex_lock(transfer_mutex);
	if (transfer_inprogress) {
		mutex_unlock(transfer_mutex);
		return;
	}
	mutex_unlock(transfer_mutex);

	master_addr = getmasteraddr(db_ni, &tag);
	if (master_addr == 0) {
		sys_errmsg("cannot locate master - transfer failed");
		return;
	}

	event_init(cb_switch);
	MM_ALLOC(info);
	info->checksum = db_checksum;
	info->sin.sin_family = AF_INET;
	info->sin.sin_port = 0;
	MM_ZERO(info->sin.sin_zero);
	info->sin.sin_addr.s_addr = master_addr;
	info->tag = tag;

	transfer_inprogress = 1; /* no thread yet, so no need to lock */
	cthread_detach(cthread_fork((cthread_fn_t)transfer, (any_t)info));
}

/*
 * Useful routine for insert the given (key, val) pair as a new property
 * the the given property list.
 */
static void
pl_insert(
	  ni_proplist *props,
	  ni_name_const key,
	  ni_name_const val
	  )
{
	ni_property prop;

	MM_ZERO(&prop);
	prop.nip_name = ni_name_dup(key);
	ni_namelist_insert(&prop.nip_val, val, NI_INDEX_NULL);
	ni_proplist_insert(props, prop, NI_INDEX_NULL);
	ni_prop_free(&prop);
}

/*
 * Create a master server
 */
ni_status
dir_mastercreate(
		 char *domain
		 )
{
	ni_status status;
	ni_name dbname;
	void *ni;
	ni_property prop;
	ni_name masterloc;
	ni_name serves;
	ni_proplist props;
	ni_id mach_id;
	ni_id mast_id;
	ni_name myname;
	ni_id id;
	struct in_addr addr;

	dir_getnames(domain, &dbname, NULL, NULL);
	if (mkdir(dbname, 0755) < 0) {
		ni_name_free(&dbname);
		return (NI_SYSTEMERR);
	}
	status = ni_init(dbname, &ni);
	if (status != NI_OK) {
		dir_destroy(dbname);
		ni_name_free(&dbname);
		return (status);
	}
	ni_setuser(ni, ACCESS_USER_SUPER);
	ni_name_free(&dbname);

	/*
	 * Initialize the master property to self
	 */
	myname = (ni_name)sys_hostname();
	masterloc = malloc(strlen(myname) + strlen(domain) + 2);
	sprintf(masterloc, "%s/%s", myname, domain);

	status = ni_root(ni, &id);
	if (status != NI_OK) {
		goto cleanup;
	}
	MM_ZERO(&prop);
	prop.nip_name = ni_name_dup(NAME_MASTER);
	ni_namelist_insert(&prop.nip_val, masterloc, NI_INDEX_NULL);
	status = ni_createprop(ni, &id, prop, NI_INDEX_NULL);
	ni_prop_free(&prop);
	if (status != NI_OK) {
		goto cleanup;
	}

	/*
	 * Create a "machines" directory
	 */
	MM_ZERO(&props);
	pl_insert(&props, NAME_NAME, NAME_MACHINES);
	mach_id.nii_object = NI_INDEX_NULL;
	status = ni_create(ni, &id, props, &mach_id, NI_INDEX_NULL);
	ni_proplist_free(&props);
	if (status != NI_OK) {
		goto cleanup;
	}

	/*
	 * Insert self into "machines" directory
	 */
	MM_ZERO(&props);
	pl_insert(&props, NAME_NAME, myname);
	addr.s_addr = sys_address();
	pl_insert(&props, NAME_IP_ADDRESS, inet_ntoa(addr));
	serves = malloc(strlen(NAME_DOT) + strlen(domain) + 2);
	sprintf(serves, "%s/%s", NAME_DOT, domain);
	pl_insert(&props, NAME_SERVES, serves);
	ni_name_free(&serves);
	mast_id.nii_object = NI_INDEX_NULL;
	status = ni_create(ni, &mach_id, props, &mast_id, NI_INDEX_NULL);
	ni_proplist_free(&props);
	if (status != NI_OK) {
		goto cleanup;
	}

 cleanup:
	ni_free(ni);
	if (status != NI_OK) {
		dir_destroy(dbname);
	}
	ni_name_free(&dbname);
	return (status);
}

/*
 * Create a clone server
 */
ni_status
dir_clonecreate(
		char *domain,
		char *master_name,
		char *master_addr, 
		char *master_domain
		)
{
	ni_status status;
	ni_name dbname;
	void *ni;
	ni_property prop;
	ni_name masterloc;
	ni_name serves;
	ni_proplist props;
	ni_id mach_id;
	ni_id mast_id;
	ni_id id;

	dir_getnames(domain, &dbname, NULL, NULL);
	if (mkdir(dbname, 0755) < 0) {
		ni_name_free(&dbname);
		return (NI_SYSTEMERR);
	}
	status = ni_init(dbname, &ni);
	if (status != NI_OK) {
		dir_destroy(dbname);
		ni_name_free(&dbname);
		return (status);
	}
	ni_setuser(ni, ACCESS_USER_SUPER);
	ni_name_free(&dbname);

	/*
	 * Initialize master property to real master
	 */
	masterloc = malloc(strlen(master_name) + strlen(master_domain) + 2);
	sprintf(masterloc, "%s/%s", master_name, master_domain);


	status = ni_root(ni, &id);
	if (status != NI_OK) {
		goto cleanup;
	}

	MM_ZERO(&prop);
	prop.nip_name = ni_name_dup(NAME_MASTER);
	ni_namelist_insert(&prop.nip_val, masterloc, NI_INDEX_NULL);
	status = ni_createprop(ni, &id, prop, NI_INDEX_NULL);
	ni_prop_free(&prop);
	if (status != NI_OK) {
		goto cleanup;
	}

	/*
	 * Create "machines" directory
	 */
	MM_ZERO(&props);
	pl_insert(&props, NAME_NAME, NAME_MACHINES);
	mach_id.nii_object = NI_INDEX_NULL;
	status = ni_create(ni, &id, props, &mach_id, NI_INDEX_NULL);
	ni_proplist_free(&props);
	if (status != NI_OK) {
		goto cleanup;
	}

	/*
	 * Put master in "machines" directory
	 */
	MM_ZERO(&props);
	pl_insert(&props, NAME_NAME, master_name);
	pl_insert(&props, NAME_IP_ADDRESS, master_addr);
	serves = malloc(strlen(NAME_DOT) + strlen(master_domain) + 2);
	sprintf(serves, "%s/%s", NAME_DOT, master_domain);
	pl_insert(&props, NAME_SERVES, serves);
	ni_name_free(&serves);
	mast_id.nii_object = NI_INDEX_NULL;
	status = ni_create(ni, &mach_id, props, &mast_id, NI_INDEX_NULL);
	ni_proplist_free(&props);
	if (status != NI_OK) {
		goto cleanup;
	}
 cleanup:
	ni_free(ni);
	if (status != NI_OK) {
		dir_destroy(dbname);
	}
	ni_name_free(&dbname);
	return (status);
}


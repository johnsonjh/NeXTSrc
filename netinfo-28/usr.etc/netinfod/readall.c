/*
 * Read an entire NetInfo database.
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * This code is executing when a slave server needs to resynchronize
 * with the master server and issues a READALL call. The master reads
 * the entire database with the call readall().
 *
 * XXX: This hangs up master service while the database is read. This
 * should be rewritten to be multi-threaded with the appropriate locking
 * so that writes can still occur while readall() executes (a tough
 * problem, otherwise we would have done it already ;-)
 */
#include <stdio.h>
#include "ni_server.h"
#include "ni_globals.h"
#include "clib.h"
#include "socket_lock.h"

#include <syslog.h>
#define debug(s, d) syslog(LOG_ERR, s, d)

/*
 * Recursively reads node and its children, writing it into xdr. 
 * The guts of readall().
 */
static bool_t
doit(
     XDR *xdr,
     void *ni,
     ni_index object_id
     )
{
	ni_index i;
	const bool_t true = TRUE;
	ni_object object;

	socket_unlock();
	object.nio_id.nii_object = object_id;
	if (ni_parent(ni, &object.nio_id, &object.nio_parent) != NI_OK) {
		socket_lock();
		debug("couldn't get parent of %d\n", object.nio_id.nii_object);
		return (FALSE);
	}
	NI_INIT(&object.nio_props);
	if (ni_read(ni, &object.nio_id, &object.nio_props) != NI_OK) {
		socket_lock();
		debug("couldn't read %d\n", object.nio_id.nii_object);
		return (FALSE);
	}
	NI_INIT(&object.nio_children);
	if (ni_children(ni, &object.nio_id, &object.nio_children) != NI_OK) {
		socket_lock();
#if !ENABLE_CACHE
		ni_proplist_free(&object.nio_props);
#endif
		debug("couldn't get children of %d\n", object.nio_id.nii_object);
		return (FALSE);
	}
	socket_lock();

	
	if (!xdr_bool(xdr, &true) ||
	    !xdr_ni_object(xdr, &object)) {
#if !ENABLE_CACHE
		ni_proplist_free(&object.nio_props);
#endif
		ni_idlist_free(&object.nio_children);
		debug("couldn't xdr %d\n", object.nio_id.nii_object);
		return (FALSE);
	}
#if !ENABLE_CACHE
	ni_proplist_free(&object.nio_props);
#endif

	for (i = 0; i < object.nio_children.niil_len; i++) {
		if (!doit(xdr, ni, object.nio_children.niil_val[i])) {
			ni_idlist_free(&object.nio_children);
			return (FALSE);
		}
	}
	ni_idlist_free(&object.nio_children);
	return (TRUE);
}

/*
 * The readall() function. Sends out the netinfo status code, and if
 * NI_OK, than sends the checksum, the highest id in the list and calls
 * doit() to recursively descend the database.
 *
 */
bool_t
readall(
	XDR *xdr,
	void *ni
	)
{
	ni_id root;
	const bool_t false = FALSE;
	ni_status status;
	unsigned checksum;
	unsigned highestid;

	socket_unlock();
	status = ni_root(ni, &root);
	socket_lock();

	if (!xdr_ni_status(xdr, &status)) {
		return (FALSE);
	}
	if (status == NI_OK) {
		checksum = db_checksum;
		
		/*
		 * Send out the checksum...
		 */
		if (!xdr_u_int(xdr, &checksum)) {
			return (FALSE);
		}

		socket_unlock();
		highestid = ni_highestid(ni);
		socket_lock();

		/*
		 * The highest ID...
		 */
		if (!xdr_u_int(xdr, &highestid)) {
			return (FALSE);
		}

		/*
		 * All the data...
		 */
		if (!doit(xdr, ni, root.nii_object)) {
			return (FALSE);
		}
		
		/*
		 * And then terminate the list (more entries == false).
		 */
		if (!xdr_bool(xdr, &false)) {
			return (FALSE);
		}
	}
	return (TRUE);
}




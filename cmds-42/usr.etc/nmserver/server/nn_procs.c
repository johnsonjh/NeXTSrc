/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nn_procs.c
 *
 * $Source: /private/Net/p1/BSD/btaylor/LIBC/libc-2/etc/nmserver/server/RCS/nn_procs.c,v $
 *
 */

char nn_procs_rcsid[] = "$Header: nn_procs.c,v 1.1 88/09/30 15:41:05 gk Locked $";

/*
 * Main routines for the network name service module.
 */

/*
 * HISTORY:
 *  4-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed queue_item_t to cthread_queue_item_t
 * 
 * 25-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Fixed netname_version for new rcsid convention.
 *
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Put the automatic exit when looking up "exit" under a PROF conditional.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  8-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added code to avoid checking-in PORT_NULL.
 *
 * 27-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added code to allow the use of an old netmsgserver to handle
 *	messages that cannot be handled by the new one (COMPAT).
 *
 *  7-Nov-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for dispatcher with version number.
 *
 *  6-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Allow a host name in netname_look_up to be treated as an IP address.
 * 
 *  2-Oct-87  Daniel Julin (dpj) at Carnegie Mellon University
 *	Modified to work with no network.
 *
 * 20-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replace lq_find_in_queue/lq_remove_from_queue by 
 *	lq_cond_delete_from_queue in nn_remove_entries.
 *
 * 15-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added some debugging output.
 *
 *  5-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Merged MEM_NNENTRY into MEM_NNREC.
 *
 * 28-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Look up name locally if a network name request is to be broadcast.
 *	Changed nn_table to consist of inline lock_queue records.
 *
 *  6-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Replaced DISP_NN_REPLY & DISP_NN_REQUEST by DISP_NETNAME.
 *
 * 26-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <netdb.h>
#include <stdio.h>
#include <strings.h>
#include <sys/message.h>

#include "debug.h"
#include "dispatcher.h"
#include "lock_queue.h"
#include "mem.h"
#include "netmsg.h"
#include "network.h"
#include "nm_extra.h"
#include "nn_defs.h"
#include "port_defs.h"
#include "transport.h"

struct lock_queue	nn_table[NN_TABLE_SIZE];


/*
 * nn_procs_init
 *	Initialise the local name hash table.
 *	Initialise the dispatcher with our request/response handling functions.
 *
 */
PUBLIC void nn_procs_init()
BEGIN("nn_procs_init")
    int	i;

    for (i = 0; i < NN_TABLE_SIZE; i++) {
	lq_init(&nn_table[i]);
    }

    dispatcher_switch[DISPE_NETNAME].disp_rr_simple = nn_handle_request;
    dispatcher_switch[DISPE_NETNAME].disp_indata_simple = nn_handle_reply;

    RET;
END


/*
 * nn_name_test
 *	test to see if the name in an entry is equal to the input name
 *
 *  Parameters
 *	q_item	: the entry on the queue
 *	name	: the input name
 *
 */
PUBLIC nn_name_test(q_item, name)
register lock_queue_t	q_item;
register int		name;
BEGIN("nn_name_test")

    RETURN((strcmp(((nn_entry_ptr_t)q_item)->nne_name, (char *)name)) == 0);

END


/*
 * _netname_check_in
 *	Performs a local name check in.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	port_name	: the name to be checked in
 * 	signature	: a port protecting this entry
 *	port_id		: the port associated with the name
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_YOURS	: this name is already checked in but with a different signature
 *
 * Design:
 *	See if this name has already been entered in the name table.
 *	If is has not, then just enter it.
 *	If it has, then replace the old entry if the signatures match.
 *
 */
PUBLIC _netname_check_in(ServPort, port_name, signature, port_id)
port_t		ServPort;
netname_name_t	port_name;
port_t		signature;
port_t		port_id;
BEGIN("_netname_check_in")
    int			hash_index;
    nn_entry_ptr_t	name_entry_ptr;
#if	COMPAT
    kern_return_t	kr;
#endif	COMPAT

#ifdef lint
    ServPort;
#endif lint

	/*
	 * If a port is deallocated before the name service request
	 * is received, it will appear as PORT_NULL.
	 */
	if (port_id == PORT_NULL) {
		RETURN(NETNAME_INVALID_PORT);
	}

#if	COMPAT
	if ((param.compat) && (ServPort != PORT_NULL)) {
		DEBUG_STRING(debug.netname,0,3016,port_name);
		kr = netname_check_in(name_server_port,port_name,signature,port_id);
		DEBUG1(debug.netname,0,3017,kr);
		if (kr == SEND_INVALID_PORT)
			panic("Compatibility server died");
	}
#endif	COMPAT

    /*
     * Convert the name to upper case and look it up in the name table.
     */
    NN_CONVERT_TO_UPPER(port_name);
    DEBUG_STRING(debug.netname,0,3024,port_name);
    NN_NAME_HASH(hash_index, port_name);
    name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index], nn_name_test, (int)port_name);

    if (name_entry_ptr == (nn_entry_ptr_t)0) {
	/*
	 * Make a new name entry and add it into the name table.
	 */
	MEM_ALLOCOBJ(name_entry_ptr,nn_entry_ptr_t,MEM_NNREC);
	(void)strcpy(name_entry_ptr->nne_name, port_name);
	name_entry_ptr->nne_port = port_id;
	name_entry_ptr->nne_signature = signature;
	lq_enqueue(&nn_table[hash_index], (cthread_queue_item_t)name_entry_ptr);
	DEBUG2(debug.netname & 0x1000,0,3025,port_id,name_entry_ptr);
	RETURN(NETNAME_SUCCESS);
    }
    else {
	if (signature == name_entry_ptr->nne_signature) {
	    name_entry_ptr->nne_port = port_id;
	    DEBUG2(debug.netname & 0x1000,0,3026,port_id,name_entry_ptr);
	    RETURN(NETNAME_SUCCESS);
	}
	else {
	    DEBUG2(debug.netname & 0x1000,0,3027,port_id,name_entry_ptr);
	    RETURN(NETNAME_NOT_YOURS);
	}
    }

END


/*
 * _netname_check_out
 *	Checks out a name that was previously checked in.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	port_name	: the name to be checked out
 * 	signature	: a port protecting this entry
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_CHECKED_IN	: this name is not checked in
 *	NETNAME_NOT_YOURS	: this name is checked in but with a different signature
 *
 * Design:
 *	Check to see if this name has been entered into the local name table.
 *	If it has and the input signature makes the signature in the entry then remove the entry.
 *
 */
PUBLIC _netname_check_out(ServPort, port_name, signature)
port_t		ServPort;
netname_name_t	port_name;
port_t		signature;
BEGIN("_netname_check_out")
    int			hash_index;
    nn_entry_ptr_t	name_entry_ptr;
#if	COMPAT
    kern_return_t	kr;
#endif	COMPAT

#ifdef lint
    ServPort;
#endif lint

#if	COMPAT
	if ((param.compat) && (ServPort != PORT_NULL)) {
		DEBUG_STRING(debug.netname,0,3018,port_name);
		kr = netname_check_out(name_server_port,port_name,signature);
		DEBUG1(debug.netname,0,3019,kr);
		if (kr == SEND_INVALID_PORT)
			panic("Compatibility server died");
	}
#endif	COMPAT

    /*
     * Convert the name to upper case and look it up in the name table.
     */
    NN_CONVERT_TO_UPPER(port_name);
    NN_NAME_HASH(hash_index, port_name);
    name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index], nn_name_test, (int)port_name);

    if (name_entry_ptr == (nn_entry_ptr_t)0) {
	RETURN(NETNAME_NOT_CHECKED_IN);
    }
    else {
	if (signature != name_entry_ptr->nne_signature) {
	    RETURN(NETNAME_NOT_YOURS);
	}
	else {
	    /*
	     * Remove the entry from the queue of hashed entries and deallocate it.
	     */
	    (void)lq_remove_from_queue(&nn_table[hash_index], (cthread_queue_item_t)name_entry_ptr);
	    MEM_DEALLOCOBJ(name_entry_ptr, MEM_NNREC);
	    RETURN(NETNAME_SUCCESS);
	}
    }
END


/*
 * nn_host_address
 *	returns a host address for a given host name.
 *
 */
PRIVATE netaddr_t nn_host_address(host_name)
char *host_name;
BEGIN("nn_host_address")
    register struct hostent *hp;

#if	NeXT
    extern char my_host_name[];

    /*
     * Shortcut to bypass lookup services
     */
    if (strcmp(host_name, my_host_name) == 0) {
	    return (my_host_id);
    }
#endif	NeXT
    if ((hp = gethostbyname(host_name)) == 0) {
	RETURN(0);
    }
    else {
	RETURN(*(long *)(hp->h_addr_list[0]));
    }
END


/*
 * _netname_look_up
 *	Performs a name look up - could be local or over the network.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	host_name	: the host where the name is to be looked up.
 *	port_name	: the name to be looked up.
 *	port_ptr	: returns the port associated with the name.
 *
 * Results:
 *	NETNAME_SUCCESS
 *	NETNAME_NOT_CHECKED_IN	: the name was not found
 *	NETNAME_NO_SUCH_HOST	: the host_name was invalid
 *	NETNAME_HOST_NOT_FOUND	: the named host did not respond
 *
 * Design:
 *	See if the host name can be treated as an IP address.
 *	Look at the host name to see if we should do a local, directed or broadcast look up.
 *	If local, just see if the name is entered in out local name table.
 *	If directed or broadcast, call nn_network_look_up.
 *
 * Note:
 *	We cannot hear our own broadcasts.
 *
 */
PUBLIC _netname_look_up(ServPort, host_name, port_name, port_ptr)
port_t		ServPort;
netname_name_t	host_name;
netname_name_t	port_name;
port_t		*port_ptr;
BEGIN("_netname_look_up")
    int			hash_index, rc;
    nn_entry_ptr_t	name_entry_ptr;
    netaddr_t		host_id;

#ifdef lint
    ServPort;
#endif lint

#if	PROF
    if ((strcmp(port_name, "exit")) == 0) _exit(2);
#endif	PROF

    *port_ptr = 0;
    /*
     * Convert the name to upper case.
     */
    NN_CONVERT_TO_UPPER(port_name);

    /*
     * Check to see if this is a local look up.
     */
    DEBUG_STRING(debug.netname, 3, 1120, host_name);
    DEBUG_STRING(debug.netname, 3, 1125, port_name);

    if ((sscanf(host_name, "0x%x", &host_id)) != 1) {
	if (host_name[0] == '\0') host_id = my_host_id;
	else if ((host_name[1] == '\0') && (host_name[0] == '*')) host_id = broadcast_address;
	else host_id = nn_host_address(host_name);
    }

    if ((host_id == my_host_id) || (host_id == broadcast_address)) {
	/*
	 * See if the name is in our local name table.
	 */
	NN_NAME_HASH(hash_index, port_name);
	name_entry_ptr = (nn_entry_ptr_t)lq_find_in_queue(&nn_table[hash_index],
						nn_name_test,(int)port_name);
	if (name_entry_ptr == (nn_entry_ptr_t)0) {
	    if ((host_id == broadcast_address) && (param.conf_network)) {
		/*
		 * Try broadcasting.
		 */
		DEBUG0(debug.netname,0,3003);
		rc = nn_network_look_up(host_id, port_name, port_ptr);
#if	COMPAT
		if ((param.compat) && (ServPort != PORT_NULL) 
					&& (rc == NETNAME_HOST_NOT_FOUND)) {
			DEBUG_STRING(debug.netname,0,3020,port_name);
			rc = netname_look_up(name_server_port,host_name,port_name,port_ptr);
			DEBUG1(debug.netname,0,3021,rc);
			if (rc == SEND_INVALID_PORT)
				panic("Compatibility server died");
		}
#endif	COMPAT
		RETURN(rc);
	    }
	    else {
		DEBUG0(debug.netname,0,3028);
		RETURN(NETNAME_NOT_CHECKED_IN);
	    }
	}
	else {
	    DEBUG2(debug.netname,0,3004,name_entry_ptr->nne_port,name_entry_ptr);
	    *port_ptr = name_entry_ptr->nne_port;
	    RETURN(NETNAME_SUCCESS);
	}
    }
    else if (host_id == 0) {
	DEBUG0(debug.netname,0,3029);
	RETURN(NETNAME_NO_SUCH_HOST);
    }
    else {
	if (param.conf_network) {
		DEBUG0(debug.netname,0,3030);
		rc = nn_network_look_up(host_id, port_name, port_ptr);
#if	COMPAT
		if ((param.compat) && (ServPort != PORT_NULL) 
					&& (rc == NETNAME_HOST_NOT_FOUND)) {
			DEBUG_STRING(debug.netname,0,3020,port_name);
			rc = netname_look_up(name_server_port,host_name,port_name,port_ptr);
			DEBUG1(debug.netname,0,3021,rc);
			if (rc == SEND_INVALID_PORT)
				panic("Compatibility server died");
		}
#endif	COMPAT
		RETURN(rc);
	} else {
		DEBUG0(debug.netname,0,3031);
		RETURN(NETNAME_NOT_CHECKED_IN);
	}
    }
END


/*
 * nn_port_test
 *	Sees whether a given queue entry contains the input port.
 *
 * Parameters:
 *	q_item	: the queue entry in question
 *	port_id	: the port in question
 *
 * Returns:
 *	TRUE if the port_id matches either the named or signature port if the queued item.
 *
 */
PRIVATE nn_port_test(q_item, port_id)
register lock_queue_t	q_item;
register int		port_id;
BEGIN("nn_port_test")


    RETURN((((nn_entry_ptr_t)q_item)->nne_port == (port_t)port_id)
		|| (((nn_entry_ptr_t)q_item)->nne_signature == (port_t)port_id));

END
 

/*
 * nn_remove_entries
 *	Remove entries in the local name table for a given port.
 *
 * Parameters:
 *	port_id	: the port in question
 *
 * Design:
 *	Looks for entries in the name table for which this port is either
 *	the signature port or the named port.
 *	These entries are deleted.
 *
 */
EXPORT void nn_remove_entries(port_id)
port_t		port_id;
BEGIN("nn_remove_entries")
    int			index;
    lock_queue_t	lq;
    cthread_queue_item_t	q_item;

    DEBUG1(TRUE, 3, 1124, port_id);
    for (index = 0; index < NN_TABLE_SIZE; index++) {
	lq = &nn_table[index];
	while ((q_item = lq_cond_delete_from_queue(lq, nn_port_test, (int)port_id)) != (cthread_queue_item_t)0) {
	    MEM_DEALLOCOBJ(q_item, MEM_NNREC);
	}
    }

    RET;

END


/*
 * _netname_version
 *	Returns the version of this network server.
 *
 * Parameters:
 *	ServPort	: the port on which the request was received.
 *	version		: where to put the version string,
 *
 * Design:
 *	Just return the rcsid of this file.
 *
 */
PUBLIC _netname_version(ServPort, version)
port_t		ServPort;
netname_name_t	version;
BEGIN("_netname_version")

#ifdef lint
    ServPort;
#endif lint

    (void)strcpy((char *)version, (char *)nn_procs_rcsid);
    RETURN(NETNAME_SUCCESS);

END



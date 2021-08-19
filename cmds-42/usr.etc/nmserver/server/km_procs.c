/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * km_procs.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/km_procs.c,v $
 *
 */

#ifndef	lint
char km_procs_rcsid[] = "$Header: km_procs.c,v 1.1 88/09/30 15:39:55 osdev Exp $";
#endif not lint

/*
 * Key management functions called as a result of incoming messages.
 */

/*
 * HISTORY:
 *  4-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed queue_item_t to cthread_queue_item_t
 * 
 * 25-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Fixed a few NM_USE_KDS conditionals to avoid compiler warnings.
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  2-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
 *	km_client_retry should only have one parameter.
 *	Use lock_queue_macros.
 *	Added a server prefix.
 *
 *  6-Aug-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	One more conditional on NM_USE_KDS, around km_retry.
 *
 *  6-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added a timer to a key request record in order to repeat key
 *	exchange requests if we get no response to them.
 *
 *  5-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Stored host order key for multperm encrytion algorithm.
 *	Only call the KDS functions if NM_USE_KDS is on.
 *	Use lq_cond_delete_from_queue instead of lq_find_in_queue/lq_remove_from_queue.
 *
 * 28-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed km_set_crypt_algorithm.
 *
 * 28-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	km_queue is now statically allocated.
 *	Lock is now inline in port record.
 *	Replaced fprintf by ERROR macro.
 *
 * 18-Mar-87  Robert Sansom (rds) at Carnegie Mellon University
 *	km_use_key_for_port is no longer needed.
 *	Removed registered_port parameter from calls.
 *
 * 12-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <mach.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "crypt.h"
#include "debug.h"
#include "key_defs.h"
#include "keyman.h"
#include "km_defs.h"
#include "lock_queue.h"
#include "lock_queue_macros.h"
#include "mem.h"
#include "multperm.h"
#include "netmsg.h"
#include "nm_defs.h"
#include "nm_extra.h"
#include "port_defs.h"
#include "portrec.h"
#include "timer.h"
#include "nn_defs.h"	/* MEM_NNREC */

#if	NM_USE_KDS
PRIVATE port_t			km_kds_port = PORT_NULL;
#endif	NM_USE_KDS

PRIVATE struct lock_queue	km_queue;



/*
 * km_procs_init
 *	Initialises the km_queue.
 *
 */
PUBLIC void km_procs_init()
BEGIN("km_procs_init")

    lq_init(&km_queue);
    RET;

END


/*
 * kmq_host_equal
 *	Checks whether the host in a queue entry is the same as the host_id parameter.
 *
 * Parameters:
 *	queue_entry	: pointer to an entry in the km_queue
 *	host_id		: the host id to be matcher
 *
 * Returns:
 *	TRUE or FALSE
 *
 */
#define kmq_host_equal(q_entry, host_id) (((kmq_entry_ptr_t)(q_entry))->kmq_host_id == (host_id))



#if	NM_USE_KDS
/*
 * _km_kds_connect
 *	Used by the local kds to connect to the network server.
 *
 * Parameters:
 *	ServPort	: ignored.
 *	kds_port	: a port to the local Key Distribution Server.
 *
 * Results:
 *	KM_SUCCESS.
 *
 * Note:
 *	the ServPort is guaranteed by keyman.c to be the km_service_port.
 *
 */
PUBLIC _km_kds_connect(ServPort, kds_port)
port_t		ServPort;
port_t		kds_port;
BEGIN("_km_kds_connect")

#ifdef lint
    ServPort;
#endif

    LOG1(TRUE, 3, 1153, kds_port);
#if	NM_USE_KDS
    km_kds_port = kds_port;
#endif	NM_USE_KDS
    RETURN(KM_SUCCESS);

END



/*
 * _km_use_key_for_host
 *	Used by local kds to set up one end of a secure connection.
 *
 * Parameters:
 *	ServPort	: ignored
 *	host_id		: the host for which a secure connection is being set up
 *	key		: the key for this connection
 *
 * Results:
 *	KM_SUCCESS or KM_FAILURE.
 *
 * Side effects:
 *	May call the retry function in the queue entries.
 *
 * Design:
 *	Enter the new key into the host entry.
 *	Look for and delete matching entries in km_queue;
 *		for each entry call the stored retry function with the client id as the parameter.
 *
 * Note:
 *	the ServPort is guaranteed by keyman.c to be the km_service_port.
 *
 */
/*ARGSUSED*/
PUBLIC _km_use_key_for_host(ServPort, host_id, key)
port_t		ServPort;
netaddr_t	host_id;
key_t		key;
BEGIN("_km_use_key_for_host")
    key_rec_ptr_t		key_rec_ptr;
    kmq_entry_ptr_t		q_entry;
    cthread_queue_item_t	ret;

    DEBUG0(TRUE, 3, 1155);
    DEBUG_KEY(TRUE, 3, key);
    DEBUG_NETADDR(TRUE, 3, host_id);

    if (((key_rec_ptr = km_host_lookup(host_id)) == KEY_REC_NULL)
	&& ((key_rec_ptr = km_host_enter(host_id)) == KEY_REC_NULL))
    {
	ERROR((msg, "_km_use_key_for_host.km_host_enter fails, host_id = %x.", host_id));
	RETURN(KM_FAILURE);
    }
    key_rec_ptr->kr_key = key;
    /*
     * Calculate the inverse and host byte order multperm keys and store them.
     */
    key_rec_ptr->kr_mpkey = key;
    NTOH_KEY(key_rec_ptr->kr_mpkey);
    key_rec_ptr->kr_mpikey = key_rec_ptr->kr_mpkey;
    invert_key(&key_rec_ptr->kr_mpikey);

    /*
     * Now use lq_cond_delete_macro to search through the km_queue.
     */
    while (1) {
	lq_cond_delete_macro(&km_queue, kmq_host_equal, (int)host_id, ret);
	if (!(q_entry = (kmq_entry_ptr_t)ret)) break;

	if (q_entry->kmq_client_retry) {
	    (void)q_entry->kmq_client_retry(q_entry->kmq_client_id);
	}
	(void)timer_stop(&q_entry->kmq_timer);
	MEM_DEALLOCOBJ(q_entry, MEM_NNREC);
    }

    RETURN(KM_SUCCESS);

END
#else	NM_USE_KDS
_km_dummy()
{
}
#endif	NM_USE_KDS



#if	0
/*
 * km_use_key_for_port
 *	Called by local and central verification servers to
 *	set up initial secure connection to central machine
 *
 * Parameters:
 *	ServPort	: ignored
 *
 * Results:
 *	KM_SUCCESS or KM_FAILURE.
 *
 * Side effects:
 *	Calls kds_new_key_for_host to inform the local kds about this new key
 *
 * Design:
 *	Looks up the port in the port records.
 *	Enters the key in the record associated with the ports receiver.
 *
 * Note:
 *	the ServPort is guaranteed by keyman.c to be the km_service_port.
 *
 */
PUBLIC km_use_key_for_port(ServPort, port, key)
port_t		ServPort;
port_t		port;
key_t		key;
BEGIN("km_use_key_for_port")
    port_rec_ptr_t	port_rec_ptr;
    key_rec_ptr_t	key_rec_ptr;
    netaddr_t		host_id;
    kern_return_t	kr;

#ifdef lint
    ServPort;
#endif

    if ((port_rec_ptr = pr_lportlookup(port)) == PORT_REC_NULL) {
	ERROR((stderr, "km_use_key_for_port: port %d not known.\n", port));
	RETURN(KM_FAILURE);
    }

    host_id = port_rec_ptr->portrec_network_port.np_receiver;
    lk_unlock(&port_rec_ptr->portrec_lock);

    if (((key_rec_ptr = km_host_lookup(host_id)) == KEY_REC_NULL)
	&& ((key_rec_ptr = km_host_enter(host_id)) == KEY_REC_NULL))
    {
	ERROR((msg, "km_use_key_for_port.km_host_enter fails, host_id = %x.", host_id));
	RETURN(KM_FAILURE);
    }
    key_rec_ptr->kr_key = key;

    if (km_kds_port == PORT_NULL) {
	ERROR((msg, "km_use_key_for_port: km_kds_port is null."));
    }
    else if ((kr = kds_new_key_for_host(km_kds_port, host_id, key)) != 0) {
	ERROR((msg, "km_use_key_for_port.kds_new_key_for_host fails, kr = %d.", kr));
    }

    RETURN(KM_SUCCESS);

END
#endif	0


#if	NM_USE_KDS

/*
 * km_retry
 *	called if the timer on a key request expires.
 *
 * Parameters:
 *	timer	: the timer that expired.
 *
 * Design:
 *	just retries the key exchange request.
 *
 */
PRIVATE km_retry(timer)
timer_t		timer;
BEGIN("km_retry")

    kmq_entry_ptr_t	kmq_entry_ptr = (kmq_entry_ptr_t)timer->info;
    kern_return_t	kr;

    if ((kr = kds_do_key_exchange(km_kds_port, kmq_entry_ptr->kmq_host_id)) != KM_SUCCESS) {
	   ERROR((msg, "km_retry.kds_do_key_exchange fails, kr = %d.", kr));
	}
    timer_restart(&kmq_entry_ptr->kmq_timer);

    RETURN(0);

END
#endif	NM_USE_KDS



/*
 * km_do_key_exchange
 *
 * Parameters:
 *	client_id	: the id of the client making this request
 *	client_retry	: a function to call when we actually get a new key
 *	host_id		: the host for which a key is required
 *
 * Design:
 *	Checks to see if there is a pending key exchange request in the km_queue.
 *	If there is just add this new request to the queue.
 *	If there is not add this new request to the queue and call the local Key
 *		Distribution Server to start a key exchange.
 *
 * Note:
 *	Use a MEM_NNREC object for a queue entry (should be large enough).
 *
 */
EXPORT void km_do_key_exchange(client_id, client_retry, host_id)
int		client_id;
int		(*client_retry)();
netaddr_t	host_id;
BEGIN("km_do_key_exchange")

#if	NM_USE_KDS
    cthread_queue_item_t	ret;
    kmq_entry_ptr_t		q_entry;
    kern_return_t		kr;

    MEM_ALLOCOBJ(q_entry,kmq_entry_ptr_t,MEM_NNREC);
    q_entry->kmq_host_id = host_id;
    q_entry->kmq_client_id = client_id;
    q_entry->kmq_client_retry = client_retry;

    lq_find_macro(&km_queue, kmq_host_equal, (int)host_id, ret);
    if (ret) {
	lq_enqueue(&km_queue, q_entry);
    }
    else {
	lq_enqueue(&km_queue, q_entry);
	if ((kr = kds_do_key_exchange(km_kds_port, host_id)) != KM_SUCCESS) {
	    ERROR((msg, "km_do_key_exchange.kds_do_key_exchange fails, kr = %d.", kr));
	}
	/*
	 * Start up the retry timer.
	 */
	q_entry->kmq_timer.action = km_retry;
	q_entry->kmq_timer.info = (char *)q_entry;
	q_entry->kmq_timer.interval.tv_sec = KM_RETRY_INTERVAL;
	q_entry->kmq_timer.interval.tv_usec = 0;
	timer_start(&q_entry->kmq_timer);
    }

#endif	NM_USE_KDS

    RET;

END


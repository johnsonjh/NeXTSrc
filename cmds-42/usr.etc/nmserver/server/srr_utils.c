/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * srr_utils.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/srr_utils.c,v $
 *
 */
#ifndef	lint
char srr_utils_rcsid[] = "$Header: srr_utils.c,v 1.1 88/09/30 15:42:28 osdev Exp $";
#endif not lint
/*
 * Utilities for the simple request-response transport protocol including:
 *	A hash table containing information about other hosts.
 *	Queueing and dequeueing of request records on host records.
 */

/*
 * HISTORY:
 * 05-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Added USE_SRR.
 *
 * 31-May-88 Mary Thompson (mrt) at Carnegie Mellon
 *	Removed \ escapes in macros since they didn't work
 * 	on the other machine. MEM_ALLOC macro has been changed.
 *
 * 24-May-88 Daniel Julin (dpj) at Carnegie Mellon 
 *	Added \ escapes before newline in invocations of
 *	macros so that it would compile on the MMAX
 *
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 22-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed the intialisation of the sbuf in srr_hash_enter.
 *	Replaced fprintf by ERROR and LOG macros.
 *	Lock and timer in host info record are now inline.
 *	Statically allocate srr_hash_table_lock.
 *	Set timer from values in param record.
 *
 * 18-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Use mem_allocobj wherever possible.
 *	Grow the hash table when it becomes approximately 80% full.
 *
 * 25-Nov-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Added intialisation of sbuf field in host info record to srr_hash_enter.
 *
 *  6-Nov-86  Robert Sansom (rds) at Carnegie-Mellon University
 *	Started.
 *
 */

#include "netmsg.h"
#include "nm_defs.h"

#if	USE_SRR

#include <sys/time.h>
#include <mach.h>

#include "debug.h"
#include "ls_defs.h"
#include "nm_extra.h"
#include "srr.h"
#include "srr_defs.h"
#include "timer.h"


/*
 * Our host hash table - just hash on the host_low field of a hosts address.
 */

typedef srr_host_info_ptr_t	*srr_hash_table_t;

#define INIT_HASH_TABLE_SIZE	256
#define INIT_HASH_TABLE_LIMIT	200
#define HASH_TABLE_ENTRY_SIZE	(sizeof(srr_host_info_ptr_t))

static srr_hash_table_t		srr_hash_table;
static int			srr_hash_table_size = INIT_HASH_TABLE_SIZE;
static int			srr_hash_table_inc = 1;
static int			srr_hash_table_limit = INIT_HASH_TABLE_LIMIT;
static int			srr_hash_table_num_entries = 0;
static struct mutex		srr_hash_table_lock;


/*
 * The current incarnation of this network server.
 */
static long current_incarnation = 0;


/*
 * srr_utils_init
 *	Initialises:
 *	    the host hash table and lock,
 *	    the current incarnation number.
 *
 */
PUBLIC void srr_utils_init()
BEGIN("srr_utils_init")
    int i;
    struct timeval tp;
    struct timezone tzp;


    MEM_ALLOC(srr_hash_table,srr_hash_table_t,	
			(INIT_HASH_TABLE_SIZE*HASH_TABLE_ENTRY_SIZE), FALSE);
    for (i = 0; i < srr_hash_table_size; i++) srr_hash_table[i] = SRR_NULL_HOST_INFO;

    mutex_init(&srr_hash_table_lock);

    (void)gettimeofday(&tp, &tzp);
    current_incarnation = (long)tp.tv_sec;

    RET;
END


/*
 * srr_grow_hash_table
 *	doubles the size of the hash table
 *
 * Side effects:
 *	a new hash table with the entries from the old one rehashed into it
 *
 * Design:
 *	Create a new hash table that is twice as big as the old one.
 *	Rehash the entries from the old table into the new one.
 *	Delete the old table.
 *
 * Note:
 *	We are responsible for our own locking;
 *	(assume that caller has not left the hash table locked).
 *
 */
PRIVATE void srr_grow_hash_table()
BEGIN("srr_grow_hash_table")
    int			old_size, new_size, i;
    srr_hash_table_t	new_hash_table, old_hash_table;

    mutex_lock(&srr_hash_table_lock);

    old_size = srr_hash_table_size;
    new_size = srr_hash_table_size = 2 * old_size;
    srr_hash_table_inc = old_size + 1;
    srr_hash_table_limit *= 2;

    /*
     * Now create and initialise the new hash table.
     */
    old_hash_table = srr_hash_table;
    MEM_ALLOC(new_hash_table,srr_hash_table_t,	
				(new_size * HASH_TABLE_ENTRY_SIZE), FALSE);
    for (i = 0; i < new_size; i++) new_hash_table[i] = SRR_NULL_HOST_INFO;
    srr_hash_table = new_hash_table;

    /*
     * Rehash the entries from the old hash table into the new one.
     */
    for (i = 0; i < old_size; i++) {
	ip_addr_t		host_ip_addr;
	int			index;

		
	host_ip_addr.ia_netaddr = old_hash_table[i]->shi_host_id;
	index = host_ip_addr.ia_bytes.ia_host_low;	

	while (new_hash_table[index] != SRR_NULL_HOST_INFO) {
	    index = (index + srr_hash_table_inc) % new_size;
	}

	new_hash_table[index] = old_hash_table[i];
    }

    mutex_unlock(&srr_hash_table_lock);
    MEM_DEALLOC((pointer_t)old_hash_table, (old_size * HASH_TABLE_ENTRY_SIZE));
END



/*
 * srr_hash_enter
 *
 * Parameters:
 *	host_id	: the host to create a new record for in the hash table
 *
 * Results:
 *	A pointer to the record for this host.
 *
 * Notes:
 *	We must lock the hash table for mutual exclusion.
 *	If the host already exists in the hash table,
 *	then just return a pointer to its record.
 *
 */
PUBLIC srr_host_info_ptr_t srr_hash_enter(host_id)
netaddr_t host_id;
BEGIN("srr_hash_enter")
    ip_addr_t host_ip_addr;
    int index, original_index;
    srr_host_info_ptr_t new_record_ptr;

    host_ip_addr.ia_netaddr = host_id;
    original_index = index = host_ip_addr.ia_bytes.ia_host_low;

    mutex_lock(&srr_hash_table_lock);

    srr_hash_table_num_entries ++;
    if (srr_hash_table_num_entries > srr_hash_table_limit) {
	mutex_unlock(&srr_hash_table_lock);
	srr_grow_hash_table();
	mutex_lock(&srr_hash_table_lock);
    }

    while ((srr_hash_table[index] != SRR_NULL_HOST_INFO)
		&& (srr_hash_table[index]->shi_host_id != host_id))
    {
	index = (index + srr_hash_table_inc) % srr_hash_table_size;
	if (index == original_index) {
	    mutex_unlock(&srr_hash_table_lock);
	    LOG0(TRUE, 3, 1050);
	    srr_grow_hash_table();
	    /*
	     * Retry this call to enter.
	     */
	    {
		register srr_host_info_ptr_t	shi_ptr;
		shi_ptr = srr_hash_enter(host_id);
		RETURN(shi_ptr);
	    }
	}
    }

    if (srr_hash_table[index] != SRR_NULL_HOST_INFO) {
	mutex_unlock(&srr_hash_table_lock);
	LOG0(TRUE, 3, 1051);
	RETURN(srr_hash_table[index]);
    }

    /*
     * Otherwise create a new record.
     */
    MEM_ALLOC(new_record_ptr,srr_host_info_ptr_t,sizeof(srr_host_info_t), FALSE);

    /*
     * Initialise the new record.
     */
    new_record_ptr->shi_host_id = host_id;
    mutex_init(&new_record_ptr->shi_lock);

    /* Request status */
    new_record_ptr->shi_request_status = SRR_INACTIVE;
    new_record_ptr->shi_request_tries = 0;
    new_record_ptr->shi_request_uid.srr_uid_incarnation = current_incarnation;
    new_record_ptr->shi_request_uid.srr_uid_sequence_no = 0;
    new_record_ptr->shi_request_q_head = SRR_NULL_Q;
    new_record_ptr->shi_request_q_tail = SRR_NULL_Q;

    /* The request retransmission timer. */
    new_record_ptr->shi_timer.interval.tv_sec = param.srr_retry_sec;
    new_record_ptr->shi_timer.interval.tv_usec = param.srr_retry_sec;
    new_record_ptr->shi_timer.action = srr_retry;
    new_record_ptr->shi_timer.info = (char *)new_record_ptr;

    /* Response status */
    new_record_ptr->shi_response_uid.srr_uid_incarnation = 0;
    new_record_ptr->shi_response_uid.srr_uid_sequence_no = 0;
    new_record_ptr->shi_response_packet = SRR_NULL_PACKET;

    srr_hash_table[index] = new_record_ptr;
    mutex_unlock(&srr_hash_table_lock);
    RETURN(new_record_ptr);

END



/*
 * srr_hash_lookup
 *
 * Parameters:
 *	host_id	: the host to look up
 *
 * Results:
 *	A pointer to the record for this host (0 if not found).
 *
 * Notes:
 *	Must lock the hash table for mutual exclusion.
 *
 */
PUBLIC srr_host_info_ptr_t srr_hash_lookup(host_id)
netaddr_t host_id;
BEGIN("srr_hash_lookup")
    ip_addr_t host_ip_addr;
    int index, original_index;

    host_ip_addr.ia_netaddr = host_id;
    original_index = index = host_ip_addr.ia_bytes.ia_host_low;

    mutex_lock(&srr_hash_table_lock);
    while ((srr_hash_table[index] != SRR_NULL_HOST_INFO)
		&& (srr_hash_table[index]->shi_host_id != host_id))
    {
	index = (index + srr_hash_table_inc) % srr_hash_table_size;
	if (index == original_index) {
	    mutex_unlock(&srr_hash_table_lock);
	    panic("srr_hash_lookup: hash table full");
	}
    }
    mutex_unlock(&srr_hash_table_lock);

    RETURN(srr_hash_table[index]);

END



/*
 * srr_enqueue
 *	Queues a request at the tail of the request queue.
 *
 * Parameters:
 *	request_ptr	: the request to be queued
 *	host_record	: the host for which the request is to be queued
 *
 * Note:
 *	We assume that the host record is already locked so that
 *	we can change the queue without fear of conflicting updates.
 *
 */
PUBLIC void srr_enqueue(request_ptr, host_record)
srr_request_q_ptr_t	request_ptr;
srr_host_info_ptr_t	host_record;
BEGIN("srr_enqueue")
    request_ptr->srq_next = SRR_NULL_Q;
    if (host_record->shi_request_q_head == SRR_NULL_Q) {
	host_record->shi_request_q_head = host_record->shi_request_q_tail = request_ptr;
    }
    else {
	host_record->shi_request_q_tail->srq_next = request_ptr;
	host_record->shi_request_q_tail = request_ptr;
    }
    RET;
END



/*
 * srr_dequeue
 *	Dequeues a request from the head of the request queue.
 *
 * Parameters:
 *	host_record	: the host for which a record is to be dequeued
 *
 * Results:
 *	a pointer to the dequeued request
 *
 * Note:
 *	We assume that the host record is already locked so that
 *	we can change the queue without fear of conflicting updates.
 *
 */
PUBLIC srr_request_q_ptr_t srr_dequeue(host_record)
srr_host_info_ptr_t	host_record;
BEGIN("srr_dequeue")
    register srr_request_q_ptr_t dequeued_request;

    dequeued_request = host_record->shi_request_q_head;
    if (dequeued_request != SRR_NULL_Q) {
	if ((host_record->shi_request_q_head = dequeued_request->srq_next) == SRR_NULL_Q) {
	    host_record->shi_request_q_tail = SRR_NULL_Q;
	}
	else {
	    dequeued_request->srq_next = SRR_NULL_Q;
	}
    }
    RETURN(dequeued_request);
END

#endif	USE_SRR

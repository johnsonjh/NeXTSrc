/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * lock_queue.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/lock_queue.h,v $
 *
 * $Header: lock_queue.h,v 1.1 88/09/30 15:44:03 osdev Exp $
 *
 */

/*
 * Contains the definitions for the timer package
 */

/*
 * HISTORY:
 *  4-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed queue_item_t to cthread_queue_item_t
 * 
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 *  6-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added versions of most routines that do not do the locking
 *	internally.
 *
 * 18-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed lq_cond_delete_from_queue to return the item deleted.
 *
 * 15-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed the explict queue_item_t parameter from 
 *	lq_cond_delete_from_queue.
 *	The test function for it is now the same as for lq_find_in_queue.
 *
 * 18-Mar-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed lock_queue_t to include the mutex_t and condition_t inline.
 *	Removed 'struct data'.
 *
 *  3-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Made definitions of functions externs; added types where relevant.
 *	Changed lq_alloc to return a lock_queue_t.
 *
 * 10-Oct-86  Sanjay Agrawal (agrawal) at Carnegie Mellon University
 *	Started
 */

#include <cthreads.h>
#include <sys/boolean.h>

#include "mem.h"

#ifndef _LOCK_QUEUE_
#define _LOCK_QUEUE_


typedef struct lock_queue {
    struct mutex		lock;
    cthread_queue_item_t	head;
    cthread_queue_item_t	tail;
    struct condition		nonempty;
} *lock_queue_t;

#define LOCK_QUEUE_NULL	(lock_queue_t)0



/*
 * LQ_ALLOC:
 *	Allocates data space for the lock and the nonempty condition field.
 *	Calls lq_init to initialize the queue.
 */
extern lock_queue_t lq_alloc();


/*
 * LQ_INIT:
 *	Initializes the head and the tail of the queue to nil.
 *	Initialises the lock and condition of the queue.
 */
void lq_init(/*q*/);
/*
lock_queue_t	q;
*/



/*
 * LQ_PREQUEUE:
 *	Inserts queue_item at the head of the queue.
 *	Locks queue while accessing queue.
 *	Signals queue nonempty.
 */
extern void lq_prequeue(/*q, x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_ENQUEUE:
 *	Enters queue item at the tail of the queue.
 *	Locks queue while accessing queue.
 */
extern void lq_enqueue(/*q, x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_INSERT_IN_QUEUE:
 *	Inserts queue_item in the correct positiion on the queue.
 *	Does so by calling a test function to do a comparison.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		x	- the queue_item passed in, and
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue.
 *	Signals queue nonempty.
 */
extern void lq_insert_in_queue(/*q, test, x, args*/);
/*
lock_queue_t		q;
int			(*test)();
cthread_queue_item_t	x;
int			args;
*/



/*
 * LQ_DEQUEUE:
 *	If queue is not empty then removes item from the head of the queue.
 *	Locks queue while accessing queue.
 */
extern cthread_queue_item_t lq_dequeue(/*q*/);
/*
lock_queue_t	q;
*/


/*
 * LQ_BLOCKING_DEQUEUE:
 *	If the queue is empty, a wait is done on the nonempty condition.
 *	Removes item from the head of the queue.
 *	Locks queue while accessing queue
 */
extern cthread_queue_item_t lq_blocking_dequeue(/*q*/);
/*
lock_queue_t	q;
*/
	

/*
 * LQ_REMOVE_FROM_QUEUE:
 *	Removes the queue_item from the queue if it is present on the queue.
 *	Returns whether the item was deleted from the queue.
 */
extern boolean_t lq_remove_from_queue(/*q,x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_COND_DELETE_FROM_QUEUE:
 *	Performs the test function with each element of the queue, 
 *	until the function returns true, or the tail of the queue is reached.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	The item is then removed from the queue.
 *	Locks queue while accessing queue.
 *	Returns the item that was deleted from the queue.
 */
extern cthread_queue_item_t lq_cond_delete_from_queue(/*q, test, args*/);
/*
lock_queue_t	q;
int		(*test)();
int		args;
*/



/*
 * LQ_ON_QUEUE:
 *	Locks queue while accessing queue.
 *	Checks to see if the cthread_queue_item_t is on the queue,
 *	if so returns true else returns false.
 */
extern boolean_t lq_on_queue(/*q,x*/);
/*
lock_queue_t		q;
cthread_queue_item_t	x;
*/


/*
 * LQ_FIND_IN_QUEUE:
 *	Returns a cthread_queue_item_t which is found by the function test.
 *	The parameters passed to the test function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	If no cthread_queue_item_t is found returns nil.
 *	Locks queue while accessing queue
 */
extern cthread_queue_item_t lq_find_in_queue(/*q, fn, args*/);
/*
lock_queue_t	q;
int		(*test)();
int		args;
*/


/*
 * LQ_MAP_QUEUE:
 *	Maps fn() onto each item on the queue;
 *	The parameters passed to the map function are:
 *		current	- the item in the queue that is being looked at,
 *		args	- the argument value passed in.
 *	Locks queue while accessing queue
 */
extern void lq_map_queue(/*q, fn, args*/);
/*
lock_queue_t	q;
int		(*fn)();
int		args;
*/

/*
 * The following routines are identical to their corresponding routines above,
 * but they expect the lock on the queue to be held before they are called, and
 * do not interact with this lock directly.
 */
extern void lqn_prequeue();
extern cthread_queue_item_t lqn_cond_delete_from_queue();
extern cthread_queue_item_t lqn_find_in_queue();

/*
 * Memory management definitions.
 */
extern mem_objrec_t		MEM_LQUEUE;


#endif _LOCK_QUEUE_


/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * timer.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/timer.c,v $
 *
 */
#ifndef	lint
char timer_rcsid[] = "$Header: timer.c,v 1.1 88/09/30 15:42:38 osdev Exp $";
#endif not lint

/*
 * Timer package.
 */

/*
 * HISTORY:
 *  4-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed queue_item_t to cthread_queue_item_t
 * 
 * 26-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Converted to use the new memory management module.
 *
 * 10-Oct-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added public timer_wake_up - allows a client to wake up the timer without delay.
 *
 * 14-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Revamp to be dumb - wake up timer thread every TIMER_QUANTUM milliseconds
 *		and calculate intervals in units of two times TIMER_QUANTUM milliseconds.
 *	Use lock_queue_macros and vax_fast_lock macros if possible.
 *
 * 10-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Go back to using mem_allocobj in timer_alloc.
 *
 *  4-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Cheat in timer_run and look directly at the head of the timer queue.
 *
 * 26-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added timer_restart which combines the functionality of timer_stop and timer_start.
 *	Changed timer_start so that it assumes that the given timer is not on the timer queue.
 *	Added some register declarations.
 *
 * 22-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed call to awake_timer in timer_start when the timer is already on the queue.
 *	timer_queue and timer_lock is now allocated statically.
 *	Conditionally use thread_lock - ensures only one thread is executing.
 *	Added call to cthread_set_name.
 *
 * 14-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Revamped for better use outside of the network server:
 *		include trace.h directly for tracing;
 *		use (cthreads) malloc to allocate timers;
 *		use msg_receive for sleeping.
 *
 * 30-Jan-87  Thomas Newton (tdn) at Carnegie Mellon University
 *      Created minor variant of this file for use in Ada+ compiler:
 *       1. Changed timer_stop to return a boolean_t indicating whether
 *	    its argument was found in the timer queue
 *       2. Added timer_kill routine, which terminates the timer thread
 *	    so that the main program can quit without core dumping (the
 *	    threads package considers a single blocked thread an error)
 *
 *  9-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added a lock to protect queue insert/delete operations from duplication.
 *
 * 18-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Use mem_allocobj to allocate timers.
 *
 *  3-Dec-86  Robert Sansom (rds) at Carnegie Mellon University
 *	Changed to use BEGIN, END, RET and RETURN macros.
 *	Changed timer_init to return a boolean_t and use new version of lq_alloc.
 *
 * 28-Nov-86  Sanjay Agrawal (agrawal) at Carnegie Mellon University
 *	Added timer_alloc()
 *
 * 14-Oct-86  Sanjay Agrawal (agrawal) at Carnegie Mellon University
 *	Started
 */

#define TIMER_DEBUG	0
/*
#define DEBUGOFF
*/

#include <cthreads.h>
#include <mach.h>
#include <sys/message.h>

#include "debug.h"
#include "lock_queue.h"
#include "lock_queue_macros.h"
#include "ls_defs.h"
#include "mem.h"
#include "timer.h"
#include "netmsg.h"
#include "trace.h"

#ifdef	vax
#undef	VAX_FAST_LOCK
#define	VAX_FAST_LOCK	1
#else	vax
#undef	VAX_FAST_LOCK
#define	VAX_FAST_LOCK	0
#endif	vax
#if	VAX_FAST_LOCK
#include "vax_fast_lock.h"
#endif	VAX_FAST_LOCK


#define TIMER_QUANTUM	param.timer_quantum


static struct lock_queue	timer_queue;		/* Structure used to queue timers. */
static struct mutex		timer_lock;		/* Lock for timer global values. */
static port_t			timer_port;		/* Port used to wait for messages. */
static long			timer_time;		/* The time that timer_run uses. */

/*
 * Memory management definitions.
 */
PUBLIC mem_objrec_t	MEM_TIMER;



/*
 * GET_ABSOLUTE_TIME
 *	Adds now to the interval of timer t and assigns the result to the deadline of t.
 */
#if	0
#define GET_ABSOLUTE_TIME(now, t) {							\
	t->deadline.tv_sec = now.tv_sec + t->interval.tv_sec;				\
	if ((t->deadline.tv_usec = now.tv_usec + t->interval.tv_usec) >= 1000000) {	\
		t->deadline.tv_usec -= 1000000;						\
		t->deadline.tv_sec ++;							\
	}										\
}
#endif	0
#define GET_ABSOLUTE_TIME(t) (t)->deadline.tv_sec = timer_time + ((t)->interval.tv_sec << 1)



/*
 * timer_run
 *	main loop of timer package.
 *
 * Design:
 *	Loops doing the following:
 *		Tries to get the first item on the timer queue;
 *		If there is nothing there then sleeps for TIMER_QUANTUM using msg_receive
 *		else if the first item is expired then do it.
 *
 */
timer_run()
BEGIN("timer_run")
	register timer_t	first_timer;
	boolean_t		removed;
	register msg_header_t	*timer_msg_ptr;
	msg_header_t		timer_msg;

#if	LOCK_THREADS
	mutex_lock(thread_lock);
#endif	LOCK_THREADS

	timer_msg_ptr = &timer_msg;

	while (1) {
		mutex_lock(&timer_queue.lock);
		first_timer = (timer_t)timer_queue.head;
		mutex_unlock(&timer_queue.lock);
		if ((first_timer) && (timer_time >= first_timer->deadline.tv_sec)) {
			lq_remove_macro(&timer_queue, first_timer, removed);
			if (removed) {
				DEBUG1(TIMER_DEBUG, 1, 1026, (long)first_timer);
				(first_timer->action)(first_timer);
			}
		}
		else {
			/*
			 * Sleep for TIMER_INTERVAL.
			 */
			timer_msg_ptr->msg_size = sizeof(msg_header_t);
			timer_msg_ptr->msg_local_port = timer_port;
			DEBUG2(TIMER_DEBUG, 2, 1027, TIMER_QUANTUM, timer_time)
			(void)msg_receive(timer_msg_ptr, RCV_TIMEOUT, TIMER_QUANTUM);
			mutex_lock(&timer_lock);
			timer_time++;
			mutex_unlock(&timer_lock);
		}
	}

END



/*
 * timer_wake_up
 *	Sends a wake-up message to the timer thread.
 *
 */
void timer_wake_up()
BEGIN("timer_wake_up")
	kern_return_t	kr;
	msg_header_t	wake_up_msg;

	wake_up_msg.msg_simple = TRUE;
	wake_up_msg.msg_type = MSG_TYPE_NORMAL;
	wake_up_msg.msg_size = sizeof(msg_header_t);
	wake_up_msg.msg_remote_port = timer_port;
	wake_up_msg.msg_local_port = PORT_NULL;
	wake_up_msg.msg_id = 1111;

	kr = msg_send(&wake_up_msg, SEND_TIMEOUT, 0);
	if (kr != KERN_SUCCESS) {
	    if (kr == SEND_TIMED_OUT) {
		ERROR((msg, "timer_wake_up.msg_send timed out."));
		LOG0(TIMER_DEBUG, 0, 1024);
	    }
	    else {
		ERROR((msg, "timer_wake_up.msg_send fails, kr = %d.", kr));
	    }
	}

	RET;
END



/*
 * insert_timer_test
 *	compares two timers - returns TRUE if t has a closer deadline than cur.
 */
#define insert_timer_test(cur,t,args) \
	(((timer_t)(t))->deadline.tv_sec < ((timer_t)(cur))->deadline.tv_sec)


/*
 * TIMER_START:
 *	If timer t is not already on the timer queue
 *	then the absolute deadline of t is computed and t is inserted in the timer queue.
 *
 * Note:
 *	Assumes that timer is NOT on the queue - timer_restart should be used if it is.
 */
void timer_start(t)
	register timer_t	t;
BEGIN("timer_start")

	mutex_lock(&timer_lock);
	GET_ABSOLUTE_TIME(t);
	mutex_unlock(&timer_lock);
	lq_insert_macro(&timer_queue, insert_timer_test, (cthread_queue_item_t)t, 0);
	DEBUG3(TIMER_DEBUG, 0, 1025, (long)t, t->deadline.tv_sec, t->interval.tv_sec);
	RET;

END



/*
 * TIMER_STOP:
 *	If timer t is present on the queue then it is removed from the queue.
 *	Returns whether there was a timer to be removed or not.
 */
boolean_t timer_stop(t)
	register timer_t	t;
BEGIN("timer_stop")
	boolean_t	ret;

	lq_remove_macro(&timer_queue, (cthread_queue_item_t)t, ret);
	if (ret) {
		DEBUG1(TIMER_DEBUG, 0, 1020, (long)t);
		RETURN(TRUE);
	} 
	else {
		DEBUG1(TIMER_DEBUG, 0, 1021, (long)t);
		RETURN(FALSE);
	}

END



/*
 * TIMER_RESTART:
 *	If timer t is present on the queue then it is removed.
 *	Queues timer t up as in timer_start.
 */
void timer_restart(t)
	register timer_t	t;
BEGIN("timer_restart")
	boolean_t	ret;

	lq_remove_macro(&timer_queue, (cthread_queue_item_t)t, ret);
	mutex_lock(&timer_lock);
	GET_ABSOLUTE_TIME(t);
	mutex_unlock(&timer_lock);
	lq_insert_macro(&timer_queue, insert_timer_test, (cthread_queue_item_t)t, 0);
	DEBUG1(TIMER_DEBUG, 0, 1028, (long)t);

	RET;

END




/*
 * TIMER_ALLOC:
 *	allocates space for a timer and initialises it.
 */
timer_t timer_alloc()
BEGIN("timer_alloc")
	register timer_t	new_timer;

	MEM_ALLOCOBJ(new_timer,timer_t,MEM_TIMER);
	DEBUG1(TIMER_DEBUG, 0, 1022, (long)new_timer);
	new_timer->link = (struct timer *)0;
	new_timer->interval.tv_sec = 0;
	new_timer->interval.tv_usec = 0;
	new_timer->action = (int (*)())0;
	new_timer->info = (char *)0;
	new_timer->deadline.tv_sec = 0;
	new_timer->deadline.tv_usec = 0;
	RETURN(new_timer);
END



/*
 * TIMER_INIT:
 *	Initilizes the timer package.
 *	Allocates the port used for wake-up messages.
 *	Creates a thread which waits for timers to expire.
 */
boolean_t timer_init()
BEGIN("timer_init")
	kern_return_t	kr;
	cthread_t	new_thread;

	/*
	 * Initialize the memory management facilities.
	 */
	mem_initobj(&MEM_TIMER,"Timer record",sizeof(struct timer),
								FALSE,140,50);


	timer_time = 0;
	lq_init(&timer_queue);
	mutex_init(&timer_lock);

	if ((kr = port_allocate(task_self(), &timer_port)) != KERN_SUCCESS) {
		ERROR((msg, "timer_init.port_allocate fails, kr = %s.\n", kr));
		RETURN(FALSE);
	}
	(void)port_disable(task_self(), timer_port);

	new_thread = cthread_fork((cthread_fn_t)timer_run, (any_t)0);
	cthread_set_name(new_thread, "timer_run");
	cthread_detach(new_thread);

	RETURN(TRUE);
END



/*
 * timer_cthread_exit
 *	just calls cthread_exit.
 *
 */
timer_cthread_exit() {

    cthread_exit((any_t)0);
}


/*
 * timer_always_true
 *	used when calling lq_find_in_queue.
 *
 */
#define timer_always_true(item,args) TRUE


/*
 * TIMER_KILL:
 *	Terminates the background timer thread by tricking it into suicide.
 *	Note that the timer thread may not be terminated upon return --
 *	the purpose of this routine is to clean up when a program is
 *	ready to terminate so that the threads package will not dump core.
 */
void timer_kill()
BEGIN("timer_kill")
	static struct timer	suicide;
	cthread_queue_item_t		ret;

	do {
		cthread_yield();
		lq_find_macro(&timer_queue, timer_always_true, 0, ret);
	} while (ret);

	suicide.interval.tv_sec  =    0L;   /* An arbitrary time which is */
	suicide.interval.tv_usec = 1000L;   /* instantaneous to humans... */
	suicide.action = (int (*)())timer_cthread_exit;
	DEBUG0(TIMER_DEBUG, 0, 1023);
	timer_start(&suicide);
	RET;
END


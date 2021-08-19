/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * $Log:	ipc.c,v $
 * Revision 1.5  89/08/28  16:10:48  mrt
 * 	Removed the masking out of SEND_NOTIFY in the mach_msg_send call.
 * 	[89/08/08            mrt]
 * 
 * Revision 1.4  89/05/05  18:54:00  mrt
 * 	  7-Dec-88 Mary R. Thompson  (mrt) @ Carnegie Mellon
 * 
 *	Changed the names of the routines to be compatible with
 *	the  new msg.c names
 */
/*
 * ipc.c  by Eric Cooper
 *
 * Simulate blocking IPC in the presence of other coroutines.
 */

#include <cthreads.h>
#include "cthread_internals.h"

extern msg_timeout_t msg_send_timeout;	/* milliseconds */
extern msg_timeout_t msg_receive_timeout;	/* milliseconds */

#if	COROUTINE

#include <sys/message.h>
#include <sys/time.h>

/*
 * C Threads imports:
 */
extern int time_compare();
extern void time_plus(), time_msec_to_timeval();

msg_return_t
msg_send(header, option, timeout)
	msg_header_t *header;
	msg_option_t option;
	msg_timeout_t timeout;
{
	msg_return_t r;
	struct timeval deadline;
	extern msg_return_t mach_msg_send();

	if (option & SEND_TIMEOUT) {
		struct timeval tv;

		time_msec_to_timeval((long) timeout, &tv);
		time_plus((struct timeval *) 0, &tv, &deadline);
	}
	for (;;) {
		if (cthread_count() <= 1) {
			/*
			 * No other threads are runnable.
			 * Go ahead and do the possibly blocking version.
			 */
			return mach_msg_send(header, option, timeout);
		}
		r = mach_msg_send(header, option | SEND_TIMEOUT, msg_send_timeout);
		if (r != SEND_TIMED_OUT ||
		    ((option & SEND_TIMEOUT) &&
		     time_compare(&deadline, (struct timeval *) 0) <= 0))
			return r;
		TRACE(printf("[%s] msg_send()\n", cthread_name(cthread_self())));
		cthread_yield();
	}
}

msg_return_t
msg_receive(header, option, timeout)
	msg_header_t *header;
	msg_option_t option;
	int timeout;
{
	msg_return_t r;
	struct timeval deadline;
	extern msg_return_t mach_msg_receive();

	if (option & RCV_TIMEOUT) {
		struct timeval tv;

		time_msec_to_timeval((long) timeout, &tv);
		time_plus((struct timeval *) 0, &tv, &deadline);
	}
	for (;;) {
		if (cthread_count() <= 1) {
			/*
			 * No other threads are runnable.
			 * Go ahead and do the possibly blocking version.
			 */
			return mach_msg_receive(header, option, timeout);
		}
		r = mach_msg_receive(header, option | RCV_TIMEOUT, msg_receive_timeout);
		if (r != RCV_TIMED_OUT ||
		    ((option & RCV_TIMEOUT) &&
		     time_compare(&deadline, (struct timeval *) 0) <= 0))
			return r;
		TRACE(printf("[%s] msg_receive()\n", cthread_name(cthread_self())));
		cthread_yield();
	}
}

msg_return_t
msg_rpc(header, option, rcv_size, send_timeout, rcv_timeout)
	msg_header_t *header;
	msg_option_t option;
	int rcv_size, send_timeout, rcv_timeout;
{
	msg_return_t r;

	if ((r = msg_send(header, option, send_timeout)) == SEND_SUCCESS) {
		header->msg_size = rcv_size;
		r = msg_receive(header, option, rcv_timeout);
	}
	return r;
}
#endif	COROUTINE

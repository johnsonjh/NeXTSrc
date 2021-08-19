/* 
 * Copyright (c) 1989 NeXT, Inc.
 *
 * Thread-safe sleep()
 */
#include <mach.h>
#include <sys/message.h>
#include <sys/time.h>

#define MAX_SECONDS	(1 << 22)	/* Maximum time to hand to msleep. */
static void msleep(unsigned int msecs);

void
sleep(unsigned int seconds)
{
	/*
	 *	Because the msg_receive time-out works with milliseconds,
	 *	we have to be careful of overflow when converting the
	 *	seconds value into milliseconds.
	 */

	while (seconds > 0) {
		unsigned int msecs;

		if (seconds > MAX_SECONDS) {
			msecs = MAX_SECONDS * 1000;
			seconds -= MAX_SECONDS;
		} else {
			msecs = seconds * 1000;
			seconds = 0;
		}

		msleep(msecs);
	}
}

static void
msleep(unsigned int msecs)
{
	msg_header_t null_msg;
	port_t port;
	struct timeval before, after;
	msg_return_t mr;

	if (port_allocate(task_self(), &port) != KERN_SUCCESS)
		return;

	null_msg.msg_local_port = port;
	null_msg.msg_size = sizeof(null_msg);

	(void) gettimeofday(&before, (struct timezone *) 0);

	for (;;) {
		/*
		 *	At this point, we have saved the current time
		 *	in "before".  If the msg_receive is interrupted,
		 *	then we will use this saved value to calculate
		 *	a new time-out for the next msg_receive.
		 */

		mr = msg_receive(&null_msg, RCV_TIMEOUT|RCV_INTERRUPT, msecs);
		if (mr != RCV_INTERRUPTED)
			break;

		/*
		 *	Adjust the saved "before" time to be the time
		 *	we were hoping to sleep until.  If the current
		 *	time (in "after") is greater than this, then
		 *	we have slept sufficiently despite being interrupted.
		 */

		before.tv_sec += msecs / 1000;
		before.tv_usec += (msecs % 1000) * 1000;
		if (before.tv_usec > 1000000) {
			before.tv_usec -= 1000000;
			before.tv_sec += 1;
		}

		(void) gettimeofday(&after, (struct timezone *) 0);

		if (timercmp(&before, &after, <))
			break;

		/*
		 *	Calculate a new time-out value that should
		 *	get us to the "before" value.  If "before"
		 *	and "after" are close, this might be zero.
		 */

		msecs = ((before.tv_sec - after.tv_sec) * 1000 +
			 (before.tv_usec - after.tv_usec) / 1000);
		if (msecs == 0)
			break;

		before = after;
	}

	(void) port_deallocate(task_self(), port);
}

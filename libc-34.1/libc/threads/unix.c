/*
 * Simulate blocking UNIX system calls in the presence
 * of other coroutines.
 */

#include <cthreads.h>
#include "cthread_internals.h"
#include <sys/time.h>

#if	COROUTINE
struct timeval unix_select_timeout = {
	0,	/* seconds */
	10000,	/* microseconds */
};

#include <sys/types.h>
#include <syscall.h>

/*
 * C Threads imports:
 */
extern int time_compare();
extern void time_plus();

/*
 * C library imports:
 */
extern syscall();

#define	FD_BYTES(n)	(howmany((n), NFDBITS) * sizeof(fd_mask))

select(nfds, readfds, writefds, exceptfds, timeout)
	int nfds;
	fd_set *readfds, *writefds, *exceptfds;
	struct timeval *timeout;
{
	int n;
	struct timeval tval, deadline;
	fd_set r, w, e;
	register fd_set *rp = (readfds == 0 ? 0 : &r);
	register fd_set *wp = (writefds == 0 ? 0 : &w);
	register fd_set *ep = (exceptfds == 0 ? 0 : &e);

	if (timeout != 0)
		time_plus((struct timeval *) 0, timeout, &deadline);
	for (;;) {
		if (cthread_count() <= 1) {
			/*
			 * No other threads are runnable.
			 * Go ahead and do the possibly blocking version.
			 */
			return syscall(SYS_select, nfds, readfds, writefds, exceptfds, timeout);
		}
		/*
		 * Only copy in portion of fd_set required by value of nfds.
		 */
		if (rp != 0)
			bcopy((char *) readfds, (char *) rp, FD_BYTES(nfds));
		if (wp != 0)
			bcopy((char *) writefds, (char *) wp, FD_BYTES(nfds));
		if (ep != 0)
			bcopy((char *) exceptfds, (char *) ep, FD_BYTES(nfds));
		tval = unix_select_timeout;
		n = syscall(SYS_select, nfds, rp, wp, ep, &tval);
		switch (n) {
		    case -1:
			return -1;
		    case 0:
			if (timeout != 0 &&
			    time_compare(&deadline, (struct timeval *) 0) <= 0)
				return 0;
			TRACE(printf("[%s] select()\n", cthread_name(cthread_self())));
			cthread_yield();
			continue;
		    default:
			/*
			 * Only copy out portion of fd_set required by value of nfds.
			 */
			if (rp != 0)
				bcopy((char *) rp, (char *) readfds, FD_BYTES(nfds));
			if (wp != 0)
				bcopy((char *) wp, (char *) writefds, FD_BYTES(nfds));
			if (ep != 0)
				bcopy((char *) ep, (char *) exceptfds, FD_BYTES(nfds));
			return n;
		}
	}
}
#endif	COROUTINE

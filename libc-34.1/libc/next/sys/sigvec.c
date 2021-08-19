/*	@(#)sigvec.c	1.0	12/20/86	(c) 1986 NeXT	*/

#include <syscall.h>
#include <signal.h>
#include <sys/signal.h>
#include <errno.h>

/*
 *	Intercept the sigvec syscall and use our signal trampoline
 *	as the signal handler instead.  The code here is derived
 *	from sigvec in sys/kern_sig.c.
 */

extern void	(*sigcatch[NSIG])();

sigvec (sig, nsv, osv)
	register struct sigvec *nsv, *osv;
{
	struct sigvec vec;
	void (*prevsig)();
	extern void _sigtramp();
	extern int errno;

	if (sig <= 0 || sig >= NSIG || sig == SIGKILL || sig == SIGSTOP) {
		errno = EINVAL;
		return (-1);
	}
	prevsig = sigcatch[sig];
	if (nsv) {
		sigcatch[sig] = nsv->sv_handler;
		vec = *nsv;  nsv = &vec;
		if (nsv->sv_handler != (void (*)())SIG_DFL && nsv->sv_handler != (void (*)())SIG_IGN)
			nsv->sv_handler = _sigtramp;
	}
	if (syscall (SYS_sigvec, sig, nsv, osv) < 0) {
		sigcatch[sig] = prevsig;
		return (-1);
	}
	if (osv)
		osv->sv_handler = prevsig;
	return (0);
}

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)system.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#include	<signal.h>

system(s)
char *s;
{
	int status, pid, w;
	register void (*istat)(), (*qstat)();

	if ((pid = vfork()) == 0) {
		execl("/bin/sh", "sh", "-c", s, 0);
		_exit(127);
	}
	istat = signal(SIGINT, SIG_IGN);
	qstat = signal(SIGQUIT, SIG_IGN);
	while ((w = wait(&status)) != pid && w != -1)
		;
	if (w == -1)
		status = -1;
	signal(SIGINT, istat);
	signal(SIGQUIT, qstat);
	return(status);
}

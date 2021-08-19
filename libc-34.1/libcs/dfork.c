#include <c.h>
/*
 * This routine does a double fork so that the (grand)child won't
 * hang on exit because the (grand)parent never wait(2)ed for it.
 * Note: grandchild will be inherited by init (proc 1) on exit.
 **********************************************************************
 * HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX.  Inserted READ and WRITE macros here
 *	rather than using <modes.h>, which is obsolete.
 *
 * 13-Mar-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Made code more careful about closing pipes in cases of system
 *	call failures.
 *
 */

#define READ 0
#define WRITE 1

dfork() {
    int pid;
    int fildes[2];

    if (pipe(fildes) == CERROR)
	return(CERROR);
    if ((pid = fork()) == CERROR) {
	close(fildes[READ]);
	close(fildes[WRITE]);
	return(CERROR);
    }
    if (pid) {
	close(fildes[WRITE]);
	while (wait(0) != pid);
	if (read(fildes[READ], &pid, sizeof(int)) != sizeof(int))
	    pid = CERROR;
	close(fildes[READ]);
	return(pid);
    } else {
	close(fildes[READ]);
	if ((pid = fork()) == CERROR)
	    exit(CERROR);
	if (pid) {
	    write(fildes[WRITE], &pid, sizeof(int));
	    close(fildes[WRITE]);
	    exit(0);
	}
	close(fildes[WRITE]);
	return(0);
    }
}

/*
 *  CMU-lib:	fgetpass - get password from stream
 *
 **********************************************************************
 * HISTORY
 * 22-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created CMU CS 4.2 version.  Increased password size to 255.
 *
 * 06-Oct-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created from getpass.c.
 *
 **********************************************************************
 *
 *  Call:	#include <stdio.h>
 *
 *		char *getpass(prompt, fi)
 *		char *prompt;
 *		FILE *fi;
 *
 *  Function:	If the file "fi" is a terminal, then echoing is disabled
 *		and the "prompt" string is displayed.  A password is
 *		read from "fi" (maximum of 255 characters) and echoing
 *		reenabled if it was previously disabled.
 *
 *		While echoing is disabled on the terminal, interrupt and
 *		quit signals are temporarily caught to permit the
 *		restoration of the terminal state before such signals
 *		are processed.
 *
 *		(Note: unlike getpass(), the terminal input buffer is never
 *		flushed during this process)
 *
 *  Returns:	A pointer to a static (255 character) buffer containing
 *		the supplied password.
 */

#include <sgtty.h>
#include <stdio.h>
#include <signal.h>

static	FILE *savefi;
static	int flags;
static	struct sigvec intvec, quitvec;
static	struct sgttyb ttyb;
static catch();

char *
fgetpass(prompt, fi)
char *prompt;
FILE *fi;
{
	register char *p;
	register c;
	int istty;
	static char pbuf[256];
	static struct sigvec nintvec, nquitvec;
	int catch();

	savefi = fi;
	if (istty=isatty(fileno(fi)))
	{
	    nintvec.sv_handler = SIG_IGN; /* ignore INT signals */
	    nintvec.sv_mask = 0;
	    nintvec.sv_onstack = 0;
	    nquitvec.sv_handler = SIG_IGN; /* ignore QUIT signals */
	    nquitvec.sv_mask = 0;
	    nquitvec.sv_onstack = 0;
	    sigvec(SIGINT, &nintvec, &intvec);
	    sigvec(SIGQUIT, &nquitvec, &quitvec);
	    if (intvec.sv_handler != SIG_IGN) {
		nintvec.sv_handler = catch;
		sigvec(SIGINT, &nintvec, 0);
	    }
	    if (quitvec.sv_handler != SIG_IGN) {
		nquitvec.sv_handler = catch;
		sigvec(SIGQUIT, &nquitvec, 0);
	    }
	    ioctl(fileno(fi), TIOCGETP, &ttyb);
	    flags = ttyb.sg_flags;
	    ttyb.sg_flags &= ~ECHO;
	    ioctl(fileno(fi), TIOCSETN, &ttyb);	/* no flush needed */
	    fprintf(stderr, prompt);
	}
	for (p=pbuf; (c = getc(fi))!='\n' && c!=EOF;) {
		if (p < &pbuf[255])
			*p++ = c;
	}
	*p = '\0';
	if (istty)
	{
	     fprintf(stderr, "\n");
	     catch(0);
	}
	return(pbuf);

}

static
catch(sign)
{
	ttyb.sg_flags = flags;
	ioctl(fileno(savefi), TIOCSETN, &ttyb);
	sigvec(SIGINT, &intvec, 0);
	sigvec(SIGQUIT, &quitvec, 0);
	if (sign != 0) kill(getpid(), sign);
}

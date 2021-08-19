/*
 * Alert handling
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * This is very NeXT specific. It only handles a single case: that of 
 * not being able to find the parent NetInfo server, allowing one to
 * abort the search. It is not required to have this facility.
 */
#include <stdio.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/errno.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <nextdev/kmreg.h>

extern int open(const char *, int, int);
extern int close(int);
extern int fcntl(int, int, int);
extern int ioctl(int, int, void *);
extern int write(int, const char *, int);
extern int read(int, char *, int);

static const char MESSAGE[] = 
	"\nThe following network error has occurred:\n\n"
	"\tCannot find parent NetInfo server, still looking...\n"
	"\tPress 'c' to continue boot without parent server.\n\n"
	"See your system administrator if you need help.\n";

static const char CONSOLE[] = "/dev/console";

static int aborted = 0;
static int console;
static int console_flags;
static struct sgttyb console_sg;
static enum { 
	ALERT_FRESH,
	ALERT_OPENED,
	ALERT_PRINTED, 
	ALERT_CLOSED
} alert_state = ALERT_CLOSED;

/*
 * Enable (or disable) alerts
 */
void
alert_enable(int enable)
{
	if (enable) {
		alert_state = ALERT_FRESH;
	} else {
		alert_state = ALERT_CLOSED;
	}
}

/*
 * Did the user abort the alert?
 */
int
alert_aborted(void)
{
	return (aborted);
}

/*
 * Close an alert
 */
void
alert_close(void)
{
#ifndef COMPAT
	if (alert_state != ALERT_PRINTED) {
		return;
	}
	alert_state = ALERT_CLOSED;

	/* Restore fd flags */
	(void)fcntl(console, F_SETFL, console_flags);

	/* Restore terminal parameters */
	(void)ioctl(console, TIOCSETP, &console_sg);

	/* Remove the window */
	(void)ioctl(console, KMIOCRESTORE, 0);
	(void)close(console);
#endif
}

/*
 * We got a SIGIO. Handle it.
 */
static void
handle_io(int ignored)
{
	char buf[512];
	char *cp;
	int nchars;

	while ((nchars = read(console, (char *)&buf, sizeof(buf))) > 0) {
		cp = buf;
		while (nchars--) {
			if (*cp == 'c' || *cp == 'C') {
				aborted = 1;
				alert_close();
				return;
			}
			cp++;
		}
	}
	if (errno != EWOULDBLOCK) {
		alert_close();
	}
}

/*
 * Open an alert
 */		
void
alert_open(void)
{
#ifndef COMPAT
	struct sgttyb	sg;

	if (alert_state != ALERT_FRESH) {
		return;
	}
	alert_state = ALERT_OPENED;

	/* Open up the console */
	if ((console = open(CONSOLE, (O_RDWR|O_ALERT), 0)) < 0) {
		return;
	}

	/* Flush any existing input */
	(void)ioctl(console, TIOCFLUSH, (void *)FREAD);

	/* Set it up to interrupt on input */
	if ((console_flags = fcntl(console, F_GETFL, 0)) < 0) {
		(void)close(console);
		return;
	}
	if (fcntl(console, F_SETFL, (console_flags|FASYNC|FNDELAY)) < 0) {
		(void)close(console);
		return;
	}
	(void)signal(SIGIO, handle_io);

	/* Put it in CBREAK mode */
	if (ioctl(console, TIOCGETP, &sg) < 0) {
		(void)close(console);
		return;
	}
	console_sg = sg;
	sg.sg_flags |= CBREAK;
	sg.sg_flags &= ~ECHO;
	if (ioctl(console, TIOCSETP, &sg) < 0) {
		(void)close(console);
		return;
	}
	(void)write(console, MESSAGE, sizeof(MESSAGE) - 1);
	alert_state = ALERT_PRINTED;
#endif
}


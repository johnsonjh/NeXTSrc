/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)siglist.c	1.2 88/04/11 4.0NFSSRC; from	5.2 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

ss(	"Signal 0",			0,  0)
ss(	"Hangup",			1,  SIGHUP)
ss(	"Interrupt",			2,  SIGINT)
ss(	"Quit",				3,  SIGQUIT)
ss(	"Illegal instruction",		4,  SIGILL)
ss(	"Trace/BPT trap",		5,  SIGTRAP)
ss(	"IOT trap",			6,  SIGIOT)
ss(	"EMT trap",			7,  SIGEMT)
ss(	"Floating point exception",	8,  SIGFPE)
ss(	"Killed",			9,  SIGKILL)
ss(	"Bus error",			10, SIGBUS)
ss(	"Segmentation fault",		11, SIGSEGV)
ss(	"Bad system call",		12, SIGSYS)
ss(	"Broken pipe",			13, SIGPIPE)
ss(	"Alarm clock",			14, SIGALRM)
ss(	"Terminated",			15, SIGTERM)
ss(	"Urgent I/O condition",		16, SIGURG)
ss(	"Stopped (signal)",		17, SIGSTOP)
ss(	"Stopped",			18, SIGTSTP)
ss(	"Continued",			19, SIGCONT)
ss(	"Child exited",			20, SIGCHLD)
ss(	"Stopped (tty input)",		21, SIGTTIN)
ss(	"Stopped (tty output)",		22, SIGTTOU)
ss(	"I/O possible",			23, SIGIO)
ss(	"Cputime limit exceeded",	24, SIGXCPU)
ss(	"Filesize limit exceeded",	25, SIGXFSZ)
ss(	"Virtual timer expired",	26, SIGVTALRM)
ss(	"Profiling timer expired",	27, SIGPROF)
ss(	"Window size changed",		28, SIGWINCH)
ss(	"Resource lost",		29, SIGLOST)
ss(	"User defined signal 1",	30, SIGUSR1)
ss(	"User defined signal 2",	31, SIGUSR2)

/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nmserver.c
 *
 * $Source: /private/Net/p1/BSD/mmeyer/CMDS/cmds-13/usr.etc/nmserver/server/RCS/nmserver.c,v $
 *
 */

#ifndef	lint
char nmserver_rcsid[] = "$Header: nmserver.c,v 1.2 89/06/07 15:10:53 mmeyer Locked $";
#endif not lint

/*
 * Test program for the network server - implements full IPC service.
 */

/*
 * HISTORY:
 * 21-Jul-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	NOTIFY: allocate the notify port for the compatibility netmsgserver.
 *
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added printout of a timestamp on startup.
 *
 *  5-Apr-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added code to exec the old netmsgserver if the kernel is not good
 *	enough to support the new netmsgserver. (COMPAT)
 *
 * 29-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified logstat initialization so that tracing can be
 *	turned on very early.
 *
 * 14-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added code to read a configuration file.
 *
 * 27-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added "-C" switch to control compatibility mode. (COMPAT)
 *
 * 21-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added call to cthread_exit() at the end of main.
 *
 * 20-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Allow print_level to be set from command line.
 *
 * 16-Apr-87  Daniel Julin (dpj) at Carnegie Mellon University
 *	Fixed to use standard logstat technology for messages.
 *
 *  2-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#include <libc.h>
#include <cthreads.h>
#include <stdio.h>
#include <mach.h>
#include <sys/message.h>
#include <sys/signal.h>

#undef	NET_DEBUG
#define	NET_DEBUG	1
#undef	NET_PRINT
#define	NET_PRINT	1

#include "debug.h"
#include "ls_defs.h"
#include "netmsg.h"
#include "network.h"
#include "nm_init.h"

int		cthread_debug;

#if	COMPAT
#define USAGE	"Usage: nmserver/nmthreads [-f config_file] [-c] [-t] [-p #] [-C]"
#else	COMPAT
#define USAGE	"Usage: nmserver/nmthreads [-f config_file] [-c] [-t] [-p #]"
#endif	COMPAT

#if	COMPAT
extern char	compat_server[];
extern int	errno;
extern int	exit_handler();
#endif	COMPAT


/*
 * main
 *
 */
main(argc, argv)
int	argc;
char	**argv;
{
    int		i;
    int		print_level = -1;
#if	COMPAT
    int		compat_mode = -1;
#endif	COMPAT
    char	*config_file = NULL;
    long	clock;

#if	NeXT
#if	PROF
    signal(SIGUSR2, exit);
#endif	PROF
    signal(SIGHUP, SIG_IGN);
#ifdef notdef
    {
	    /*
	     * Make sure that we can drop a core file in case of error
	     */
	     struct rlimit rlim;

	    rlim.rlim_cur = RLIM_INFINITY;
	    rlim.rlim_max = RLIM_INFINITY;
	    setrlimit(RLIMIT_CORE, &rlim);
    }
#endif
#else	NeXT
    /*
     * Initialise the cthreads package.
     */
    cthread_init();
#endif	NeXT

    /*
     * Initialise the stuff needed for debugging.
     */
    if (!(ls_init_1())) {
	panic("ls_init_1 failed.");
    }

    for (i = 1; i < argc; i++) {
	if ((strcmp(argv[i], "-f") == 0) && ((i + 1) < argc)) {
	    i ++;
	    config_file = argv[i];
	}
	else if (strcmp(argv[i], "-t") == 0) tracing_on = 1;
	else if (strcmp(argv[i], "-c") == 0) cthread_debug = 1;
	else if ((strcmp(argv[i], "-p") == 0) && ((i + 1) < argc)) {
	    i ++;
	    print_level = atoi(argv[i]);
	}
#if	COMPAT
	else if (strcmp(argv[i], "-C") == 0) compat_mode = 1;
#endif	COMPAT
	else {
	    fprintf(stderr, "%s\n", USAGE);
	    (void)fflush(stderr);
	    _exit(-1);
	}
    }

    if (ls_read_config_file(config_file) != TRUE) {
	fprintf(stderr,"%s: Invalid configuration file - Exiting\n",argv[0]);
	(void)fflush(stderr);
	_exit(-1);
    }
    if (print_level != -1)
	debug.print_level = print_level;
#if	COMPAT
    if (compat_mode != -1)
	param.compat = compat_mode;
#endif	COMPAT

    /*
     * Initialize syslog if appropriate.
     */
    if (param.syslog)
	openlog("netmsgserver", LOG_PID | LOG_NOWAIT, LOG_USER);

    /*
     * Put out a timestamp when starting
     */
    clock = time(0);
    fprintf(stdout, "Started %s at %24.24s \n", argv[0], ctime(&clock));
    if (param.syslog)
	syslog(LOG_INFO, "Started %s at %24.24s ", argv[0], ctime(&clock));

    fprintf(stdout, "%s: %s %s print_level = %d.\n", argv[0],
			(tracing_on ? "tracing" : ""),
			(cthread_debug ? "cthread_debug" : ""),
			debug.print_level);
    (void)fflush(stdout);

#if	COMPAT
    /*
     * This ugly code to allow the network server to start up on
     * machines that do not have an up-to-date Mach kernel.
     */
    {
	struct thread_basic_info	ti;
	int				count;
	kern_return_t			kr;

	count = sizeof(struct thread_basic_info);
	kr = thread_info(thread_self(), THREAD_BASIC_INFO, &ti, &count);
	if (kr != KERN_SUCCESS) {
		ERROR((msg,"*** This machine does not run a good Mach kernel ***"));
		ERROR((msg,"*** Starting the old netmsgserver standalone ***"));
		execl(compat_server,"old_netmsgserver",0);
		ERROR((msg,"execl of old netmsgserver failed, errno=%d", errno));
		_exit(2);
	}
    }

    /*
     * Fork a process for the old netmsgserver in compatibility mode.
     * We fork here before we are multi-threaded, but the child will wait
     * until we are ready by looking at a flag.
     */
    if (param.compat) {
	extern int	compat_pid;

	/*
	 * First establish signal handlers to kill the child
	 * if we have to exit.
	 */
#define	HANDLER(sig)	{							\
	if (signal(sig,exit_handler) == (int (*)()) -1) {			\
		ERROR((msg,"netname_init.signal(sig) failed, errno=%d"));	\
		panic("netname_init.signal");					\
	}									\
}
	HANDLER(SIGHUP);
	HANDLER(SIGINT);
	HANDLER(SIGQUIT);
	HANDLER(SIGILL);
	HANDLER(SIGIOT);
	HANDLER(SIGEMT);
	HANDLER(SIGFPE);
	HANDLER(SIGBUS);
	HANDLER(SIGSEGV);
	HANDLER(SIGSYS);
	HANDLER(SIGTERM);
	HANDLER(SIGURG);
	HANDLER(SIGIO);
	HANDLER(SIGCHLD);
#undef	HANDLER

	/*
	 * Fork the old server.
	 */
	compat_pid = fork();
	if (compat_pid < 0) {
		ERROR((msg,"netname_init.fork: cannot start an old network server (COMPAT), errno=%d",errno));
		panic("cannot fork");
	}
	if (compat_pid == 0) {
		msg_header_t	sleep_msg;
		kern_return_t	kr;
		int		fd;
		port_t		notify_port;

		/*
		 * Child. Close all non-essential file descriptors,
		 * wait for the go signal,
		 * then exec the old network server.
		 */
		for (fd = getdtablesize(); fd > 2; fd--)
			close(fd);

#if	NOTIFY
		kr = port_allocate(task_self(),&notify_port);
		if (kr != KERN_SUCCESS) {
			ERROR((msg,"[COMPAT,NOTIFY] port_allocate(notify_port) failed, kr=%d",kr));
			_exit(0);
		}
		kr = task_set_special_port(task_self(),TASK_NOTIFY_PORT,notify_port);
		if (kr != KERN_SUCCESS) {
			ERROR((msg,"[COMPAT,NOTIFY] task_set_special_port(notify_port) failed, kr=%d",kr));
			_exit(0);
		}
		(void)port_disable(task_self(),notify_port);
#else	NOTIFY
		notify_port = task_notify();
#endif	NOTIFY

		sleep_msg.msg_size = sizeof(msg_header_t);
		sleep_msg.msg_local_port = notify_port;
		kr = msg_receive(&sleep_msg, RCV_TIMEOUT, 60000);
		if (kr == KERN_SUCCESS) {
			execl(compat_server,"old_netmsgserver",0);
			ERROR((msg,"execl of old netmsgserver failed, errno=%d", errno));
			_exit(0);
		} else {
			ERROR((msg,"old netmsgserver timed-out while waiting for the GO signal, kr=%d\n",kr));
			_exit(0);
		}
	} else {
		/*
		 * Parent.
		 */
		ERROR((msg,"Started old netmsgserver process for compatibility mode, pid=%d",compat_pid));
	}
    }
#endif	COMPAT

    if (nm_init()) {
#if	NeXT
	char buf[BUFSIZ];
	strcpy (buf, "Network Server Initialised.");	
    	DEBUG0(TRUE, 3, buf);
#else	NeXT
	ERROR((msg,"Network Server initialised."));
#endif	NeXT
	DEBUG0(TRUE, 3, 1260);
	DEBUG_NETADDR(TRUE, 3, my_host_id);
    }
    else {
	panic("Network Server initialisation failed.");
    }

#if	PROF
    {
	/*
	 * Wait for ever so that we do not drop off the end of main.
	 */
	port_t		prof_port;
	msg_header_t	prof_msg;
	(void)port_allocate(task_self(), &prof_port);
	_netname_check_in(PORT_NULL, "PROF_EXIT", task_self(), prof_port);
	prof_msg.msg_size = sizeof(prof_msg);
	prof_msg.msg_local_port = prof_port;
	msg_receive(&prof_msg, MSG_OPTION_NONE, 0);
	fprintf(stderr, "Received profile exit message - exitting.\n");
	_exit(1);
    }
#endif	PROF

    cthread_exit(0);

}


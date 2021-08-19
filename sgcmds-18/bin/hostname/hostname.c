/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1983 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)hostname.c	5.1 (Berkeley) 4/30/85";
#endif not lint

/*
 * hostname -- get (or set hostname)
 */
#include <stdio.h>

#if	NeXT
#include <netdb.h>
#include <signal.h>
#include <sys/boolean.h>
#include <sys/types.h>
#include <sys/file.h>
#include <sys/fcntl.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <nextdev/kmreg.h>
#include <net/if.h>
#include <rpc/rpc.h>
#include <rpcsvc/bootparam.h>

#define	CONSOLE	"/dev/console"

#endif	NeXT

char hostname[32];
extern int errno;

#if	NeXT
int		console;
int		console_flags;
struct sgttyb	console_sg;
boolean_t	alert_printed = FALSE;

alert_done()
{
	if (!alert_printed)
	    return;

	alert_printed = FALSE;

	/* Restore fd flags */
	(void) fcntl(console, F_SETFL, console_flags);

	/* Restore terminal parameters */
	(void) ioctl(console, TIOCSETP, &console_sg);

	/* Remove the window */
	(void) ioctl(console, KMIOCRESTORE, 0);
	(void) close(console);
}

handle_io()
{
	char		buf[512];
	char		*cp;
	int		nchars;

	while( (nchars = read(console, &buf, sizeof (buf))) > 0 ) {
		cp = buf;
		while (nchars--) {
			if (*cp == 'c' || *cp == 'C') {
				alert_done();
				fprintf(stderr, "hostname: aborted.\n");
				errno = ETIMEDOUT;
				exit(errno);
			}
			cp++;
		}
	}
	if ( errno != EWOULDBLOCK ) {
		alert_done();
		perror("hostname");
		exit(errno);
	}
	return(0);
}
		
print_alert()
{
	char		buf[512];
	struct sgttyb	sg;

	/* Open up the console */
	if ( (console = open(CONSOLE, (O_RDWR|O_ALERT), 0)) < 0 ) {
		perror("hostname");
		return (-1);
	}

	/* Flush any existing input */
	ioctl(console, TIOCFLUSH, FREAD);

	/* Set it up to interrupt on input */
	if ( (console_flags = fcntl(console, F_GETFL, 0)) == -1 ) {
		perror("hostname");
		return (-1);
	}
	if ( fcntl(console, F_SETFL, (console_flags|FASYNC|FNDELAY)) == -1 ) {
		perror("hostname");
		return (-1);
	}
	signal(SIGIO, handle_io);

	/* Put it in CBREAK mode */
	if ( ioctl(console, TIOCGETP, &sg) == -1 ) {
		perror("hostname");
		return (-1);
	}
	console_sg = sg;
	sg.sg_flags |= CBREAK;
	sg.sg_flags &= ~ECHO;
	if ( ioctl(console, TIOCSETP, &sg) == -1 ) {
		perror("hostname");
		return (-1);
	}
	sprintf(buf, "\nConfiguration server not responding to %s%s",
		"request for hostname.\n",
		"Waiting.  Press 'c' to enter single user mode.\n");
	write(console, buf, strlen(buf));
	alert_printed = TRUE;
	return(0);
}

bool_t
each_whoresult(result, from)
	bp_whoami_res		*result;
	struct sockaddr_in	*from;
{
	extern char		*inet_ntoa();
	struct hostent		*host;
	char			*hname;

	if (result) {
		signal(SIGIO, SIG_DFL);
		if (sethostname(result->client_name,
				strlen(result->client_name))) {
			perror("hostname: each_whoresult");
			exit(errno);
		}
		/*
		 * Try to get the BOOTPARAMS server's name
		 */
		host = gethostbyaddr(&from->sin_addr,
				     sizeof (from->sin_addr), AF_INET);
		if (host) {
			hname = host->h_name;
		} else {
			hname = inet_ntoa(from->sin_addr);
		}
		/*
		 * Get rid of the window now so that the next printf
		 * goes into the console log.
		 */
		alert_done();
		printf("%s returned new hostname: %s\n",
		       hname, result->client_name);
		return(TRUE);
	}
#if	NeXT
	printf("hostname: each_whoresult: null results pointer\n");
#else	NeXT
	fprintf("hostname: each_whoresult: null results pointer\n");
#endif	NeXT
	return(FALSE);
}

#define	MAXADDRS	5	/* We only want INET on the first interface */
/*
 * Routine:	sethostname_fromnet
 * Function:
 *	Do a BOOTPARAMS WHOAMI RPC to find out our hostname
 *
 */
int
sethostname_fromnet()
{
	struct bp_whoami_arg	who_arg;
	struct bp_whoami_res	who_res;
	enum clnt_stat		stat;
	extern enum clnt_stat	clnt_broadcast();
	struct sockaddr_in	*sin;
	struct ifreq		reqbuf[MAXADDRS];
	struct ifreq		*ifrp;
	struct ifconf		ifc;
	int			len;
	int			so;
	int			error;

	/*
	 * Find our primary internet address
	 */
	so = socket(PF_INET, SOCK_DGRAM, 0);
	if (so < 0) {
		perror("hostname: socket");
		return(errno);
	}

	ifc.ifc_len = (sizeof (reqbuf));
	ifc.ifc_req = reqbuf;
	if (ioctl(so, SIOCGIFCONF, &ifc)) {
		error = errno;
		perror("hostname: SIOCGIFCONF");
		close(so);
		return(error);
	}
	close(so);

	ifrp = reqbuf;
	for (len = ifc.ifc_len; len > 0;
	     len -= sizeof (*ifrp), ifrp++) {
		if (ifrp->ifr_addr.sa_family == PF_INET)
			break;
	}
	if (len == 0) {
		fprintf(stderr, "hostname: No primary internet address\n");
		return(-1);
	}

	sin = (struct sockaddr_in *) &ifrp->ifr_addr;
	who_arg.client_address.bp_address.ip_addr.net =
		sin->sin_addr.s_net;
	who_arg.client_address.bp_address.ip_addr.host =
		sin->sin_addr.s_host;
	who_arg.client_address.bp_address.ip_addr.lh =
		sin->sin_addr.s_lh;
	who_arg.client_address.bp_address.ip_addr.impno =
		sin->sin_addr.s_impno;
	who_arg.client_address.address_type = IP_ADDR_TYPE;
	bzero(&who_res, sizeof (who_res));

	while (TRUE) {
		/*
		 * Broadcast the whoami.
		 */
		stat = clnt_broadcast(BOOTPARAMPROG, BOOTPARAMVERS,
				      BOOTPARAMPROC_WHOAMI, xdr_bp_whoami_arg,
				      &who_arg, xdr_bp_whoami_res, &who_res,
				      each_whoresult);

		if (stat == RPC_SUCCESS) {
			return (0);
		} else if (stat != RPC_TIMEDOUT) {
			break;
		}
		/*
		 * We timed out.  Warn the user that we are waiting and
		 * continue looking for the hostname.
		 */
		if (!alert_printed) {
			if (error = print_alert())
				return (error);
		}
	}
	alert_done();
	fprintf(stderr, "hostname: ");
	clnt_perrno(stat);
	return (-1);
}
#endif	NeXT


main(argc,argv)
	char *argv[];
{
	int	myerrno;

	argc--;
	argv++;
	if (argc) {
#if	NeXT
		myerrno = 0;
		if (strcmp(*argv, "-AUTOMATIC-") == 0) {
			myerrno = sethostname_fromnet();
		} else if (sethostname(*argv,strlen(*argv))) {
			myerrno = errno;
			perror("sethostname");
		}
#else	NeXT
		if (sethostname(*argv,strlen(*argv)))
			perror("sethostname");
		myerrno = errno;
#endif	NeXT
	} else {
		gethostname(hostname,sizeof(hostname));
		myerrno = errno;
		printf("%s\n",hostname);
	}
	exit(myerrno);
}

/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/


# include <errno.h>
# include "sendmail.h"

#ifndef lint
#ifdef DAEMON
static char sccsid[] = "@(#)daemon.c	5.30 (Berkeley) 1/19/89 (with daemon mode)";
#else
static char sccsid[] = "@(#)daemon.c	5.30 (Berkeley) 1/19/89 (without daemon mode)";
#endif
#endif /* not lint */
# ifdef DAEMON

# include <netdb.h>		/* FIXME: Do I want this to be "netdb.h"? */
#if	NeXT_MOD
#include <sys/wait.h>
#include <sys/param.h>
#else	NeXT_MOD
# include <sys/signal.h>
# include <sys/wait.h>
# include <sys/time.h>
# include <sys/resource.h>
#endif	NeXT_MOD

/*
**  DAEMON.C -- routines to use when running as a daemon.
**
**	This entire file is highly dependent on the 4.2 BSD
**	interprocess communication primitives.  No attempt has
**	been made to make this file portable to Version 7,
**	Version 6, MPX files, etc.  If you should try such a
**	thing yourself, I recommend chucking the entire file
**	and starting from scratch.  Basic semantics are:
**
**	getrequests()
**		Opens a port and initiates a connection.
**		Returns in a child.  Must set InChannel and
**		OutChannel appropriately.
**	clrdaemon()
**		Close any open files associated with getting
**		the connection; this is used when running the queue,
**		etc., to avoid having extra file descriptors during
**		the queue run and to avoid confusing the network
**		code (if it cares).
**	makeconnection(host, port, outfile, infile)
**		Make a connection to the named host on the given
**		port.  Set *outfile and *infile to the files
**		appropriate for communication.  Returns zero on
**		success, else an exit status describing the
**		error.
**	maphostname(hbuf, hbufsize)
**		Convert the entry in hbuf into a canonical form.  It
**		may not be larger than hbufsize.
*/
#if	NeXT_MOD
static struct hostinfo *host_from_fd[NOFILE];	/* Host entry for each */


static setuphost(sp, hp)
    STAB *sp;
    struct hostent *hp;
  {
# if BSD >= 43
    int i;
    for (i= 0; i < MAXMXHOSTS && *hp->h_addr_list; i++)
	sp->s_value.sv_host.h_addrlist[ i ] = 
		*(struct in_addr *)(*hp->h_addr_list++);
    sp->s_value.sv_host.h_addrlist[ i ].s_addr = INADDR_ANY;
# else 
    sp->s_value.sv_host.h_addrlist[ 0 ] = *(struct in_addr *)(hp->h_addr);
    sp->s_value.sv_host.h_addrlist[ 1 ].s_addr = INADDR_ANY;
# endif
  }

static jmp_buf NameTimeout;
time_t nametime = 90;		/* seconds to wait for name server */

nametimeout()  {longjmp(NameTimeout, 1);}

/*
**  LOOKUPHOST -- determine host address and other remembered info.
**
**	Parameters:
**		host -- a char * providing a host name or [a.b.c.d].
**
**	Returns:
**		The address of a struct hostinfo for this host.
**		The caller may modify h_fd, h_down, and h_open in
**		this structure.  h_addr and h_exists should not be
**		modified.
**
**	Side Effects:
**		If the hostname has previously been looked up, the
**		existing symbol table entry is returned immediately.
**		Otherwise, a new symbol table entry is created, 
**		a system-dependent hostname->address lookup is done,
**		and the new entry is returned.
**
**		Note that for information to be remembered in the
**		symbol table, the process must do several lookups
**		without forking and exiting.
*/
struct hostinfo *
lookuphost(host)
	char *host;			/* Host name */
{
	register struct hostent *hp = (struct hostent *)NULL;
	extern char *inet_ntoa();
	STAB	*sp;			/* Symbol table entry */
	EVENT *ev;

	sp = stab(host, ST_HOST, ST_ENTER);
	if (sp->s_value.sv_host.h_valid) {
		AlreadyKnown = sp->s_value.sv_host.h_down;
		return &sp->s_value.sv_host;
	}

	/*
	**  Create a new symbol table entry.  Initially it is cleared,
	**  thus h_exists is 0.  Ditto for the other flags.
	**  Then look up the address for the host.
	**	Accept "[a.b.c.d]" syntax for host name.
	*/
	sp->s_value.sv_host.h_valid = 1;
	if (host[0] == '[')
	{
		long hid;
		register char *p = index(host, ']');

		if (p != NULL)
		{
			*p = '\0';
			hid = inet_addr(&host[1]);
			*p = ']';
		}
		if (p == NULL || hid == -1)
		{
			usrerr("Invalid numeric domain spec \"%s\"", host);
		}
		else
		{
			sp->s_value.sv_host.h_addrlist[0].s_addr = hid;
			sp->s_value.sv_host.h_index = 0;
			sp->s_value.sv_host.h_exists = 1;
		}
		return &sp->s_value.sv_host;		/* Return [] entry */
	}

	if (setjmp(NameTimeout) != 0) {
		sp->s_value.sv_host.h_exists = 1;
		sp->s_value.sv_host.h_down = 1;
		sp->s_value.sv_host.h_errno = ENAMESER;
		return &sp->s_value.sv_host;
	}
	ev = setevent(nametime, nametimeout, 0);
	hp = gethostbyname(host);
	if (hp != NULL && hp->h_addrtype == AF_INET)
	{
		setuphost(sp, hp);
		sp->s_value.sv_host.h_index = 0;
		sp->s_value.sv_host.h_exists = 1;
	}
# ifdef MXDOMAIN
   /*
    * The gethostbyname failed -- in 4.2BSD we just return
    * a hard error.  4.3BSD gives us more infomation on the failure.
    */
	else if (h_errno == TRY_AGAIN) 
	{
		sp->s_value.sv_host.h_exists = 1;

		sp->s_value.sv_host.h_down = 1;
		sp->s_value.sv_host.h_errno = ENAMESER;
	}
# endif MXDOMAIN
	clrevent(ev);
	return &sp->s_value.sv_host;		/* Return new entry */
}
#endif	NeXT_MOD
/*
**  GETREQUESTS -- open mail IPC port and get requests.
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Waits until some interesting activity occurs.  When
**		it does, a child is created to process it, and the
**		parent waits for completion.  Return from this
**		routine is always in the child.  The file pointers
**		"InChannel" and "OutChannel" should be set to point
**		to the communication channel.
*/

struct sockaddr_in	SendmailAddress;/* internet address of sendmail */

int	DaemonSocket	= -1;		/* fd describing socket */
char	*NetName;			/* name of home (local?) network */

getrequests()
{
	int t;
	register struct servent *sp;
	int on = 1;
	extern reapchild();

	/*
	**  Set up the address for the mailer.
	*/

	sp = getservbyname("smtp", "tcp");
	if (sp == NULL)
	{
		syserr("server \"smtp\" unknown");
		goto severe;
	}
	SendmailAddress.sin_family = AF_INET;
	SendmailAddress.sin_addr.s_addr = INADDR_ANY;
	SendmailAddress.sin_port = sp->s_port;

	/*
	**  Try to actually open the connection.
	*/

	if (tTd(15, 1))
		printf("getrequests: port 0x%x\n", SendmailAddress.sin_port);

	/* get a socket for the SMTP connection */
	DaemonSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (DaemonSocket < 0)
	{
		/* probably another daemon already */
		syserr("getrequests: can't create socket");
	  severe:
# ifdef LOG
#if	NeXT_MOD
# ifdef LOG_SALERT
		if (LogLevel > 0)
			syslog(LOG_SALERT, "cannot get connection");
# else LOG_SALERT
		if (LogLevel > 0)
			syslog(LOG_CRIT|LOG_MAIL, "cannot get connection");
# endif LOG_SALERT
#else	NeXT_MOD
		if (LogLevel > 0)
			syslog(LOG_ALERT, "cannot get connection");
#endif	NeXT_MOD
# endif LOG
		finis();
	}

	/* turn on network debugging? */
	if (tTd(15, 15))
		(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof on);

	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof on);
	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof on);

	if (bind(DaemonSocket, (struct sockaddr *) &SendmailAddress,
		 sizeof SendmailAddress) < 0)
	{
		syserr("getrequests: cannot bind");
		(void) close(DaemonSocket);
		goto severe;
	}
	if (listen(DaemonSocket, 10) < 0)
	{
		syserr("getrequests: cannot listen");
		(void) close(DaemonSocket);
		goto severe;
	}

	(void) signal(SIGCHLD, reapchild);

	if (tTd(15, 1))
		printf("getrequests: %d\n", DaemonSocket);

#if	NeXT_MOD
# ifdef LOG
	/* Tell the log that a new daemon has started. */
	syslog(LOG_INFO, "network daemon starting");
# endif LOG
#endif	NeXT_MOD
	for (;;)
	{
		register int pid;
		auto int lotherend;
		extern int RefuseLA;
		register int la;

		/* see if we are rejecting connections */
#if	NeXT_MOD
		while (RefuseLA > 0 && ((la = getla()) > RefuseLA)) {
#else	NeXT_MOD
		while ((la = getla()) > RefuseLA)  {
#endif	NeXT_MOD
			setproctitle("rejecting connections: load average: %.2f", la);
			sleep(5);
		}

		/* wait for a connection */
		setproctitle("accepting connections");
		do
		{
			errno = 0;
			lotherend = sizeof RealHostAddr;
			t = accept(DaemonSocket, (struct sockaddr *)&RealHostAddr,
			    &lotherend);
		} while (t < 0 && errno == EINTR);
		if (t < 0)
		{
			syserr("getrequests: accept");
			sleep(5);
			continue;
		}

		/*
		**  Create a subprocess to process the mail.
		*/

		if (tTd(15, 2))
			printf("getrequests: forking (fd = %d)\n", t);

		pid = fork();
		if (pid < 0)
		{
			syserr("daemon: cannot fork");
			sleep(10);
			(void) close(t);
			continue;
		}

		if (pid == 0)
		{
			extern struct hostent *gethostbyaddr();
			register struct hostent *hp;
			char buf[MAXNAME];
#if	NeXT_MOD
			EVENT *ev;
#endif	NeXT_MOD
			/*
			**  CHILD -- return to caller.
			**	Collect verified idea of sending host.
			**	Verify calling user id if possible here.
			*/

			(void) signal(SIGCHLD, SIG_DFL);

#if	NeXT_MOD
			/* determine host name with timeout */
			if (setjmp(NameTimeout) != 0) {
			    hp = NULL;
			}
			else {
			    ev = setevent(nametime, nametimeout, 0);
			    hp = gethostbyaddr(
		&otherend.sin_addr, sizeof otherend.sin_addr, AF_INET);
			    clrevent(ev);
			}
#else	NeXT_MOD
			/* determine host name */
			hp = gethostbyaddr((char *) &otherend.sin_addr, sizeof otherend.sin_addr, AF_INET);
#endif	NeXT_MOD
			if (hp != NULL)
			{
				(void) strcpy(buf, hp->h_name);
				if (NetName != NULL && NetName[0] != '\0' &&
				    index(hp->h_name, '.') == NULL)
				{
					(void) strcat(buf, ".");
					(void) strcat(buf, NetName);
				}
			}
			else
			{
				extern char *inet_ntoa();

				/* produce a dotted quad */
				(void) sprintf(buf, "[%s]",
					inet_ntoa(otherend.sin_addr));
			}

			/* should we check for illegal connection here? XXX */

			RealHostName = newstr(buf);

#if	NeXT_MOD
			setproctitle("From %s", (int)RealHostName);
			define('r', "TCP", CurEnv);
#endif	NeXT_MOD
			(void) close(DaemonSocket);
			InChannel = fdopen(t, "r");
#if	NeXT_MOD
			OutChannel = fdopen(t, "w");
#else	NeXT_MOD
			OutChannel = fdopen(dup(t), "w");
#endif	NeXT_MOD
# ifdef DEBUG
			if (tTd(15, 2))
				printf("getreq: returning\n");
# endif DEBUG
# ifdef LOG
			if (LogLevel > 11)
				syslog(LOG_DEBUG, "connected, pid=%d", getpid());
# endif LOG
			return;
		}

		/* close the port so that others will hang (for a while) */
		(void) close(t);
	}
	/*NOTREACHED*/
}
/*
**  CLRDAEMON -- reset the daemon connection
**
**	Parameters:
**		none.
**
**	Returns:
**		none.
**
**	Side Effects:
**		releases any resources used by the passive daemon.
*/

clrdaemon()
{
	if (DaemonSocket >= 0)
		(void) close(DaemonSocket);
	DaemonSocket = -1;
}
/*
**  MAKECONNECTION -- make a connection to an SMTP socket on another machine.
**
**	Parameters:
**		host -- the name of the host.
**		port -- the port number to connect to.
**		outfile -- a pointer to a place to put the outfile
**			descriptor.
**		infile -- ditto for infile.
**
**	Returns:
**		An exit code telling whether the connection could be
**			made and if not why not.
**
**	Side Effects:
**	  For NeXT_MOD:
**		On an error, global <errno> is set describing the error.
**		A previously existing connection will be reused.
**		Previously cached data will be used to find the host
**			and determine its status.  This cache is updated
**			with the host's current status.
**	  Otherwise:
**		None.
*/

#if	NeXT_MOD
#define	IPPORT_SMTP	25
#else	NeXT_MOD
int	h_errno;	/*this will go away when code implemented*/
#endif	NeXT_MOD

makeconnection(host, port, outfile, infile)
	char *host;
	u_short port;
	FILE **outfile;
	FILE **infile;
{
#if	NeXT_MOD
	register struct hostinfo *hp;
	extern char *inet_ntoa();
	int s;
	struct sockaddr_in	addr;


	/* Remember who we're talking to, for error messages */
	RealHostName = newstr(host);

	/*
	**  Determine the address of the host.
	**  If we have tried to connect before and failed, don't try,
	**  Unless this is the last attempt.
	*/
	setproctitle("%s To %s", CurEnv->e_id, RealHostName);
	AlreadyKnown = FALSE;
	hp = lookuphost(host);
	errno = 0;
	if (!hp->h_exists)
		return (EX_NOHOST);
	if (hp->h_down && (curtime() < CurEnv->e_ctime + TimeOut))
	{
		errno = hp->h_errno;	/* for a better message */
		return (EX_TEMPFAIL);
	}
	addr.sin_family = AF_INET;
	addr.sin_addr = hp->h_addrlist[hp->h_index];

	/*
	**  Determine the port number.
	*/
	if (port != 0)
		addr.sin_port = htons(port);
	else
		addr.sin_port = htons(IPPORT_SMTP);

	if (!hp->h_open || hp->h_port != port)
	{
		s = openhost(hp, addr);
		if (s != EX_OK) return s;
	}
	*outfile = fdopen(hp->h_fd, "w");
	*infile = fdopen(dup(hp->h_fd), "r");
	return (EX_OK);
}

int
openhost(hp, addr)
	struct hostinfo *hp;
	struct sockaddr_in	addr;
{
	register int s;
	int error_code;
	int on = 1;
#else	NeXT_MOD
	register int s;
	int sav_errno;

	/*
	**  Set up the address for the mailer.
	**	Accept "[a.b.c.d]" syntax for host name.
	*/

	h_errno = 0;
	errno = 0;

	if (host[0] == '[')
	{
		long hid;
		register char *p = index(host, ']');

		if (p != NULL)
		{
			*p = '\0';
			hid = inet_addr(&host[1]);
			*p = ']';
		}
		if (p == NULL || hid == -1)
		{
			usrerr("Invalid numeric domain spec \"%s\"", host);
			return (EX_NOHOST);
		}
		SendmailAddress.sin_addr.s_addr = hid;
	}
	else
	{
		register struct hostent *hp = gethostbyname(host);

		if (hp == NULL)
		{
			if (errno == ETIMEDOUT || h_errno == TRY_AGAIN)
				return (EX_TEMPFAIL);

			/*
			**  XXX Should look for mail forwarder record here
			**  XXX if (h_errno == NO_ADDRESS).
			*/

			return (EX_NOHOST);
		}
		bcopy(hp->h_addr, (char *) &SendmailAddress.sin_addr, hp->h_length);
	}

	/*
	**  Determine the port number.
	*/

	if (port != 0)
		SendmailAddress.sin_port = htons(port);
	else
	{
		register struct servent *sp = getservbyname("smtp", "tcp");

		if (sp == NULL)
		{
			syserr("makeconnection: server \"smtp\" unknown");
			return (EX_OSFILE);
		}
		SendmailAddress.sin_port = sp->s_port;
	}

#endif	NeXT_MOD
	/*
	**  Try to actually open the connection.
	*/

# ifdef DEBUG
	if (tTd(16, 1))
#if	NeXT_MOD
		printf("openhost (%x)\n", hp->h_addrlist[hp->h_index].s_addr);
#else	NeXT_MOD
		printf("makeconnection (%s)\n", host);
#endif	NeXT_MOD
# endif DEBUG

#if	NeXT_MOD
	if (hp->h_addrlist[hp->h_index].s_addr == INADDR_ANY)
		 /*
		  * restart the address list search if we hit the end,
		  * otherwise use the last one that worked.
		  */
		hp->h_index = 0;
	for ( ;*(int *)&(hp->h_addrlist[hp->h_index]);hp->h_index++) {

	addr.sin_addr = hp->h_addrlist[hp->h_index];
#endif	NeXT_MOD

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s < 0)
	{
#if	NeXT_MOD
		error_code = errno;	/* Save errno for <failure> */
		syserr("makeconnection: no socket");
		errno = error_code;	/* Save errno for <failure> */
#else	NeXT_MOD
		syserr("makeconnection: no socket");
		sav_errno = errno;
#endif	NeXT_MOD
		goto failure;
	}

# ifdef DEBUG
	if (tTd(16, 1))
		printf("makeconnection: %d\n", s);

	/* turn on network debugging? */
	if (tTd(16, 14))
	{
		int on = 1;
		(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_DEBUG, (char *)&on, sizeof on);
	}
# endif DEBUG
#if	NeXT_MOD
	if (Verbose) {
		printf("Trying %s... ", inet_ntoa(addr.sin_addr) );
		fflush(stdout);
	}

	(void) setsockopt(DaemonSocket, SOL_SOCKET, SO_KEEPALIVE,
				 &on, sizeof(on));

	if (CurEnv->e_xfp)
	    (void) fflush(CurEnv->e_xfp);

	errno = 0;					/* for debugging */
	hp->h_tried = 1;		/* We are trying to connect */
	if (connect(s, (struct sockaddr *)&addr, sizeof addr) < 0)
	{
		/* failure, decide if temporary or not */
	failure:
		error_code = errno;
		(void) close(s);	/* Free the socket */
		if (Verbose)
		    printf("%s\r\n", errstring(error_code) );
		errno = error_code;
		switch (errno)
		{
		  case ETIMEDOUT:
		  	errno = EHOSTDOWN;	/* for a better message */
		  case EISCONN:
#else	NeXT_MOD
	(void) fflush(CurEnv->e_xfp);			/* for debugging */
	errno = 0;					/* for debugging */
	SendmailAddress.sin_family = AF_INET;
	if (connect(s, &SendmailAddress, sizeof SendmailAddress) < 0)
	{
		sav_errno = errno;
		(void) close(s);
		/* failure, decide if temporary or not */
	failure:
		switch (sav_errno)
		{
		  case EISCONN:
		  case ETIMEDOUT:
#endif	NeXT_MOD
		  case EINPROGRESS:
		  case EALREADY:
		  case EADDRINUSE:
		  case EHOSTDOWN:
		  case ENETDOWN:
		  case ENETRESET:
		  case ENOBUFS:
		  case ECONNREFUSED:
		  case ECONNRESET:
		  case EHOSTUNREACH:
		  case ENETUNREACH:
#if	NeXT_MOD
			/* there are others, I'm sure..... */
			continue;
#else	NeXT_MOD
			/* there are others, I'm sure..... */
			return (EX_TEMPFAIL);

		  case EPERM:
			/* why is this happening? */
			syserr("makeconnection: funny failure, addr=%lx, port=%x",
				SendmailAddress.sin_addr.s_addr, SendmailAddress.sin_port);
			return (EX_TEMPFAIL);

#endif	NeXT_MOD
		  default:
			return (EX_UNAVAILABLE);
		}
	}

#if	NeXT_MOD
	if (Verbose)
	    printf(" connected.\r\n");
	/* connection ok, put it into canonical form */
	host_from_fd[s] = hp;	/* Create cross reference pointer */
	hp->h_port = addr.sin_port;
	hp->h_fd = s;		/* Save file descriptor */
	hp->h_open = 1;		/* And indicate that it's open */
  
  	return (EX_OK);
	}
	
	hp->h_down = 1;		/* Mark down host */
	hp->h_errno = errno;
	/*
	** Note that the down flag is never turned off.
	** We depend on sendmail's exiting to throw away
	** this information.  It should apply to one
	** queue run only.
	*/
	return (EX_TEMPFAIL);
#else	NeXT_MOD
	/* connection ok, put it into canonical form */
	*outfile = fdopen(s, "w");
	*infile = fdopen(s, "r");

	return (EX_OK);
#endif	NeXT_MOD
}

#if	NeXT_MOD
/*
**  CLOSECONNECTION -- mark an open connection as closed.
**
**	Parameters:
**		fd -- the file descriptor of the connection.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Marks the host (which had a connection opened via
**		makeconnection()) as not having a current connection.
**		Note that this does not actually close the file
**		descriptor.  That's the caller's responsibility.
*/
closeconnection(fd)
	int fd;
{
	register struct hostinfo *hp;

	if (fd < NOFILE) {
		hp = host_from_fd[fd];
		if (hp != NULL && hp->h_open) {
			hp->h_open = 0;
		}
		host_from_fd[fd] = NULL;
	}
}
#endif	NeXT_MOD
/*
**  MYHOSTNAME -- return the name of this host.
**
**	Parameters:
**		hostbuf -- a place to return the name of this host.
**		size -- the size of hostbuf.
**
**	Returns:
**		A list of aliases for this host.
**
**	Side Effects:
**		none.
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	extern struct hostent *gethostbyname();
	struct hostent *hp;

	if (gethostname(hostbuf, size) < 0)
	{
		(void) strcpy(hostbuf, "localhost");
	}
	hp = gethostbyname(hostbuf);
	if (hp != NULL)
	{
		(void) strncpy(hostbuf, hp->h_name, size-1);
		return (hp->h_aliases);
	}
	else
		return (NULL);
}
/*
**  MAPHOSTNAME -- turn a hostname into canonical form
**
**	Parameters:
**		hbuf -- a buffer containing a hostname.
**		hbsize -- the size of hbuf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Looks up the host specified in hbuf.  If it is not
**		the canonical name for that host, replace it with
**		the canonical name.  If the name is unknown, or it
**		is already the canonical name, leave it unchanged.
*/

maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	register struct hostent *hp;
	extern struct hostent *gethostbyname();

	/*
	**  If first character is a bracket, then it is an address
	**  lookup.  Address is copied into a temporary buffer to
	**  strip the brackets and to preserve hbuf if address is
	**  unknown.
	*/

	if (*hbuf == '[')
	{
		extern struct hostent *gethostbyaddr();
		u_long in_addr;
		char ptr[256];
		char *bptr;

		(void) strcpy(ptr, hbuf);
		bptr = index(ptr,']');
		*bptr = '\0';
		in_addr = inet_addr(&ptr[1]);
		hp = gethostbyaddr((char *) &in_addr, sizeof(struct in_addr), AF_INET);
		if (hp == NULL)
			return;
	}
	else
	{
		makelower(hbuf);
#ifdef MXDOMAIN
		getcanonname(hbuf, hbsize);
		return;
#else MXDOMAIN
		hp = gethostbyname(hbuf);
#endif MXDOMAIN
	}
	if (hp != NULL)
	{
		int i = strlen(hp->h_name);

		if (i >= hbsize)
#if	NeXT_MOD
			hp->h_name[hbsize - 1] = '\0';
#else	NeXT_MOD
			hp->h_name[--i] = '\0';
#endif	NeXT_MOD
		(void) strcpy(hbuf, hp->h_name);
	}
}

# else DAEMON
# ifndef lint
static char	SccsId[] = "@(#)daemon.c	5.19 (Berkeley) 5/6/86	(w/o daemon mode)";
# endif not lint

/* code for systems without sophisticated networking */

/*
**  MYHOSTNAME -- stub version for case of no daemon code.
**
**	Can't convert to upper case here because might be a UUCP name.
**
**	Mark, you can change this to be anything you want......
*/

char **
myhostname(hostbuf, size)
	char hostbuf[];
	int size;
{
	register FILE *f;

	hostbuf[0] = '\0';
	f = fopen("/usr/include/whoami", "r");
	if (f != NULL)
	{
		(void) fgets(hostbuf, size, f);
		fixcrlf(hostbuf, TRUE);
		(void) fclose(f);
	}
	return (NULL);
}
/*
**  MAPHOSTNAME -- turn a hostname into canonical form
**
**	Parameters:
**		hbuf -- a buffer containing a hostname.
**		hbsize -- the size of hbuf.
**
**	Returns:
**		none.
**
**	Side Effects:
**		Looks up the host specified in hbuf.  If it is not
**		the canonical name for that host, replace it with
**		the canonical name.  If the name is unknown, or it
**		is already the canonical name, leave it unchanged.
*/

/*ARGSUSED*/
maphostname(hbuf, hbsize)
	char *hbuf;
	int hbsize;
{
	return;
}

#endif DAEMON

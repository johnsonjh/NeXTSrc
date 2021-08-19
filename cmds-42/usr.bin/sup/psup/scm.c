char scmversion[] = "4.2 BSD";
/*
 * SUP Communication Module for 4.2 BSD
 *
 * SUP COMMUNICATION MODULE SPECIFICATIONS:
 *
 * IN THIS MODULE:
 *
 * CONNECTION ROUTINES
 *
 *   FOR SERVER
 *	servicesetup (port)		establish TCP port connection
 *	  char *port;			  name of service
 *	service ()			accept TCP port connection
 *	servicekill ()			close TCP port in use by another process
 *	serviceprep ()			close temp ports used to make connection
 *	serviceend ()			close TCP port
 *
 *   FOR CLIENT
 *	request (port,hostname,retry)	establish TCP port connection
 *	  char *port,*hostname;		  name of service and host
 *	  int retry;			  true if retries should be used
 *	requestend ()			close TCP port
 *
 * HOST NAME CHECKING
 *	p = remotehost ()		remote host name (if known)
 *	  char *p;
 *	i = samehost ()			whether remote host is also this host
 *	  int i;
 *	i = localhost ()		whether remote host is on local net
 *	  int i;
 *	i = matchhost (name)		whether remost host is same as name
 *	  int i;
 *	  char *name;
 *
 * RETURN CODES
 *	All procedures return values as indicated above.  Other routines
 *	normally return SCMOK on success, SCMERR on error.
 *
 * COMMUNICATION PROTOCOL
 *
 *	Described in scmio.c.
 *
 **********************************************************************
 * HISTORY
 * 25-May-86  Jonathan J. Chew (jjc) at Carnegie-Mellon University
 *	Added default ports for sup servers just in case can't find their
 *	ports using "getservbyname".
 *
 * 19-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Changed setsockopt SO_REUSEADDR to be non-fatal.  Added fourth
 *	parameter as described in 4.3 manual entry.
 *
 * 15-Feb-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added call of readflush() to requestend() routine.
 *
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Major rewrite for protocol version 4.  All read/write and crypt
 *	routines are now in scmio.c.
 *
 * 14-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added setsockopt SO_REUSEADDR call.
 *
 * 01-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Removed code to "gracefully" handle unexpected messages.  This
 *	seems reasonable since it didn't work anyway, and should be
 *	handled at a higher level anyway by adhering to protocol version
 *	number conventions.
 *
 * 26-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed scm.c to free space for remote host name when connection
 *	is closed.
 *
 * 07-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Fixed 4.2 retry code to reload sin values before retry.
 *
 * 22-Oct-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to retry initial connection open request.
 *
 * 22-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Merged 4.1 and 4.2 versions together.
 *
 * 21-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Add close() calls after pipe() call.
 *
 * 12-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Converted for 4.2 sockets; added serviceprep() routine.
 *
 * 04-Jun-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for 4.2 BSD.
 *
 **********************************************************************
 */

#include <libc.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include "sup.h"

extern int errno;

/*************************
 ***    M A C R O S    ***
 *************************/

/* networking parameters */
#define NCONNECTS 5

/* default server names */
#define	SUPFILESRV	"supfilesrv"
#define	SUPNAMESRV	"supnamesrv"

/* default server ports */
#define	SUPFILESRVPORT	871
#define	SUPNAMESRVPORT	869

/*********************************************
 ***    G L O B A L   V A R I A B L E S    ***
 *********************************************/

extern char program[];			/* name of program we are running */
extern int progpid;			/* process id to display */

int netfile = -1;			/* network file descriptor */

static int sock = -1;			/* socket used to make connection */
static char *remoteaddr = NULL;		/* remote host name */
static int swapmode;			/* byte-swapping needed on server? */

/***************************************************
 ***    C O N N E C T I O N   R O U T I N E S    ***
 ***    F O R   S E R V E R                      ***
 ***************************************************/

servicesetup (server)		/* listen for clients */
char *server;
{
	struct sockaddr_in sin;
	struct servent mysp;
	struct servent *sp;
	int one = 1;

	if ((sp = getservbyname(server,"tcp")) == 0){
		if (strcmp(server, SUPFILESRV) == 0){
			mysp.s_port = htons((u_short)SUPFILESRVPORT);
			sp = &mysp;
		}
		else if (strcmp(server, SUPNAMESRV) == 0){
			mysp.s_port = htons((u_short)SUPNAMESRVPORT);
			sp = &mysp;
		}
		else
			return (scmerr (-1,"Can't find %s server description",server));
	}
	sock = socket (AF_INET,SOCK_STREAM,0);
	if (sock < 0)
		return (scmerr (errno,"Can't create socket for connections"));
	if (setsockopt (sock,SOL_SOCKET,SO_REUSEADDR,&one,sizeof(int)) < 0)
		(void) scmerr (errno,"Can't set SO_REUSEADDR socket option");
	bzero ((char *)&sin,sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_port = sp->s_port;
	if (bind (sock,(caddr_t)&sin,sizeof(sin)) < 0)
		return (scmerr (errno,"Can't bind socket for connections"));
	if (listen (sock,NCONNECTS) < 0)
		return (scmerr (errno,"Can't listen on socket"));
	return (SCMOK);
}

service ()
{
	struct sockaddr_in from;
	int x,len;
	struct hostent *h;

	remoteaddr = NULL;
	len = sizeof (from);
	do {
		netfile = accept (sock,&from,&len);
	} while (netfile < 0 && errno == EINTR);
	if (netfile < 0)
		return (scmerr (errno,"Can't accept connections"));
	h = gethostbyaddr (&(from.sin_addr),sizeof(from.sin_addr),AF_INET);
	if (h == 0)
		return (scmerr (-1,"Can't find remote host entry"));
	remoteaddr = salloc(h->h_name);
	if (read(netfile,(char *)&x,sizeof(int)) != sizeof(int))
		return (scmerr (errno,"Can't transmit data on connection"));
	if (x == 0x01020304)
		swapmode = 0;
	else if (x == 0x04030201)
		swapmode = 1;
	else
		return (scmerr (-1,"Unexpected byteswap mode %x",x));
	return (SCMOK);
}

serviceprep ()		/* kill temp socket in daemon */
{
	if (sock >= 0) {
		close (sock);
		sock = -1;
	}
	return (SCMOK);
}

servicekill ()		/* kill net file in daemon's parent */
{
	if (netfile >= 0) {
		close (netfile);
		netfile = -1;
	}
	if (remoteaddr) {
		free (remoteaddr);
		remoteaddr = NULL;
	}
	return (SCMOK);
}

serviceend ()		/* kill net file after use in daemon */
{
	if (netfile >= 0) {
		close (netfile);
		netfile = -1;
	}
	if (remoteaddr) {
		free (remoteaddr);
		remoteaddr = NULL;
	}
	return (SCMOK);
}

/***************************************************
 ***    C O N N E C T I O N   R O U T I N E S    ***
 ***    F O R   C L I E N T                      ***
 ***************************************************/

request (server,hostname,retry)		/* connect to server */
char *server;
char *hostname;
int retry;
{
	int x, backoff, sltime;
	struct hostent *h;
	struct servent mysp;
	struct servent *sp;
	struct sockaddr_in sin;
	struct timeval tt;

	sp = getservbyname (server,"tcp");
	if (sp == NULL){
		if (strcmp(server, SUPFILESRV) == 0){
			mysp.s_port = htons((u_short)SUPFILESRVPORT);
			sp = &mysp;
		}
		else if (strcmp(server, SUPNAMESRV) == 0){
			mysp.s_port = htons((u_short)SUPNAMESRVPORT);
			sp = &mysp;
		}
		else
			return (scmerr (-1,"Can't find server entry for %s",server));
	}
	endservent ();
	h = gethostbyname (hostname);
	if (h == NULL)
		return (scmerr (-1,"Can't find host entry for %s",hostname));
	endhostent ();
	backoff = 1;
	for (;;) {
		netfile = socket (AF_INET,SOCK_STREAM,0);
		if (netfile < 0)
			return (scmerr (errno,"Can't create socket"));
		bzero ((char *)&sin,sizeof(sin));
		bcopy (h->h_addr,(char *)&sin.sin_addr,h->h_length);
		sin.sin_family = AF_INET;
		sin.sin_port = sp->s_port;
		if (connect(netfile,(char *)&sin,sizeof(sin)) >= 0)
			break;
		(void) scmerr (errno,"Can't connect to server for %s",server);
		close(netfile);
		if (!retry) return (SCMERR);
		sltime = backoff * 30;
		if (gettimeofday(&tt,(struct timezone *)NULL) >= 0)
			sltime += (tt.tv_usec >> 8) % sltime;
		(void) scmerr (-1,"Will retry in %d seconds",sltime);
		sleep (sltime);
		if (backoff < 32) backoff <<= 1;
	}
	remoteaddr = salloc(h->h_name);
	x = 0x01020304;
	write (netfile,(char *)&x,sizeof(int));
	swapmode = 0;		/* swap only on server, not client */
	return (SCMOK);
}

requestend ()			/* end connection to server */
{
	(void) readflush ();
	if (netfile >= 0) {
		close (netfile);
		netfile = -1;
	}
	if (remoteaddr) {
		free (remoteaddr);
		remoteaddr = NULL;
	}
	return (SCMOK);
}

/*************************************************
 ***    H O S T   N A M E   C H E C K I N G    ***
 *************************************************/

char *remotehost ()	/* remote host name (if known) */
{
	return (remoteaddr);
}

int samehost ()		/* is remote host same as local host? */
{
	struct hostent *h;
	char name[1000];
	gethostname (name,1000);
	h = gethostbyname (name);
	return (strcmp(h->h_name,remoteaddr) == 0);
}

int localhost ()	/* is remote host on local network? */
{
	struct hostent *h;
	char name[1000];
	int thisnet,thatnet;
	gethostname (name,1000);
	h = gethostbyname (name);
	thisnet = inet_netof(*(struct in_addr *)h->h_addr);
	h = gethostbyname (remoteaddr);
	thatnet = inet_netof(*(struct in_addr *)h->h_addr);
	return (thisnet == thatnet);
}

int matchhost (name)	/* is this name of remote host? */
char *name;
{
	struct hostent *h;
	h = gethostbyname (name);
	if (h == 0)  return (0);
	return (strcmp(remoteaddr,h->h_name) == 0);
}

/* VARARGS2 */
int scmerr (errno,fmt,args)
int errno;
char *fmt;
{
	fflush (stdout);
	if (progpid > 0)
		fprintf (stderr,"%s %d: ",program,progpid);
	else
		fprintf (stderr,"%s: ",program);
	_doprnt(fmt, &args, stderr);
	if (errno >= 0)
		fprintf (stderr,": %s\n",errmsg(errno));
	else
		fprintf (stderr,"\n");
	fflush (stderr);
	return (SCMERR);
}

/*******************************************************
 ***    I N T E G E R   B Y T E - S W A P P I N G    ***
 *******************************************************/

union intchar {
	int ui;
	char uc[sizeof(int)];
};

int byteswap (in)
int in;
{
	union intchar x,y;
	register int ix,iy;

	if (swapmode == 0)  return (in);
	x.ui = in;
	iy = sizeof(int);
	for (ix=0; ix<sizeof(int); ix++) {
		--iy;
		y.uc[iy] = x.uc[ix];
	}
	return (y.ui);
}

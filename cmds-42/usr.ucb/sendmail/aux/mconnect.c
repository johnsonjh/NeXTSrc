/*
**  Sendmail
**  Copyright (c) 1983  Eric P. Allman
**  Berkeley, California
**
**  Copyright (c) 1983 Regents of the University of California.
**  All rights reserved.  The Berkeley software License Agreement
**  specifies the terms and conditions for redistribution.
*/

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char	SccsId[] = "@(#)mconnect.c	5.2 (Berkeley) 7/13/85";
#endif not lint

# include <stdio.h>
# include <signal.h>
# include <ctype.h>
# include <sgtty.h>
# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <netdb.h>

struct sockaddr_in	SendmailAddress;
struct sgttyb		TtyBuf;

main(argc, argv)
	int argc;
	char **argv;
{
	register int s;
#ifdef	NeXT_NFS
	char *host = NULL;
#else	NeXT_NFS
	char *host;
	register int i;
#endif	NeXT_NFS
	int pid;
	struct servent *sp;
	int raw = 0;
	char buf[1000];
#ifdef	NeXT_NFS
	int on = 1;
	register struct hostent *hp;
	u_long theaddr;
#else	NeXT_NFS
	register char *p;
#endif	NeXT_NFS
	extern char *index();
	register FILE *f;
	extern u_long inet_addr();
	extern finis();

	(void) gtty(0, &TtyBuf);
	(void) signal(SIGINT, finis);
#ifdef	NeXT_NFS
	s = socket(AF_INET, SOCK_STREAM, 0);
#else	NeXT_NFS
	s = socket(AF_INET, SOCK_STREAM, 0, 0);
#endif	NeXT_NFS
	if (s < 0)
	{
		perror("socket");
		exit(-1);
	}
#ifdef	NeXT_NFS
	(void) setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&on, sizeof(on));
#endif	NeXT_NFS
	sp = getservbyname("smtp", "tcp");
	if (sp != NULL)
		SendmailAddress.sin_port = sp->s_port;

	while (--argc > 0)
	{
		register char *p = *++argv;

		if (*p == '-')
		{
			switch (*++p)
			{
			  case 'h':		/* host */
				break;

			  case 'p':		/* port */
				SendmailAddress.sin_port = htons(atoi(*++argv));
				argc--;
				break;

			  case 'r':		/* raw connection */
				raw = 1;
				TtyBuf.sg_flags &= ~CRMOD;
				stty(0, &TtyBuf);
				TtyBuf.sg_flags |= CRMOD;
				break;
			}
		}
		else if (host == NULL)
			host = p;
	}
	if (host == NULL)
		host = "localhost";

#ifdef	NeXT_NFS
	hp = gethostbyname(host);
	if (hp == NULL)
	{
		/* Try for dotted pair or whatever */
		theaddr = inet_addr(host);
		SendmailAddress.sin_addr.s_addr = theaddr;
		if (-1 == theaddr) {
#else	NeXT_NFS
	if (isdigit(*host))
		SendmailAddress.sin_addr.s_addr = inet_addr(host);
	else
	{
		register struct hostent *hp = gethostbyname(host);

		if (hp == NULL)
		{
#endif	NeXT_NFS
			fprintf(stderr, "mconnect: unknown host %s\r\n", host);
			finis();
		}
#ifdef	NeXT_NFS
	} else {
#endif	NeXT_NFS
		bcopy(hp->h_addr, &SendmailAddress.sin_addr, hp->h_length);
	}
	SendmailAddress.sin_family = AF_INET;
#ifdef	NeXT_NFS
	printf("connecting to host %s (%s), port %d\r\n", host,
	       inet_ntoa(SendmailAddress.sin_addr),
	       SendmailAddress.sin_port);
	if (connect(s, (struct sockaddr *)&SendmailAddress,
	    sizeof SendmailAddress) < 0)
#else	NeXT_NFS
	printf("connecting to host %s (0x%x), port 0x%x\r\n", host,
	       SendmailAddress.sin_addr.s_addr, SendmailAddress.sin_port);
	if (connect(s, &SendmailAddress, sizeof SendmailAddress, 0) < 0)
#endif	NeXT_NFS
	{
		perror("connect");
		exit(-1);
	}

	/* good connection, fork both sides */
	printf("connection open\n");
	pid = fork();
	if (pid < 0)
	{
		perror("fork");
		exit(-1);
	}
	if (pid == 0)
	{
		/* child -- standard input to sendmail */
		int c;

		f = fdopen(s, "w");
		while ((c = fgetc(stdin)) >= 0)
		{
			if (!raw && c == '\n')
				fputc('\r', f);
			fputc(c, f);
			if (c == '\n')
				fflush(f);
		}
#ifdef	NeXT_NFS
		shutdown(s,1);
		sleep(10);
#endif	NeXT_NFS
	}
	else
	{
		/* parent -- sendmail to standard output */
		f = fdopen(s, "r");
		while (fgets(buf, sizeof buf, f) != NULL)
		{
			fputs(buf, stdout);
			fflush(stdout);
		}
	}
	finis();
}

finis()
{
	stty(0, &TtyBuf);
	exit(0);
}

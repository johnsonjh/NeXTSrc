/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = 	"@(#)dumprmt.c	1.4 88/05/26 4.0NFSSRC SMI"; 
/* from UCB 5.4 12/11/85 */
/* @(#) from SUN 1.9      */
#endif not lint

#include <sys/param.h>
#include <sys/mtio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>

#include <netinet/in.h>

#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <netdb.h>
#include <protocols/dumprestore.h>

#define	TS_CLOSED	0
#define	TS_OPEN		1

static	int rmtstate = TS_CLOSED;
int	rmtape;
int	rmtconnaborted();
char	*rmtpeer;

extern int ntrec;		/* blocking factor on tape */

rmthost(host)
	char *host;
{

	rmtpeer = host;
	signal(SIGPIPE, rmtconnaborted);
	rmtgetconn();
	if (rmtape < 0)
		return (0);
	return (1);
}

rmtconnaborted()
{

	fprintf(stderr, "rdump: Lost connection to remote host.\n");
	exit(1);
}

rmtgetconn()
{
	static struct servent *sp = 0;
	static struct passwd *pwd = 0;
#ifdef NeXT_MOD
	char *tuser, *mark;
#endif NeXT_MOD
	char *name = "root";
	int size;
#ifdef NeXT_MOD
	int uid;
#endif NeXT_MOD

	if (sp == 0) {
		sp = getservbyname("shell", "tcp");
		if (sp == 0) {
			fprintf(stderr, "rdump: shell/tcp: unknown service\n");
			exit(1);
		}
	}
#ifdef NeXT_MOD
	uid = getuid();
	pwd = getpwuid(uid);
	if (pwd == NULL)  {
		fprintf(stderr, "rdump: Who are you?\n");
		exit(1);
	}

 	if ((tuser = (char *) rindex(rmtpeer, '.'))) {       /* host.user */
 		*tuser++ = 0;
 		if (!okname(tuser))
 			exit(1);
 	} else if ((mark = (char *) index(rmtpeer, '@'))) {  /* user@host */
 		tuser = (char *) malloc(mark - rmtpeer + 1);
 		strncpy(tuser, rmtpeer, mark - rmtpeer);
 		tuser[mark - rmtpeer] = NULL;
 		if (!okname(tuser))
 			exit(1);
 		while (*rmtpeer > *mark) {
 			*++rmtpeer;
 		}
 		*++rmtpeer;
 	} else {
 		tuser = pwd->pw_name;
 	}
#endif NeXT_MOD
	if (pwd && pwd->pw_name)
		name = pwd->pw_name;
	rmtape = rcmd(&rmtpeer, sp->s_port, pwd->pw_name, tuser, 
		       "/etc/rmt", 0);
#ifdef NeXT_MOD
	if (rmtape < 0)
		rmtape = rcmd(&rmtpeer, sp->s_port, pwd->pw_name, tuser,
				 "/usr/etc/rmt", 0);
	if (rmtape < 0) {
		fprintf(stderr, "rdump: rcmd of /etc/rmt and "
			"/usr/etc/rmt failed\n");
		exit(1);
	}
#endif NeXT_MOD
	size = ntrec * TP_BSIZE;
	while (size > TP_BSIZE &&
	    setsockopt(rmtape, SOL_SOCKET, SO_SNDBUF, &size, sizeof (size)) < 0)
		size -= TP_BSIZE;
}

#ifdef NeXT_MOD
okname(cp0)
	char *cp0;
{
	register char *cp;
	register int c;

	for (cp = cp0; *cp; cp++) {
		c = *cp;
		if (!isascii(c) || !(isalnum(c) || c == '_' || c == '-')) {
			fprintf(stderr, "rdump: invalid user name %s\n", cp0);
			return (0);
		}
	}
	return (1);
}
#endif NeXT_MOD

rmtopen(tape, mode)
	char *tape;
	int mode;
{
	char buf[256];

	sprintf(buf, "O%s\n%d\n", tape, mode);
	rmtstate = TS_OPEN;
	return (rmtcall(tape, buf));
}

rmtclose()
{

	if (rmtstate != TS_OPEN)
		return;
	rmtcall("close", "C\n");
	rmtstate = TS_CLOSED;
}

rmtread(buf, count)
	char *buf;
	int count;
{
	char line[30];
	int n, i, cc;
	extern errno;

	sprintf(line, "R%d\n", count);
	n = rmtcall("read", line);
	if (n < 0) {
		errno = n;
		return (-1);
	}
	for (i = 0; i < n; i += cc) {
		cc = read(rmtape, buf+i, n - i);
		if (cc <= 0) {
			rmtconnaborted();
		}
	}
	return (n);
}

rmtwrite(buf, count)
	char *buf;
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
	write(rmtape, buf, count);
	return (rmtreply("write"));
}

rmtwrite0(count)
	int count;
{
	char line[30];

	sprintf(line, "W%d\n", count);
	write(rmtape, line, strlen(line));
}

rmtwrite1(buf, count)
	char *buf;
	int count;
{

	write(rmtape, buf, count);
}

rmtwrite2()
{
	int i;

	return (rmtreply("write"));
}

rmtseek(offset, pos)
	int offset, pos;
{
	char line[80];

	sprintf(line, "L%d\n%d\n", offset, pos);
	return (rmtcall("seek", line));
}

#ifdef NeXT_MOD
rmteject (char *diskname)
{
	return (rmtcall("eject", "J%s\n"));
}
#endif NeXT_MOD
		
struct	mtget mts;

struct mtget *
rmtstatus()
{
	register int i;
	register char *cp;

	if (rmtstate != TS_OPEN)
		return (0);
	rmtcall("status", "S\n");
	for (i = 0, cp = (char *)&mts; i < sizeof(mts); i++)
		*cp++ = rmtgetb();
	return (&mts);
}

rmtioctl(cmd, count)
	int cmd, count;
{
	char buf[256];

	if (count < 0)
		return (-1);
	sprintf(buf, "I%d\n%d\n", cmd, count);
	return (rmtcall("ioctl", buf));
}

rmtcall(cmd, buf)
	char *cmd, *buf;
{

	if (write(rmtape, buf, strlen(buf)) != strlen(buf))
		rmtconnaborted();
	return (rmtreply(cmd));
}

rmtreply(cmd)
	char *cmd;
{
	register int c;
	char code[30], emsg[BUFSIZ];

	rmtgets(code, sizeof (code));
	if (*code == 'E' || *code == 'F') {
		rmtgets(emsg, sizeof (emsg));
		msg("%s: %s\n", cmd, emsg, code + 1);
		if (*code == 'F') {
			rmtstate = TS_CLOSED;
			return (-1);
		}
		return (-1);
	}
	if (*code != 'A') {
		msg("Protocol to remote tape server botched (code %s?).\n",
		    code);
		rmtconnaborted();
	}
	return (atoi(code + 1));
}

rmtgetb()
{
	char c;

	if (read(rmtape, &c, 1) != 1)
		rmtconnaborted();
	return (c);
}

rmtgets(cp, len)
	char *cp;
	int len;
{

	while (len > 1) {
		*cp = rmtgetb();
		if (*cp == '\n') {
			cp[1] = 0;
			return;
		}
		cp++;
		len--;
	}
	msg("Protocol to remote tape server botched (in rmtgets).\n");
	rmtconnaborted();
}

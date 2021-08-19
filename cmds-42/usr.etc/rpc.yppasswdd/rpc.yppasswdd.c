#ifndef lint
static char sccsid[] = 	"@(#)rpc.yppasswdd.c	1.6 88/08/15 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1985 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <signal.h>
#include <ctype.h>
#include <sys/file.h>
#include <rpc/rpc.h>
#include <pwd.h>
#include <rpcsvc/yppasswd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <syslog.h>

#define pwmatch(name, line, cnt) \
	(line[cnt] == ':' && strncmp(name, line, cnt) == 0)
	
char *file;			/* file in which passwd's are found */
char *filea;			/* adjunct file in which passwd's are found */
char	temp[96];		/* lockfile for modifying 'file' */
int mflag;			/* do a make */

char *index();
int boilerplate();
extern int errno;
int Argc;
char **Argv;

struct passwd *my_getpwnam();

main(argc, argv)
	char **argv;
{
	SVCXPRT *transp;
	int s;
	char	*cp;
	
	Argc = argc;
	Argv = argv;
	if (argc < 2) {
	    	fprintf(stderr,"usage: %s file [-m arg1 arg2 ...]\n",
				argv[0]);
		exit(1);
	}
	file = argv[1];
	if (access(file, W_OK) < 0) {
		fprintf(stderr, "can't write %s\n", file);
		exit(1);
	}
	if (argc > 2 && argv[2][0] == '-' && argv[2][1] == 'm')
		mflag++;
		
	if (chdir("/etc/yp") < 0) {
		fprintf(stderr, "yppasswdd: can't chdir to /etc/yp\n");
		exit(1);
	}

	/* make a temp file in the same dir at the passwd file */
	strcpy(temp,file);
	/* find end of the path ... */
	for (cp = &(temp[strlen(temp)]); (cp != temp) && (*cp != '/'); cp--)
		;
	if (*cp == '/') 
		strcat(cp+1,".ptmp");
	else
		strcat(cp,".ptmp");
	/* temp now has either '.ptmp' or 'filepath/.ptmp' in it */

	if ((s = rresvport()) < 0) {
		fprintf(stderr,
		    "yppasswdd: can't bind to a privileged socket\n");
		exit(1);
	}
	transp = svcudp_create(s);
	if (transp == NULL) {
		fprintf(stderr, "yppasswdd: couldn't create an RPC server\n");
		exit(1);
	}
	pmap_unset(YPPASSWDPROG, YPPASSWDVERS);
	if (!svc_register(transp, YPPASSWDPROG, YPPASSWDVERS,
	    boilerplate, IPPROTO_UDP)) {
		fprintf(stderr, "yppasswdd: couldn't register yppasswdd\n");
		exit(1);
	}

	if (fork())
		exit(0);
	{ int t;
	for (t = getdtablesize()-1; t >= 0; t--)
		if (t != s)
			(void) close(t);
	}
	(void) open("/", O_RDONLY);
	(void) dup2(0, 1);
	(void) dup2(0, 2);
	{ int tt = open("/dev/tty", O_RDWR);
	  if (tt > 0) {
		ioctl(tt, TIOCNOTTY, 0);
		close(tt);
	  }
	}

	svc_run();
	fprintf(stderr, "yppasswdd: svc_run shouldn't have returned\n");
}

boilerplate(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	switch(rqstp->rq_proc) {
		case NULLPROC:
			if (!svc_sendreply(transp, xdr_void, 0))
				syslog(LOG_ERR,
				    "yppasswdd: couldn't reply to RPC call\n");
			break;
		case YPPASSWDPROC_UPDATE:
			changepasswd(rqstp, transp);
			break;
	}
}

changepasswd(rqstp, transp)
	struct svc_req *rqstp;
	SVCXPRT *transp;
{
	int tempfd, i, len;
	static int ans;
	FILE *tempfp, *filefp, *fp;
	char buf[256], *p;
	char cmdbuf[BUFSIZ];
	int (*f1)(), (*f2)(), (*f3)();
	struct passwd *oldpw, *newpw;
	struct passwd *getpwnam();
	struct passwd_adjunct *oldpwa;
	struct yppasswd yppasswd;
	union wait status;
	char *ptr;
	char str1[224];
	char str2[224];
	
	bzero(&yppasswd, sizeof(yppasswd));
	if (!svc_getargs(transp, xdr_yppasswd, &yppasswd)) {
		svcerr_decode(transp);
		return;
	}
	/* 
	 * Clean up from previous changepasswd() call
	 */
	while (wait3(&status, WNOHANG, 0) > 0)
		continue;

	newpw = &yppasswd.newpw;
	ans = 1;
	oldpw = my_getpwnam(newpw->pw_name);
	if (oldpw == NULL) {
		fprintf(stderr, "yppasswdd: no passwd for %s\n",
		    newpw->pw_name);
		goto done;
	}
	if (oldpw->pw_passwd && *oldpw->pw_passwd &&
    	    strcmp(crypt(yppasswd.oldpass,oldpw->pw_passwd),
    	    oldpw->pw_passwd) != 0) {
		fprintf(stderr, "yppasswdd: %s: bad passwd\n", newpw->pw_name);
		goto done;
	}
	(void) umask(0);

	f1 = signal(SIGHUP, SIG_IGN);
	f2 = signal(SIGINT, SIG_IGN);
	f3 = signal(SIGQUIT, SIG_IGN);
	tempfd = open(temp, O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (tempfd < 0) {
		fprintf(stderr, "yppasswdd: ");
		if (errno == EEXIST)
			fprintf(stderr, "password file busy - try again.\n");
		else
			perror(temp);
		goto cleanup;
	}
	signal(SIGTSTP, SIG_IGN);
	if ((tempfp = fdopen(tempfd, "w")) == NULL) {
		fprintf(stderr, "yppasswdd: fdopen failed?\n");
		goto cleanup;
	}
	/*
	 * Copy passwd to temp, replacing matching lines
	 * with new password.
	 */
	if ((filefp = fopen(file, "r")) == NULL) {
		fprintf(stderr, "yppasswdd: fopen of %s failed?\n",
		    file);
		goto cleanup;
	}
	len = strlen(newpw->pw_name);
 	/*
	* This fixes a really bogus security hole, basically anyone can
	* call the rpc passwd deamon, give them their own passwd and a
	* new one that consists of ':0:0:Im root now:/:/bin/csh^J' and
	* give themselves root access. With this code it will simply make
	* it impossible for them to login again, and as a bonus leave 
	* a cookie for the always vigilant system administrator to ferret
	* them out.
	*/
	for (p = newpw->pw_name; (*p != '\0'); p++)
		if ((*p == ':') || !(isprint(*p)))
			*p = '$';	/* you lose buckwheat */

	while (fgets(buf, sizeof(buf), filefp)) {
		p = index(buf, ':');
		if (p && p - buf == len
		    && strncmp(newpw->pw_name, buf, p - buf) == 0) {
			fprintf(tempfp,"%s:%s:%d:%d:%s:%s:%s\n",
			    oldpw->pw_name,
			    newpw->pw_passwd,
			    oldpw->pw_uid,
			    oldpw->pw_gid,
			    oldpw->pw_gecos,
			    oldpw->pw_dir,
			    oldpw->pw_shell);
		}
		else
			fputs(buf, tempfp);
	}
	fclose(filefp);
	fclose(tempfp);
	if (rename(temp, file) < 0) {
		fprintf(stderr, "yppasswdd: "); perror("rename");
		unlink(temp);
		goto cleanup;
	}
	ans = 0;
	if (mflag && fork() == 0) {
		strcpy(cmdbuf, "make");
		for (i = 3; i < Argc; i++) {
			strcat(cmdbuf, " ");
			strcat(cmdbuf, Argv[i]);
		}
#ifdef DEBUG
		fprintf(stderr, "about to execute %s\n", cmdbuf);
#else
		system(cmdbuf);
#endif
		exit(0);
	}
    cleanup:
	fclose(tempfp);
	signal(SIGHUP, f1);
	signal(SIGINT, f2);
	signal(SIGQUIT, f3);
    done:
	if (!svc_sendreply(transp, xdr_int, &ans))
		fprintf(stderr, "yppasswdd: couldnt reply to RPC call\n");
}

rresvport()
{
	struct sockaddr_in sin;
	int s, alport = IPPORT_RESERVED - 1;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s < 0)
		return (-1);
	for (;;) {
		sin.sin_port = htons((u_short)alport);
		if (bind(s, (struct sockaddr *)&sin, sizeof (sin)) >= 0)
			return (s);
		if (errno != EADDRINUSE && errno != EADDRNOTAVAIL) {
			perror("socket");
			return (-1);
		}
		(alport)--;
		if (alport == IPPORT_RESERVED/2) {
			fprintf(stderr, "socket: All ports in use\n");
			return (-1);
		}
	}
}

static char *
pwskip(p)
register char *p;
{
	while( *p && *p != ':' && *p != '\n' )
		++p;
	if( *p ) *p++ = 0;
	return(p);
}

struct passwd *
my_getpwnam(name)
	char *name;
{
	FILE *pwf;
	int cnt;
	char *p;
	static char line[BUFSIZ+1];
	static struct passwd passwd;
	
	pwf = fopen(file, "r");
	if (pwf == NULL)
		return (NULL);
	cnt = strlen(name);
	while ((p = fgets(line, BUFSIZ, pwf)) && !pwmatch(name, line, cnt))
		;
	if (p) {
		passwd.pw_name = p;
		p = pwskip(p);
		passwd.pw_passwd = p;
		p = pwskip(p);
		passwd.pw_uid = atoi(p);
		p = pwskip(p);
		passwd.pw_gid = atoi(p);
		passwd.pw_quota = 0;
		passwd.pw_comment = "";
		p = pwskip(p);
		passwd.pw_gecos = p;
		p = pwskip(p);
		passwd.pw_dir = p;
		p = pwskip(p);
		passwd.pw_shell = p;
		while(*p && *p != '\n') p++;
		*p = '\0';
		fclose(pwf);
		return (&passwd);
	}
	else {
		fclose(pwf);
		return (NULL);
	}
}



/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)su.c	5.4 (Berkeley) 1/13/86";
#endif not lint

#include <stdio.h>
#include <pwd.h>
#include <grp.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/resource.h>
#if	NeXT
#include <sys/param.h>
#endif /* def NeXT*/

char	userbuf[16]	= "USER=";
char	homebuf[128]	= "HOME=";
char	shellbuf[128]	= "SHELL=";
char	pathbuf[128]	= "PATH=:/usr/ucb:/bin:/usr/bin";
#if	NeXT
char	termbuf[1024]   = "TERM=";
#endif	NeXT
char	*cleanenv[] = { userbuf, homebuf, shellbuf, pathbuf, termbuf, 0 };
char	*user = "root";
char	*shell = "/bin/sh";
int	fulllogin;
int	fastlogin;

extern char	**environ;
struct	passwd *pwd;
#if	NeXT
char	pwdbuf[1000];
#endif	NeXT
char	*crypt();
char	*getpass();
char	*getenv();
char	*getlogin();

main(argc,argv)
	int argc;
	char *argv[];
{
	char *password;
	char buf[1000];
	FILE *fp;
	register char *p;
#if	NeXT
	int imawheel = 0;	/* Flag for user in group 0 */
#endif	NeXT

	openlog("su", LOG_ODELAY, LOG_AUTH);

again:
	if (argc > 1 && strcmp(argv[1], "-f") == 0) {
		fastlogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && strcmp(argv[1], "-") == 0) {
		fulllogin++;
		argc--, argv++;
		goto again;
	}
	if (argc > 1 && argv[1][0] != '-') {
		user = argv[1];
		argc--, argv++;
	}
	if ((pwd = getpwuid(getuid())) == NULL) {
		fprintf(stderr, "Who are you?\n");
		exit(1);
	}
	strcpy(buf, pwd->pw_name);
#if	NeXT
	/*
	 * Copy my important pwd information into local storage
	 */
	strcpy(pwdbuf, pwd->pw_passwd);
	if (pwd->pw_gid == 0) {
		imawheel++;
	}
#endif	NeXT
	if ((pwd = getpwnam(user)) == NULL) {
		fprintf(stderr, "Unknown login: %s\n", user);
		exit(1);
	}

	/*
	 * Only allow those in group zero to su to root.
	 */
	if (pwd->pw_uid == 0) {
		struct group *grp;
		int i;

#if	NeXT
		if (imawheel) {
			goto userok;
		}

		/*
		 * Compute group membership the same way login and friends
		 * do. getgrgid(0) doesn't work because it finds only the
		 * first wheel group. It should probably be fixed, but it
		 * doesn't hurt to fix it here too.
		 */
		setgrent();
		while (grp = getgrent()) {
			if (grp->gr_gid != 0) {
				continue;
			}
			for (i = 0; grp->gr_mem[i]; i++) {
				if (strcmp(grp->gr_mem[i], buf) == 0) {
					imawheel++;
					endgrent();
					goto userok;
				}
			}
		}
		endgrent();
		
		fprintf(stderr, 
			"You do not have permission to su %s\n",
			user);
		exit(1);
#else
		struct	group *gr;
		int i;

		if ((gr = getgrgid(0)) != NULL) {
			for (i = 0; gr->gr_mem[i] != NULL; i++)  {
				if (strcmp(buf, gr->gr_mem[i]) == 0)
					goto userok;
			}
			fprintf(stderr, 
				"You do not have permission to su %s\n",
				user);
			exit(1);
		}
#endif	NeXT
	userok:
		setpriority(PRIO_PROCESS, 0, -2);
	}

#define Getlogin()  (((p = getlogin()) && *p) ? p : buf)
	if (pwd->pw_passwd[0] == '\0' || getuid() == 0)
		goto ok;
	password = getpass("Password:");
#ifdef  WHEEL
	if ((strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) &&
	    (!imawheel ||
	      strcmp(pwdbuf, crypt(password, pwdbuf)) != 0)) {
#else   WHEEL
	if (strcmp(pwd->pw_passwd, crypt(password, pwd->pw_passwd)) != 0) {
#endif  WHEEL
		fprintf(stderr, "Sorry\n");
		if (pwd->pw_uid == 0) {
			syslog(LOG_CRIT, "BAD SU %s on %s",
					Getlogin(), ttyname(2));
		}
		exit(2);
	}
ok:
	endpwent();
	if (pwd->pw_uid == 0) {
		syslog(LOG_NOTICE, "%s on %s", Getlogin(), ttyname(2));
		closelog();
	}
	if (setgid(pwd->pw_gid) < 0) {
		perror("su: setgid");
		exit(3);
	}
	if (initgroups(user, pwd->pw_gid)) {
		fprintf(stderr, "su: initgroups failed\n");
		exit(4);
	}
	if (setuid(pwd->pw_uid) < 0) {
		perror("su: setuid");
		exit(5);
	}
	if (pwd->pw_shell && *pwd->pw_shell)
		shell = pwd->pw_shell;
	if (fulllogin) {
#if	NeXT
		strcat (cleanenv[4], getenv ("TERM"));
#else	NeXT
		cleanenv[4] = getenv("TERM");
#endif	NeXT
		environ = cleanenv;
	}
	if (strcmp(user, "root"))
		setenv("USER", pwd->pw_name, userbuf);
	setenv("SHELL", shell, shellbuf);
	setenv("HOME", pwd->pw_dir, homebuf);
	setpriority(PRIO_PROCESS, 0, 0);
	if (fastlogin) {
		*argv-- = "-f";
		*argv = "su";
	} else if (fulllogin) {
		if (chdir(pwd->pw_dir) < 0) {
			fprintf(stderr, "No directory\n");
			exit(6);
		}
		*argv = "-su";
	} else
		*argv = "su";
	execv(shell, argv);
	fprintf(stderr, "No shell\n");
	exit(7);
}

setenv(ename, eval, buf)
	char *ename, *eval, *buf;
{
	register char *cp, *dp;
	register char **ep = environ;

	/*
	 * this assumes an environment variable "ename" already exists
	 */
	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0)) {
			strcat(buf, eval);
			*--ep = buf;
			return;
		}
	}
}

#if	NeXT
#else	NeXT
/*  
 * Nuke this getenv() in favor of libc version.
 */
char *
getenv(ename)
	char *ename;
{
	register char *cp, *dp;
	register char **ep = environ;

	while (dp = *ep++) {
		for (cp = ename; *cp == *dp && *cp; cp++, dp++)
			continue;
		if (*cp == 0 && (*dp == '=' || *dp == 0))
			return (*--ep);
	}
	return ((char *)0);
}
#endif	NeXT

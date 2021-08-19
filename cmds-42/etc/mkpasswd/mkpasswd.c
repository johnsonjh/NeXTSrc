/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1980 Regents of the University of California.\n\
 All rights reserved.\n";
#endif not lint

#ifndef lint
static char sccsid[] = "@(#)mkpasswd.c        1.2 88/03/13 4.0NFSSRC; from    5.1 (Berkeley) 5/28/85";
#endif not lint

#include <sys/file.h>
#include <stdio.h>
#include <pwd.h>
#include <ndbm.h>

char	buf[BUFSIZ];

#ifdef	NeXT_NFS
static setpwfile();
#else	NeXT_NFS
struct	passwd *fgetpwent();
#endif	NeXT_NFS

main(argc, argv)
	char *argv[];
{
	DBM *dp;
	datum key, content;
	register char *cp, *tp;
	register struct passwd *pwd;
	int verbose = 0, entries = 0, maxlen = 0;

	if (argc > 1 && strcmp(argv[1], "-v") == 0) {
		verbose++;
		argv++, argc--;
	}
	if (argc != 2) {
		fprintf(stderr, "usage: mkpasswd [ -v ] file\n");
		exit(1);
	}
	if (access(argv[1], R_OK) < 0) {
		fprintf(stderr, "mkpasswd: ");
		perror(argv[1]);
		exit(1);
	}
	umask(0);
	dp = dbm_open(argv[1], O_WRONLY|O_CREAT|O_EXCL, 0644);
	if (dp == NULL) {
		fprintf(stderr, "mkpasswd: ");
		perror(argv[1]);
		exit(1);
	}
	setpwfile(argv[1]);
	while (pwd = getpwent()) {
		cp = buf;
#ifdef	__GNU__
#define	COMPACT(e)	tp = pwd->pw_##e; while (*cp++ = *tp++);
#else	__GNU__
#define	COMPACT(e)	tp = pwd->pw_/**/e; while (*cp++ = *tp++);
#endif	__GNU__
		COMPACT(name);
		COMPACT(passwd);
		bcopy((char *)&pwd->pw_uid, cp, sizeof (int));
		cp += sizeof (int);
		bcopy((char *)&pwd->pw_gid, cp, sizeof (int));
		cp += sizeof (int);
		bcopy((char *)&pwd->pw_quota, cp, sizeof (int));
		cp += sizeof (int);
		COMPACT(comment);
		COMPACT(gecos);
		COMPACT(dir);
		COMPACT(shell);
		content.dptr = buf;
		content.dsize = cp - buf;
		if (verbose)
			printf("store %s, uid %d\n", pwd->pw_name, pwd->pw_uid);
		key.dptr = pwd->pw_name;
		key.dsize = strlen(pwd->pw_name);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			fprintf(stderr, "mkpasswd: ");
			perror("dbm_store failed");
			exit(1);
		}
		key.dptr = (char *)&pwd->pw_uid;
		key.dsize = sizeof (int);
		if (dbm_store(dp, key, content, DBM_INSERT) < 0) {
			fprintf(stderr, "mkpasswd: ");
			perror("dbm_store failed");
			exit(1);
		}
		entries++;
		if (cp - buf > maxlen)
			maxlen = cp - buf;
	}
	dbm_close(dp);
	printf("%d password entries, maximum length %d\n", entries, maxlen);
	exit(0);
}

#ifdef	NeXT_NFS
/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *  version of getpwent which does not use Yellow Pages
 */

/*
 * Copyright (c) 1984 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 *  @(#)from getpwent.c	5.2 (Berkeley) 3/9/86
 */

static char EMPTY[] = "";
static FILE *pwf = NULL;
static char line[BUFSIZ+1];
static struct passwd passwd;

static char	*_pw_file = "/etc/passwd";

static char *
pwskip(p)
register char *p;
{
	while (*p && *p != ':' && *p != '\n')
		++p;
	if (*p)
		*p++ = 0;
	return(p);
}

static struct passwd *
getpwent()
{
	register char *p;

	if (pwf == NULL) {
		if ((pwf = fopen( _pw_file, "r" )) == NULL)
			return(0);
	}
	p = fgets(line, BUFSIZ, pwf);
	if (p == NULL)
		return(0);
	passwd.pw_name = p;
	p = pwskip(p);
	passwd.pw_passwd = p;
	p = pwskip(p);
	passwd.pw_uid = atoi(p);
	p = pwskip(p);
	passwd.pw_gid = atoi(p);
	passwd.pw_quota = 0;
	passwd.pw_comment = EMPTY;
	p = pwskip(p);
	passwd.pw_gecos = p;
	p = pwskip(p);
	passwd.pw_dir = p;
	p = pwskip(p);
	passwd.pw_shell = p;
	while (*p && *p != '\n')
		p++;
	*p = '\0';
	return(&passwd);
}

static
setpwfile(file)
	char *file;
{
	_pw_file = file;
}
#endif	NeXT_NFS

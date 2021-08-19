/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)getwd.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

/*
 * getwd() returns the pathname of the current working directory. On error
 * an error message is copied to pathname and null pointer is returned.
 **********************************************************************
 * HISTORY
 * 08-Apr-86  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created CMU CS 4.3 version of getwd.  Understands super-root
 *	and self-parent concepts.  Added many debugging printfs.
 *
 **********************************************************************
 */
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>
#if	NeXT
#include <stdlib.h>
#endif	NeXT

#define GETWDERR(s)	strcpy(pathname, (s));
#ifdef	CMUCS
#ifdef	DEBUG
#define DEBUGF(x) x
#else	DEBUG
#define DEBUGF(x)
#endif	DEBUG
#endif	CMUCS

char *strcpy();
static int pathsize = 0;		/* pathname length */
#ifdef	CMUCS
static int rpathsize;			/* root pathname length */
#endif	CMUCS

#if	NeXT
static char *prepend();		/* prepend dirname to pathname */
static char *real_getwd();
#endif

#if	NeXT
char *
getwd(pathname)
	char *pathname;
{
	char *result;
	static int cacheValid = 0;
	static char *cachedName = NULL;

	if (cacheValid) {				/* try the cache */
		struct stat curDirStat, nameStat;

		if ( (stat(".", &curDirStat) >= 0) &&	/* same directory? */
		     (stat(cachedName, &nameStat) >= 0) &&
		     (curDirStat.st_ino == nameStat.st_ino) &&
		     (curDirStat.st_dev == nameStat.st_dev) &&
		     (curDirStat.st_rdev == nameStat.st_rdev) ) {
			
			strcpy(pathname, cachedName);	/* cache hit */
			return pathname;

		}
	}

	result = real_getwd(pathname);			/* do it the hard way */
	if (result != NULL) {
		cacheValid = 1;				/* reload cache */
		if (cachedName == NULL)
			cachedName = (char *) malloc(MAXPATHLEN);
		strcpy(cachedName, pathname);
	}
}
#endif	NeXT

#if	NeXT
static char *
real_getwd(pathname)
#else	NeXT
char *
getwd(pathname)
#endif	NeXT
	char *pathname;
{
	char pathbuf[MAXPATHLEN];		/* temporary pathname buffer */
	char *pnptr = &pathbuf[(sizeof pathbuf)-1]; /* pathname pointer */
	char curdir[MAXPATHLEN];	/* current directory buffer */
	char *dptr = curdir;		/* directory pointer */
#if	NeXT
#else	NeXT
	char *prepend();		/* prepend dirname to pathname */
#endif
	dev_t cdev, rdev;		/* current & root device number */
	ino_t cino, rino;		/* current & root inode number */
	DIR *dirp;			/* directory stream */
	struct direct *dir;		/* directory entry struct */
	struct stat d, dd;		/* file status struct */
#ifdef	CMUCS
	char rootpath[MAXPATHLEN];		/* temporary pathname buffer */
	char *rpptr = &rootpath[(sizeof rootpath)-1]; /* pathname pointer */
	int selfparent;			/* true if found selfparent */
#endif	CMUCS

	pathsize = 0;
	*pnptr = '\0';
	if (stat("/", &d) < 0) {
		GETWDERR("getwd: can't stat /");
		return (NULL);
	}
	rdev = d.st_dev;
	rino = d.st_ino;
	strcpy(dptr, "./");
	dptr += 2;
	if (stat(curdir, &d) < 0) {
		GETWDERR("getwd: can't stat .");
		return (NULL);
	}
#ifdef	CMUCS
	DEBUGF(printf("starting at dev %d, inode %d\n", d.st_dev, d.st_ino);)
	selfparent = 0;
#endif	CMUCS
	for (;;) {
#ifdef	CMUCS
		if (!selfparent)
#endif	CMUCS
		if (d.st_ino == rino && d.st_dev == rdev)
#ifdef	CMUCS
		{
			DEBUGF(printf(" -- we've hit the root\n");)
#endif	CMUCS
			break;		/* reached root directory */
#ifdef	CMUCS
		}
#endif	CMUCS
		cino = d.st_ino;
		cdev = d.st_dev;
		strcpy(dptr, "../");
		dptr += 3;
		if ((dirp = opendir(curdir)) == NULL) {
			GETWDERR("getwd: can't open ..");
			return (NULL);
		}
		fstat(dirp->dd_fd, &d);
#ifdef	CMUCS
		DEBUGF(printf("parent is dev %d, inode %d", d.st_dev, d.st_ino);)
#endif	CMUCS
		if (cdev == d.st_dev) {
			if (cino == d.st_ino) {
#ifdef	CMUCS
				/* reached self parent */
				DEBUGF(printf(" -- we've hit the self parent\n");)
#else	CMUCS
				/* reached root directory */
#endif	CMUCS
				closedir(dirp);
#ifdef	CMUCS
				if (!selfparent) {
					selfparent++;
					d.st_dev = rdev;
					d.st_ino = rino;
					dptr = curdir;
					*dptr++ = '/';
					*dptr = '\0';
					rpathsize = 0;
					*rpptr = '\0';
					continue;
				}
				DEBUGF(printf("rootpath %s\n", rpptr);)
				DEBUGF(printf("pathbuf  %s\n", pnptr);)
				triminitial(&rpptr, &pnptr);
				DEBUGF(printf("rootpath %s\n", rpptr);)
				DEBUGF(printf("pathbuf  %s\n", pnptr);)
				for (; *rpptr != 0; rpptr++)
					if (*rpptr == '/')
						pnptr = prepend("../", pnptr, &pathsize);
				DEBUGF(printf("rootpath %s\n", rpptr);)
				DEBUGF(printf("pathbuf  %s\n", pnptr);)
#endif	CMUCS
				break;
			}
#ifdef	CMUCS
			DEBUGF(printf(" -- same device");)
#endif	CMUCS
			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					return (NULL);
				}
			} while (dir->d_ino != cino);
		} else
#ifdef	CMUCS
		{
			DEBUGF(printf(" -- different device");)
#endif	CMUCS
			do {
				if ((dir = readdir(dirp)) == NULL) {
					closedir(dirp);
					GETWDERR("getwd: read error in ..");
					return (NULL);
				}
				strcpy(dptr, dir->d_name);
#ifdef	CMUCS
				if (lstat(curdir, &dd) < 0)
					(void)stat(curdir, &dd);
#else	CMUCS
				lstat(curdir, &dd);
#endif	CMUCS
			} while(dd.st_ino != cino || dd.st_dev != cdev);
#ifdef	CMUCS
		}
#endif	CMUCS
#ifdef	CMUCS
		DEBUGF(printf(" -- child is \"%s\"\n",dir->d_name);)
		if (selfparent) {
			rpptr = prepend(dir->d_name, prepend("/", rpptr, &rpathsize), &rpathsize);
			continue;
		}
		pnptr = prepend(dir->d_name, prepend("/", pnptr, &pathsize), &pathsize);
#else	CMUCS
		pnptr = prepend("/", prepend(dir->d_name, pnptr));
#endif	CMUCS
		closedir(dirp);
	}
	if (*pnptr == '\0')		/* current dir == root dir */
		strcpy(pathname, "/");
	else
#ifdef	CMUCS
	{
		pnptr = prepend("/", pnptr, &pathsize);
		pathbuf[(sizeof pathbuf)-2] = '\0';
#endif	CMUCS
		strcpy(pathname, pnptr);
#ifdef	CMUCS
	}
#endif	CMUCS
	return (pathname);
}

/*
 * prepend() tacks a directory name onto the front of a pathname.
 */
static char *
#ifdef	CMUCS
prepend(dirname, pathname, pathsizep)
#else	CMUCS
prepend(dirname, pathname)
#endif	CMUCS
	register char *dirname;
	register char *pathname;
#ifdef	CMUCS
	register int *pathsizep;
#endif	CMUCS
{
	register int i;			/* directory name size counter */

	for (i = 0; *dirname != '\0'; i++, dirname++)
		continue;
#ifdef	CMUCS
	if ((*pathsizep += i) < MAXPATHLEN)
#else	CMUCS
	if ((pathsize += i) < MAXPATHLEN)
#endif	CMUCS
		while (i-- > 0)
			*--pathname = *--dirname;
	return (pathname);
}
#ifdef	CMUCS
static
triminitial(rootpp, pathpp)
char **rootpp, **pathpp;
{
	register char *rootptr;
	register char *pathptr;
	register int rc, pc;

	rootptr = *rootpp;
	pathptr = *pathpp;
	for (;;) {
		rc = *rootptr++&0177;
		pc = *pathptr++&0177;
		if (rc == 0 || pc == 0) {
			if ((rc + pc) == 0 || (rc + pc) == '/') {
				*pathpp = pathptr;
				*rootpp = rootptr;
			}
			break;
		}
		if (rc != pc)
			break;
		if (rc == '/') {
			*pathpp = pathptr;
			*rootpp = rootptr;
		}
	}
}
#endif	CMUCS

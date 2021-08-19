#ifndef lint
static char sccsid[] = 	"@(#)where.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 *  where.c - get full pathname including host:
 *
 * Copyright (c) 1985 Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <mntent.h>
#include <sys/param.h>
#include <sys/stat.h>

extern errno, sys_nerr;
extern char *sys_errlist[];
#define errstr() (errno < sys_nerr ? sys_errlist[errno] : "unknown error")

/*
 * where(pn, host, fsname, within)
 *
 * pn is the pathname we are looking for,
 * host gets the name of the host owning the file system,
 * fsname gets the file system name on the host,
 * within gets whatever is left from the pathname
 *
 * Returns: 0 if ERROR, 1 if OK
 */
where(pn, host, fsname, within)
	char *pn;
	char *host;
	char *fsname;
	char *within;
{
	struct stat sb;
	char curdir[MAXPATHLEN];
	char qualpn[MAXPATHLEN];
	char *p, *rindex();

	if (stat(pn, &sb) < 0) {
		strcpy(within, errstr());
		return (0);
	}
	/*
	 * first get the working directory,
	 */
	if (getwd(curdir) == 0) {
		sprintf(within, "Unable to get working directory (%s)",
			curdir);
		return (0);
	}
	if (chdir(pn) == 0) {
		getwd(qualpn);
		chdir(curdir);
	} else {
		if (p = rindex(pn, '/')) {
			*p = 0;
			chdir(pn);
			(void) getwd(qualpn);
			chdir(curdir);
			strcat(qualpn, "/");
			strcat(qualpn, p+1);
		} else {
			strcpy(qualpn, curdir);
			strcat(qualpn, "/");
			strcat(qualpn, pn);
		}
	}
	return findmount(qualpn, host, fsname, within);
}

/*
 * findmount(qualpn, host, fsname, within)
 *
 * Searches the mount table to find the appropriate file system
 * for a given absolute path name.
 * host gets the name of the host owning the file system,
 * fsname gets the file system name on the host,
 * within gets whatever is left from the pathname
 *
 * Returns: 0 on failure, 1 on success.
 */
findmount(qualpn, host, fsname, within)
	char *qualpn;
	char *host;
	char *fsname;
	char *within;
{
	FILE *mfp;
	char bestname[MAXPATHLEN];
	int bestlen = 0, bestnfs = 0;
	struct mntent *mnt;
   	char *endhost;			/* points past the colon in name */
	extern char *index();
	int i, len;

	for (i = 0; i < 10; i++) {
		mfp = setmntent("/etc/mtab", "r");
		if (mfp != NULL)
			break;
		sleep(1);
	}
	if (mfp == NULL) {
		sprintf(within, "mount table problem");
		return (0);
	}
	while ((mnt = getmntent(mfp)) != NULL) {
		len = preflen(qualpn, mnt->mnt_dir);
		if (qualpn[len] != '/' && qualpn[len] != '\0' && len > 1)
		   /*
		    * If the last matching character is neither / nor 
		    * the end of the pathname, not a real match
		    * (except for matching root, len==1)
		    */
			continue;
		if (len > bestlen) {
			bestlen = len;
			strcpy(bestname, mnt->mnt_fsname);
		}
	}
	endmntent(mfp);
	endhost = index(bestname,':');
	  /*
	   * If the file system was of type NFS, then there should already
	   * be a host name, otherwise, use ours.
	   */
	if (endhost) {
		*endhost++ = 0;
		strcpy(host,bestname);
		strcpy(fsname,endhost);
		  /*
		   * special case to keep the "/" when we match root
		   */
		if (bestlen == 1)
			bestlen = 0;
	} else {
		gethostname(host, 255);
		strncpy(fsname,qualpn,bestlen);
		fsname[bestlen] = 0;
	}
	strcpy(within,qualpn+bestlen);
	return 1;
}

/*
 * Returns: length of second argument if it is a prefix of the
 * first argument, otherwise zero.
 */
preflen(str, pref)
	char *str, *pref;
{
	int len; 

	len = strlen(pref);
	if (strncmp(str, pref, len) == 0)
		return (len);
	return (0);
}

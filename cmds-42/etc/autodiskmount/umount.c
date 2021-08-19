/*
 * umount
 */

#include <sys/param.h>
#include <sys/file.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <stdio.h>
#include <mntent.h>
#include <errno.h>
#include <sys/time.h>
#include <netdb.h>

/* exit codes */
#define	E_OK		0	/* normal exit */
#define	E_UNEXP		1	/* unexpected error */
#define	E_NOT_OD	2	/* device not an optical drive */
#define	E_FAIL		3	/* operation failed (expected) */

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mtab.
 */
struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

char	*xmalloc();
char	*index();
struct mntlist *mkmntlist();
struct mntent *mntdup();

struct stat statb;

extern	int errno;

umount(mount_pt,ejectAfter,dev)
	char *mount_pt;
	int ejectAfter;
	dev_t dev;
{
	int i, pid, rtn = 0;
	int didit;
	struct mntent *mnt;
	struct mntlist *mntl,*firstmnt, *mntcur = (struct mntlist *)NULL;
	struct mntlist *mntrev = NULL;
	int tmpfd;
	char *colon;
	FILE *tmpmnt;
#ifdef NeXT
	char linkname[MAXPATHLEN+1];
	char tmpname[MAXPATHLEN+1];
	extern char *lnresolve();
#else
	char *tmpname = "/etc/umountXXXXXX";
#endif NeXT

/*	sync(); */
	umask(0);
#ifdef NeXT
	strcpy(linkname, lnresolve(MOUNTED));
	strcpy(tmpname, linkname);
	if (strlen(tmpname)+6 > MAXPATHLEN) {
		return ENAMETOOLONG;
	}
	strcat(tmpname, "XXXXXX");
#endif NeXT
	mktemp(tmpname);
	
	/* yes, create /etc/mtab mode 664! */
	if ((tmpfd = open(tmpname, O_RDWR|O_CREAT|O_TRUNC, 0664)) < 0) {
	        rtn = errno;
		panic (tmpname);
		return rtn;
	}
	close(tmpfd);
	tmpmnt = setmntent(tmpname, "w");
	if (tmpmnt == NULL) {
	        rtn = errno;
		if (!rtn)
		        rtn = ENOENT;
		panic (tmpname);
		return rtn;
	}
	/*
	 * get a last first list of mounted stuff, reverse list and
	 * null out entries that get unmounted.
	 */
	 
	firstmnt = mkmntlist(MOUNTED);
	if (!firstmnt) {
	        rtn = errno;
		endmntent(tmpmnt);
		return rtn;
	}
	for (mntl = firstmnt; mntl != NULL;
	    mntcur = mntl, mntl = mntl->mntl_next,
	    mntcur->mntl_next = mntrev, mntrev = mntcur) {
		mnt = mntl->mntl_mnt;
		if (strcmp(mnt->mnt_dir, "/") == 0) {
			continue;
		}
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0) {
			continue;
		}

		if ((strcmp(mnt->mnt_dir, mount_pt) == 0) ||
		    (strcmp(mnt->mnt_fsname, mount_pt) == 0) ) {
			if (unmount(mnt->mnt_dir) < 0) {
				rtn = errno;
			} else {
				free(mntl->mntl_mnt->mnt_fsname);
				free(mntl->mntl_mnt->mnt_dir);
				free(mntl->mntl_mnt->mnt_opts);
				free(mntl->mntl_mnt);
				mntl->mntl_mnt = NULL;
			}
		}
	}
        if (ejectAfter && rtn == 0)
	    eject(dev);
	
	/*
	 * Build new temporary mtab by walking mnt list
	 */
	for (; mntcur != NULL; mntcur = mntcur->mntl_next) {
		if (mntcur->mntl_mnt) {
			addmntent(tmpmnt, mntcur->mntl_mnt);
		}
	}
	endmntent(tmpmnt);
	freemntlist(firstmnt);

	if (rename(tmpname, linkname) < 0) {
		panic (MOUNTED);
		rtn = errno;
	}
	return (rtn);
}

char *
xmalloc(size)
	int size;
{
	char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		log ("umount: ran out of memory!\n");
		exit(E_UNEXP);
	}
	return (ret);
}

freemntlist(mntl)
	struct mntlist *mntl;
{
	struct mntlist *nextmntl;
	struct mntent *ment;
	
	while (mntl) {
		ment = mntl->mntl_mnt;
		if (ment) {
		    free(ment->mnt_fsname);
		    free(ment->mnt_dir);
		    free(ment->mnt_opts);
		    free(ment);
		}	
		nextmntl = mntl->mntl_next;
		free(mntl);
	        mntl = nextmntl;
	}
}



struct mntent *
mntdup(mnt)
	struct mntent *mnt;
{
	struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

struct mntlist *
mkmntlist(file)
	char *file;
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	mounted = setmntent(MOUNTED, "r");
	if (mounted == NULL) {
		panic (MOUNTED);
		return NULL;
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	endmntent (mounted);
	return(mntst);
}


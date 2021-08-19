/*
 * mount
 */
#include <sys/types.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <rpc/rpc.h>
#include <sys/errno.h>
#include <sys/time.h>
#include <sys/vfs.h>
#include <sys/file.h>
#include <nfs/nfs.h>
#include <rpcsvc/mount.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netdb.h>
#include <stdio.h>
#include <mntent.h>
#include <sys/mount.h>
#import <sys/dir.h>

int	ro = 0;
int	freq = 1;
int	passno = 2;
extern	char raw_part[];
extern	int userId;

extern int errno;

char	*index(), *rindex();
char	type[MNTMAXSTR];
char	opts[MNTMAXSTR];
char	sblock[SBSIZE];
struct	stat sb;

#define TOUCH_ME_HERE(p) { extern int p();\
volatile char *foo___,c___;\
foo___ = (char *) &p;\
c___ = *foo___;\
c___ = foo___[500];}

/*
 * Structure used to build a mount tree.  The tree is traversed to do
 * the mounts and catch dependancies
 */
struct mnttree {
	struct mntent *mt_mnt;
	struct mnttree *mt_sib;
	struct mnttree *mt_kid;
};
struct mnttree *maketree();

struct fs *fs = (struct fs*) sblock;

etc_mount(name, dir, read_only)
	char *name, *dir;
{
	struct mntent mnt;

	strcpy(type, MNTTYPE_43);		/* default type = 4.3 */
	strcpy(opts, read_only ? MNTOPT_RO : MNTOPT_RW);
	strcat(opts, ",");
	strcat(opts, MNTOPT_NOQUOTA);
	mnt.mnt_fsname = name;
	mnt.mnt_dir = dir;
	mnt.mnt_type = type;
	mnt.mnt_opts = opts;
	mnt.mnt_freq = freq;
	mnt.mnt_passno = passno;
	return mounttree(maketree(NULL, &mnt));
}

/*
 * attempt to mount file system, return errno or 0
 */
mountfs(print, mnt)
	int print;
	struct mntent *mnt;
{
	int error, od_fd, old_euid;
	extern int errno;
	int type = -1;
	int flags = 0;
	static char opts[1024];
	char *optp, *optend;
	union data {
		struct ufs_args	ufs_args;
	} data;
	
	if (mounted(mnt)) {
		if (print) {
			fprintf(stderr, "mount: %s already mounted\n",
			    mnt->mnt_fsname);
		}
		return (0);
	}
	if (strcmp(mnt->mnt_type, MNTTYPE_43) == 0) {
		type = MOUNT_UFS;
		error = mount_43(mnt, &data.ufs_args);
	} else {
		fprintf(stderr,
		    "mount: unknown file system type %s\n",
		    mnt->mnt_type);
		error = EINVAL;
	}

	if (error) {
		return (error);
	}

	/* touch superblock buffer to page it in while vol #0 is still in */
	bzero (sblock, sizeof (sblock));

	/* same with these library routines.. */
	TOUCH_ME_HERE(lseek)
	TOUCH_ME_HERE(read)
	TOUCH_ME_HERE(open)
	
	flags |= (hasmntopt(mnt, MNTOPT_RO) == NULL) ? 0 : M_RDONLY;
	flags |= (hasmntopt(mnt, MNTOPT_NOSUID) == NULL) ? 0 : M_NOSUID;

	/* bring raw_part name into the dnlc so it doesn't fault after mount */
	stat (raw_part, &sb);

	/* set effective user id */
	old_euid = geteuid();
	setreuid (-1, userId);
	
	/* mount starts with vol #0 inserted and ends with vol #1 inserted */
	if (mount(type, mnt->mnt_dir, flags, &data) < 0) {
		if (print) {
			fprintf(stderr, "mount: %s on ", mnt->mnt_fsname);
			perror(mnt->mnt_dir);
		}
		setreuid (-1, old_euid);
		return (errno);
	}
	setreuid (-1, old_euid);
	
	/* see if the filesystem is dirty and requiring a fsck */
	if ((od_fd = open (raw_part, O_RDONLY)) < 0)
		return (errno);
	if (lseek (od_fd, dbtob(SBLOCK), L_SET) < 0) {
	        close (od_fd);
		return (errno);
	}
	if (read (od_fd, fs, SBSIZE) < 0) {
	        close (od_fd);
		return (errno);
	}

	/* to avoid some disk swaps */
	getFiles(mnt->mnt_dir);
	
	/* this will write to vol #0 */
	addtomtab(mnt);
	
	/* close of vol #1 wants vol #0 inserted for some reason */
	close (od_fd);
	return (fs->fs_state == FS_STATE_CORRUPTED? -1 : 0);
}

mount_43(mnt, args)
	struct mntent *mnt;
	struct ufs_args *args;
{
	static char name[MNTMAXSTR];

	strcpy(name, mnt->mnt_fsname);
	args->fspec = name;
	return (0);
}

struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

/*
 * Check to see if mntck is already mounted.
 * We have to be careful because getmntent modifies its static struct.
 */
mounted(mntck)
	struct mntent *mntck;
{
	int found = 0;
	struct mntent *mnt;
	struct mntlist *mntl;
	extern struct mntlist *mountList;

	for (mntl = mountList; mntl; mntl = mntl->mntl_next) {
		mnt = mntl->mntl_mnt;
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0)
			continue;
		if ((mnt && strcmp(mntck->mnt_fsname, mnt->mnt_fsname) == 0) &&
		    (strcmp(mntck->mnt_dir, mnt->mnt_dir) == 0) ) {
			found = 1;
			break;
		}
	}
	return (found);
}

mntcp(mnt1, mnt2)
	struct mntent *mnt1, *mnt2;
{
	static char fsname[MAXPATHLEN], dir[MAXPATHLEN], type[MAXPATHLEN],
		opts[MAXPATHLEN];

	mnt2->mnt_fsname = fsname;
	strcpy(fsname, mnt1->mnt_fsname);
	mnt2->mnt_dir = dir;
	strcpy(dir, mnt1->mnt_dir);
	mnt2->mnt_type = type;
	strcpy(type, mnt1->mnt_type);
	mnt2->mnt_opts = opts;
	strcpy(opts, mnt1->mnt_opts);
	mnt2->mnt_freq = mnt1->mnt_freq;
	mnt2->mnt_passno = mnt1->mnt_passno;
}

/*
 * update /etc/mtab
 */
addtomtab(mnt)
	struct mntent *mnt;
{
	FILE *mnted;

	mnted = setmntent(MOUNTED, "r+");
	if (mnted == NULL) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	if (addmntent(mnted, mnt)) {
		fprintf(stderr, "mount: ");
		perror(MOUNTED);
		exit(1);
	}
	endmntent(mnted);
}

/*
 * Build the mount dependency tree
 */
struct mnttree *
maketree(mt, mnt)
	struct mnttree *mt;
	struct mntent *mnt;
{
	if (mt == NULL) {
		mt = (struct mnttree *)xmalloc(sizeof (struct mnttree));
		mt->mt_mnt = (struct mntent*) mntdup(mnt);
		mt->mt_sib = NULL;
		mt->mt_kid = NULL;
	} else {
		if (substr(mt->mt_mnt->mnt_dir, mnt->mnt_dir)) {
			mt->mt_kid = maketree(mt->mt_kid, mnt);
		} else {
			mt->mt_sib = maketree(mt->mt_sib, mnt);
		}
	}
	return (mt);
}

mounttree(mt)
	struct mnttree *mt;
{
	int error, dirty = 0;

	if (mt) {
		mounttree(mt->mt_sib);
		error = mountfs(0, mt->mt_mnt);
		if (error == -1)
			dirty = -1, error = 0;
		if (!error) {
			mounttree(mt->mt_kid);
		} else {
			fprintf(stderr, "mount: giving up on:\n");
			fprintf(stderr, "   %s\n", mt->mt_mnt->mnt_dir);
		}
	}
	return (error > 0? error : dirty);
}

/*
 * Returns true if s1 is a pathname substring of s2.
 */
substr(s1, s2)
	char *s1;
	char *s2;
{
	while (*s1 == *s2) {
		s1++;
		s2++;
	}
	if (*s1 == '\0' && *s2 == '/') {
		return (1);
	}
	return (0);
}


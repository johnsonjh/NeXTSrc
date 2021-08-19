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
static char sccsid[] = 	"@(#)df.c	1.3 88/06/17 4.0NFSSRC SMI"; /* from UCB 5.1 4/30/85 */
						/* @(#) from SUN 1.22    */
#endif not lint

#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <errno.h>

#include <stdio.h>
#include <mntent.h>
#if	NeXT
#include <stdlib.h>
#include <string.h>
#include <sys/file.h>
#include <sys/ioctl.h>

int termwidth;
#endif	NeXT

/*
 * df
 */
char	*mpath();
int	iflag;
int	aflag;
int	type;
char	*typestr;

struct mntent *getmntpt(), *mntdup();

union {
	struct fs iu_fs;
	char dummy[SBSIZE];
} sb;
#define sblock sb.iu_fs

daddr_t	alloc();
char	*strcpy();

/*
 * This structure is used to build a list of mntent structures
 * in reverse order from /etc/mtab.
 */
struct mntlist {
	struct mntent *mntl_mnt;
	struct mntlist *mntl_next;
};

struct mntlist *mkmntlist();
struct mntent *mntdup();

main(argc, argv)
	int argc;
	char **argv;
{
	register struct mntent *mnt;

	termwidth = termsize();
	argc--, argv++;
	while (argc > 0 && argv[0][0]=='-') {
		switch (argv[0][1]) {

		case 'i':
			iflag++;
			break;

		case 't':
			type++;
			argv++;
			argc--;
			if (argc <= 0)
				fprintf(stderr, "usage: df [ -i ] [-a] [-t type | file... ]\n");
			typestr = *argv;
			break;

		case 'a':
			aflag++;
			break;

		default:
			fprintf(stderr, "usage: df [ -i ] [-a] [-t type | file... ]\n");
			exit(0);
		}
		argc--, argv++;
	}
	if (argc > 0 && type) {
		fprintf(stderr, "usage: df [ -i ] [-a] [-t type | file... ]\n");
		exit(0);
	}
	sync();
	if (iflag)
		printf("Filesystem             iused   ifree  %%iused");
	else
		printf("Filesystem            kbytes    used   avail capacity");
	printf("  Mounted on\n");
	if (argc <= 0) {
		register FILE *mtabp;

		if ((mtabp = setmntent(MOUNTED, "r")) == NULL) {
			(void) fprintf(stderr, "df: ");
			perror(MOUNTED);
			exit(1);
		}
		while ((mnt = getmntent(mtabp)) != NULL) {
			if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
			    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
				continue;
			if (type && strcmp(typestr, mnt->mnt_type) != 0) {
				continue;
			}
			dfreemnt(mnt->mnt_dir, mnt);
		}
		(void) endmntent(mtabp);
	} else {
		int num = argc ;
		int i ;
		struct mntlist *mntl ;

		aflag++;
		/*
		 * Reverse the list and start comparing.
		 */
		for (mntl = mkmntlist(); mntl != NULL && num ; 
				mntl = mntl->mntl_next) {
		   struct stat dirstat, filestat ;

		   mnt = mntl->mntl_mnt ;
		   if (stat(mnt->mnt_dir, &dirstat)<0) {
			continue ;
		   }
		   for (i = 0; i < argc; i++) {
			if (argv[i]==NULL) continue ;
			if (stat(argv[i], &filestat) < 0) {
				(void) fprintf(stderr, "df: ");
				perror(argv[i]);
				argv[i] = NULL ;
				--num;
			} else {
			       if ((filestat.st_mode & S_IFMT) == S_IFBLK ||
			          (filestat.st_mode & S_IFMT) == S_IFCHR) {
					char *cp ;

					cp = mpath(argv[i]);
					if (*cp == '\0') {
						dfreedev(argv[i]);
						argv[i] = NULL ;
						--num;
						continue;
					}
					else {
					  if (stat(cp, &filestat) < 0) {
						(void) fprintf(stderr, "df: ");
						perror(argv[i]);
						argv[i] = NULL ;
						--num;
						continue ;
					  }
					}
				}
				if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
				    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
					continue;
				if ((filestat.st_dev == dirstat.st_dev) &&
				    (!type || strcmp(typestr, mnt->mnt_type)==0)){
					dfreemnt(mnt->mnt_dir, mnt);
					argv[i] = NULL ;
					--num ;
				}
			}
		}
	     }
	     if (num) {
		     for (i = 0; i < argc; i++) 
			if (argv[i]==NULL) 
				continue ;
			else
			   (void) fprintf(stderr, 
				"Could not find mount point for %s\n", argv[i]) ;
	     }
	}
	exit(0);
	/*NOTREACHED*/
}

dfreedev(file)
	char *file;
{
	long totalblks, availblks, avail, free, used;
	int fi;

	fi = open(file, 0);
	if (fi < 0) {
		perror(file);
		return;
	}
	if (bread(file, fi, SBLOCK, (char *)&sblock, SBSIZE) == 0) {
		(void) close(fi);
		return;
	}
	if (sblock.fs_magic != FS_MAGIC) {
		(void) fprintf(stderr, "df: %s: not a UNIX filesystem\n", 
		    file);
		(void) close(fi);
		return;
	}
#if	NeXT
	if (termwidth > 80)
		printf("%-*.*s", (termwidth - 80), (termwidth - 80), file);
	else
		printf("%-20.20s", file);
#else	
	(void) printf("%-20.20s", file);
#endif	NeXT
	totalblks = sblock.fs_dsize;
	free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
	    sblock.fs_cstotal.cs_nffree;
	used = totalblks - free;
	availblks = totalblks * (100 - sblock.fs_minfree) / 100;
	avail = availblks > used ? availblks - used : 0;
	printf("%8d%8d%8d",
	    totalblks * sblock.fs_fsize / 1024,
	    used * sblock.fs_fsize / 1024,
	    avail * sblock.fs_fsize / 1024);
	printf("%6.0f%%",
	    availblks == 0 ? 0.0 : (double) used / (double) availblks * 100.0);
	if (iflag) {
		int inodes = sblock.fs_ncg * sblock.fs_ipg;
		used = inodes - sblock.fs_cstotal.cs_nifree;
		printf("%8ld%8ld%6.0f%% ", used, sblock.fs_cstotal.cs_nifree,
		    inodes == 0 ? 0.0 : (double)used / (double)inodes * 100.0);
	} else {
		totalblks = sblock.fs_dsize;
		free = sblock.fs_cstotal.cs_nbfree * sblock.fs_frag +
		    sblock.fs_cstotal.cs_nffree;
		used = totalblks - free;
		availblks = totalblks * (100 - sblock.fs_minfree) / 100;
		avail = availblks > used ? availblks - used : 0;
		(void) printf("%8d%8d%8d",
		    totalblks * sblock.fs_fsize / 1024,
		    used * sblock.fs_fsize / 1024,
		    avail * sblock.fs_fsize / 1024);
		(void) printf("%6.0f%%",
		    availblks==0? 0.0: (double)used/(double)availblks * 100.0);
		(void) printf("  ");
	}
	(void) printf("  %s\n", mpath(file));
	(void) close(fi);
}

dfreemnt(file, mnt)
	char *file;
	struct mntent *mnt;
{
#if	NeXT
	int expand = termwidth - 80;
	char spaces [1024];
#endif	NeXT
	struct statfs fs;

	if (statfs(file, &fs) < 0) {
		perror(file);
		return;
	}

	if (!aflag && fs.f_blocks == 0) {
		return;
	}
#if	NeXT
	if (strlen(mnt->mnt_fsname) > (20 + expand)) {
		printf("%s\n", mnt->mnt_fsname);
		sprintf (spaces, "%*c", 20 + expand, ' ');
		printf("%s", spaces);
	} else {
		expand += 20;
		printf("%-*.*s", expand, expand, mnt->mnt_fsname);
	}
#else	
	if (strlen(mnt->mnt_fsname) > 20) {
		printf("%s\n", mnt->mnt_fsname);
		printf("                    ");
	} else {
		printf("%-20.20s", mnt->mnt_fsname);
	}
#endif	NeXT
	if (iflag) {
		long files, used;

		files = fs.f_files;
		used = files - fs.f_ffree;
		(void) printf("%8ld%8ld%6.0f%% ", used, fs.f_ffree,
		    files == 0? 0.0: (double)used / (double)files * 100.0);
	} else {
		long totalblks, avail, free, used, reserved;

		totalblks = fs.f_blocks;
		free = fs.f_bfree;
		used = totalblks - free;
		avail = fs.f_bavail;
		reserved = free - avail;
		if (avail < 0)
			avail = 0;
		(void) printf("%8d%8d%8d", totalblks * fs.f_bsize / 1024,
		    used * fs.f_bsize / 1024, avail * fs.f_bsize / 1024);
		totalblks -= reserved;
		(void) printf("%6.0f%%",
		    totalblks==0? 0.0: (double)used/(double)totalblks * 100.0);
		(void) printf("  ");
	}
	printf("  %s\n", mnt->mnt_dir);
}

/*
 * Given a name like /usr/src/etc/foo.c returns the mntent
 * structure for the file system it lives in.
 */
struct mntent *
getmntpt(file)
	char *file;
{
	FILE *mntp;
	struct mntent *mnt, *mntsave;
	struct stat filestat, dirstat;

	if (stat(file, &filestat) < 0) {
		perror(file);
		return(NULL);
	}

	if ((mntp = setmntent(MOUNTED, "r")) == 0) {
		perror(MOUNTED);
		exit(1);
	}

	mntsave = NULL;
	while ((mnt = getmntent(mntp)) != 0) {
		if (strcmp(mnt->mnt_type, MNTTYPE_IGNORE) == 0 ||
		    strcmp(mnt->mnt_type, MNTTYPE_SWAP) == 0)
			continue;
		if ((stat(mnt->mnt_dir, &dirstat) >= 0) &&
		   (filestat.st_dev == dirstat.st_dev)) {
			mntsave = mntdup(mnt);
		}
	}
	(void) endmntent(mntp);
	if (mntsave) {
		return(mntsave);
	} else {
		fprintf(stderr, "Couldn't find mount point for %s\n", file);
		exit(1);
	}
	/*NOTREACHED*/
}


long lseek();

bread(file, fi, bno, buf, cnt)
	char *file;
	int fi;
	daddr_t bno;
	char *buf;
	int cnt;
{
	int n;
	extern errno;

	(void) lseek(fi, (long)(bno * DEV_BSIZE), 0);
	if ((n=read(fi, buf, cnt)) != cnt) {
		/* probably a dismounted disk if errno == EIO */
		if (errno != EIO) {
			printf("\nread error bno = %ld\n", bno);
			printf("count = %d; errno = %d\n", n, errno);
		}
		return (0);
	}
	return (1);
}

/*
 * Given a name like /dev/rrp0h, returns the mounted path, like /usr.
 */
char *
mpath(file)
	char *file;
{
        FILE *mntp;
        register struct mntent *mnt;
	char *raw_to_cooked ();

        if ((mntp = setmntent(MOUNTED, "r")) == 0) {
                (void) fprintf(stderr, "df: ");
                perror(MOUNTED);
                exit(1);
        }
 
        while ((mnt = getmntent(mntp)) != 0) {
#if	NeXT	
                if (strcmp(file, mnt->mnt_fsname) == 0 ||
		    	strcmp(raw_to_cooked(file), mnt->mnt_fsname) == 0) {
#else
                if (strcmp(file, mnt->mnt_fsname) == 0) {
#endif	NeXT
                        (void) endmntent(mntp);
                        return (mnt->mnt_dir);
                }
        }
        (void) endmntent(mntp);
	return "";
}

#if	NeXT
char *raw_to_cooked (char *filename) 
{
	char *ret;
	
	ret = calloc (1, strlen (filename) + 2);
	if (index (filename, '/') == 0) 
		return (filename);
	else if (index (filename, '/') != filename)
		return (filename);
	else if (strncmp (filename + 1, "dev", 3) != 0)
		return (filename);
	else {
		strncpy (ret, filename, strlen ("/dev/"));
		strcat (ret, rindex (filename, '/') + 2);
		ret[strlen (filename) + 1] = '\0';
		return (ret);
	}
}
#endif	NeXT

char *
xmalloc(size)
	unsigned int size;
{
	register char *ret;
	
	if ((ret = (char *)malloc(size)) == NULL) {
		(void) fprintf(stderr, "umount: ran out of memory!\n");
		exit(1);
	}
	return (ret);
}

struct mntent *
mntdup(mnt)
	register struct mntent *mnt;
{
	register struct mntent *new;

	new = (struct mntent *)xmalloc(sizeof(*new));

	new->mnt_fsname = (char *)xmalloc(strlen(mnt->mnt_fsname) + 1);
	(void) strcpy(new->mnt_fsname, mnt->mnt_fsname);

	new->mnt_dir = (char *)xmalloc(strlen(mnt->mnt_dir) + 1);
	(void) strcpy(new->mnt_dir, mnt->mnt_dir);

	new->mnt_type = (char *)xmalloc(strlen(mnt->mnt_type) + 1);
	(void) strcpy(new->mnt_type, mnt->mnt_type);

	new->mnt_opts = (char *)xmalloc(strlen(mnt->mnt_opts) + 1);
	(void) strcpy(new->mnt_opts, mnt->mnt_opts);

	new->mnt_freq = mnt->mnt_freq;
	new->mnt_passno = mnt->mnt_passno;

	return (new);
}

struct mntlist *
mkmntlist()
{
	FILE *mounted;
	struct mntlist *mntl;
	struct mntlist *mntst = NULL;
	struct mntent *mnt;

	if ((mounted = setmntent(MOUNTED, "r"))== NULL) {
		(void) fprintf(stderr, "df : ") ;
		perror(MOUNTED);
		exit(1);
	}
	while ((mnt = getmntent(mounted)) != NULL) {
		mntl = (struct mntlist *)xmalloc(sizeof(*mntl));
		mntl->mntl_mnt = mntdup(mnt);
		mntl->mntl_next = mntst;
		mntst = mntl;
	}
	(void) endmntent(mounted);
	return(mntst);
}

#if	NeXT
int termsize (void)
{
	char bp[1024];
	int width, fd;
	struct winsize ws;

	if (getenv("TERM") == NULL) {
		width = 80;
	} 
	else {
		tgetent(bp, getenv("TERM"));
		width = tgetnum("co");
		fd = open ("/dev/tty", O_RDONLY, 0);
		if ((fd != -1) && (ioctl (fd, TIOCGWINSZ, &ws) != -1)) {
			if (ws.ws_col > width)
				width = ws.ws_col;
			(void) close (fd);
		}
	}
	return (width);
}
#endif	NeXT
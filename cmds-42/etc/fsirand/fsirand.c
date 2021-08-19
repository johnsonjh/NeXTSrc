/* @(#)fsirand.c	1.1 87/09/21 3.2/4.3NFSSRC */
#ifndef lint
static  char sccsid[] = "@(#)fsirand.c        1.2 88/07/11 4.0NFSSRC; from 1.5 88/02/07 Copyr 1984 Sun Micro";
#endif

/*
 * fsirand - Copyright (c) 1984 Sun Microsystems.
 */

#include <stdio.h>
#include <sys/param.h>
#include <sys/time.h>
#include <ufs/fs.h>
#include <sys/vnode.h>
#include <ufs/inode.h>

char fsbuf[SBSIZE];
struct dinode dibuf[8192/sizeof (struct dinode)];
extern int errno;

main(argc, argv)
int	argc;
char	*argv[];
{
	struct fs *fs;
	int fd;
	char *dev;
	int bno;
	struct dinode *dip;
	int inum, imax;
	int n;
	int seekaddr, bsize;
	int pflag = 0;
	struct timeval timeval;

	argv++;
	argc--;
	if (argc > 0 && strcmp(*argv, "-p") == 0) {
		pflag++;
		argv++;
		argc--;
	}
	if (argc <= 0) {
		fprintf(stderr, "Usage: fsirand [-p] special\n");
		exit(1);
	}
	dev = *argv;
	fd = open(dev, pflag ? 0 : 2);
	if (fd == -1) {
		fprintf(stderr, "Cannot open %s\n", dev);
		exit(1);
	}
	if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
		fprintf(stderr, "Seek to superblock failed\n");
		exit(1);
	}
	fs = (struct fs *) fsbuf;
	if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
		fprintf(stderr, "Read of superblock failed %d\n", errno);
		exit(1);
	}
	if (fs->fs_magic != FS_MAGIC) {
		fprintf(stderr, "Not a superblock\n");
		exit(1);
	}
	if (pflag) {
#ifndef	NeXT_MOD
		printf("fsid: %x %x\n", fs->fs_id[0], fs->fs_id[1]);
#endif	!NeXT_MOD
	} else {
		n = getpid();
		srandom(timeval.tv_sec + timeval.tv_usec + n);
		while (n--) {
			random();
		}
	}
	bsize = INOPB(fs) * sizeof (struct dinode);
	inum = 0;
	imax = fs->fs_ipg * fs->fs_ncg;
	while (inum < imax) {
		bno = itod(fs, inum);
		seekaddr = fsbtodb(fs, bno) * DEV_BSIZE;
		if (lseek(fd, seekaddr, 0) == -1) {
			fprintf(stderr, "lseek to %d failed\n", seekaddr);
			exit(1);
		}
		n = read(fd, (char *) dibuf, bsize);
		if (n != bsize) {
			printf("premature EOF\n");
			exit(1);
		}
		for (dip = dibuf; dip < &dibuf[INOPB(fs)]; dip++) {
			if (pflag) {
				printf("ino %d gen %x\n", inum, dip->di_gen);
			} else {
				dip->di_gen = random();
			}
			inum++;
		}
		if (!pflag) {
			if (lseek(fd, seekaddr, 0) == -1) {
				fprintf(stderr, "lseek to %d failed\n",
				    seekaddr);
				exit(1);
			}
			n = write(fd, (char *) dibuf, bsize);
			if (n != bsize) {
				printf("premature EOF\n");
				exit(1);
			}
		}
	}
	if (!pflag) {
		gettimeofday(&timeval, 0);
#ifndef	NeXT_MOD
		fs->fs_id[0] = timeval.tv_sec;
		fs->fs_id[1] = timeval.tv_usec + getpid();
#endif	!NeXT_MOD
		if (lseek(fd, SBLOCK * DEV_BSIZE, 0) == -1) {
			fprintf(stderr, "Seek to superblock failed\n");
			exit(1);
		}
		if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
			fprintf(stderr, "Write of superblock failed %d\n",
			    errno);
			exit(1);
		}
	}
	for (n = 0; n < fs->fs_ncg; n++ ) {
		seekaddr = fsbtodb(fs, cgsblock(fs, n)) * DEV_BSIZE;
		if (lseek(fd,  seekaddr, 0) == -1) {
			fprintf(stderr, "Seek to alt superblock failed\n");
			exit(1);
		}
		if (pflag) {
			if (read(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Read of  alt superblock failed %d %d\n",
				    errno, seekaddr);
				exit(1);
			}
			if (fs->fs_magic != FS_MAGIC) {
				fprintf(stderr, "Not an alt superblock\n");
				exit(1);
			}
		} else {
			if (write(fd, (char *) fs, SBSIZE) != SBSIZE) {
				fprintf(stderr,
				    "Write of alt superblock failed\n");
				exit(1);
			}
		}
	}
	exit(0);
}

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
static char sccsid[] = 	"@(#)main.c	1.6 88/05/23 4.0NFSSRC SMI"; /* from UCB 5.4 3/5/86 */
						/* @(#) from SUN 1.32   */
#endif not lint

#include <stdio.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/time.h>
#include <sys/vnode.h>
#include <ufs/inode.h>
#include <sys/stat.h>
#include <sys/wait.h>
#define KERNEL
#include <ufs/fsdir.h>
#undef KERNEL
#include <mntent.h>
#include <strings.h>
#include "fsck.h"

int	exitstat;	/* exit status (set to 8 if 'No' response) */
char	*rawname(), *unrawname(), *blockcheck();
int	catch(), catchquit(), voidquit();
int	returntosingle;
int	(*signal())();

main(argc, argv)
	int	argc;
	char	*argv[];
{
	int pid, passno, anygtr, sumstatus;
	char *name;
#if	NeXT
	Pflag = 0;
#endif	NeXT

	sync();
	while (--argc > 0 && **++argv == '-') {
		switch (*++*argv) {
		case 'p':
			preen++;
			break;
#if	NeXT
		case 'P':
		    	Pflag++;
			preen++;
			break;
#endif NeXT
		case 'b':
			if (argv[0][1] != '\0') {
				bflag = atoi(argv[0]+1);
			} else {
				bflag = atoi(*++argv);
				argc--;
			}
			printf("Alternate super block location: %d\n", bflag);
			break;

		case 'd':
			debug++;
			break;

		case 'n':	/* default no answer flag */
		case 'N':
			nflag++;
			yflag = 0;
			break;

		case 'y':	/* default yes answer flag */
		case 'Y':
			yflag++;
			nflag = 0;
			break;

		default:
			errexit("%c option?\n", **argv);
		}
	}
	if (signal(SIGINT, SIG_IGN) != SIG_IGN)
		(void)signal(SIGINT, catch);
	if (preen)
		(void)signal(SIGQUIT, catchquit);
	if (argc) {
		while (argc-- > 0) {
			checkfilesys(*argv);
			argv++;
		}
		exit(exitstat);
	}
	sumstatus = 0;
	passno = 1;
	do {
		FILE *fstab;
		struct mntent *mnt;
		char	*raw;

		anygtr = 0;
		/*
		 *  This might not work.  
		 */
		if ((fstab = setmntent(MNTTAB, "r")) == NULL)
			errexit("Can't open checklist file: %s\n", MNTTAB);
		while ((mnt = getmntent(fstab)) != 0) {
			if (strcmp(mnt->mnt_type, MNTTYPE_43)) {
				continue;
			}
			if (!hasmntopt(mnt,MNTOPT_RW) &&
			    !hasmntopt(mnt,MNTOPT_RO) &&
			    !hasmntopt(mnt,MNTOPT_QUOTA)){
				continue;
			}
			mnt = mntdup(mnt);
			if (preen == 0 ||
			  passno == 1 && mnt->mnt_passno == passno) {
				name = blockcheck(mnt->mnt_fsname);
				if (name != NULL)
					checkfilesys(name);
				else if (preen)
					exit(8);
			} else if (mnt->mnt_passno > passno) 
				anygtr = 1;
			else if (mnt->mnt_passno == passno) {
				pid = fork();
				if (pid < 0) {
					perror("fork");
					exit(8);
				}
				if (pid == 0) {
					(void)signal(SIGQUIT, voidquit);
					name = blockcheck(mnt->mnt_fsname);
					if (name == NULL)
						exit(8);
					checkfilesys(name);
					exit(exitstat);
				}
			}
		}
		endmntent(fstab);
		if (preen) {
			union wait status;
			while (wait(&status) != -1)
				sumstatus |= status.w_retcode;
		}
		passno++;
	} while (anygtr);
	if (sumstatus)
		exit(8);
	(void)endfsent();
	if (returntosingle)
		exit(2);
	exit(exitstat);
}

checkfilesys(filesys)
	char *filesys;
{
	daddr_t n_ffree, n_bfree;
	struct dups *dp;
	struct zlncnt *zlnp;
#if	NeXT
	int	expected_state;
#endif	NeXT

	if ((devname = setup(filesys)) == 0) {
		if (preen)
			pfatal("CAN'T CHECK FILE SYSTEM.");
		return;
	}
#if	NeXT
	/* setup determines if file system is mounted */
	expected_state = (mountedfs && ! readonlyfs)
		? FS_STATE_DIRTY : FS_STATE_CLEAN;
	if (preen && sblock.fs_state == expected_state && !Pflag) {
		pinfo("file system clean: skipping check\n");
		return;
	}
#endif	NeXT
	/*
	 * 1: scan inodes tallying blocks used
	 */
	if (preen == 0) {
		if (mountedfs)
			printf("** Currently Mounted on %s\n", sblock.fs_fsmnt);
		else
			printf("** Last Mounted on %s\n", sblock.fs_fsmnt);
		printf("** Phase 1 - Check Blocks and Sizes\n");
	}
	pass1();

	/*
	 * 1b: locate first references to duplicates, if any
	 */
	if (duplist) {
		if (preen)
			pfatal("INTERNAL ERROR: dups with -p");
		printf("** Phase 1b - Rescan For More DUPS\n");
		pass1b();
	}

	/*
	 * 2: traverse directories from root to mark all connected directories
	 */
	if (preen == 0)
		printf("** Phase 2 - Check Pathnames\n");
	pass2();

	/*
	 * 3: scan inodes looking for disconnected directories
	 */
	if (preen == 0)
		printf("** Phase 3 - Check Connectivity\n");
	pass3();

	/*
	 * 4: scan inodes looking for disconnected files; check reference counts
	 */
	if (preen == 0)
		printf("** Phase 4 - Check Reference Counts\n");
	pass4();

	/*
	 * 5: check and repair resource counts in cylinder groups
	 */
	if (preen == 0)
		printf("** Phase 5 - Check Cyl groups\n");
	pass5();

	/*
	 * print out summary statistics
	 */
	n_ffree = sblock.fs_cstotal.cs_nffree;
	n_bfree = sblock.fs_cstotal.cs_nbfree;
#if	NeXT
	pinfo("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
#else	NeXT
	pwarn("%d files, %d used, %d free ",
	    n_files, n_blks, n_ffree + sblock.fs_frag * n_bfree);
#endif	NeXT
	printf("(%d frags, %d blocks, %.1f%% fragmentation)\n",
	    n_ffree, n_bfree, (float)(n_ffree * 100) / sblock.fs_dsize);
	if (debug && (n_files -= imax - ROOTINO - sblock.fs_cstotal.cs_nifree))
		printf("%d files missing\n", n_files);
	if (debug) {
		n_blks += sblock.fs_ncg *
			(cgdmin(&sblock, 0) - cgsblock(&sblock, 0));
		n_blks += cgsblock(&sblock, 0) - cgbase(&sblock, 0);
		n_blks += howmany(sblock.fs_cssize, sblock.fs_fsize);
		if (n_blks -= fmax - (n_ffree + sblock.fs_frag * n_bfree))
			printf("%d blocks missing\n", n_blks);
		if (duplist != NULL) {
			printf("The following duplicate blocks remain:");
			for (dp = duplist; dp; dp = dp->next)
				printf(" %d,", dp->dup);
			printf("\n");
		}
		if (zlnhead != NULL) {
			printf("The following zero link count inodes remain:");
			for (zlnp = zlnhead; zlnp; zlnp = zlnp->next)
				printf(" %d,", zlnp->zlncnt);
			printf("\n");
		}
	}
	zlnhead = (struct zlncnt *)0;
	duplist = (struct dups *)0;
#if	NeXT
	/*
	 *	Clear state variable if preening (we would have aborted
	 *	if there were problems) or if no errors were detected on
	 *	a manual fsck.
	 */
	if (preen || (error_count == 0 && sblock.fs_state != expected_state)) {
		/* Disk is known to be clean, change fs_state */
		if (mountedfs) {
			/* separate this from below to indicate that if
			   some day we sync the super block with the buffer
			   cache we will have to use FS_STATE_DIRTY in
			   this case. */
			sblock.fs_state = FS_STATE_CLEAN;
			sbdirty();
			printf("Mounted file system set to clean state");
			if (readonlyfs && !rootfs)
				printf(" -- must umount and then re-mount\n");
			else
				printf(" -- must reboot without sync\n");
		} else {
			sblock.fs_state = FS_STATE_CLEAN;
			sbdirty();
		}
	} else if (error_count) {
		/* Don't set clean state until fsck without errors */
		printf("File system not may not be clean!  ");
		printf("Run fsck again to clean.\n");
	}
#endif	NeXT
	if (dfile.mod) {
		(void)time(&sblock.fs_time);
		sbdirty();
	}
	ckfini();
	free(blockmap);
	free(statemap);
	free((char *)lncntp);
	if (!dfile.mod)
		return;
	if (!preen) {
		printf("\n***** FILE SYSTEM WAS MODIFIED *****\n");
		if (mountedfs) {
			if (readonlyfs && !rootfs) {
				printf("\n***** UNMOUNT AND RE-MOUNT"
				 " FILE SYSTEM *****\n");
			} else {
				printf("\n***** REBOOT THE SYSTEM"
				 " (NO SYNC) *****\n");
			}
		}
	}
#if	NeXT
	if (usingblkdev && (!mountedfs || readonlyfs))
		fsync(dfile.wfdes);
#endif	NeXT
	if (mountedfs)
		exit(4);
}

char *
blockcheck(name)
	char *name;
{
	struct stat statb;
	int looped = 0;
	static char nametmp[MAXPATHLEN];

	strcpy(nametmp, name);
	
	if (stat(nametmp, &statb) < 0){
		printf("Can't stat %s\n", nametmp);
		return (0);
	}
retry:
	switch (statb.st_mode & S_IFMT) {
	case S_IFCHR:
		return (nametmp);

	case S_IFBLK:
		strcpy(nametmp, rawname(name));
		if (looped++ == 0 && stat(nametmp, &statb) >= 0)
			goto retry;
		break;
	}
	if (looped)
		printf("Can't determine raw device name for %s\n", name);
	else
		printf("%s is not block or character device\n", name);
	return (0);
}

char *
unrawname(cp)
	char *cp;
{
	char *dp = rindex(cp, '/');
	struct stat stb;

	if (dp == 0)
		return (cp);
	if (stat(cp, &stb) < 0)
		return (cp);
	if ((stb.st_mode&S_IFMT) != S_IFCHR)
		return (cp);
	if (*(dp+1) != 'r')
		return (cp);
	(void)strcpy(dp+1, dp+2);
	return (cp);
}

char *
rawname(cp)
	char *cp;
{
	static char rawbuf[MAXPATHLEN];
	char *dp = rindex(cp, '/');

	if (dp == 0)
		return (0);
	*dp = 0;
	(void)strcpy(rawbuf, cp);
	*dp = '/';
	(void)strcat(rawbuf, "/r");
	(void)strcat(rawbuf, dp+1);
	return (rawbuf);
}


/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char sccsid[] = "@(#)dumptraverse.c	5.3 (Berkeley) 1/9/86";
#endif not lint

#include "dump.h"
#include "swent.h"

#if	NeXT
#include <mntent.h>
#endif	NeXT

pass(fn, map)
	register int (*fn)();
	register char *map;
{
	register int bits;
	ino_t maxino;

#ifdef DEBUG
	written = 0;
#endif DEBUG	
	maxino = sblock->fs_ipg * sblock->fs_ncg - 1;
	for (ino = 0; ino < maxino; ) {
		if ((ino % NBBY) == 0) {
			bits = ~0;
			if (map != NULL)
				bits = *map++;
		}
		ino++;
		if (bits & 1)
			(*fn)(getino(ino));
		bits >>= 1;
	}
}

#if	NeXT
#ifdef DEBUG
dumpmap (char *front, char *map)
{
	register int bits;
	int i;
	ino_t maxino;

	printf ("%s: ", front);
	maxino = sblock->fs_ipg * sblock->fs_ncg - 1;
	for (ino = 0; ino < maxino; ) {
		if ((ino % NBBY) == 0) {
			bits = ~0;
			if (map != NULL)
				bits = *map++;
		}
		ino++;
		if (bits & 1)
			printf ("%d ", ino);
		bits >>= 1;
	}
	printf ("\n");
}
#endif DEBUG	

void nukeswapfiles (char *dumpdev)
{
	FILE *mntp;
	swapent_t	sw;
	struct stat	swapstat,devstat;
	struct mntent	*mnt;

	/*
	 * Set up to read swap entries.
	 */
	if (swent_start(NULL) != 0) {
		return;
	}
	
	/*
	 * Call mach_swapon on the file if it is not "noauto" [sic].
	 * Skip swapfiles not on the dev we are dumping.
	 */
	while ((sw = swent_get()) != NULL) {
		if (stat (sw->sw_file, &swapstat) == -1) {
			swent_rele(sw);
			swent_end();
			return;
		}			

		if ((mntp = setmntent(MNTTAB, "r")) == 0) {
			perror(MNTTAB);
			exit(1);
		}
		while ((mnt = getmntent(mntp)) != 0) {
			if (strcmp(rawname(mnt->mnt_fsname), dumpdev) != 0)
				continue;
			if ((stat(mnt->mnt_dir, &devstat) >= 0) &&
			    (swapstat.st_dev == devstat.st_dev)) {
				BIC(swapstat.st_ino, nodmap);
				BIC(swapstat.st_ino, clrmap);
				revise_estimate (getino (swapstat.st_ino));
				swent_rele(sw);
			}
		}
		(void) endmntent(mntp);
	}

	/*
	 * Close down the swaptab file.
	 */
	swent_end();

	return;
}
#endif	NeXT

mark(ip)
	struct dinode *ip;
{
	register int f;
	extern int anydskipped;

	f = ip->di_mode & IFMT;
	if (f == 0)
		return;
	BIS(ino, clrmap);
	if (f == IFDIR)
		BIS(ino, dirmap);
	if ((ip->di_mtime >= spcl.c_ddate || ip->di_ctime >= spcl.c_ddate) &&
	    !BIT(ino, nodmap)) {
		BIS(ino, nodmap);
		if (f != IFREG && f != IFDIR && f != IFLNK) {
			esize += 1;
			return;
		}
		est(ip);
	} else if (f == IFDIR)
		anydskipped = 1;
}

add(ip)
	register struct	dinode	*ip;
{
	register int i;
	long filesize;

	if(BIT(ino, nodmap))
		return;
	nsubdir = 0;
	dadded = 0;
	filesize = ip->di_size;
	for (i = 0; i < NDADDR; i++) {
		if (ip->di_db[i] != 0)
			dsrch(ip->di_db[i], dblksize(sblock, ip, i), filesize);
		filesize -= sblock->fs_bsize;
	}
	for (i = 0; i < NIADDR; i++) {
		if (ip->di_ib[i] != 0)
			indir(ip->di_ib[i], i, &filesize);
	}
	if(dadded) {
		nadded++;
		if (!BIT(ino, nodmap)) {
			BIS(ino, nodmap);
			est(ip);
		}
	}
	if(nsubdir == 0)
		if(!BIT(ino, nodmap))
			BIC(ino, dirmap);
}

indir(d, n, filesize)
	daddr_t d;
	int n, *filesize;
{
	register i;
	daddr_t	idblk[MAXNINDIR];

	bread(fsbtodb(sblock, d), (char *)idblk, sblock->fs_bsize);
	if(n <= 0) {
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				dsrch(d, sblock->fs_bsize, *filesize);
			*filesize -= sblock->fs_bsize;
		}
	} else {
		n--;
		for(i=0; i < NINDIR(sblock); i++) {
			d = idblk[i];
			if(d != 0)
				indir(d, n, filesize);
		}
	}
}

dirdump(ip)
	struct dinode *ip;
{
	/* watchout for dir inodes deleted and maybe reallocated */
	if ((ip->di_mode & IFMT) != IFDIR)
		return;
	dump(ip);
}

dump(ip)
	struct dinode *ip;
{
	register int i;
	long size;

#ifdef DEBUG
	fprintf (stderr, "%d ", ino);
	if ((written++ % 16) == 0)
		fprintf (stderr, "\n");
#endif DEBUG	
	if(newtape) {
		newtape = 0;
		bitmap(nodmap, TS_BITS);
	}
	BIC(ino, nodmap);
	spcl.c_dinode = *ip;
	spcl.c_type = TS_INODE;
	spcl.c_count = 0;
	i = ip->di_mode & IFMT;
	if (i == 0) /* free inode */
		return;
	if ((i != IFDIR && i != IFREG && i != IFLNK) || ip->di_size == 0) {
		spclrec();
		return;
	}
#if	NeXT_MOD
	if (ip->di_icflags & IC_FASTLINK)  {
	    	if (ip->di_size >= MAX_FASTLINK_SIZE)  {
		    	msg("Found FASTLINK inode with di_size too big\n");
			if (!query("Do you want to attempt to continue?"))  {
				dumpabort();
				/*NOTREACHED*/
			}
		}
		else  {
			fastsymlinkout(ip);
		}
		return;
	}
#endif	NeXT_MOD
	if (ip->di_size > NDADDR * sblock->fs_bsize)
		i = NDADDR * sblock->fs_frag;
	else
		i = howmany(ip->di_size, sblock->fs_fsize);
	blksout(&ip->di_db[0], i);
	size = ip->di_size - NDADDR * sblock->fs_bsize;
	if (size <= 0)
		return;
	for (i = 0; i < NIADDR; i++) {
		dmpindir(ip->di_ib[i], i, &size);
		if (size <= 0)
			return;
	}
}

dmpindir(blk, lvl, size)
	daddr_t blk;
	int lvl;
	long *size;
{
	int i, cnt;
	daddr_t idblk[MAXNINDIR];

	if (blk != 0)
		bread(fsbtodb(sblock, blk), (char *)idblk, sblock->fs_bsize);
	else
		bzero(idblk, sblock->fs_bsize);
	if (lvl <= 0) {
		if (*size < NINDIR(sblock) * sblock->fs_bsize)
			cnt = howmany(*size, sblock->fs_fsize);
		else
			cnt = NINDIR(sblock) * sblock->fs_frag;
		*size -= NINDIR(sblock) * sblock->fs_bsize;
		blksout(&idblk[0], cnt);
		return;
	}
	lvl--;
	for (i = 0; i < NINDIR(sblock); i++) {
		dmpindir(idblk[i], lvl, size);
		if (*size <= 0)
			return;
	}
}

#if	NeXT_MOD
fastsymlinkout(ip)
	struct dinode *ip;
{
	int i;
	char linkrec[TP_BSIZE];

	spcl.c_addr[0] = 1;
	for (i = 1; i < TP_NINDIR; i++)
		spcl.c_addr[i] = 0;
	spcl.c_count = 1;
	spclrec();
	bcopy(ip->di_symlink, linkrec, ip->di_size);
	linkrec[ip->di_size] = 0;	/* null terminate! */
	taprec(linkrec);
}
#endif	NeXT_MOD

blksout(blkp, frags)
	daddr_t *blkp;
	int frags;
{
	int i, j, count, blks, tbperdb;

	blks = howmany(frags * sblock->fs_fsize, TP_BSIZE);
	tbperdb = sblock->fs_bsize / TP_BSIZE;
	for (i = 0; i < blks; i += TP_NINDIR) {
		if (i + TP_NINDIR > blks)
			count = blks;
		else
			count = i + TP_NINDIR;
		for (j = i; j < count; j++)
			if (blkp[j / tbperdb] != 0)
				spcl.c_addr[j - i] = 1;
			else
				spcl.c_addr[j - i] = 0;
		spcl.c_count = count - i;
		spclrec();
		for (j = i; j < count; j += tbperdb)
			if (blkp[j / tbperdb] != 0)
				if (j + tbperdb <= count)
					dmpblk(blkp[j / tbperdb],
					    sblock->fs_bsize);
				else
					dmpblk(blkp[j / tbperdb],
					    (count - j) * TP_BSIZE);
		spcl.c_type = TS_ADDR;
	}
}

bitmap(map, typ)
	char *map;
{
	register i;
	char *cp;

	spcl.c_type = typ;
	spcl.c_count = howmany(msiz * sizeof(map[0]), TP_BSIZE);
	spclrec();
	for (i = 0, cp = map; i < spcl.c_count; i++, cp += TP_BSIZE)
		taprec(cp);
}

#ifdef	NeXT
/*
 *	Major funny business here.  We want to make sure that we have
 *	a spclrec at the start of any N>1 disk/tape, so that we don't
 *	have to lose blocks in restore when it resyncronizes.  We
 *	also want a new spclrec (TS_EOT) output to inform restore that it is
 *	time to change disks/tapes, since disks do not have a TAPEMARK
 *	that restore is otherwise relying on.
 *	We have moved the otape call to this routine, and use endoftape
 *	to signal when we need to start another tape.  Since we will be
 *	called recursively as we write TS_EOTs, we need to be careful
 */
 
int	endoftape;
union u_spcl	savespcl;
extern int ntrec, trecno;

spclrec()
{
	static int recursed = 0;

	if (endoftape)  {
		if (!recursed)  {
			int	cnt;

			recursed++;
			savespcl = u_spcl;

#define	T_EOT		7		/* XXX move to dumprestore.h (10/90) */

			spcl.c_type = T_EOT;
			for(cnt = ntrec - trecno; cnt--; ) {
				dospcl(&spcl);
			}
			close_rewind();
			otape();			// also recurses
			recursed = 0;
			endoftape = 0;
			u_spcl = savespcl;
		}
	}
	dospcl(&spcl);
}

dospcl(union u_spcl *spclp)
#define spcl	spclp->s_spcl
#else NeXT
spclrec()
#endif	NeXT
{
	register int s, i, *ip;

	spcl.c_inumber = ino;
	spcl.c_magic = NFS_MAGIC;
	spcl.c_checksum = 0;
	ip = (int *)&spcl;
	s = 0;
	i = sizeof(union u_spcl) / (4*sizeof(int));
	while (--i >= 0) {
		s += *ip++; s += *ip++;
		s += *ip++; s += *ip++;
	}
	spcl.c_checksum = CHECKSUM - s;
	taprec((char *)&spcl);
}

dsrch(d, size, filesize)
	daddr_t d;
	int size, filesize;
{
	register struct direct *dp;
	long loc;
	char dblk[MAXBSIZE];

	if(dadded)
		return;
	if (filesize > size)
		filesize = size;
	bread(fsbtodb(sblock, d), dblk, filesize);
	for (loc = 0; loc < filesize; ) {
		dp = (struct direct *)(dblk + loc);
		if (dp->d_reclen == 0) {
			msg("corrupted directory, inumber %d\n", ino);
			break;
		}
		loc += dp->d_reclen;
		if(dp->d_ino == 0)
			continue;
		if(dp->d_name[0] == '.') {
			if(dp->d_name[1] == '\0')
				continue;
			if(dp->d_name[1] == '.' && dp->d_name[2] == '\0')
				continue;
		}
		if(BIT(dp->d_ino, nodmap)) {
			dadded++;
			return;
		}
		if(BIT(dp->d_ino, dirmap))
			nsubdir++;
	}
}

struct dinode *
getino(ino)
	daddr_t ino;
{
	static daddr_t minino, maxino;
	static struct dinode itab[MAXINOPB];

	if (ino >= minino && ino < maxino) {
		return (&itab[ino - minino]);
	}
	bread(fsbtodb(sblock, itod(sblock, ino)), itab, sblock->fs_bsize);
	minino = ino - (ino % INOPB(sblock));
	maxino = minino + INOPB(sblock);
	return (&itab[ino - minino]);
}

int	breaderrors = 0;		
#define	BREADEMAX 32

bread(da, ba, cnt)
	daddr_t da;
	char *ba;
	int	cnt;	
{
	int n;

loop:
	if (lseek(fi, (long)(da * DEV_BSIZE), 0) < 0){
		msg("bread: lseek fails\n");
	}
	n = read(fi, ba, cnt);
	if (n == cnt)
		return;
	if (da + (cnt / DEV_BSIZE) > fsbtodb(sblock, sblock->fs_size)) {
		/*
		 * Trying to read the final fragment.
		 *
		 * NB - dump only works in TP_BSIZE blocks, hence
		 * rounds DEV_BSIZE fragments up to TP_BSIZE pieces.
		 * It should be smarter about not actually trying to
		 * read more than it can get, but for the time being
		 * we punt and scale back the read only when it gets
		 * us into trouble. (mkm 9/25/83)
		 */
		cnt -= DEV_BSIZE;
		goto loop;
	}
	msg("(This should not happen)bread from %s [block %d]: count=%d, got=%d\n",
		disk, da, cnt, n);
	if (++breaderrors > BREADEMAX){
		msg("More than %d block read errors from %d\n",
			BREADEMAX, disk);
		broadcast("DUMP IS AILING!\n");
		msg("This is an unrecoverable error.\n");
		if (!query("Do you want to attempt to continue?")){
			dumpabort();
			/*NOTREACHED*/
		} else
			breaderrors = 0;
	}
}

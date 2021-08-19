/*
 * Copyright (c) 1983 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)disktab.c	5.3 (Berkeley) 3/9/86";
#endif LIBC_SCCS and not lint

#if	NeXT
#include <nextdev/disk.h>
#include <sys/file.h>
#else	NeXT
#include <disktab.h>
#endif	NeXT
#include <stdio.h>

static	char *dgetstr();
#if	NeXT
static char dev_name[MAXDNMLEN];
#endif	NeXT

struct disktab *
getdiskbyname(name)
	char *name;
{
	static struct disktab disk;
	static char localbuf[1024], *cp = localbuf;
	register struct	disktab *dp = &disk;
	register struct partition *pp;
	char p, psize[3], pbsize[3], pfsize[3];
#if	NeXT
	char pbase[3];
	register struct fs_info *fp;
	char pcpg[3], pdensity[3], pminfree[3], popt[3], pnewfs[3],
		pmountpt[3], pautomnt[3], ptype[3];
	char *sp;
	register int i;
#endif	NeXT
	char buf[BUFSIZ];

	if (dgetent(buf, name) <= 0)
		return ((struct disktab *)0);
#if	NeXT
	strncpy(dp->d_name, dev_name, MAXDNMLEN);
#else	!NeXT
	dp->d_name = cp;
	strcpy(cp, name);
	cp += strlen(name) + 1;
#endif	!NeXT
	sp = dgetstr("ty", &cp);
	strncpy(dp->d_type, sp ? sp : "", MAXTYPLEN);
	dp->d_secsize = dgetnum("ss");
	if (dp->d_secsize < 0)
		dp->d_secsize = 512;
	dp->d_ntracks = dgetnum("nt");
	dp->d_nsectors = dgetnum("ns");
	dp->d_ncylinders = dgetnum("nc");
	dp->d_rpm = dgetnum("rm");
	if (dp->d_rpm < 0)
		dp->d_rpm = 3600;
#if	NeXT
	dp->d_front = dgetnum("fp");
	dp->d_back = dgetnum("bp");
	dp->d_ngroups = dgetnum("ng");
	dp->d_ag_size = dgetnum("gs");
	dp->d_ag_alts = dgetnum("ga");
	dp->d_ag_off = dgetnum("ao");
	sp = dgetstr("os", &cp);
	strncpy(dp->d_bootfile, sp ? sp : "", MAXBFLEN);
	strcpy(pbase, "zx");
	for (i = 0; i < NBOOTS; i++) {
		pbase[1] = '0' + i;
		dp->d_boot0_blkno[i] = dgetnum(pbase);
	}
	sp = dgetstr("hn", &cp);
	strncpy(dp->d_hostname, sp ? sp : "", MAXHNLEN);
	sp = dgetstr("ro", &cp);
	if (sp)
		dp->d_rootpartition = sp[0];
	sp = dgetstr("rw", &cp);
	if (sp)
		dp->d_rwpartition = sp[0];
	strcpy(pbase, "px");
	strcpy(psize, "sx");
	strcpy(pbsize, "bx");
	strcpy(pfsize, "fx");
	strcpy(pcpg, "cx");
	strcpy(pdensity, "dx");
	strcpy(pminfree, "rx");
	strcpy(popt, "ox");
	strcpy(pnewfs, "ix");
	strcpy(pmountpt, "mx");
	strcpy(pautomnt, "ax");
	strcpy(ptype, "tx");
	for (p = 'a'; p < 'a' + NPART; p++) {
		pbase[1] = psize[1] = pbsize[1] = pfsize[1] = p;
		pcpg[1] = pdensity[1] = pminfree[1] = popt[1] = pnewfs[1] = p;
		pmountpt[1] = pautomnt[1] = ptype[1] = p;
		pp = &dp->d_partitions[p - 'a'];
		pp->p_base = dgetnum(pbase);
		pp->p_size = dgetnum(psize);
		pp->p_bsize = dgetnum(pbsize);
		pp->p_fsize = dgetnum(pfsize);
		pp->p_cpg = dgetnum(pcpg);
		pp->p_density = dgetnum(pdensity);
		pp->p_minfree = dgetnum(pminfree);
		sp = dgetstr(popt, &cp);
		if (sp)
			pp->p_opt = sp[0];
		pp->p_newfs = dgetflag(pnewfs);
		sp = dgetstr(pmountpt, &cp);
		strncpy(pp->p_mountpt, sp ? sp : "", MAXMPTLEN);
		pp->p_automnt = dgetflag(pautomnt);
		sp = dgetstr(ptype, &cp);
		strncpy(pp->p_type, sp ? sp : "", MAXFSTLEN);
	}
#else	NeXT
	dp->d_badsectforw = dgetflag("sf");
	dp->d_sectoffset = dgetflag("so");
	strcpy(psize, "px");
	strcpy(pbsize, "bx");
	strcpy(pfsize, "fx");
	for (p = 'a'; p < 'i'; p++) {
		psize[1] = pbsize[1] = pfsize[1] = p;
		pp = &dp->d_partitions[p - 'a'];
		pp->p_size = dgetnum(psize);
		pp->p_bsize = dgetnum(pbsize);
		pp->p_fsize = dgetnum(pfsize);
	}
#endif	NeXT
	return (dp);
}

#include <ctype.h>

static	char *tbuf;
static	char *dskip();
static	char *ddecode();

/*
 * Get an entry for disk name in buffer bp,
 * from the diskcap file.  Parse is very rudimentary;
 * we just notice escaped newlines.
 */
static
dgetent(bp, name)
	char *bp, *name;
{
	register char *cp;
	register int c;
	register int i = 0, cnt = 0;
	char ibuf[BUFSIZ];
	int tf;

	tbuf = bp;
	tf = open(DISKTAB, 0);
	if (tf < 0)
		return (-1);
	for (;;) {
		cp = bp;
		for (;;) {
			if (i == cnt) {
				cnt = read(tf, ibuf, BUFSIZ);
				if (cnt <= 0) {
					close(tf);
					return (0);
				}
				i = 0;
			}
			c = ibuf[i++];
			if (c == '\n') {
				if (cp > bp && cp[-1] == '\\'){
					cp--;
					continue;
				}
				break;
			}
			if (cp >= bp+BUFSIZ) {
				write(2,"Disktab entry too long\n", 23);
				break;
			} else
				*cp++ = c;
		}
		*cp = 0;

		/*
		 * The real work for the match.
		 */
		if (dnamatch(name)) {
			close(tf);
			return (1);
		}
	}
}

/*
 * Dnamatch deals with name matching.  The first field of the disktab
 * entry is a sequence of names separated by |'s, so we compare
 * against each such name.  The normal : terminator after the last
 * name (before the first field) stops us.
 */
static
dnamatch(np)
	char *np;
{
	register char *Np, *Bp;

	Bp = tbuf;
	if (*Bp == '#')
		return (0);
	for (;;) {
		for (Np = np; *Np && *Bp == *Np; Bp++, Np++)
			continue;
		if (*Np == 0 && (*Bp == '|' || *Bp == ':' || *Bp == 0)) {
#if	NeXT
			strncpy(dev_name, tbuf, sizeof(dev_name));
			for (Bp = dev_name; *Bp && *Bp!=':' && *Bp!='|'; Bp++)
				continue;
			*Bp = 0;
#endif	!NeXT
			return (1);
		}
		while (*Bp && *Bp != ':' && *Bp != '|')
			Bp++;
		if (*Bp == 0 || *Bp == ':')
			return (0);
		Bp++;
	}
}

/*
 * Skip to the next field.  Notice that this is very dumb, not
 * knowing about \: escapes or any such.  If necessary, :'s can be put
 * into the diskcap file in octal.
 */
static char *
dskip(bp)
	register char *bp;
{

	while (*bp && *bp != ':')
		bp++;
	if (*bp == ':')
		bp++;
	return (bp);
}

/*
 * Return the (numeric) option id.
 * Numeric options look like
 *	li#80
 * i.e. the option string is separated from the numeric value by
 * a # character.  If the option is not found we return -1.
 * Note that we handle octal numbers beginning with 0.
 */
static
dgetnum(id)
	char *id;
{
	register int i, base;
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (*bp == 0)
			return (-1);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (-1);
		if (*bp != '#')
			continue;
		bp++;
		base = 10;
		if (*bp == '0')
			base = 8;
		i = 0;
		while (isdigit(*bp))
			i *= base, i += *bp++ - '0';
		return (i);
	}
}

/*
 * Handle a flag option.
 * Flag options are given "naked", i.e. followed by a : or the end
 * of the buffer.  Return 1 if we find the option, or 0 if it is
 * not given.
 */
static
dgetflag(id)
	char *id;
{
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ == id[0] && *bp != 0 && *bp++ == id[1]) {
			if (!*bp || *bp == ':')
				return (1);
			else if (*bp == '@')
				return (0);
		}
	}
}

/*
 * Get a string valued option.
 * These are given as
 *	cl=^Z
 * Much decoding is done on the strings, and the strings are
 * placed in area, which is a ref parameter which is updated.
 * No checking on area overflow.
 */
static char *
dgetstr(id, area)
	char *id, **area;
{
	register char *bp = tbuf;

	for (;;) {
		bp = dskip(bp);
		if (!*bp)
			return (0);
		if (*bp++ != id[0] || *bp == 0 || *bp++ != id[1])
			continue;
		if (*bp == '@')
			return (0);
		if (*bp != '=')
			continue;
		bp++;
		return (ddecode(bp, area));
	}
}

/*
 * ddecode does the grung work to decode the
 * string capability escapes.
 */
static char *
ddecode(str, area)
	register char *str;
	char **area;
{
	register char *cp;
	register int c;
	register char *dp;
	int i;

	cp = *area;
	while ((c = *str++) && c != ':') {
		switch (c) {

		case '^':
			c = *str++ & 037;
			break;

		case '\\':
			dp = "E\033^^\\\\::n\nr\rt\tb\bf\f";
			c = *str++;
nextc:
			if (*dp++ == c) {
				c = *dp++;
				break;
			}
			dp++;
			if (*dp)
				goto nextc;
			if (isdigit(c)) {
				c -= '0', i = 2;
				do
					c <<= 3, c |= *str++ - '0';
				while (--i && isdigit(*str));
			}
			break;
		}
		*cp++ = c;
	}
	*cp++ = 0;
	str = *area;
	*area = cp;
	return (str);
}

#if	NeXT
struct disktab *
getdiskbydev (dev)
	char *dev;
{
	static struct disk_label label;
	int fd;

	if ((fd = open (dev, O_RDONLY)) < 0)
		return (0);
	if (ioctl (fd, DKIOCGLABEL, &label) < 0)
		return (0);
	return (&label.dl_dt);
}
#endif	NeXT

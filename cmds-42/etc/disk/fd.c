/*	@(#)fd.c	2.0	03/05/90	(c) 1990 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 05-Mar-89	Doug Mitchell at NeXT
 *	Created.
 *
 **********************************************************************
 */
 
#include <sys/types.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/ioctl.h>
#include <nextdev/disk.h>
#include <sys/time.h>
#include "disk.h"
#include <errno.h>
#include <nextdev/fd_extern.h>
#include <nextdev/scsireg.h>

extern int density;

#ifndef	TRUE
#define TRUE	(1)
#endif	TRUE
#ifndef	FALSE
#define FALSE	(0)
#endif	FALSE

fd_conf()
{
}

fd_init()
{
}

fd_wlabel()
{
	register struct disk_label *l = &disk_label;

	if (ioctl (fd, DKIOCSLABEL, l) < 0)
		panic (S_MEDIA, "can't write label -- disk unusable!");
}

fd_glabel (l, bb, rtn)
	register struct disk_label *l;
	struct bad_block *bb;
{
	extern int errno;

	if (ioctl (fd, DKIOCGLABEL, l) < 0) {
		if (rtn)
			return (-1);
		if (errno == ENXIO)
			bomb (S_NEVER, "no label on disk");
		panic (S_NEVER, "get label");
	}
	return (0);
}

fd_cyl (l)
	struct disk_label *l;
{
	dsp->ds_maxcyl = l->dl_ncyl;
}

fd_geterr()
{
}

fd_req (cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	struct fd_rawio rawio;
	struct fd_ioreq ioreq;
	int rtn;
	
	bzero (&rawio, sizeof (struct fd_rawio));

	switch (cmd) {
	case CMD_EJECT:
		bzero (&ioreq, sizeof (struct fd_ioreq));
		ioreq.command = FDCMD_EJECT;
		return(ioctl(fd, FDIOCREQ, &ioreq));
		
	case CMD_SEEK:
		/* TBD */
		goto out;

	case CMD_READ:
		rawio.read = TRUE;
		goto rw;

	case CMD_WRITE:
		rawio.read = FALSE;
rw:
		rawio.sector = p1 * DEV_BSIZE / devblklen;
		rawio.sector_count = howmany(p3,devblklen);
		rawio.dma_addrs = (caddr_t) p2;
		break;
	}
	rtn = ioctl (fd, FDIOCRRW, &rawio);
	if((rtn == 0) && rawio.status)
		rtn = -1;		/* catch device errors */
out:
	exec_time.tv_sec = 0;		/* TBD */	
	exec_time.tv_usec = 1;		/* TBD */
	return(rtn);
	
} /* fd_req() */


fd_pstats()
{
}

fd_ecc()
{
	return (0);
}

static struct disktab fd_dt;

#define FPORCH	96
#define BPORCH	0
#define	BLKSIZE	8192
#define	FRGSIZE	1024
#define	CYLPGRP	32
#define	BYTPERINO 4096
#define	MINFREE	0

#define	NELEM(x)		(sizeof(x)/sizeof(x[0]))
#define	LAST_ELEM(x)		((x)[NELEM(x)-1])

#define FD_SECTSIZE_DEF		512
#define FD_NSECTS_DEF		36
#define FD_NCYLS_DEF		80
#define FD_TOTALSECT_DEF	5760

struct disktab *
fd_inferdisktab(int fd, char *devname, int init_flag, int density)
{
	struct drive_info di;
	int sfactor, ncyl, ntracks, nsectors, nblocks;
	char namebuf[512];
	struct fd_format_info format_info;
	char command_str[80];
	
	/*
	 * Get physical disk parameters. We'll format the disk if init_flag
	 * is true (i.e., we're either doing a Format or an init command).
	 * Density of 0 means use default density for given media_id.
	 *
	 * Note we format here instead of in the call to (*dsp->ds_format)()
	 * in init() to set up the proper disktab entry.
	 *
	 * If init_flag is false and the disk is unformatted, we'll wing it
	 * and generate just the disktab entries we need for other things.
	 */
	if(init_flag) {
		if(fd_format(fd, devname, TRUE))
			return NULL;
	}
	if(ioctl(fd, FDIOCGFORM, &format_info)) 
		return NULL;
	/*
	 * Generate bogus format info is not formatted at this point. 
	 */
	if (ioctl(fd, DKIOCINFO, &di))
		return NULL;
	if(!(format_info.flags & FFI_FORMATTED)) {
		format_info.sectsize_info.sect_size = FD_SECTSIZE_DEF;
		format_info.disk_info.tracks_per_cyl = 2;
		format_info.sectsize_info.sects_per_trk = FD_NSECTS_DEF;
		format_info.total_sects = FD_TOTALSECT_DEF;
		format_info.disk_info.num_cylinders = FD_NCYLS_DEF;
	}

	/*
	 * One way or another, we have reasonable format info.
	 */
	sfactor = 1024/(format_info.sectsize_info.sect_size);
	if (sfactor != 1 && sfactor != 2 && sfactor != 4)
		return NULL;

	ntracks  = format_info.disk_info.tracks_per_cyl;
	nsectors = format_info.sectsize_info.sects_per_trk/sfactor;
	nblocks  = format_info.total_sects/sfactor;
	ncyl     = format_info.disk_info.num_cylinders;

	sprintf(namebuf, "%.*s-%d", MAXDNMLEN, di.di_name, 
		format_info.sectsize_info.sect_size);
	if (strlen(namebuf) < MAXDNMLEN)
		strcpy(fd_dt.d_name, namebuf);
	else {
		strncpy(fd_dt.d_name, di.di_name, MAXDNMLEN);
		LAST_ELEM(fd_dt.d_name) = '\0';
	}
	strcpy(fd_dt.d_type, "removable_rw_floppy");
	fd_dt.d_secsize = 1024;
	fd_dt.d_ntracks = ntracks;
	fd_dt.d_nsectors = nsectors;
	fd_dt.d_ncylinders = ncyl;
	fd_dt.d_rpm = 300;
	fd_dt.d_front = FPORCH;
	fd_dt.d_back = BPORCH;
	fd_dt.d_ngroups = fd_dt.d_ag_size = 0;
	fd_dt.d_ag_alts = fd_dt.d_ag_off = 0;
	fd_dt.d_boot0_blkno[0] = -1;		/* only one boot block */
	fd_dt.d_boot0_blkno[1] = 32;
	strcpy(fd_dt.d_bootfile, "fdmach");
	gethostname(fd_dt.d_hostname, MAXHNLEN);
	fd_dt.d_hostname[MAXHNLEN-1] = '\0';
	fd_dt.d_rootpartition = 'a';
	fd_dt.d_rwpartition = 'b';

	{
		struct partition *pp;
		int i;

		pp = &fd_dt.d_partitions[0];
		pp->p_base = 0;
		pp->p_size = nblocks - FPORCH - BPORCH;
		pp->p_bsize = BLKSIZE;
		pp->p_fsize = FRGSIZE;
		pp->p_cpg = CYLPGRP;
		pp->p_density = BYTPERINO;
		pp->p_minfree = MINFREE;
		pp->p_newfs = 1;
		pp->p_mountpt[0] = '\0';
		pp->p_automnt = 1;
		pp->p_opt = 't';
		strcpy(pp->p_type, "4.3BSD");

		for (i = 1; i < NPART; i++) {
			pp = &fd_dt.d_partitions[i];
			pp->p_base = -1;
			pp->p_size = -1;
			pp->p_bsize = -1;
			pp->p_fsize = -1;
			pp->p_cpg = -1;
			pp->p_density = -1;
			pp->p_minfree = -1;
			pp->p_newfs = 0;
			pp->p_mountpt[0] = '\0';
			pp->p_automnt = 0;
			pp->p_opt = '\0';
			pp->p_type[0] = '\0';
		}
	}

	return &fd_dt;
	
} /* fd_inferdisktab() */

static int fd_formatted = 0;	/* disk was formatted in this exec of 'disk' */
 
int fd_format(int fd, char *devname, int force)  
{
	/*
	 * Format disk if we haven't already done so (we might have already 
	 * done this in fd_inferdisktab()...).
	 * Returns an errno. Assumes global density from command line (0 means
	 * format at default density).
	 */
	char command_str[80];
	int rtn;
	
	if(fd_formatted) {
		fd_formatted = 0;	/* for interactive use */
		return(0);
	}
	if(density)
		sprintf(command_str, "/usr/etc/fdform %sa d=%d\n",
			devname, density);
	else
		sprintf(command_str, "/usr/etc/fdform %sa\n", devname);
	rtn = system(command_str);
	if(rtn == 0)
		fd_formatted = 1;
	return(rtn);
}





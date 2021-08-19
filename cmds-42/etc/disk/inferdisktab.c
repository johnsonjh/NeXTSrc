/*	inferdisktab.c
 *
 * History
 * -------
 * 02-Feb-90	Mike DeMoney at NeXT
 *	Created
 */
 
#include <sys/param.h>
#include <sys/file.h>
#include <nextdev/disk.h>
#include <nextdev/scsireg.h>
#include <nextdev/fd_extern.h>

#ifndef DEVTYPE_RWOPTICAL
#define	DEVTYPE_RWOPTICAL	0x07
#endif

#define	FPORCH_HARD	160
#define FPORCH_FLOPPY	96		/* floppy only has one boot block */
#define	BPORCH		0

#define RPM_HARD	3600
#define RPM_FLOPPY	300

#define	BLKSIZE		8192
#define	FRGSIZE		1024
#define	CYLPGRP		16
#define	BYTPERINO	4096

#define SCSI_TIMEOUT	15		/* command timeout in seconds */

/*
 * Disk capacities above MINFREE_THRESH use MINFREE_LARGE,
 * capacities under MINFREE_THRESH use MINFREE_SMALL.
 *
 * MINFREE_THRESH is given in number of DEV_BSIZE blocks to avoid
 * overflow with large disks, so take capacity and divide by DEV_BSIZE.
 */
#define	MINFREE_LARGE_THRESH	(150*1024*1024/DEV_BSIZE) 	/* 150MB */
#define MINFREE_FLOPPY_THRESH	(6*1024*1024/DEV_BSIZE) 	/* 6 MB */
#define	MINFREE_LARGE	10		/* % minfree for large drives */
#define	MINFREE_SMALL	5		/* % minfree for small drives */
#define MINFREE_FLOPPY	0		/* % minfree for floppy disks */

/*
 * Constants for faking hard disk entries
 */
#define NUM_HD_HEADS 	4		/* number of tracks/cylinder */
#define NUM_HD_SECT	32		/* sectors/track (DEV_BSIZE) */

#define	NELEM(x)		(sizeof(x)/sizeof(x[0]))
#define	LAST_ELEM(x)		((x)[NELEM(x)-1])

#include <stdio.h>

static struct disktab dt;

#define	MS_DASD		3		/* Direct access device mode page */
#define	MS_RDG		4		/* Rigid geometry mode page */

struct mode_sense_cmd {
	u_int	msc_opcode:8,
		msc_lun:3,
		msc_mbz1:5,
		msc_pcf:2,
		msc_page:6,
		msc_mbz2:8;
	u_char	msc_len;
	u_char	msc_ctrl;
};

struct param_list_header {
	u_char	plh_len;
	u_char	plh_medium;
	u_char	plh_wp:1,
		plh_reserved:7;
	u_char	plh_blkdesclen;
};

struct block_descriptor {
	u_int	bd_density:8,
		bd_nblk:24;
	u_int	bd_reserved:8,
		bd_blklen:24;
};

struct device_format_params {
	u_char	dfp_savable:1,
		dfp_reserved:1,
		dfp_pagecode:6;
	u_char	dfp_pagelen;
	u_short	dfp_trkszone;
	u_short	dfp_altsecszone;
	u_short	dfp_alttrkszone;
	u_short	dfp_alttrksvol;
	u_short	dfp_sectors;
	u_short	dfp_bytessector;
	u_short	dfp_interleave;
	u_short	dfp_trkskew;
	u_short	dfp_cylskew;
	u_char	dfp_ssec:1,
		dfp_hsec:1,
		dfp_rmb:1,
		dfp_surf:1,
		dfp_reserved2:4;
	u_char	dfp_reserved3;
	u_char	dfp_reserved4;
	u_char	dfp_reserved5;
};

struct rigid_drive_params {
	u_char	rdp_savable:1,
		rdp_reserved:1,
		rdp_pagecode:6;
	u_char	rdp_pagelen;

	u_char	rdp_maxcylmsb;
	u_char	rdp_maxcylinb;
	u_char	rdp_maxcyllsb;

	u_char	rdp_maxheads;

	u_char	rdp_wpstartmsb;
	u_char	rdp_wpstartinb;
	u_char	rdp_wpstartlsb;

	u_char	rdp_rwcstartmsb;
	u_char	rdp_rwcstartinb;
	u_char	rdp_rwcstartlsb;

	u_char	rdp_stepratemsb;
	u_char	rdp_stepratelsb;

	u_char	rdp_landcylmsb;
	u_char	rdp_landcylinb;
	u_char	rdp_landcyllsb;

	u_char	rdp_reserved2;
	u_char	rdp_reserved3;
	u_char	rdp_reserved4;
};

#define	THREE_BYTE(x)	\
		(((x##msb)<<16)|((x##inb)<<8)|(x##lsb))

#define	TWO_BYTE(x)	\
		(((x##msb)<<8)|(x##lsb))

struct mode_sense_reply {
	struct param_list_header msr_plh;
	struct block_descriptor msr_bd;
	union {
		struct device_format_params u_msr_dfp;
		struct rigid_drive_params u_msr_rdp;
	}u;
};

#define msr_dfp	u.u_msr_dfp
#define msr_rdp	u.u_msr_rdp

char *progname;

int
do_inquiry(int fd, struct inquiry_reply *irp)
{
	struct scsi_req sr;
	struct cdb_6 *c6p;

	bzero((char *)&sr, sizeof(sr));

	c6p = (struct cdb_6 *)&sr.sr_cdb;
	c6p->c6_opcode = C6OP_INQUIRY;
	c6p->c6_len = sizeof(*irp);

	sr.sr_addr = (char *)irp;
	sr.sr_dma_max = sizeof(*irp);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;

	return ioctl(fd, SDIOCSRQ, &sr);
}

static int
do_modesense(int fd, struct mode_sense_reply *msrp, int page)
{
	struct scsi_req sr;
	struct mode_sense_cmd *mscp;

	bzero((char *)&sr, sizeof(sr));

	mscp = (struct mode_sense_cmd *)&sr.sr_cdb;
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = page;
	mscp->msc_len = sizeof(*msrp);

	sr.sr_addr = (char *)msrp;
	sr.sr_dma_max = sizeof(*msrp);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;

	return ioctl(fd, SDIOCSRQ, &sr);
}

int
do_readcapacity(int fd, struct capacity_reply *crp)
{
	struct scsi_req sr;
	struct cdb_10 *c10p;

	bzero((char *)&sr, sizeof(sr.sr_cdb));

	c10p = (struct cdb_10 *)&sr.sr_cdb;
	c10p->c10_opcode = C10OP_READCAPACITY;

	sr.sr_addr = (char *)crp;
	sr.sr_dma_max = sizeof(*crp);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;

	return ioctl(fd, SDIOCSRQ, &sr);
}

struct disktab *
sd_inferdisktab(int fd)
{
	struct inquiry_reply ir;
	struct mode_sense_reply dasd, rd;
	struct capacity_reply cr;
	struct drive_info di;
	int sfactor, ncyl, ntracks, nsectors, nblocks;
	char namebuf[512];
	int min_free;
	
	if (do_inquiry(fd, &ir))
		return NULL;

	if (do_readcapacity(fd, &cr))
		return NULL;

	if (ioctl(fd, DKIOCINFO, &di))
		return NULL;

	bzero(&dasd, sizeof(struct mode_sense_reply));
	if (do_modesense(fd, &dasd, MS_DASD))
		return NULL;
	if(dasd.u.u_msr_dfp.dfp_pagelen == 0) {
		/*
		 * Page not supported. Fake it. 
		 */
fake_floppy:
		nblocks = (cr.cr_lastlba + 1) * cr.cr_blklen / DEV_BSIZE;
		if(ir.ir_removable) {
			/*
			 * Treat it like a floppy.
			 */
			ncyl     = NUM_FD_CYL;
			ntracks  = NUM_FD_HEADS;
			nsectors = nblocks / (ncyl * ntracks);
		}
		else {
			/*
			 * Dumb hard disk.
			 */
			ntracks  = NUM_HD_HEADS;
			nsectors = NUM_HD_SECT;
			ncyl     = nblocks / (ntracks * nsectors);
		}
		goto gen_entry;		
	}

	bzero(&rd, sizeof(struct mode_sense_reply));
	if (do_modesense(fd, &rd, MS_RDG))
		return NULL;
	if(rd.u.u_msr_rdp.rdp_pagelen == 0)
		goto fake_floppy;

	if (ir.ir_devicetype != DEVTYPE_DISK
	    && ir.ir_devicetype != DEVTYPE_RWOPTICAL)
		return NULL;
		
	sfactor = DEV_BSIZE/(cr.cr_blklen);
	if (sfactor != 1 && sfactor != 2 && sfactor != 4)
		return NULL;

	if (dasd.msr_dfp.dfp_sectors == 0) {
		int wedgesize;
		
		/*
		 * Bummer -- this is likely zone-sectored....
		 * Best to choose nsectors as max sectors in
		 * any zone.  For now, we just add 1 and hope.
		 */
		wedgesize = THREE_BYTE(rd.msr_rdp.rdp_maxcyl)
		  * rd.msr_rdp.rdp_maxheads;
		dasd.msr_dfp.dfp_sectors =
		  1 + ((cr.cr_lastlba + 1 + (wedgesize / 2)) / wedgesize);
		fprintf(stderr,
		    "Zone sectored device, guessing sectors at %d\n",
		    dasd.msr_dfp.dfp_sectors);
	}

	ncyl = THREE_BYTE(rd.msr_rdp.rdp_maxcyl);
	ntracks = rd.msr_rdp.rdp_maxheads;
	nsectors = (dasd.msr_dfp.dfp_sectors + sfactor - 1)/sfactor;
	nblocks = (cr.cr_lastlba + 1)/sfactor;
	
	if (dasd.msr_dfp.dfp_trkszone == 1)
		nsectors -= dasd.msr_dfp.dfp_altsecszone/sfactor;
gen_entry:
	sprintf(namebuf, "%.*s-%d", MAXDNMLEN, di.di_name, cr.cr_blklen);
	if (strlen(namebuf) < MAXDNMLEN)
		strcpy(dt.d_name, namebuf);
	else {
		strncpy(dt.d_name, di.di_name, MAXDNMLEN);
		LAST_ELEM(dt.d_name) = '\0';
	}
	strcpy(dt.d_type, ir.ir_removable ? "removable" : "fixed");
	strcat(dt.d_type, "_rw_scsi");
	dt.d_secsize = DEV_BSIZE;
	dt.d_ntracks = ntracks;
	dt.d_nsectors = nsectors;
	dt.d_ncylinders = ncyl;
	if(nblocks > MINFREE_FLOPPY_THRESH) {
		dt.d_rpm            = RPM_HARD;
		dt.d_front          = FPORCH_HARD;
		dt.d_boot0_blkno[0] = 32;
		dt.d_boot0_blkno[1] = 96;
		if(nblocks > MINFREE_LARGE_THRESH)
			min_free = MINFREE_LARGE;
		else 
			min_free = MINFREE_SMALL;
	}
	else {
		dt.d_rpm            = RPM_FLOPPY;
		dt.d_front          = FPORCH_FLOPPY;
		dt.d_boot0_blkno[0] = -1;	/* floppy has 1 boot block */
		dt.d_boot0_blkno[1] = 32;
		min_free = MINFREE_FLOPPY;
	}
	dt.d_back = BPORCH;
	dt.d_ngroups = dt.d_ag_size = 0;
	dt.d_ag_alts = dt.d_ag_off = 0;
	strcpy(dt.d_bootfile, "sdmach");
	gethostname(dt.d_hostname, MAXHNLEN);
	dt.d_hostname[MAXHNLEN-1] = '\0';
	dt.d_rootpartition = 'a';
	dt.d_rwpartition = 'b';

	{
		struct partition *pp;
		int i;

		pp = &dt.d_partitions[0];
		pp->p_base = 0;
		pp->p_size = nblocks - dt.d_front - BPORCH;
		pp->p_bsize = BLKSIZE;
		pp->p_fsize = FRGSIZE;
		pp->p_cpg = CYLPGRP;
		pp->p_density = BYTPERINO;
		pp->p_minfree = min_free;
		pp->p_newfs = 1;
		pp->p_mountpt[0] = '\0';
		pp->p_automnt = 1;
		pp->p_opt = min_free < 10 ? 's' : 't';
		strcpy(pp->p_type, "4.3BSD");

		for (i = 1; i < NPART; i++) {
			pp = &dt.d_partitions[i];
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

	return &dt;
}



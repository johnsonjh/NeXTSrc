/*	@(#)sd.c	1.0	08/28/87	(c) 1987 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added scsi_req support.
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

#include <nextdev/scsireg.h>

sd_conf()
{
	/* eventually do mode select stuff here */
}

sd_init()
{
}

sd_wlabel()
{
	register struct disk_label *l = &disk_label;

	if (ioctl (fd, DKIOCSLABEL, l) < 0)
		panic (S_MEDIA, "can't write label -- disk unusable!");
}

sd_glabel (l, bb, rtn)
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

sd_cyl (l)
	struct disk_label *l;
{
	dsp->ds_maxcyl = l->dl_ncyl;
}

sd_geterr()
{
}

sd_req (cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	struct scsi_req sr;
	register struct cdb_6 *c6p = (struct cdb_6*)&sr.sr_cdb;
	int rtn;
	
	bzero (&sr, sizeof (sr));
	bzero (c6p, sizeof (*c6p));

	switch (cmd) {
	case CMD_SEEK:
		c6p->c6_opcode = C6OP_SEEK;

		/* convert cyl # (p1) to a block address */
		c6p->c6_lba = p1 * dt->d_nsectors * dt->d_ntracks;
		sr.sr_dma_max = 0;
		sr.sr_ioto = 2;
		break;

	case CMD_READ:
		c6p->c6_opcode = C6OP_READ;
		sr.sr_dma_dir = SR_DMA_RD;
		goto rw;

	case CMD_WRITE:
		c6p->c6_opcode = C6OP_WRITE;
		sr.sr_dma_dir = SR_DMA_WR;
rw:
		c6p->c6_lba = p1 * DEV_BSIZE / devblklen;
		c6p->c6_len = howmany(p3,devblklen);
		sr.sr_addr = (caddr_t) p2;
		sr.sr_dma_max = p3;
		sr.sr_ioto = 5;
		break;
	}
	rtn = ioctl (fd, SDIOCSRQ, &sr);
	if((rtn == 0) && sr.sr_io_status)	/* catch SCSI errors */
		rtn = -1;
	exec_time = sr.sr_exec_time;
	return(rtn);
	
} /* sd_req() */

sd_pstats()
{
}

sd_ecc()
{
	return (0);
}

#define FORMAT_CMD		"/usr/etc/sdform"
#define SD_FORMAT_SIZE_MAX	6000000

sd_format(int fd, char *devname, int force) 
{
	int rtn;
	int data;
	char cmd_str[80];
	
	/*
	 * 'force' false means we're just doing an init; in that case, we only
	 * do a format for small removable disks.
	 */
	if(!force) {
		struct inquiry_reply ir;		
		struct capacity_reply cr;
		
		/*
		 * Removable?
		 */
		if(rtn = do_inquiry(fd, &ir))
			return(rtn);
		if(!ir.ir_removable || (ir.ir_devicetype != DEVTYPE_DISK)) 
			return(0);
		/*
		 * Small enough?
		 */
		if(rtn = do_readcapacity(fd, &cr))
			return(rtn);
		if((cr.cr_blklen * (cr.cr_lastlba + 1)) > SD_FORMAT_SIZE_MAX)
			return(0);
	}
	/*
	 * Format the disk - use partition a
	 */
	sprintf(cmd_str, "%s %sa n\n", FORMAT_CMD, devname);
	if(system(cmd_str)) 
		return(1);		/* bomb on error */
	/*
	 * Set volume to formatted state
	 */
	data = 1;
	ioctl(fd, DKIOCSFORMAT, &data);
	return(0);
}


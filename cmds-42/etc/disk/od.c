/*	@(#)od.c	1.0	08/28/87	(c) 1987 NeXT	*/
/* 
 **********************************************************************
 * HISTORY
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Added exec_time mechanism for scsi_req support.
 *
 **********************************************************************
 */
#include <sys/types.h>
#include <sys/param.h>
#include <ufs/fs.h>
#include <sys/ioctl.h>
#include <nextdev/disk.h>
#include <nextdev/odvar.h>
#include "disk.h"
#include <errno.h>

od_conf()
{
}

od_init()
{
	register int size;
	register struct disk_label *l = &disk_label;

	/* reset the bitmap to untested for every half track */
	if (bit_map)
		free (bit_map);
	if (l->dl_version == DL_V1)
		size = (d_size / l->dl_nsect) >> 1;
	else
		size = d_size >> 2;
	bit_map = (int*) malloc (size);
	bzero (bit_map, size);
	write_bitmap();
}

od_wlabel (bb)
	struct bad_block *bb;
{
	register struct disk_label *l = &disk_label;

	if (ioctl (fd, DKIOCSLABEL, l) < 0)
		panic (S_MEDIA, "can't write label -- disk unusable!");
	if (bb && ioctl (fd, DKIOCSBBT, bb) < 0)
		panic (S_MEDIA, "can't write bad block table!");
}

od_glabel (l, bb, rtn)
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
	if (bb && ioctl (fd, DKIOCGBBT, bb) < 0) {
		if (rtn)
			return (-1);
		panic (S_NEVER, "get bbt");
	}
	return (0);
}

od_geterr()
{
	register struct dr_errmap *de = (struct dr_errmap*) &req.dr_errblk;

	if (de->de_err == E_ECC)
		return (ERR_ECC);
	return (ERR_UNKNOWN);
}

od_cyl (l)
	struct disk_label *l;
{
	/* undo pseudo-cylinders (-10 to stay away from bitmap) */
	dsp->ds_maxcyl = l->dl_ncyl * l->dl_ntrack - 10;
}

od_req (cmd, p1, p2, p3, p4, p5, p6, p7, p8)
{
	register struct disk_req *dr = &req;
	register struct dr_cmdmap *dc = (struct dr_cmdmap*) dr->dr_cmdblk;
	register struct disk_label *l = &disk_label;
	int rtn;
	
	bzero (dr, sizeof (*dr));
	bzero (dc, sizeof (*dc));

	switch (cmd) {
		case CMD_SEEK:
			dc->dc_cmd = OMD_SEEK;
			dc->dc_blkno = p1;
			dc->dc_wait = p2;
			break;

		case CMD_READ:
			dc->dc_cmd = OMD_READ;
			goto rwv;

		case CMD_WRITE:
			dc->dc_cmd = OMD_WRITE;
			goto rwv;

		case CMD_ERASE:
			dc->dc_cmd = OMD_ERASE;
			goto rwv;

		case CMD_VERIFY:
			dc->dc_cmd = OMD_VERIFY;
rwv:
			dc->dc_blkno = p1 * DEV_BSIZE / devblklen;
			dr->dr_addr = (caddr_t) p2;
			dr->dr_bcount = p3;
			if (p4 == SPEC_RETRY) {
				dc->dc_flags = DRF_SPEC_RETRY;
				dc->dc_retry = p5;
				dc->dc_rtz = p6;
			}
			break;

		case CMD_EJECT:
			dc->dc_cmd = OMD_EJECT;
			break;

		case CMD_RESPIN:
			dc->dc_cmd = OMD_RESPIN;
			break;
	}
	rtn = ioctl (fd, DKIOCREQ, dr);
	exec_time = dr->dr_exec_time;
	return(rtn);
}

od_pstats() {
	register struct od_stats *s = (struct od_stats*) stats.s_stats;

	printf ("\t%7d verify retries\n", s->s_vfy_retry);
	printf ("\t%7d verify failures\n", s->s_vfy_failed);
}

od_ecc()
{
	register struct dr_errmap *de = (struct dr_errmap*) &req.dr_errblk;

	return (de->de_ecc_count);
}

od_format(int fd, char *devname, int force)
{
	if(force) {
		printf("disk: can not format optical disk!\n");
		return(EINVAL);
	}
	else
		return(0);	/* always formatted */
}

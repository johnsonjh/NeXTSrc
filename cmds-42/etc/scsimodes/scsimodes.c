/*	scsimodes.c
 *
 * History
 * -------
 * 17-Feb-89	Doug Mitchell at NeXT
 *	Changed to use scsi_req mechanism
 */
 
/* #define DEBUG 1 */
#define USE_SCSI_REQ 1

#include <nextdev/disk.h>
#ifdef 	DEBUG
#include </os/dmitch/MK/mk/nextdev/scsireg.h>
#else	DEBUG
#include <nextdev/scsireg.h>
#endif	DEBUG

#include <stdio.h>

#define SCSI_TIMEOUT	15		/* command timeout in seconds */

#ifdef notdef
typedef unsigned char u_char;
typedef unsigned int u_int;
typedef unsigned short u_short;
#endif notdef

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

struct error_recovery_params {
	u_char	erp_savable:1,
		erp_reserved:1,
		erp_pagecode:6;
	u_char	erp_pagelen;
	u_char	erp_awre:1,
		erp_arre:1,
		erp_tb:1,
		erp_rc:1,
		erp_eec:1,
		erp_per:1,
		erp_dte:1,
		erp_dcr:1;
	u_char	erp_retrycnt;
	u_char	erp_correctspan;
	u_char	erp_headoffcnt;
	u_char	erp_dstrobecnt;
	u_char	erp_recoverytime;
};

struct connect_params {
	u_char	cp_savable:1,
		cp_reserved:1,
		cp_pagecode:6;
	u_char	cp_pagelen;
	u_char	cp_buffull;
	u_char	cp_bufempty;
	u_short	cp_businactive;
	u_short	cp_disconnect;
	u_short	cp_connect;
	u_char	cp_reserved2;
	u_char	cp_reserved3;
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

main(argc, argv)
int argc;
char **argv;
{
	int fd;
#ifdef	USE_SCSI_REQ
	struct scsi_req sr;
#else	USE_SCSI_REQ
	struct disk_req dr;
#endif	USE_SCSI_REQ
	struct mode_sense_cmd *mscp;
	struct mode_sense_reply *msrp1;
	struct mode_sense_reply *msrp2;
	struct cdb_10 *c10p;
	struct capacity_reply *crp;
	char reply_buf1[256];
	char reply_buf2[256];
	char reply_buf3[256];
	struct drive_info di;
	int err, Cflag, vflag;
	int exitflag;

	exitflag = 0;
	progname = *argv++;
	argc--;
	for (; argc > 1 && **argv == '-'; argv++, argc--) {
		switch ((*argv)[1]) {
		case 'C':
			Cflag++;
			break;
		case 'v':
			vflag++;
			break;
		default:
			fprintf(stderr, "%s: Unknown option: %s\n",
			    progname, *argv);
			exit(1);
		}
	}
	if (argc != 1)
		fatal("Usage: %s [ -B ] RAW_DEV", progname);
	fd = open(*argv, 0);
	if (fd < 0)
		fatal("Can't open %s", *argv);
#ifdef	USE_SCSI_REQ
	mscp = (struct mode_sense_cmd *)&sr.sr_cdb;
	bzero((char *)mscp, sizeof(sr.sr_cdb));
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = 0x03;	/* dasd format params */
	mscp->msc_len = 255;
	sr.sr_addr = reply_buf1;
	sr.sr_dma_max = sizeof(reply_buf1);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;
	err = ioctl(fd, SDIOCSRQ, &sr);
#else	USE_SCSI_REQ
	mscp = (struct mode_sense_cmd *)dr.dr_cmdblk;
	bzero((char *)mscp, sizeof(dr.dr_cmdblk));
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = 0x03;	/* dasd format params */
	mscp->msc_len = 255;
	dr.dr_addr = reply_buf1;
	dr.dr_bcount = sizeof(reply_buf1);
	err = ioctl(fd, DKIOCREQ, &dr);
#endif	USE_SCSI_REQ
	if (err) {
		fprintf(stderr, "mode sense for dasd params failed\n");
		exitflag++;
	}
	msrp1 = (struct mode_sense_reply *)reply_buf1;

#ifdef	USE_SCSI_REQ
	bzero((char *)mscp, sizeof(sr.sr_cdb));
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = 0x04;	/* rigid drive parameters */
	mscp->msc_len = 255;
	sr.sr_addr = reply_buf2;
	sr.sr_dma_max = sizeof(reply_buf2);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;
	err = ioctl(fd, SDIOCSRQ, &sr);
#else	USE_SCSI_REQ
	bzero((char *)mscp, sizeof(dr.dr_cmdblk));
	mscp->msc_opcode = C6OP_MODESENSE;
	mscp->msc_pcf = 0;	/* report current values */
	mscp->msc_page = 0x04;	/* rigid drive parameters */
	mscp->msc_len = 255;
	dr.dr_addr = reply_buf2;
	dr.dr_bcount = sizeof(reply_buf2);
	err = ioctl(fd, DKIOCREQ, &dr);
#endif	USE_SCSI_REQ
	if (err) {
		fprintf(stderr, "mode sense for rigid drive params failed\n");
		exitflag++;
	}
	msrp2 = (struct mode_sense_reply *)reply_buf2;

#ifdef	USE_SCSI_REQ
	c10p = (struct cdb_10 *)&sr.sr_cdb;
	bzero((char *)c10p, sizeof(sr.sr_cdb));
	c10p->c10_opcode = C10OP_READCAPACITY;
	sr.sr_addr = reply_buf3;
	sr.sr_dma_max = sizeof(reply_buf3);
	sr.sr_ioto = SCSI_TIMEOUT;
	sr.sr_dma_dir = SR_DMA_RD;
	err = ioctl(fd, SDIOCSRQ, &sr);
#else	USE_SCSI_REQ
	c10p = (struct cdb_10 *)dr.dr_cmdblk;
	bzero((char *)c10p, sizeof(dr.dr_cmdblk));
	c10p->c10_opcode = C10OP_READCAPACITY;
	dr.dr_addr = reply_buf3;
	dr.dr_bcount = sizeof(reply_buf3);
	err = ioctl(fd, DKIOCREQ, &dr);
#endif	USE_SCSI_REQ
	if (err) {
		fprintf(stderr, "read capacity failed\n");
		exitflag++;
	}
	crp = (struct capacity_reply *)reply_buf3;

	if (vflag) {
		printf("plh_len = %d\n", msrp1->msr_plh.plh_len);
		printf("plh_medium = %d\n", msrp1->msr_plh.plh_medium);
		printf("plh_wp = %d\n", msrp1->msr_plh.plh_wp);
		printf("plh_blkdesclen = %d\n", msrp1->msr_plh.plh_blkdesclen);

		printf("bd_density = %d\n", msrp1->msr_bd.bd_density);
		printf("bd_nblk = %d\n", msrp1->msr_bd.bd_nblk);
		printf("bd_blklen = %d\n", msrp1->msr_bd.bd_blklen);

		printf("dfp_savable = %d\n", msrp1->msr_dfp.dfp_savable);
		printf("dfp_pagecode = %d\n", msrp1->msr_dfp.dfp_pagecode);
		printf("dfp_pagelen = %d\n", msrp1->msr_dfp.dfp_pagelen);
		printf("dfp_trkszone = %d\n", msrp1->msr_dfp.dfp_trkszone);
		printf("dfp_altsecszone = %d\n",
		    msrp1->msr_dfp.dfp_altsecszone);
		printf("dfp_alttrkszone = %d\n",
		    msrp1->msr_dfp.dfp_alttrkszone);
		printf("dfp_alttrksvol = %d\n", msrp1->msr_dfp.dfp_alttrksvol);
		printf("dfp_sectors = %d\n", msrp1->msr_dfp.dfp_sectors);
		printf("dfp_bytessector = %d\n",
		    msrp1->msr_dfp.dfp_bytessector);
		printf("dfp_interleave = %d\n", msrp1->msr_dfp.dfp_interleave);
		printf("dfp_trkskew = %d\n", msrp1->msr_dfp.dfp_trkskew);
		printf("dfp_cylskew = %d\n", msrp1->msr_dfp.dfp_cylskew);
		printf("dfp_ssec = %d\n", msrp1->msr_dfp.dfp_ssec);
		printf("dfp_hsec = %d\n", msrp1->msr_dfp.dfp_hsec);
		printf("dfp_rmb = %d\n", msrp1->msr_dfp.dfp_rmb);
		printf("dfp_surf = %d\n", msrp1->msr_dfp.dfp_surf);

		printf("plh_len = %d\n", msrp2->msr_plh.plh_len);
		printf("plh_medium = %d\n", msrp2->msr_plh.plh_medium);
		printf("plh_wp = %d\n", msrp2->msr_plh.plh_wp);
		printf("plh_blkdesclen = %d\n", msrp2->msr_plh.plh_blkdesclen);

		printf("bd_density = %d\n", msrp2->msr_bd.bd_density);
		printf("bd_nblk = %d\n", msrp2->msr_bd.bd_nblk);
		printf("bd_blklen = %d\n", msrp2->msr_bd.bd_blklen);

		printf("rdp_savable = %d\n", msrp2->msr_rdp.rdp_savable);
		printf("rdp_pagecode = %d\n", msrp2->msr_rdp.rdp_pagecode);
		printf("rdp_pagelen = %d\n", msrp2->msr_rdp.rdp_pagelen);

#define	THREE_BYTE(x)	\
		(((x##msb)<<16)|((x##inb)<<8)|(x##lsb))

#define	TWO_BYTE(x)	\
		(((x##msb)<<8)|(x##lsb))

		printf("rdp_maxcyl = %d\n",
		    THREE_BYTE(msrp2->msr_rdp.rdp_maxcyl));

		printf("rdp_maxheads = %d\n", msrp2->msr_rdp.rdp_maxheads);

		printf("rdp_wpstart = %d\n",
		    THREE_BYTE(msrp2->msr_rdp.rdp_wpstart));
		printf("rdp_rwcstart = %d\n",
		    THREE_BYTE(msrp2->msr_rdp.rdp_rwcstart));

		printf("rdp_steprate = %d\n",
		    TWO_BYTE(msrp2->msr_rdp.rdp_steprate));

		printf("rdp_landcyl = %d\n",
		    THREE_BYTE(msrp2->msr_rdp.rdp_landcyl));

		printf("last logical block=%d\n", crp->cr_lastlba);
		printf("block length=%d\n", crp->cr_blklen);
		goto done;
	}
	if (Cflag) {
		printf("%d\n",
		  ((crp->cr_lastlba+1) * msrp1->msr_dfp.dfp_bytessector) /
		  (1024*1024));
		goto done;
	}

	err = ioctl(fd, DKIOCINFO, &di);
	if (err) {
		fprintf(stderr, "Couldn't get drive info for %s\n", *argv);
		exitflag++;
	} else {
		printf("SCSI information for %s\n", *argv);
		printf("Drive type: %s\n", di.di_name);
	}

	printf("%d bytes per sector\n", msrp1->msr_dfp.dfp_bytessector);
	printf("%d sectors per track\n", msrp1->msr_dfp.dfp_sectors);
	printf("%d tracks per cylinder\n", msrp2->msr_rdp.rdp_maxheads);
	printf("%d cylinder per volume (including spare cylinders)\n",
	    THREE_BYTE(msrp2->msr_rdp.rdp_maxcyl));
	if (msrp1->msr_dfp.dfp_trkszone > 1)
		printf("%d spare sectors per cylinder\n",
		    msrp1->msr_dfp.dfp_altsecszone);
	else if (msrp1->msr_dfp.dfp_trkszone == 1)
		printf("%d spare sectors per track\n",
		    msrp1->msr_dfp.dfp_altsecszone);
	else
		printf("Host bad block handling\n");
	printf("%d alternate tracks per volume\n",
	    msrp1->msr_dfp.dfp_alttrksvol);

	printf("%d usable sectors on volume\n", crp->cr_lastlba);

done:
	close(fd);
	exit(exitflag);
}

fatal(msg, arg)
char *msg;
{
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, msg, arg);
	fprintf(stderr, "\n");
	exit(1);
}

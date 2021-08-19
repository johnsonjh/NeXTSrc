/* 
 * SCSI disk format program 
 */
 
/* History
 *
 * 20-Jul-90	Doug Mitchell at NeXT
 *	Added no_prompt option
 * 02-Oct-89	Doug Mitchell at NeXT
 *	Created.
 */
 
#include <sys/types.h>
#include <nextdev/scsireg.h>
#include <nextdev/disk.h>
#include <fcntl.h>

/*	
 * Format Command CDB
 */
 
struct fmt_cdb {
	u_char 	fc_opcode;
	u_char  fc_lun:3,
		fc_fmtdata:1,		/* enables data out phase */
		fc_cmplst:1,		/* indicates D list is complete */
		fc_dlf:3;		/* defect list format - values below */
	u_char  fc_rsvd2;		/* reserved */
	u_short fc_ileave;		/* interleave factor */
	u_char	fc_ctrl;		/* control byte */
};

#define FMTD_BLOCK		0x00	/* defect list = block format */
#define FMTD_INDEX		0x04	/* defect list = bytes from index */
#define FMTD_PHYS		0x05	/* defect list = physical sector */

#define MB_PER_MINUTE		30	/* time to format */
#define FORMAT_TIME_MINIMUM	5	/* for floppy only */
#define	SHOW_FIRST_FAIL		1	/* show Maxtor's persnicketiness */

struct status_val {
	int io_stat;
	char *io_text;
};

struct status_val stat_array[] = {

	{ SR_IOST_GOOD,			"successful"			   },
	{ SR_IOST_SELTO,		"selection timeout" 		   },
	{ SR_IOST_CHKSV,		"check status, sr_esense valid"    },
	{ SR_IOST_CHKSNV,		"check status, sr_esense not valid"},
	{ SR_IOST_DMAOR,		"sr_dma_max exceeded" 		   },
	{ SR_IOST_IOTO,			"sr_ioto exceeded" 		   },
	{ SR_IOST_BV,			"SCSI Bus violation"		   },
	{ SR_IOST_CMDREJ,		"command reject (by driver)"	   },
	{ SR_IOST_MEMALL,		"memory allocation failure" 	   },
	{ SR_IOST_MEMF,			"memory fault" 			   },
	{ SR_IOST_PERM,			"not super user" 		   },
	{ SR_IOST_NOPEN,		"device not open"		   },
	{ SR_IOST_TABT,			"target aborted command" 	   },
	{ ST_IOST_BADST,		"bad SCSI status byte"  	   },
	{ ST_IOST_INT,			"internal driver error" 	   },
	{ 0,				0				   }

}; /* stat_array[] */


main(int argc, char **argv) {

	struct capacity_reply cr;	/* see nextdev/scsireg.h */
	struct esense_reply sense_buf;	/* ditto */
	int format_time;		/* approx. time to format disk in
					 * minutes */
	int capacity_mbytes;		/* disk capacity in MBytes */
	char inch;
	int fd;
	int no_prompt=0;
	int arg;
	int one = 1;
		
	if(argc < 2) 
		usage(argv);
	if(check_rootdev(argv[1])) {
		printf("\n%s is mounted as root. You cannot format "
		       "your root device.\n", argv[1]);
		exit(1);
	}
	if ((fd = open (argv[1], O_RDWR)) <= 0) {
		printf("\nCould not open %s\n",argv[1]);
		perror("open()");
		exit(-1);
	}
	for(arg=2; arg<argc; arg++) {
		inch = argv[arg][0];
		switch(inch) {
		    case 'n':
		    	no_prompt++;
			break;
		    default:
		    	usage(argv);
		}
	} 
	/*
	 * request sense - make sure drive is alive
	 */
	if(do_request_sense(fd, &sense_buf))
		exit(1);

	/* 
	 * Read Capacity - just to find out how long this will take
	 */
	if(do_read_capacity(fd, &cr))
		exit(1);
	capacity_mbytes = (cr.cr_blklen * (cr.cr_lastlba + 1)) / (1024*1024);
	format_time = capacity_mbytes / MB_PER_MINUTE;
	if(format_time < FORMAT_TIME_MINIMUM) {
		/*
		 * Floppy...
		 */
		format_time = FORMAT_TIME_MINIMUM;
	}
	
	printf("device = %s   block size = %d   capacity = %d MBytes\n",
		argv[1], cr.cr_blklen, capacity_mbytes);
	if(!no_prompt) {
		printf("\n***FORMATTING THIS DISK CAUSES ALL DISK DATA TO BE"
			" LOST***\n");
		printf("   This will take approximately %d minutes.\n", 
			format_time);
		printf("\n   Do you wish to proceed? (Y/anything) ");
		inch = getchar();
		if(inch != 'Y') {
			printf("\nFormat Aborted\n");
			exit(1);
		}
		printf("\n");
	}
	printf("Disk Format in progress...\n");
	if(do_format(fd, format_time))
		exit(1);
	/*
	 * ignore error on this for compatibility with old kernels
	 */
	ioctl(fd, DKIOCSFORMAT, &one);
	printf("***Format Complete***\n");	
	exit(0);
	
} /* main() */ 


usage(char **argv) {
	printf("Usage: %s raw_device [n(o prompt)]\n", argv[0]);
	exit(1);
}

do_request_sense(int fd, struct esense_reply *sbp) {

	struct scsi_req sr;	
	struct cdb_6 *cdbp = &sr.sr_cdb.cdb_c6;
	
	cdb_clr(cdbp);
	cdbp->c6_opcode = C6OP_REQSENSE;
	cdbp->c6_len	= 0x40;
	sr.sr_dma_dir	= SR_DMA_RD;
	sr.sr_addr	= (caddr_t)sbp;
	sr.sr_dma_max	= sizeof(struct esense_reply);
	sr.sr_ioto	= 10;
	return(do_ioc(fd, &sr, "Request Sense"));

} /* do_request_sense() */

int do_read_capacity(int fd, struct capacity_reply *crp) {

	struct scsi_req sr;
	struct cdb_10 *cdbp = &sr.sr_cdb.cdb_c10;
	int i;
	char *cp;
	
	cdb_clr(cdbp);
	cp = (char *)(crp);
	for(i=0; i<sizeof(struct capacity_reply); i++)
		*cp++ = 0;
		
	cdbp->c10_opcode = C10OP_READCAPACITY;
	cdbp->c10_lun    = 0;
	sr.sr_dma_dir	 = SR_DMA_RD;
	sr.sr_addr	 = (char *)crp;
	sr.sr_dma_max	 = sizeof(struct capacity_reply);
	sr.sr_ioto	 = 10;
	return(do_ioc(fd, &sr, "Read Capacity"));
}

do_format(int fd, int timeout) {	/* timeout in minutes */

	struct scsi_req sr, *srp=&sr;
	struct fmt_cdb *cdb = (struct fmt_cdb *)&sr.sr_cdb;
	int rtn=0, ioc_rtn=0;
		
	/* current implementation: 
	 *
	 *	FMT data = 0
	 *
	 *	This uses P list only.
	 */
	bzero(srp,sizeof(*srp));

	/* build the CDB */
	cdb->fc_opcode = C6OP_FORMAT;
	cdb->fc_fmtdata = 0;
	cdb->fc_dlf = FMTD_BLOCK;	
	cdb->fc_ileave = 0;		/* default - should force 1:1 */

	/* necessary scsi_req stuff */
	sr.sr_dma_dir	= SR_DMA_WR;
	sr.sr_ioto	= 60 * timeout * 4;

	/* note the first one may fail...don't use do_ioc(), since that
	 * would always report any errors.
	 */
	ioc_rtn = ioctl(fd, SDIOCSRQ, &sr);
	if(ioc_rtn || sr.sr_io_status) {
	
		/* well, try with another defect list format. Some Maxtors
		 * bitch about this for no reason.
		 */
		 
#ifdef	SHOW_FIRST_FAIL
		printf("...Retrying with cdb->fc_dlf = FMTD_INDEX\n");
		printf("...rtn = %d(d)   sr_io_status = %XH\n",
			rtn, sr.sr_io_status);
		if(sr.sr_io_status) {
			pr_stat_text(sr.sr_io_status);
			if(sr.sr_io_status == SR_IOST_CHKSV) {
			    printf("sense key = %02XH   sense code = %02XH\n",
				    sr.sr_esense.er_sensekey,
				    sr.sr_esense.er_addsensecode);
			    dump_buf(&sr.sr_esense,"Sense Data",
				      sr.sr_esense.er_addsenselen+8);
			}
			printf("SCSI status = %02XH\n",sr.sr_scsi_status);
		}
#endif	SHOW_FIRST_FAIL
		bzero(srp,sizeof(*srp));
		
		/* build the CDB */
		cdb->fc_opcode = C6OP_FORMAT;
		cdb->fc_fmtdata = 0;
		cdb->fc_dlf = FMTD_INDEX;	
		cdb->fc_ileave = 0;	
	
		/* necessary scsi_req stuff */
		sr.sr_dma_dir	= SR_DMA_WR;
		sr.sr_ioto	= 60 * timeout * 4;
	
		/* this one HAS to work...use standard I/O routine */
		rtn = do_ioc(fd, &sr, "Format");
	}
	if(rtn) {
		printf("\n\n***FORMAT UNIT COMMAND FAILED***\n");
		if(ioc_rtn)
		    	perror("ioctl(SDIOCSRQ)");
	}    
	return(rtn | ioc_rtn);	
	
} /* do_format */
 

/**
 ** utilities
 **/
 
cdb_clr(cdbp)
struct cdb_10 *cdbp;
{
	int i;
	char *p;
	
	p = (char *)cdbp;
	for(i=0; i<sizeof(*cdbp); i++)
		*p++ = 0;
}

do_ioc(int fd, struct scsi_req *sr, char *command_str)
{	
	
	if (ioctl(fd, SDIOCSRQ, sr) < 0) {
		printf("%s command failed\n",command_str);
	    	perror("ioctl(SDIOCSRQ)");
	    	return(1);
	}
	if(sr->sr_io_status) {
		printf("%s command failed\n",command_str);
		pr_stat_text(sr->sr_io_status);
		switch(sr->sr_io_status) {
		    case SR_IOST_CHKSNV: {
		    	struct esense_reply sense_buf, *sb=&sense_buf;
			
		    	/* 
			 * do a request sense command 
			 */
			if(do_request_sense(fd, sb))
				break;
			printf("   sense key = %02XH   sense code = %02XH\n",
				sb->er_sensekey,
				sb->er_addsensecode);
			break;
		    }
		    default:
		    	printf("SCSI status = %02XH\n",sr->sr_scsi_status);
		}
	    	return(1);
	}
	return(0);
} /* do_ioc() */

pr_stat_text(io_stat)
int io_stat;
{
	struct status_val *sp = stat_array;
	
	printf("sr_io_status = %d ",io_stat);
	while(sp->io_text) {
		if(sp->io_stat == io_stat) {
			printf("; %s\n",sp->io_text);
			return;
		}
		sp++;
	}
	printf("; Undefined\n");
}

int dump_buf(u_char *buf, char *name, int size) {

	int i;
	char c;
	
	printf("%s:\n",name);
	for(i=0; i<size; i++) {
		if((i>0) && (i%0x100 == 0)) {
			printf("\n...More? (y/n)\n");
			/* newline alone - take as 'y'; no need
			 * for second CR
			 */
			c = getchar();
			if(c != '\n')
				getchar();
			if(c == 'n')
				return;
		}
		if(i%0x10 == 0)
			printf("\n %03X: ",i);
		else if(i%8 == 0)
			printf("  ");
		printf("%02X ",buf[i]);
	}
	printf("\n");
} /* dump_buf() */

#include <sys/param.h>
#include <sys/stat.h>
#include <sys/time.h>		/* for sys/vnode.h */
#include <sys/vnode.h>		/* for sys/inode.h */
#include <ufs/inode.h>
#include <ufs/fs.h> 

check_rootdev(char *raw_dev_name) {

	/* 
	 * see if block device associated with *raw_dev_name is mounted as
	 * root.
	 *
	 * Returns 0 if *raw_dev_name is not mounted as root; 1 if it is or
	 * any error occurs (like can't stat root). 
	 */
	 
	struct stat statf_root, statf_dev;
	char block_dev_name[30];
	int block_dev_dex, raw_dev_dex;
	
	/* get block device associated with *devname. Force a trailing 'a',
	 * since all partitions are treated equally here.
	 */

	strcpy(block_dev_name, "/dev/");
	block_dev_dex = 5;
	raw_dev_dex = 6; 
	while(raw_dev_name[raw_dev_dex]) 
		block_dev_name[block_dev_dex++] = raw_dev_name[raw_dev_dex++];
	block_dev_name[block_dev_dex-1] = 'a';
	block_dev_name[block_dev_dex] = '\0';
	
	/* stat the root directory and the block device */
	if(stat("/",&statf_root)) {
		perror("stat(/)");
		return(1);
	}
	if(stat(block_dev_name, &statf_dev)) {
		/* this is OK here. Caller must handle non-existent device. */
		return(0);
	}
	
	/* the st_rev field of block_dev_name is the actual device 
	 * it represents; this will match st_dev of "/" if the disk is
	 * mounted as root.
	 */
	if(statf_root.st_dev == statf_dev.st_rdev) 
		return(1);
	else
		return(0);		/* OK */
		
} /* check_rootdev() */

/* end of format.c */

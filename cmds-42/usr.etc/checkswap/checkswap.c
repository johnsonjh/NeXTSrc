/*
 *   checkswap - see if sd0 is a swap device.
 *  
 *   The crieria for "swap device" is:
 *	-- sd0 exists, and 
 *	-- The first 8 chars of the label name (disk_label.dl_label) are
 *	   "swapdisk".
 */
 
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <nextdev/disk.h>

/*
 *  exit codes
 */
 
#define FSR_NONE	0	/* disk not present */
#define FSR_NOLABEL	1	/* disk present, no valid label */
#define FSR_LABEL	2	/* disk present, valid label, not swapdisk */
#define FSR_SWAPDISK	3	/* present, label, swapdisk */
#define FSR_ERROR	-1

#define SWAPDISK_NAME	"swapdisk"
#define DEV_NAME 	"/dev/rsd0a"	/* default device */

int verbose=0;

main(int argc, char **argv) {

	int fd;
	struct disk_label label;
	char diskname[80];
	int arg;
	char *devname=DEV_NAME;
	
	if(argc > 4) 
		usage(argv);
	for(arg=1; arg<argc; arg++) {
		if(argv[arg][1] == 'v') {
			verbose++;
			continue;
		}
		if(argv[arg][1] == 'f') {
			devname = argv[++arg];
			continue;
		}
	}
	if((fd = open(devname,O_RDONLY|O_NDELAY)) <= 0)
		quit(FSR_NONE);
	if(ioctl(fd, DKIOCGLABEL, &label))
		quit(FSR_NOLABEL);		/* no label present */
	if(strncmp(label.dl_label,SWAPDISK_NAME,8) != 0)
		quit(FSR_LABEL);		/* nope */
	else
		quit(FSR_SWAPDISK);		/* TA DA! */
	
} /* main() */

usage(char **argv) {
	printf("\nusage: %s [-v] [-f raw_device]\n", argv[0]);
	quit(FSR_ERROR);
}

quit(int exit_code) {
	if(verbose)
		printf("..exit_code = %d\n",exit_code);
	exit(exit_code);
}

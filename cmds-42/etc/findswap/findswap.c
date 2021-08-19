/*
 *   findswap - see if sd0 is a swap device.
 *  
 *   The crieria for "swap device" is:
 *	-- sd0 exists, and 
 *	-- its capacity is less than or equal to "threshold" bytes. 
 *	   "threshold" is passed as argv[1].
 */
 
#include <sys/types.h>
#include <sys/param.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <fcntl.h>
#include <nextdev/disk.h>

#define DISK_NAME "/dev/rsd0a"
/*
 *  exit codes
 */
 
#define FSR_NONE	0	/* sd0 not present */
#define FSR_TOOLARGE	1	/* sd0 present; capacity > threshold */
#define FSR_NODISKTAB	2	/* sd0 present, no disktab entry */
#define FSR_NOLABEL	3	/* swapdisk present, no valid label */
#define FSR_LABEL	4	/* swapdisk with valid label present */
#define FSR_ERROR	-1

int verbose=0;

main(int argc, char **argv) {

	int fd;
	struct disk_label label;
	struct drive_info info;
	struct disktab *dtp;
	u_int capacity;			/* disk capacity in bytes */
	u_int threshold;
	char diskname[80];
	
	if(argc < 2) 
		usage(argv);
	threshold = atoi(argv[1]);
	if(argc == 3) {
		if(argv[2][1] == 'v')
			verbose++;
	}
	if(threshold <= 0)
		usage(argv);
	if((fd = open(DISK_NAME,O_RDONLY)) <= 0)
		quit(FSR_NONE);
	if(ioctl(fd, DKIOCINFO, &info))
		quit(FSR_ERROR);
	/* 
	 * we have a device type name for sd0. get the disktab struct for it.
	 */
	sprintf(diskname, "%s-%d", info.di_name, info.di_devblklen);
	dtp = getdiskbyname(diskname);
	if(dtp == 0)
		quit(FSR_NODISKTAB);
	capacity = dtp->d_ncylinders * dtp->d_ntracks * 
	           dtp->d_nsectors * dtp->d_secsize;
	if(capacity == 0)
		quit(FSR_ERROR);
	if(capacity > threshold)		/* this disk is too big to
						 * be a swapdisk */
		quit(FSR_TOOLARGE);
	if(ioctl(fd, DKIOCGLABEL, &label))
		quit(FSR_NOLABEL);		/* no label present */
	quit(FSR_LABEL);			/* usable swapdisk */
	
} /* main() */

usage(char **argv) {
	printf("\nusage: %s threshold [-v]\n");
	quit(FSR_ERROR);
}

quit(int exit_code) {
	if(verbose)
		printf("..exit_code = %d\n",exit_code);
	exit(exit_code);
}

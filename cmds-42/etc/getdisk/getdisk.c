#define	KERNEL_FEATURES
#import	<sys/features.h>
#import <sys/types.h>
#import <sys/param.h>
#import <sys/ioctl.h>
#import <nextdev/disk.h>
#import <nextdev/insertmsg.h>
#import <sys/file.h>
#import <sys/signal.h>
#import <sys/dir.h>
#import <sys/stat.h>
#import <sys/kern_return.h>
#import <sys/message.h>
#import <errno.h>
#import <libc.h>
#import <stdlib.h>
#import <stdio.h>

#define	CTRL_DEV	"/dev/vol0"
#define	DEV_DIR		"/dev"

/*
 * getdisk [od | fd] LABEL
 *
 * Wait for a disk to be inserted, after it's inserted check that the
 * label matches LABEL and that the device is either the od or fd as
 * indicated.
 *
 * Outputs on stdout a string of the form:
 *	[RIGHT|WRONG|INTERRUPTED|ERROR] LABEL RAWDEV BLKDEV
 *
 * RAWDEV and BLKDEV may not be returned if INTERRUPTED or ERROR.
 *
 * Exit status is:
 *	0 if RIGHT
 *	1 if WRONG
 *	2 if INTERRUPTED
 *	3 if ERROR
 */

#define	streq(a,b)			(strcmp(a, b) == 0)

typedef enum {
	NO=0, YES=1
} logical_t;

typedef enum {
	RIGHT=0, WRONG=1, INTERRUPTED=2, ERROR=3
} status_t;

static char *status[] = {
	"RIGHT", "WRONG", "INTERRUPTED", "ERROR"
};

static const char *progname;

static int disk_inserted = NO;
static int debug = NO;

static int cfd = -1, rfd = -1;
static char blk_dev[50], raw_dev[50];
static struct disk_label dl;

static void terminate(status_t code);
static void int_handler(void);
static void fatal(const char *format, ...);
static void eject(int rfd);
static void devfind(int mode, dev_t dev, char *name, int namesize);

main(int argc, const char * const argv[])
{
	int volume, efd;
	const char *dev, *label, *argp;
	char ejct_dev[50];
	char c;
	struct sigvec sv;
	struct stat st;
	union {
		msg_header_t hdr;
		struct of_insert_notify_dev oid;
		char body[MSG_SIZE_MAX];
	} msg;
	kern_return_t result;
	port_t kern_notify_port;
	int desired_major_cdev;
	
	if (debug)
		freopen("/dev/console", "w", stderr);
	
	progname = *argv++; argc--;
	
	while (argc > 0 && **argv == '-') {
		argp = *argv++ + 1; argc --;
		while (c = *argp++) {
			switch (c) {
			case 'd':
				debug = YES;
				break;
			default:
				fatal("Unknown option: %c", c);
			}
		}
	}
	
	if (argc != 2)
		fatal("Usage: %s [fd | od] LABEL", progname);
	
	dev = *argv++;
	label = *argv;
		
	sprintf(ejct_dev, "/dev/r%s0a", dev);
	
	if (debug)
		fprintf(stderr, "Opening %s O_NDELAY\n", ejct_dev);
	efd = open(ejct_dev, O_NDELAY|O_RDONLY, 0);
	if (efd >= 0)
		eject(efd);
	else if (errno != EWOULDBLOCK)
		fatal("Can't open %s O_NDELAY", ejct_dev);
	if (stat(ejct_dev, &st) < 0)
		fatal("Can't stat %s", ejct_dev);
	desired_major_cdev = major(st.st_rdev);
	close(efd);
		
	if (debug)
		fprintf(stderr, "Opening %s\n", CTRL_DEV);

	if ((cfd = open(CTRL_DEV, O_RDONLY, 0)) < 0)
		fatal("Can't open %s", CTRL_DEV);
	
	sv.sv_handler = int_handler;
	sv.sv_mask = 0;
	sv.sv_flags = 0;
	if (sigvec(SIGINT, &sv, NULL))
		fatal("Can't sigvec");
	
	/*
	 * Get a controlling tty, so interrupts work
	 */
	if (debug)
		fprintf(stderr, "Opening /dev/console\n");
	open("/dev/console", O_RDWR, 0);
	
	result = port_allocate(task_self(), &kern_notify_port);
	if (result != KERN_SUCCESS)
		fatal("Port allocate failed: %s", mach_error_string(result));
	if (ioctl(cfd, DKIOCMNOTIFY, &kern_notify_port))
		fatal("DKIOCMNOTIFY failed");

	if (debug)
		fprintf(stderr, "Scavenging old insertions\n");
	while (1) {
		msg.hdr.msg_local_port = kern_notify_port;
		msg.hdr.msg_size = sizeof(msg);
		result = msg_receive(&msg, RCV_TIMEOUT, 0);
		if (result != RCV_SUCCESS)
			break;
	}
	if (result != RCV_TIMED_OUT)
		fatal("msg_receive failed: %s", mach_error_string(result));

	msg.hdr.msg_local_port = kern_notify_port;
	msg.hdr.msg_size = sizeof(msg);
	result = msg_receive(&msg, MSG_OPTION_NONE, 0);
	if (result != RCV_SUCCESS)
		fatal("msg_receive failed: %s", mach_error_string(result));
	if (msg.oid.oid_header.in_vol_state != IND_VS_LABEL) {
		strcpy(dl.dl_label, "Unlabeled");
		fprintf(stderr, "Unlabeled disk!\n");
		terminate(WRONG);
	}
	if (msg.oid.oid_header.in_dev_desc != IND_DD_DEV) {
		fprintf(stderr, "in_dev_desc incorrect!\n");
		terminate(ERROR);
	}
	if (major(msg.oid.oid_cdev_t) != desired_major_cdev) {
		strcpy(dl.dl_label, "Wrong_Device");
		terminate(WRONG);
	}
	
	devfind(S_IFCHR, msg.oid.oid_cdev_t, raw_dev, sizeof(raw_dev));
	devfind(S_IFBLK, msg.oid.oid_bdev_t, blk_dev, sizeof(blk_dev));

	if (debug) {
		fprintf(stderr, "Raw dev is %s, blk dev is %s\n", raw_dev,
		 blk_dev);
		fprintf(stderr, "Opening %s\n", raw_dev);
	}
	
	if ((rfd = open(raw_dev, O_RDONLY, 0)) < 0)
		fatal("Can't open %s", raw_dev);

	if (debug)
		fprintf(stderr, "Reading label\n");
		
	if (ioctl(rfd, DKIOCGLABEL, &dl) < 0)
		fatal("Can't read label on %s", raw_dev);
		
	if (debug)
		fprintf(stderr, "Label was %s\n", dl.dl_label);
	if (!streq(label, dl.dl_label)) {
		fprintf(stderr, "Wanted \"%s\", Got \"%s\"\n", label,
		 dl.dl_label);
		terminate(WRONG);
	}
	terminate(RIGHT);
}

static void
terminate(status_t code)
{
	port_t null_port = PORT_NULL;
	
	if (cfd >= 0)
		ioctl(cfd, DKIOCMNOTIFY, &null_port);
	if (dl.dl_label[0] == '\0')
		strcpy(dl.dl_label, "Unknown");
	if (raw_dev[0] == '\0')
		strcpy(raw_dev, "Unknown");
	if (blk_dev[0] == '\0')
		strcpy(blk_dev, "Unknown");
	if (debug)
		fprintf(stderr, "Exiting with %s %s %s %s\n",
		 status[code], dl.dl_label, raw_dev, blk_dev);		
	printf("%s %s %s %s\n", status[code], dl.dl_label, raw_dev, blk_dev);		
	exit(code);
}

static void
int_handler(void)
{
	fprintf(stderr, "Interrupted\n");
	terminate(INTERRUPTED);
}

static void
fatal(const char *format, ...)
{
	va_list ap;
	
	va_start(ap, format);
	fprintf(stderr, "%s: ", progname);
	vfprintf(stderr, format, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	terminate(ERROR);
}

static void
eject(int rfd)
{
	if (ioctl (rfd, DKIOCEJECT, 0) < 0)
		fatal("Can't eject disk");
}

static void
devfind(int mode, dev_t dev, char *name, int namesize)
{
	DIR *dirp;
	struct direct *dp;
	struct stat st;
	
	if ((dirp = opendir(DEV_DIR)) == NULL)
		fatal("Can't open %s", DEV_DIR);
	if (chdir(DEV_DIR) < 0)
		fatal("Can't chdir to %s", DEV_DIR);
	while ((dp = readdir(dirp)) != NULL) {
		dp->d_name[dp->d_namlen] = '\0';
		if (stat(dp->d_name, &st) < 0)
			continue;
		if ((st.st_mode & S_IFMT) != mode)
			continue;
		if (st.st_rdev == dev) {
			bzero(name, namesize);
			strncpy(name, DEV_DIR, namesize - 1);
			if (name[strlen(name) - 1] != '/')
				strncat(name, "/", namesize - 1);
			strncat(name, dp->d_name, namesize - 1);
			if (strlen(name) >= namesize - 1)
				fatal("Device name too long");
			closedir(dirp);
			return;
		}
	}
	closedir(dirp);
	fatal("Can't find entry in %s for major %d minor %d", DEV_DIR,
	  major(dev), minor(dev));
}	

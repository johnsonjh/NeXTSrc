/*
 * BLIT
 * Copyright 1989: NeXT, Inc.
 * Author: Morris Meyer
 *
 * Modifications:
 * 5/15/89 mmeyer	Cleanup, added getopt()
 * 5/12/89 mmeyer	Created
 */


#include "defs.h"
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <nextdev/video.h>
#include <nextdev/kmreg.h>
#include <nlist.h>
#include <syslog.h>


main (argc, argv)
    int argc;
    char *argv[];
{
	int console, vidfd, fbuf, ret, ch, way_hacked, retval, kmflag;
	char *image, *read_eps();
	struct stat stbuf;
	extern int optind;
	extern char *optarg;
	int bflag = FALSE;		/* We are booting; check km.flag */
	int width = 0;
	int height = 0;
	int xstart = 0;
	int ystart = 0;

	while ((ch = getopt (argc, argv, "bx:y:")) != EOF) {
		switch ((char)ch)  {
		case 'x':
			xstart = atoi (optarg);
			break;
		case 'y':
			ystart = atoi (optarg);
			break;
		case 'b':
		    	bflag = TRUE;
			break;
		default:
			fprintf (stderr, 
		   "Usage: %s [-b] [-x xstart] [-y ystart] epsfiles..\n", 
				 argv[0]);
			exit (1);
		}
	}
	/*
	 * Open /dev/vid0 and get a file descriptor and the address
	 * of the base of the frame buffer.
	 */
	if ((vidfd = open_video ("/dev/vid0", &fbuf)) == -1)  {
#ifdef DEBUG
		syslog (LOG_ERR, "BLIT: Error opening /dev/vid0: %m");
#endif DEBUG
		exit (0);
	}
	    	
	/*
	 * Check the kernel keyboard/mouse structure in <nextdev/kmreg.h>
	 * We check for km.flags & KMF_SEE_MSGS.  We don't want to blit
	 * anything in the console window.
	 */
	if (bflag == TRUE)  {
		if ((console = open ("/dev/console", O_RDONLY, 0)) == -1)  {
#ifdef DEBUG
			syslog (LOG_ERR, 
				"BLIT: Open of /dev/console failed: %m");
#endif DEBUG
			exit (0);
		}
		if (ioctl (console, KMIOCGFLAGS, &kmflag)  == -1)  {
#ifdef DEBUG
			syslog (LOG_ERR, 
				"BLIT: ioctl KMIOCGFLAGS failed: %m");
#endif DEBUG
			exit (0);
		}
		if (kmflag & KMF_SEE_MSGS)   {
#ifdef DEBUG
			syslog (LOG_ERR, "BLIT: EXITING kmflag (%d) set", kmflag);
#endif DEBUG
		    	exit (0);
		}
		(void) close (console);
    	}

	/*
	 * Read and parse the image from a file.  The return string is the
	 * bytes contained in a width*height inverted image.  blit_image
	 * will invert before putting in the frame buffer.
	 */
	
	for (; optind < argc; optind++)  {
		if ((image = read_eps (argv[optind], &width, &height)) == 
		    		(char *)NULL)
		    exit (0);
		blit_image (vidfd, image, fbuf, width, height, xstart, ystart);
#ifdef DEBUG
		syslog (LOG_ERR, "Blitting %s", argv[optind]);
#endif DEBUG
	}

	(void) close (vidfd);
	exit (0);
}

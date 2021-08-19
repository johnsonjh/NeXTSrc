#include "defs.h"
#include <sys/file.h>
#include <sys/ioctl.h>
#include <nextdev/video.h>
#include <nlist.h>
#include <syslog.h>

/*
 * open_video:  Handle opening /dev/vid0
 */

int open_video (const char *filename, int *framebuffer)
{
	int fd;

	if ((fd = open ("/dev/vid0", O_RDWR)) < 0)  {
		syslog (LOG_ERR, "Opening /dev/vid0: %m");
		exit (1);
	}
	*framebuffer = 0x12345678;
	if (ioctl (fd, DKIOCGADDR, &(*framebuffer)) < 0) {
		syslog (LOG_ERR, "BLIT ioctl: DKIOCGADDR: %m");
		exit (1);
	}
	return (fd);
}


void blit_image (const int fd, const char *image, const int fbuf, 
		 const int width,  const int height, 
		 const int xstart, const int ystart)
{
	unsigned int screenOffsets[4];
	unsigned int memoryOffsets[4];

	struct mwf mmwf;
	int yval, xval, i;
	int buffs[2];
	char *first, *last;
	char *ip;

#ifdef notdef  /* Beginning of fancy fade-in code */
	first = &buffs;
	last = &buffs + sizeof (buffs);

	mmwf.min = (int)first;
	mmwf.max = (int)last;

	ioctl (fd, DKIOCSMWF, &mmwf);
	for (i = 0; i < 4; i++)
	    	memoryOffsets[i] = mmwf.min + i*mmwf.max;

	for (i = 0; i < 4; i++)
	    	screenOffsets[i] = (i + 1) * 0x0100000;
#endif notdef

	ip = (char *)image;
	for (yval = ystart; yval < (ystart + height - 2); yval++)  {
	    /* change to roundup */
	    /* comment the heck outta this */
	    for (xval = xstart / 4; 
		 xval < (( ((xstart + width + 7)& ~0x3) - 8) / 4); 
		 xval++)  {
        *(unsigned char *)(fbuf + ((1152  * 2 / 8) * yval) + xval) = ~(*ip++);
	    }
	    *ip++;
        }
}

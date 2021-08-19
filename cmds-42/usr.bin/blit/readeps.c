#include "defs.h"
#include <stdio.h>
#include <sys/file.h>
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <nextdev/video.h>

char *read_eps (const char *epsfile, int *width, int *height)
{
	FILE *fp, *fopen();
	char *image, *bufp, *ip;
	char line [128], junk [100];
	struct stat stbuf;
	int imagesize, count;

	int in_eps = FALSE;
	int next_line_important = FALSE;

	fp = fopen (epsfile, "r");
	if (fp == (FILE *)NULL)  {
		perror (epsfile);
		exit (0);
	}
	
	/*
	 * Allocate a buffer at least the size of the file.
	 * Cheap way to do it.
	 */
	fstat (fileno (fp), &stbuf);
	bufp = (char *) malloc (stbuf.st_size);

	while (fgets (line, sizeof (line), fp) != NULL)  {
		if (strncmp (line, "image", 5) == 0)  {
		    	in_eps = TRUE;
			continue;
		}
		/*
		 * We are looking for the line right after the PS scaling
		 * factor.  It contains the width and height of the image.
		 * There might be a more robust way to parse this.
		 *
		 * gsave
		 * 0.000 0.000 translate
		 * 1.000 1.000 scale
		 * 125 70 scale
		 * 125 70 2
		 */
		if (strncmp (line, "1.000 1.000 scale", 17) == 0)  {
			next_line_important = TRUE;
			continue;
		}
		if (in_eps)  {
		    	/* 
			 * Take everything but the newline.
			 */
		    	strncat (bufp, line, strlen (line) -1);
		}
		if (next_line_important)  {
			next_line_important = FALSE;
			sscanf (line, "%d %d %s", &(*width), &(*height), junk);
		}

	}
	bufp[strlen (bufp) - strlen("grestore")] = '\0';

	imagesize = (*width) * (*height) / PIXELS_PER_BYTE;
	image = (char *) malloc (imagesize);
	
	/*
	 * For every two characters in the ASCII image, coalesce
	 * them down to one image byte.
	 */
	ip = image;
	for (count = 0; count < imagesize; count++)  {
		char first, second;
		first = *bufp++;
		second = *bufp++;
		if (first >= '0' && first <= '9')
		    	first -= '0';
		else if (first >= 'A' && first <= 'F')
		    	first = first - 'A' + 10;
		else if (first >= 'a' && first <= 'f')
		    	first = first - 'a' + 10;
		else
		   	return ((char *)NULL);

		if (second >= '0' && second <= '9')
		    	second -= '0';
		else if (second >= 'A' && second <= 'F')
		    	second = second - 'A' + 10;
		else if (second >= 'a' && second <= 'f')
		    	second = second - 'a' + 10;
		else
		   	return ((char *)NULL);


		*(unsigned char *)ip++ =  first << 4 | second;

	}
	return (image);
}


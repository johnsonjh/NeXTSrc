/*
	tiff.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#define NeXT_MOD

/*
 * This file contains the original 
 * tif_read.c, tif_write.c, tif_flush.c, and tif_swab.c
 *
 * Original copyright:
 *
 * Copyright (c) 1988, 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

/*
 * TIFF Library.
 * Scanline-oriented Read Support
 */
#ifdef NeXT_MOD
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>

#import "tiffPrivate.h"
#import "imagemessage.h"
#import <stdio.h>
#import <assert.h>
#import <mach.h>
#import "appkitPrivate.h"

#define	howmany(x, y)	(((x)+((y)-1))/(y))
#else
#include "tiffio.h"
#endif

#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#if USE_PROTOTYPES
static	TIFFSeek(TIFF *, u_int, u_int);
static	int TIFFReadRawStrip1(TIFF *, u_int, u_char *, u_int, char []);
static	TIFFFillStrip(TIFF *, u_int);
static	TIFFStartStrip(TIFF *, u_int);
#ifdef NeXT_MOD
static void TIFFReverseBits(register unsigned char *cp, register int n);
#endif
#else
static	TIFFSeek();
static	int TIFFReadRawStrip1();
static	TIFFFillStrip();
static	TIFFStartStrip();
#endif

/*VARARGS3*/
#ifdef NeXT_MOD
/*
 * TIFFReadScanLine returns 1 if the scanline was read in fine, -1 if there
 * was an error.
 */
int _NXTIFFReadScanline(tif, buf, row, sample)
#else
TIFFReadScanline(tif, buf, row, sample)
#endif
	register TIFF *tif;
	u_char *buf;
	u_int row, sample;
{
	int e;

#ifndef NeXT_MOD
	if (tif->tif_mode == O_WRONLY) {
		TIFFError(tif->tif_name, "File not open for reading");
		return (-1);
	}
#endif
	if (e = TIFFSeek(tif, row, sample)) {
		/*
		 * Decompress desired row into user buffer
		 */
		e = (*tif->tif_decoderow)(tif, buf, tif->tif_scanlinesize);
		tif->tif_row++;
	}
	return (e ? 1 : -1);
}

/*
 * Seek to a random row+sample in a file.
 */
static
/*VARARGS2*/
#ifdef NeXT_MOD
int
#endif
TIFFSeek(tif, row, sample)
	register TIFF *tif;
	u_int row, sample;
{
	register TIFFDirectory *td = &tif->tif_dir;
	int strip;

	if (row >= td->td_imagelength) {	/* out of range */
		TIFFError(tif->tif_name, "%d: Row out of range, max %d",
		    row, td->td_imagelength);
		return (0);
	}
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFError(tif->tif_name,
			    "%d: Sample out of range, max %d",
			    sample, td->td_samplesperpixel);
			return (0);
		}
		strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
	} else
		strip = row / td->td_rowsperstrip;
	if (strip != tif->tif_curstrip) { 	/* different strip, refill */
		if (!TIFFFillStrip(tif, strip))
			return (0);
	} else if (row < tif->tif_row) {
		/*
		 * Moving backwards within the same strip: backup
		 * to the start and then decode forward (below).
		 *
		 * NB: If you're planning on lots of random access within a
		 * strip, it's better to just read and decode the entire
		 * strip, and then access the decoded data in a random fashion.
		 */
		if (!TIFFStartStrip(tif, strip))
			return (0);
	}
	if (row != tif->tif_row) {
		if (tif->tif_seek) {
			/*
			 * Seek forward to the desired row.
			 */
			if (!(*tif->tif_seek)(tif, row - tif->tif_row))
				return (0);
			tif->tif_row = row;
		} else {
			TIFFError(tif->tif_name,
		    "Compression algorithm does not support random access");
			return (0);
		}
	}
	return (1);
}

#ifndef NeXT_MOD
/*
 * Read a strip of data and decompress the specified
 * amount into the user-supplied buffer.
 */
TIFFReadEncodedStrip(tif, strip, buf, size)
	TIFF *tif;
	u_int strip;
	u_char *buf;
	u_int size;
{
	TIFFDirectory *td = &tif->tif_dir;

	if (tif->tif_mode == O_WRONLY) {
		TIFFError(tif->tif_name, "File not open for reading");
		return (-1);
	}
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Strip out of range, max %d",
		    strip, td->td_nstrips);
		return (-1);
	}
	if (size == (u_int)-1)
		size = td->td_rowsperstrip * tif->tif_scanlinesize;
	else if (size > td->td_rowsperstrip * tif->tif_scanlinesize)
		size = td->td_rowsperstrip * tif->tif_scanlinesize;
	return (TIFFFillStrip(tif, strip) && 
	    (*tif->tif_decoderow)(tif, buf, size) ? size : -1);
}
#endif


/*
 * Read a strip of data from the file.
 */
#ifdef NeXT_MOD
int _NXTIFFReadRawStrip(tif, strip, buf, size)
#else
TIFFReadRawStrip(tif, strip, buf, size)
#endif
	TIFF *tif;
	u_int strip;
	u_char *buf;
	u_int size;
{
#ifdef NeXT_MOD
	static char module[] = "";
#else
	static char module[] = "TIFFReadRawStrip";
#endif
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

#ifndef NeXT_MOD
	if (tif->tif_mode == O_WRONLY) {
		TIFFError(tif->tif_name, "File not open for reading");
		return (-1);
	}
#endif
	if (strip >= td->td_nstrips) {
		TIFFError(tif->tif_name, "%d: Strip out of range, max %d",
		    strip, td->td_nstrips);
		return (-1);
	}
	bytecount = td->td_stripbytecount[strip];
	if (size == (u_int)-1)
		size = bytecount;
	else if (bytecount > size)
		bytecount = size;
	return (TIFFReadRawStrip1(tif, strip, buf, bytecount, module));
}

static int
TIFFReadRawStrip1(tif, strip, buf, size, module)
	TIFF *tif;
	u_int strip;
	u_char *buf;
	u_int size;
	char module[];
{
	if (!SeekOK(tif->tif_fd, tif->tif_dir.td_stripoffset[strip])) {
		TIFFError(module, "%s: Seek error at scanline %d, strip %d",
		    tif->tif_name, tif->tif_row, strip);
		return (-1);
	}
	if (!ReadOK(tif->tif_fd, buf, size)) {
		TIFFError(module, "%s: Read error at scanline %d",
		    tif->tif_name, tif->tif_row);
		return (-1);
	}
	return (size);
}

/*
 * Read the specified strip and setup for decoding. 
 * The data buffer is expanded, as necessary, to
 * hold the strip's data.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFFillStrip(tif, strip)
	TIFF *tif;
	u_int strip;
{
#ifdef NeXT_MOD
	char module[] = "";
#else
	static char module[] = "TIFFFillStrip";
#endif
	TIFFDirectory *td = &tif->tif_dir;
	u_long bytecount;

	/*
	 * Expand raw data buffer, if needed, to
	 * hold data strip coming from file
	 * (perhaps should set upper bound on
	 *  the size of a buffer we'll use?).
	 */
	bytecount = td->td_stripbytecount[strip];
	if (bytecount > tif->tif_rawdatasize) {
		tif->tif_curstrip = -1;		/* unknown state */
		if (tif->tif_rawdata) {
			free(tif->tif_rawdata);
			tif->tif_rawdata = NULL;
		}
		tif->tif_rawdatasize = roundup(bytecount, 1024);
		tif->tif_rawdata = malloc(tif->tif_rawdatasize);
		if (tif->tif_rawdata == NULL) {
			TIFFError(module,
			    "%s: No space for data buffer at scanline %d",
			    tif->tif_name, tif->tif_row);
			tif->tif_rawdatasize = 0;
			return (0);
		}
	}
	if (TIFFReadRawStrip1(tif,
#ifdef NeXT_MOD
	    strip, (u_char *)tif->tif_rawdata, bytecount, module) != bytecount)
#else
	    strip, tif->tif_rawdata, bytecount, module) != bytecount)
#endif
		return (0);
	if (td->td_fillorder != tif->tif_fillorder &&
	    (tif->tif_flags & TIFF_NOBITREV) == 0)
#ifdef NeXT_MOD
		TIFFReverseBits((u_char *)tif->tif_rawdata, bytecount);
#else
		TIFFReverseBits(tif->tif_rawdata, bytecount);
#endif
	return (TIFFStartStrip(tif, strip));
}

/*
 * Set state to appear as if a
 * strip has just been read in.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFStartStrip(tif, strip)
	register TIFF *tif;
	u_int strip;
{
	TIFFDirectory *td = &tif->tif_dir;

	tif->tif_curstrip = strip;
	tif->tif_row = (strip % td->td_stripsperimage) * td->td_rowsperstrip;
	tif->tif_rawcp = tif->tif_rawdata;
	tif->tif_rawcc = td->td_stripbytecount[strip];
	return (tif->tif_stripdecode == NULL || (*tif->tif_stripdecode)(tif));
}



/* End of original tif_read.c */

/* Start of original tif_write.c */

/*
 * TIFF Library.
 *
 * Scanline-oriented Write Support
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#include <stdio.h>
#include <assert.h>

#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))
#define	howmany(x, y)	(((x)+((y)-1))/(y))
#endif

#define	STRIPINCR	20		/* expansion factor on strip array */

#if USE_PROTOTYPES
#ifndef NeXT_MOD
static	TIFFWriteCheck(TIFF *, char []);
#endif
static	TIFFBufferSetup(TIFF *, char []);
static	TIFFGrowStrips(TIFF *, int, char []);
static	TIFFAppendToStrip(TIFF *, u_int, u_char *, u_int);
#else
#ifndef NeXT_MOD
static	TIFFWriteCheck();
#endif
static	TIFFBufferSetup();
static	TIFFGrowStrips();
static	TIFFAppendToStrip();
#endif

/*VARARGS3*/
#ifdef NeXT_MOD
int _NXTIFFWriteScanline(tif, buf, row, sample)
#else
TIFFWriteScanline(tif, buf, row, sample)
#endif
	register TIFF *tif;
	u_char *buf;
	u_int row, sample;
{
#ifdef NeXT_MOD
	char module[] = "";
#else
	static char module[] = "TIFFWriteScanline";
#endif
	register TIFFDirectory *td;
	int strip, status, imagegrew = 0;

	if (!TIFFWriteCheck(tif, module))
		return (-1);
	/*
	 * Handle delayed allocation of data buffer.  This
	 * permits it to be sized more intelligently (using
	 * directory information).
	 */
	if ((tif->tif_flags & TIFF_BUFFERSETUP) == 0) {
		if (!TIFFBufferSetup(tif, module))
			return (-1);
		tif->tif_flags |= TIFF_BUFFERSETUP;
	}
	td = &tif->tif_dir;
	/*
	 * Extend image length if needed
	 * (but only for PlanarConfig=1).
	 */
	if (row >= td->td_imagelength) {	/* extend image */
		if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
			TIFFError(tif->tif_name,
		"Can not change \"ImageLength\" when using separate planes");
			return (-1);
		}
		td->td_imagelength = row+1;
		imagegrew = 1;
	}
	/*
	 * Calculate strip and check for crossings.
	 */
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE) {
		if (sample >= td->td_samplesperpixel) {
			TIFFError(tif->tif_name,
			    "%d: Sample out of range, max %d",
			    sample, td->td_samplesperpixel);
			return (-1);
		}
		strip = sample*td->td_stripsperimage + row/td->td_rowsperstrip;
	} else
		strip = row / td->td_rowsperstrip;
	if (strip != tif->tif_curstrip) {
		/*
		 * Changing strips -- flush any data present.
		 */
#ifdef NeXT_MOD
		if (tif->tif_rawcc > 0 && !_NXTIFFFlushData(tif))
#else
		if (tif->tif_rawcc > 0 && !TIFFFlushData(tif))
#endif
			return (-1);
		tif->tif_curstrip = strip;
		/*
		 * Watch out for a growing image.  The value of
		 * strips/image will initially be 1 (since it
		 * can't be deduced until the imagelength is known).
		 */
		if (strip >= td->td_stripsperimage && imagegrew)
			td->td_stripsperimage =
			    howmany(td->td_imagelength, td->td_rowsperstrip);
		tif->tif_row =
		    (strip % td->td_stripsperimage) * td->td_rowsperstrip;
		if (tif->tif_stripencode && !(*tif->tif_stripencode)(tif))
			return (-1);
	}
	/*
	 * Check strip array to make sure there's space.
	 * We don't support dynamically growing files that
	 * have data organized in separate bitplanes because
	 * it's too painful.  In that case we require that
	 * the imagelength be set properly before the first
	 * write (so that the strips array will be fully
	 * allocated above).
	 */
	if (strip >= td->td_nstrips && !TIFFGrowStrips(tif, 1, module))
		return (-1);
	/*
	 * Ensure the write is either sequential or at the
	 * beginning of a strip (or that we can randomly
	 * access the data -- i.e. no encoding).
	 */
	if (row != tif->tif_row) {
		if (tif->tif_seek) {
			if (row < tif->tif_row) {
				/*
				 * Moving backwards within the same strip:
				 * backup to the start and then decode
				 * forward (below).
				 */
				tif->tif_row = (strip % td->td_stripsperimage) *
				    td->td_rowsperstrip;
				tif->tif_rawcp = tif->tif_rawdata;
			}
			/*
			 * Seek forward to the desired row.
			 */
			if (!(*tif->tif_seek)(tif, row - tif->tif_row))
				return (-1);
			tif->tif_row = row;
		} else {
			TIFFError(tif->tif_name,
		    "Compression algorithm does not support random access");
			return (-1);
		}
	}
	status = (*tif->tif_encoderow)(tif, buf, tif->tif_scanlinesize);
	tif->tif_row++;
	return (status);
}

#ifndef NeXT_MOD
/*
 * Encode the supplied data and write it to the
 * specified strip.  There must be space for the
 * data; we don't check if strips overlap!
 *
 * NB: Image length must be setup before writing; this
 *     interface does not support automatically growing
 *     the image on each write (as TIFFWriteScanline does).
 */
TIFFWriteEncodedStrip(tif, strip, data, cc)
	register TIFF *tif;
	u_int strip;
	u_char *data;
	u_int cc;
{
	static char module[] = "TIFFWriteEncodedStrip";

	if (!TIFFWriteCheck(tif, module))
		return (-1);
	if (strip >= tif->tif_dir.td_nstrips) {
		TIFFError(module, "%s: Strip %d out of range, max %d",
		    tif->tif_name, strip, tif->tif_dir.td_nstrips);
		return (-1);
	}
	/*
	 * Handle delayed allocation of data buffer.  This
	 * permits it to be sized more intelligently (using
	 * directory information).
	 */
	if ((tif->tif_flags & TIFF_BUFFERSETUP) == 0) {
		if (!TIFFBufferSetup(tif, module))
			return (-1);
		tif->tif_flags |= TIFF_BUFFERSETUP;
	}
	tif->tif_curstrip = strip;
	if (tif->tif_encodestrip && !(*tif->tif_encodestrip)(tif))
		return (0);
	/*
	 * Note that this assumes that encoding routines
	 * can handle multiple scanlines.  All the "standard"
	 * ones in the library do!
	 */
	if (!(*tif->tif_encoderow)(tif, data, cc))
		return (0);
	if (tif->tif_dir.td_fillorder != tif->tif_fillorder &&
	    (tif->tif_flags & TIFF_NOBITREV) == 0)
#ifdef NeXT_MOD
		TIFFReverseBits((u_char *)tif->tif_rawdata, tif->tif_rawcc);
#else
		TIFFReverseBits(tif->tif_rawdata, tif->tif_rawcc);
#endif
	if (tif->tif_rawcc > 0 &&
#ifdef NeXT_MOD
	    !TIFFAppendToStrip(tif, strip, (u_char *)tif->tif_rawdata, 
			tif->tif_rawcc))
#else
	    !TIFFAppendToStrip(tif, strip, tif->tif_rawdata, tif->tif_rawcc))
#endif
		return (-1);
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	return (cc);
}
#endif

/*
 * Write the supplied data to the specified strip.
 * There must be space for the data; we don't check
 * if strips overlap!
 *
 * NB: Image length must be setup before writing; this
 *     interface does not support automatically growing
 *     the image on each write (as TIFFWriteScanline does).
 */
#ifdef NeXT_MOD
int _NXTIFFWriteRawStrip(tif, strip, data, cc)
#else
TIFFWriteRawStrip(tif, strip, data, cc)
#endif
	TIFF *tif;
	u_int strip;
	u_char *data;
	u_int cc;
{
#ifdef NeXT_MOD
	static char module[] = "";
#else
	static char module[] = "TIFFWriteRawStrip";
#endif

	if (!TIFFWriteCheck(tif, module))
		return (-1);
	if (strip >= tif->tif_dir.td_nstrips) {
		TIFFError(module, "%s: Strip %d out of range, max %d",
		    tif->tif_name, strip, tif->tif_dir.td_nstrips);
		return (-1);
	}
#ifdef NeXT_MOD
	return (TIFFAppendToStrip(tif, strip, data, cc) ? cc : -1);
#else
	return (TIFFAppendToStrip(tif, strip, data, cc) ? -1 : cc);
#endif
}

/*
 * Verify file is writable and that the directory
 * information is setup properly.  In doing the latter
 * we also "freeze" the state of the directory so
 * that important information is not changed.
 */
#ifdef NeXT_MOD
int
_NXTIFFWriteCheck(tif, module)
#else
static
int
TIFFWriteCheck(tif, module)
#endif
	register TIFF *tif;
	char module[];
{
#ifndef NeXT_MOD
	if (tif->tif_mode == O_RDONLY) {
		TIFFError(module, "%s: File not open for writing",
		    tif->tif_name);
		return (0);
	}
#endif
	/*
	 * On the first write verify all the required information
	 * has been setup and initialize any data structures that
	 * had to wait until directory information was set.
	 * Note that a lot of our work is assumed to remain valid
	 * because we disallow any of the important parameters
	 * from changing after we start writing (i.e. once
	 * TIFF_BEENWRITING is set, TIFFSetField will only allow
	 * the image's length to be changed).
	 */
	if ((tif->tif_flags & TIFF_BEENWRITING) == 0) {
		if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS)) {
			TIFFError(module,
			    "%s: Must set \"ImageWidth\" before writing data",
			    tif->tif_name);
			return (0);
		}
		if (!TIFFFieldSet(tif, FIELD_PLANARCONFIG)) {
			TIFFError(module,
		    "%s: Must set \"PlanarConfiguration\" before writing data",
			    tif->tif_name);
			return (0);
		}
		if (tif->tif_dir.td_stripoffset == NULL) {
			register TIFFDirectory *td = &tif->tif_dir;

#ifdef NeXT_MOD
			if (td->td_usetiles) {
			    td->td_tilesacross = (td->td_imagewidth + td->td_tilewidth - 1) / td->td_tilewidth;
			    td->td_tilesdown = (td->td_imagelength + td->td_tilelength - 1) / td->td_tilelength;
			    td->td_stripsperimage = td->td_tilesdown;
			    td->td_rowsperstrip = td->td_tilelength;
			} else {
			    td->td_stripsperimage =
			    	(td->td_rowsperstrip == 0xffffffff ||
			     	td->td_imagelength == 0) ? 1 :
			     	howmany(td->td_imagelength, td->td_rowsperstrip);
			}
#else
			td->td_stripsperimage =
			    (td->td_rowsperstrip == 0xffffffff ||
			     td->td_imagelength == 0) ? 1 :
			     howmany(td->td_imagelength, td->td_rowsperstrip);
#endif
			td->td_nstrips = td->td_stripsperimage;
			if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
				td->td_nstrips *= td->td_samplesperpixel;
			td->td_stripoffset = (u_long *)
			    malloc(td->td_nstrips * sizeof (u_long));
			td->td_stripbytecount = (u_long *)
			    malloc(td->td_nstrips * sizeof (u_long));
			if (td->td_stripoffset == NULL ||
			    td->td_stripbytecount == NULL) {
				td->td_nstrips = 0;
				TIFFError(module,
				    "%s: No space for strip arrays",
				    tif->tif_name);
				return (0);
			}
			/*
			 * Place data at the end-of-file
			 * (by setting offsets to zero).
			 */
			bzero((char *)td->td_stripoffset,
			    td->td_nstrips * sizeof (u_long));
			bzero((char *)td->td_stripbytecount,
			    td->td_nstrips * sizeof (u_long));
			TIFFSetFieldBit(tif, FIELD_STRIPOFFSETS);
			TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
		}
		tif->tif_flags |= TIFF_BEENWRITING;
	}
	return (1);
}

/*
 * Setup the raw data buffer used for encoding.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFBufferSetup(tif, module)
	register TIFF *tif;
	char module[];
{
	int scanline;

	tif->tif_scanlinesize = scanline = TIFFScanlineSize(tif);

	/*
	 * Make raw data buffer at least 8K
	 */
	if (scanline < 8*1024)
		scanline = 8*1024;
	tif->tif_rawdata = malloc(scanline);
	if (tif->tif_rawdata == NULL) {
		TIFFError(module, "%s: No space for output buffer",
		    tif->tif_name);
		return (0);
	}
	tif->tif_rawdatasize = scanline;
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	return (1);
}

/*
 * Grow the strip data structures by delta strips.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFGrowStrips(tif, delta, module)
	TIFF *tif;
	int delta;
	char module[];
{
	TIFFDirectory *td = &tif->tif_dir;

	assert(td->td_planarconfig == PLANARCONFIG_CONTIG);
	td->td_stripoffset = (u_long *)realloc(td->td_stripoffset,
	    (td->td_nstrips + delta) * sizeof (u_long));
	td->td_stripbytecount = (u_long *)realloc(td->td_stripbytecount,
	    (td->td_nstrips + delta) * sizeof (u_long));
	if (td->td_stripoffset == NULL || td->td_stripbytecount == NULL) {
		td->td_nstrips = 0;
		TIFFError(module, "%s: No space to expand strip arrays",
		    tif->tif_name);
		return (0);
	}
	bzero(td->td_stripoffset+td->td_nstrips, delta*sizeof (u_long));
	bzero(td->td_stripbytecount+td->td_nstrips, delta*sizeof (u_long));
	td->td_nstrips += delta;
	return (1);
}

/*
 * Append the data to the specified strip.
 *
 * NB: We don't check that there's space in the
 *     file (i.e. that strips do not overlap).
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFAppendToStrip(tif, strip, data, cc)
	TIFF *tif;
	u_int strip;
	u_char *data;
	u_int cc;
{
	TIFFDirectory *td = &tif->tif_dir;
#ifdef NeXT_MOD
	char module[] = "";
#else
	static char module[] = "TIFFAppendToStrip";
#endif

	if (td->td_stripoffset[strip] == 0 || tif->tif_curoff == 0) {
		/*
		 * No current offset, set the current strip.
		 */
		if (td->td_stripoffset[strip] != 0) {
			if (!SeekOK(tif->tif_fd, td->td_stripoffset[strip])) {
				TIFFError(module,
				    "%s: Seek error at scanline %d",
				    tif->tif_name, tif->tif_row);
				return (0);
			}
		} else
#ifdef NeXT_MOD
			NXSeek(tif->tif_fd, 0L, NX_FROMEND);
			td->td_stripoffset[strip] =
			    NXTell(tif->tif_fd);
#else
			td->td_stripoffset[strip] =
			    lseek(tif->tif_fd, 0L, L_XTND);
#endif
		tif->tif_curoff = td->td_stripoffset[strip];
	}
	if (!WriteOK(tif->tif_fd, data, cc)) {
		TIFFError(module, "%s: Write error at scanline %d",
		    tif->tif_name, tif->tif_row);
		return (0);
	}
	tif->tif_curoff += cc;
	td->td_stripbytecount[strip] += cc;
	return (1);
}

/*
 * Flush buffered data to the file.
 */
#ifdef NeXT_MOD
int _NXTIFFFlushData(tif)
#else
TIFFFlushData(tif)
#endif
	TIFF *tif;
{
	if ((tif->tif_flags & TIFF_BEENWRITING) == 0)
		return (0);
	if (tif->tif_encodestrip && !(*tif->tif_encodestrip)(tif))
		return (0);
#ifdef NeXT_MOD
	return (_NXTIFFFlushData1(tif));
#else
	return (TIFFFlushData1(tif));
#endif
}

/*
 * Internal version of TIFFFlushData that can be
 * called by ``encodestrip routines'' w/o concern
 * for infinite recursion.
 */
#ifdef NeXT_MOD
int _NXTIFFFlushData1(tif)
#else
TIFFFlushData1(tif)
#endif
	register TIFF *tif;
{
	if (tif->tif_dir.td_fillorder != tif->tif_fillorder &&
	    (tif->tif_flags & TIFF_NOBITREV) == 0)
#ifdef NeXT_MOD
		TIFFReverseBits((u_char *)tif->tif_rawdata, tif->tif_rawcc);
#else
		TIFFReverseBits(tif->tif_rawdata, tif->tif_rawcc);
#endif
	if (!TIFFAppendToStrip(tif,
#ifdef NeXT_MOD
	    tif->tif_curstrip, (u_char *)tif->tif_rawdata, tif->tif_rawcc))
#else
	    tif->tif_curstrip, tif->tif_rawdata, tif->tif_rawcc))
#endif
		return (0);
	tif->tif_rawcc = 0;
	tif->tif_rawcp = tif->tif_rawdata;
	return (1);
}



/* End of original tif_write.c */

/* Start of original tif_flush.c */

/*
 * TIFF Library.
 */
#ifndef NeXT_MOD
#include "tiffio.h"
#endif

#ifdef NeXT_MOD
int _NXTIFFFlush(TIFF *tif)
#else
TIFFFlush(tif)
	TIFF *tif;
#endif
{

	if (tif->tif_mode != O_RDONLY) {
#ifdef NeXT_MOD
		if (tif->tif_rawcc > 0 && !_NXTIFFFlushData(tif))
#else
		if (tif->tif_rawcc > 0 && !TIFFFlushData(tif))
#endif
			return (0);
		if ((tif->tif_flags & TIFF_DIRTYDIRECT) &&
		    !TIFFWriteDirectory(tif))
			return (0);
	}
	return (1);
}

/* End of original tif_flush.c */

/* Start of original tif_swab.c */

/*
 * TIFF Library Bit & Byte Swapping Support.
 *
 * XXX We assume short = 16-bits and long = 32-bits XXX
 */

#ifdef NeXT_MOD
/*
 * If any of the below functions are defined as macros, they are defined
 * in tiffPrivate.h.
 */
#else
#include "machdep.h"
#endif

#ifdef NeXT_MOD

#ifndef _NXTIFFSwabShort
void _NXTIFFSwabShort(wp)
	unsigned short *wp;
{
	register unsigned char *cp = (unsigned char *)wp;
	int t;

	t = cp[1]; cp[1] = cp[0]; cp[0] = t;
}
#endif

#ifndef _NXTIFFSwabLong
void _NXTIFFSwabLong(lp)
	unsigned long *lp;
{
	register unsigned char *cp = (unsigned char *)lp;
	int t;

	t = cp[3]; cp[3] = cp[0]; cp[0] = t;
	t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}
#endif

#ifndef _NXTIFFSwabArrayOfShort
void _NXTIFFSwabArrayOfShort(wp, n)
	unsigned short *wp;
	register int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)wp;
		t = cp[1]; cp[1] = cp[0]; cp[0] = t;
		wp++;
	}
}
#endif

#ifndef _NXTIFFSwabArrayOfLong
void _NXTIFFSwabArrayOfLong(lp, n)
	register unsigned long *lp;
	register int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)lp;
		t = cp[3]; cp[3] = cp[0]; cp[0] = t;
		t = cp[2]; cp[2] = cp[1]; cp[1] = t;
		lp++;
	}
}
#endif

#else
#ifndef _NXTIFFSwabShort
_NXTIFFSwabShort(wp)
	unsigned short *wp;
{
	register unsigned char *cp = (unsigned char *)wp;
	int t;

	t = cp[1]; cp[1] = cp[0]; cp[0] = t;
}
#endif

#ifndef _NXTIFFSwabLong
_NXTIFFSwabLong(lp)
	unsigned long *lp;
{
	register unsigned char *cp = (unsigned char *)lp;
	int t;

	t = cp[3]; cp[3] = cp[0]; cp[0] = t;
	t = cp[2]; cp[2] = cp[1]; cp[1] = t;
}
#endif

#ifndef _NXTIFFSwabArrayOfShort
_NXTIFFSwabArrayOfShort(wp, n)
	unsigned short *wp;
	register int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)wp;
		t = cp[1]; cp[1] = cp[0]; cp[0] = t;
		wp++;
	}
}
#endif

#ifndef _NXTIFFSwabArrayOfLong
_NXTIFFSwabArrayOfLong(lp, n)
	register unsigned long *lp;
	register int n;
{
	register unsigned char *cp;
	register int t;

	/* XXX unroll loop some */
	while (n-- > 0) {
		cp = (unsigned char *)lp;
		t = cp[3]; cp[3] = cp[0]; cp[0] = t;
		t = cp[2]; cp[2] = cp[1]; cp[1] = t;
		lp++;
	}
}
#endif
#endif
/*
 * Bit reversal tables.  TIFFBitRevTable[<byte>] gives
 * the bit reversed value of <byte>.  Used in various
 * places in the library when the FillOrder requires
 * bit reversal of byte values (e.g. CCITT Fax 3
 * encoding/decoding).  TIFFNoBitRevTable is provided
 * for algorithms that want an equivalent table that
 * do not reverse bit values.
 */
#ifdef NeXT_MOD
static const unsigned char TIFFBitRevTable[256] = {
#else
unsigned char TIFFBitRevTable[256] = {
#endif
    0x00, 0x80, 0x40, 0xc0, 0x20, 0xa0, 0x60, 0xe0,
    0x10, 0x90, 0x50, 0xd0, 0x30, 0xb0, 0x70, 0xf0,
    0x08, 0x88, 0x48, 0xc8, 0x28, 0xa8, 0x68, 0xe8,
    0x18, 0x98, 0x58, 0xd8, 0x38, 0xb8, 0x78, 0xf8,
    0x04, 0x84, 0x44, 0xc4, 0x24, 0xa4, 0x64, 0xe4,
    0x14, 0x94, 0x54, 0xd4, 0x34, 0xb4, 0x74, 0xf4,
    0x0c, 0x8c, 0x4c, 0xcc, 0x2c, 0xac, 0x6c, 0xec,
    0x1c, 0x9c, 0x5c, 0xdc, 0x3c, 0xbc, 0x7c, 0xfc,
    0x02, 0x82, 0x42, 0xc2, 0x22, 0xa2, 0x62, 0xe2,
    0x12, 0x92, 0x52, 0xd2, 0x32, 0xb2, 0x72, 0xf2,
    0x0a, 0x8a, 0x4a, 0xca, 0x2a, 0xaa, 0x6a, 0xea,
    0x1a, 0x9a, 0x5a, 0xda, 0x3a, 0xba, 0x7a, 0xfa,
    0x06, 0x86, 0x46, 0xc6, 0x26, 0xa6, 0x66, 0xe6,
    0x16, 0x96, 0x56, 0xd6, 0x36, 0xb6, 0x76, 0xf6,
    0x0e, 0x8e, 0x4e, 0xce, 0x2e, 0xae, 0x6e, 0xee,
    0x1e, 0x9e, 0x5e, 0xde, 0x3e, 0xbe, 0x7e, 0xfe,
    0x01, 0x81, 0x41, 0xc1, 0x21, 0xa1, 0x61, 0xe1,
    0x11, 0x91, 0x51, 0xd1, 0x31, 0xb1, 0x71, 0xf1,
    0x09, 0x89, 0x49, 0xc9, 0x29, 0xa9, 0x69, 0xe9,
    0x19, 0x99, 0x59, 0xd9, 0x39, 0xb9, 0x79, 0xf9,
    0x05, 0x85, 0x45, 0xc5, 0x25, 0xa5, 0x65, 0xe5,
    0x15, 0x95, 0x55, 0xd5, 0x35, 0xb5, 0x75, 0xf5,
    0x0d, 0x8d, 0x4d, 0xcd, 0x2d, 0xad, 0x6d, 0xed,
    0x1d, 0x9d, 0x5d, 0xdd, 0x3d, 0xbd, 0x7d, 0xfd,
    0x03, 0x83, 0x43, 0xc3, 0x23, 0xa3, 0x63, 0xe3,
    0x13, 0x93, 0x53, 0xd3, 0x33, 0xb3, 0x73, 0xf3,
    0x0b, 0x8b, 0x4b, 0xcb, 0x2b, 0xab, 0x6b, 0xeb,
    0x1b, 0x9b, 0x5b, 0xdb, 0x3b, 0xbb, 0x7b, 0xfb,
    0x07, 0x87, 0x47, 0xc7, 0x27, 0xa7, 0x67, 0xe7,
    0x17, 0x97, 0x57, 0xd7, 0x37, 0xb7, 0x77, 0xf7,
    0x0f, 0x8f, 0x4f, 0xcf, 0x2f, 0xaf, 0x6f, 0xef,
    0x1f, 0x9f, 0x5f, 0xdf, 0x3f, 0xbf, 0x7f, 0xff
};
#ifdef NeXT_MOD
#ifdef FAX3_SUPPORT
#ifdef NeXT_MOD
static const unsigned char TIFFNoBitRevTable[256] = {
#else
unsigned char TIFFNoBitRevTable[256] = {
#endif
    0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 
    0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 
    0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 
    0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, 
    0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 
    0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f, 
    0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 
    0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f, 
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 
    0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f, 
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 
    0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f, 
    0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 
    0x68, 0x69, 0x6a, 0x6b, 0x6c, 0x6d, 0x6e, 0x6f, 
    0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 
    0x78, 0x79, 0x7a, 0x7b, 0x7c, 0x7d, 0x7e, 0x7f, 
    0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 
    0x88, 0x89, 0x8a, 0x8b, 0x8c, 0x8d, 0x8e, 0x8f, 
    0x90, 0x91, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 
    0x98, 0x99, 0x9a, 0x9b, 0x9c, 0x9d, 0x9e, 0x9f, 
    0xa0, 0xa1, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7, 
    0xa8, 0xa9, 0xaa, 0xab, 0xac, 0xad, 0xae, 0xaf, 
    0xb0, 0xb1, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 
    0xb8, 0xb9, 0xba, 0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 
    0xc0, 0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 
    0xc8, 0xc9, 0xca, 0xcb, 0xcc, 0xcd, 0xce, 0xcf, 
    0xd0, 0xd1, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 
    0xd8, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf, 
    0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 
    0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef, 
    0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 
    0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff, 
};

#ifdef NeXT_MOD
const unsigned char *_NXTIFFBitTable (BOOL reverse)
{
    return reverse ? TIFFBitRevTable : TIFFNoBitRevTable;
}
#endif
#endif
#endif


#ifdef NeXT_MOD
static void TIFFReverseBits(register unsigned char *cp, register int n)
#else
TIFFReverseBits(cp, n)
	register unsigned char *cp;
	register int n;
#endif
{
	while (n-- > 0)
		*cp = TIFFBitRevTable[*cp], cp++;
}

/* End of original tif_swab.c */

/*
  
Modifications (since 77):

77
--
 2/19/90 aozer	Created from Sam Leffler's freely distributable library
 3/6/90 aozer	Made short and long byte swap routines into macros in 
		tiffprivate.h

86
--
 6/9/90 aozer	Integrated changes for Sam Leffler's TIFF Library v2.2.

87
--
 6/15/90 aozer	Made static const arrays TIFFBitRevTable & TIFFNoBitRevTable
		available externally (for FAX compression) through a function.

91
--
 8/9/90 aozer	Made _NXTIFFWriteRawStrip & _NXTIFFReadRawStrip extern 
		functions. Also changed _NXTIFFWriteRawStrip's return value --- 
		it seemed to be wrong.  If you use this function, make sure 
		things are in sync.

99
--
 10/19/90 aozer	Made TIFFWriteCheck() a private extern.

*/

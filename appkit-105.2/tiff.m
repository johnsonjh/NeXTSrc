/*
	tiff.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#define NeXT_MOD

/*
 * This file contains code from the original 
 * tif_open.c, tif_close.c, tif_error.c, tif_warning.c
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
 */
#ifdef NeXT_MOD
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>
#import <stdio.h>
#import <zone.h>
#import <mach.h>
#import "appkitPrivate.h"
#import "nextstd.h"
#import "tiffPrivate.h"
#import "tiff.h"
#import "imagemessage.h"

#else
#include "tiffio.h"
#endif

#define	ord(e)	((int)e)

/*
 * Initialize the bit fill order, the
 * shift & mask tables, and the byte
 * swapping state according to the file
 * contents and the machine architecture.
 */
static
#ifdef NeXT_MOD
void
#endif
TIFFInitOrder(tif, magic, bigendian)
	register TIFF *tif;
	int magic, bigendian;
{

	/* XXX how can we deduce this dynamically? */
	tif->tif_fillorder = FILLORDER_MSB2LSB;

	tif->tif_typemask[0] = 0;
	tif->tif_typemask[ord(TIFF_BYTE)] = 0xff;
	tif->tif_typemask[ord(TIFF_SHORT)] = 0xffff;
	tif->tif_typemask[ord(TIFF_LONG)] = 0xffffffff;
	tif->tif_typemask[ord(TIFF_RATIONAL)] = 0xffffffff;
	tif->tif_typeshift[0] = 0;
	tif->tif_typeshift[ord(TIFF_LONG)] = 0;
	tif->tif_typeshift[ord(TIFF_RATIONAL)] = 0;
	if (magic == TIFF_BIGENDIAN) {
		tif->tif_typeshift[ord(TIFF_BYTE)] = 24;
		tif->tif_typeshift[ord(TIFF_SHORT)] = 16;
		if (!bigendian)
			tif->tif_flags |= TIFF_SWAB;
	} else {
		tif->tif_typeshift[ord(TIFF_BYTE)] = 0;
		tif->tif_typeshift[ord(TIFF_SHORT)] = 0;
		if (bigendian)
			tif->tif_flags |= TIFF_SWAB;
	}
}

#ifndef NeXT_MOD
/*
 * Open a TIFF file for read/writing.
 */
TIFF *
TIFFOpen(name, mode)
	char *name, *mode;
{
	static char module[] = "TIFFOpen";
	TIFF *tif;
	int m, fd, bigendian;

	switch (mode[0]) {
	case 'r':
		m = O_RDONLY;
		if (mode[1] == '+')
			m = O_RDWR;
		break;
	case 'w':
	case 'a':
		m = O_RDWR|O_CREAT;
		if (mode[0] == 'w')
			m |= O_TRUNC;
		break;
	default:
		TIFFError(module, "\"%s\": Bad mode", mode);
		return ((TIFF *)0);
	}
	fd = TIFFOpenFile(name, m, 0666);
	if (fd < 0) {
		TIFFError(module, "%s: Cannot open", name);
		return ((TIFF *)0);
	}
	tif = (TIFF *)malloc(sizeof (TIFF) + strlen(name) + 1);
	if (tif == NULL) {
		TIFFError(module, "%s: Out of memory (TIFF structure)", name);
		(void) close(fd);
		return ((TIFF *)0);
	}
	bzero((char *)tif, sizeof (*tif));
	tif->tif_name = (char *)tif + sizeof (TIFF);
	strcpy(tif->tif_name, name);
	tif->tif_fd = fd;
	tif->tif_mode = m &~ (O_CREAT|O_TRUNC);
	tif->tif_curoff = 0;
	tif->tif_curstrip = -1;		/* invalid strip */
	tif->tif_row = -1;		/* read/write pre-increment */
	{ int one = 1; bigendian = (*(char *)&one == 0); }
	/*
	 * Read in TIFF header.
	 */
	if (!ReadOK(fd, &tif->tif_header, sizeof (TIFFHeader))) {
		int one = 1;

		if (tif->tif_mode == O_RDONLY) {
			TIFFError(name, "Cannot read TIFF header");
			goto bad;
		}
		/*
		 * Setup header and write.
		 */
		tif->tif_header.tiff_magic =  bigendian ?
		    TIFF_BIGENDIAN : TIFF_LITTLEENDIAN;
		tif->tif_header.tiff_version = TIFF_VERSION;
		tif->tif_header.tiff_diroff = 0;	/* filled in later */
		if (!WriteOK(fd, &tif->tif_header, sizeof (TIFFHeader))) {
			TIFFError(name, "Error writing TIFF header");
			goto bad;
		}
		/*
		 * Setup the byte order handling.
		 */
		TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
		/*
		 * Setup default directory.
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		tif->tif_diroff = 0;
		return (tif);
	}
	/*
	 * Setup the byte order handling.
	 */
	if (tif->tif_header.tiff_magic != TIFF_BIGENDIAN &&
	    tif->tif_header.tiff_magic != TIFF_LITTLEENDIAN) {
		TIFFError(name,  "Not a TIFF file, bad magic number %d (0x%x)",
		    tif->tif_header.tiff_magic,
		    tif->tif_header.tiff_magic);
		goto bad;
	}
	TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
	/*
	 * Swap header if required.
	 */
	if (tif->tif_flags & TIFF_SWAB) {
		TIFFSwabShort(&tif->tif_header.tiff_version);
		TIFFSwabLong(&tif->tif_header.tiff_diroff);
	}
	/*
	 * Now check version (if needed, it's been byte-swapped).
	 * Note that this isn't actually a version number, it's a
	 * magic number that doesn't change (stupid).
	 */
	if (tif->tif_header.tiff_version != TIFF_VERSION) {
		TIFFError(name,
		    "Not a TIFF file, bad version number %d (0x%x)",
		    tif->tif_header.tiff_version,
		    tif->tif_header.tiff_version); 
		goto bad;
	}
	/*
	 * Setup initial directory.
	 */
	switch (mode[0]) {
	case 'r':
		tif->tif_nextdiroff = tif->tif_header.tiff_diroff;
#ifdef NeXT_MOD
		tif->tif_freeprev = 0;
#endif
		if (TIFFReadDirectory(tif)) {
			tif->tif_rawcc = -1;
			tif->tif_flags |= TIFF_BUFFERSETUP;
			return (tif);
		}
		break;
	case 'a':
		/*
		 * Don't append to file that has information
		 * byte swapped -- we will write data that is
		 * in the opposite order.
		 */
		if (tif->tif_flags & TIFF_SWAB) {
			TIFFError(name,
		"Cannot append to file that has opposite byte ordering");
			goto bad;
		}
		/*
		 * New directories are automatically append
		 * to the end of the directory chain when they
		 * are written out (see TIFFWriteDirectory).
		 */
		if (!TIFFDefaultDirectory(tif))
			goto bad;
		return (tif);
	}
bad:
	tif->tif_mode = O_RDONLY;	/* XXX avoid flush */
	(void) TIFFClose(tif);
	return ((TIFF *)0);
}

TIFFScanlineSize(tif)
	TIFF *tif;
{
	TIFFDirectory *td = &tif->tif_dir;
	long scanline;
	
	scanline = td->td_bitspersample * td->td_imagewidth;
	if (td->td_planarconfig == PLANARCONFIG_CONTIG)
		scanline *= td->td_samplesperpixel;
#define	howmany(x, y)	(((x)+((y)-1))/(y))
	return (howmany(scanline, 8));
}
#endif

#ifdef NeXT_MOD
/*
 * Open a stream for TIFF read/writing. err can be NULL.
 */
TIFF *_NXTIFFOpenStream(NXStream *s, BOOL readMode, int *err)
{
    return _NXTIFFOpenStreamWithMode
		(s, readMode ? NX_TIFFREADMODE : NX_TIFFWRITEMODE, err);
}

TIFF *_NXTIFFOpenStreamWithMode(NXStream *s, int m, int *err)
{
    TIFF *tif;
    int bigendian;

    tif = (TIFF *)malloc(sizeof (TIFF) + 1);// One extra byte for the zero
    bzero((char *)tif, sizeof (*tif));
    tif->tif_name = (char *)tif + sizeof (TIFF);
    *(tif->tif_name) = '\0';
    tif->tif_fd = s;
    tif->tif_mode = m;
    tif->tif_curoff = 0;
    tif->tif_curstrip = -1;		/* invalid strip */
    tif->tif_row = -1;		/* read/write pre-increment */

    // Determine the sex of the machine we are on.

    { int one = 1; bigendian = (*(char *)&one == 0); }

    // Read in TIFF header after seeking to the front of the stream.

    (void)SeekOK(s, 0);

    if (m == NX_TIFFWRITEMODE) { 
	/*
	 * Setup header and write.
	 */
	tif->tif_header.tiff_magic =  bigendian ?
	    TIFF_BIGENDIAN : TIFF_LITTLEENDIAN;
	tif->tif_header.tiff_version = TIFF_VERSION;
	tif->tif_header.tiff_diroff = 0;	/* filled in later */
	if (!WriteOK(s, &tif->tif_header, sizeof (TIFFHeader))) {
	    if (err) *err = NX_FILE_IO_ERROR;
	    TIFFError(tif->tif_name, "Error writing TIFF header");
	    goto bad;
	}
	/*
	 * Setup the byte order handling.
	 */
	TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
	/*
	 * Setup default directory.
	 */
	if (!_NXTIFFDefaultDirectory(tif)) {
	    goto badformat;
	}
	tif->tif_diroff = 0;
	return (tif);

    } else if (!ReadOK(s, &tif->tif_header, sizeof (TIFFHeader))) {

	TIFFError(tif->tif_name, "Cannot read TIFF header");
	goto badformat;

    }
    /*
     * Setup the byte order handling.
     */
    if (tif->tif_header.tiff_magic != TIFF_BIGENDIAN &&
	tif->tif_header.tiff_magic != TIFF_LITTLEENDIAN) {
#ifdef DEBUG
	TIFFError(tif->tif_name,  "Not a TIFF file, bad magic number");
#endif
	goto badformat;
    }
    TIFFInitOrder(tif, tif->tif_header.tiff_magic, bigendian);
    /*
     * Swap header if required.
     */
    if (tif->tif_flags & TIFF_SWAB) {
	_NXTIFFSwabShort(&tif->tif_header.tiff_version);
	_NXTIFFSwabLong(&tif->tif_header.tiff_diroff);
    }
    /*
     * Now check version (if needed, it's been byte-swapped).
     * Note that this isn't actually a version number, it's a
     * magic number that doesn't change (stupid).
     */
    if (tif->tif_header.tiff_version != TIFF_VERSION) {
#ifdef DEBUG
	TIFFError(tif->tif_name, "Not a TIFF file, bad version number"); 
#endif
	goto badformat;
    }
    /*
     * Setup initial directory if reading.
     */
    if (m == NX_TIFFREADMODE) {
	tif->tif_nextdiroff = tif->tif_header.tiff_diroff;
	if (_NXTIFFReadDirectory(tif)) {
	    tif->tif_rawcc = -1;
	    tif->tif_flags |= TIFF_BUFFERSETUP;
	    return (tif);
	}
    } else if (m == NX_TIFFAPPENDMODE) {
	if (tif->tif_flags & TIFF_SWAB) {
	    TIFFError(tif->tif_name,
		"Cannot append to file that has opposite byte ordering");
	    if (err) *err = NX_TIFF_CANT_APPEND;
	    goto bad;
	}
	if (!TIFFDefaultDirectory(tif)) {
	    if (err) *err = NX_TIFF_CANT_APPEND;
	    goto bad;
	}
	return (tif);
    }	

badformat:
    if (err) *err = NX_BAD_TIFF_FORMAT;
bad:
    tif->tif_mode = O_RDONLY;	/* XXX avoid flush */
    (void) _NXTIFFClose(tif);
    return ((TIFF *)0);
}

int _NXTIFFScanlineSize(TIFF *tif)
{
    TIFFDirectory *td = &tif->tif_dir;
    int scanline = td->td_bitspersample * td->td_imagewidth;
    if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
	scanline *= td->td_samplesperpixel;
    }
    return ((scanline + 7) / 8);
}

static int _NXGetTIFFInfoAux(int imageNumber, NXStream *s, NXTIFFInfo *info, TIFF **tiffOut)
{
    TIFF *tiff;
    int numImages, err;
    int size = 0;
    
    info->error = 0;	/* Assume no error... */

    if (!(tiff = _NXTIFFOpenStream (s, YES, &err))) {
	info->error = err;
	return 0;
    }
    
    // After this point, need to deallocate the tiff structure in case of
    // error.  Return value of 0 indicates error; otherwise return value 
    // indicates the number of bytes needed to hold the image.
    
    numImages = _NXTIFFNumDirectories(tiff) - 1;
    
    if (imageNumber > numImages) {
    
	info->error = NX_IMAGE_NOT_FOUND;
    
    } else if ((imageNumber > 0) && !_NXTIFFSetDirectory (tiff, imageNumber)) {
    
	info->error = NX_BAD_TIFF_FORMAT;
    
    } else if (tiff->tif_dir.td_nstrips > NX_PAGEHEIGHT) {
    
	info->error = NX_FORMAT_NOT_YET_SUPPORTED;
    
    } else if (tiff->tif_dir.td_photometric < 0 || 
		tiff->tif_dir.td_photometric > 5 ||
	 	(tiff->tif_dir.td_photometric == PHOTOMETRIC_PALETTE &&
		 tiff->tif_dir.td_bitspersample != 8) ||
		tiff->tif_dir.td_photometric == PHOTOMETRIC_MASK) {
    
	info->error = NX_FORMAT_NOT_YET_SUPPORTED;
    
    } else {
    
	TIFFDirectory *td = &(tiff->tif_dir);		
	NXImageInfo *image = &(info->image);
	TIFFHeader *th = &(tiff->tif_header);
	int stripCount;
    
	// Now be nice and copy the various info from the TIFF structure
	// to the old NXTIFFInfo structure.
    
	info->imageNumber = imageNumber;
	info->subfileType = (int)td->td_subfiletype;
	info->rowsPerStrip = (int)td->td_rowsperstrip;
	info->stripsPerImage = (int)td->td_stripsperimage;
	info->compression = (int)td->td_compression;
	info->numImages = numImages;
    
	image->width = (int)td->td_imagewidth;
	image->height = (int)td->td_imagelength;
	image->bitsPerSample = (int)td->td_bitspersample;
	image->samplesPerPixel = (int)td->td_samplesperpixel;
	image->planarConfig = (int)td->td_planarconfig;
	image->photoInterp = (int)td->td_photometric;

	if (image->photoInterp == PHOTOMETRIC_PALETTE) {
	    image->photoInterp = PHOTOMETRIC_RGB;	/* Don't like colormaps */
	    image->samplesPerPixel = 3;
	    image->planarConfig = PLANARCONFIG_CONTIG;
	}

	if (td->td_matteing != 0) {
	    image->photoInterp |= NX_ALPHAMASK;
	}
		
	for (stripCount = 0; stripCount < td->td_nstrips; stripCount++) {
	    info->stripOffsets[stripCount] = 
				    td->td_stripoffset[stripCount];
	    info->stripByteCounts[stripCount] = 
				    td->td_stripbytecount[stripCount];			
	}
    
	info->endian = (th->tiff_magic == TIFF_BIGENDIAN) ?
					NX_BIGENDIAN : 
					NX_LITTLEENDIAN;
	info->version = th->tiff_version;
	info->firstIFD = th->tiff_diroff;
    
	if ((size = _NXTIFFSize(tiff) *
			(td->td_photometric == PHOTOMETRIC_PALETTE ? 3 : 1)) == 0) {
	    info->error = NX_IMAGE_NOT_FOUND;
	}
    }
    
    if (size == 0) {
	_NXTIFFClose (tiff);
    } else {
	*tiffOut = tiff;
    }
    
    return size; 
}

/*
 * Gets some information about the TIFF file pointed to by stream and stuffs
 * it into the (user-supplied) structure info.  If an error occurs the error
 * code is returned in info->error and the return value is zero.  Otherwise
 * the return value is the number of bytes necessary to hold the whole 
 * uncompressed image.
 */
int NXGetTIFFInfo(int imageNumber, NXStream *s, NXTIFFInfo *info)
{
    TIFF *tiff;
    int size = _NXGetTIFFInfoAux(imageNumber, s, info, &tiff);
    if (size) {
	/*
         * The next line emulates the (undocumented but used in Mail and
	 * Bitmap) NXGetTIFFInfo behaviour of leaving the stream pointer
	 * pointing at the image.
	 */
	if (tiff->tif_dir.td_nstrips == 1) { 
	    NXSeek (s, info->stripOffsets[0], NX_FROMSTART);
	}
	_NXTIFFClose(tiff);
    }
    return size;
}

/*
 * Return the total number of bytes needed to read the whole image.
 */
int _NXTIFFSize(TIFF *tiff)
{
    TIFFDirectory *td = &(tiff->tif_dir);		
    int size = _NXTIFFScanlineSize(tiff);
    if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
	size *= td->td_imagelength;
    } else {
	size = size * td->td_imagelength * td->td_samplesperpixel;
    }
    return size;
}

/*
 * A historical cover for _NXTIFFReadFromZone(). Returns 0 on success, -1 on error.
 */
int _NXTIFFRead(TIFF *tiff, unsigned char *data)
{
    return _NXTIFFReadFromZone (tiff, data, NXDefaultMallocZone()) ? 0 : -1;
}

/*
 * Read the image into the memory pointed to by data. All TIFF reading in the Kit
 * should probably go through this function.
 * If data is NULL, this method will allocate memory.  Memory will be allocated
 * either through zoned-malloc() or valloc(); thus it can be freed with free(). If 
 * not NULL, the argument data should point to an area containing at least 
 * _NXTIFFSize() bytes. Returns NULL on error; no memory is allocated in this case.
 */
unsigned char *_NXTIFFReadFromZone(TIFF *tiff, unsigned char *data, NXZone *zone)
{
    int err = 0;
    TIFFDirectory *td = &(tiff->tif_dir);
    int lineSize = _NXTIFFScanlineSize(tiff);
    u_char *line;
    register u_int row;
    int len = td->td_imagelength;
    int numLines;  /* Num of scanlines to read */

#ifdef JPEG_SUPPORT

    if (td->td_compression == COMPRESSION_JPEG) {

	err = _NXTIFFReadJPEG (tiff, &data);

    } else

#endif
    {
	unsigned char *buffer;

	if (td->td_photometric == PHOTOMETRIC_PALETTE) {

	    unsigned short *red = td->td_redcolormap;
	    unsigned short *green = td->td_greencolormap;
	    unsigned short *blue = td->td_bluecolormap;

	    if (!data) {
		NX_ZONEMALLOC(zone, buffer, unsigned char, _NXTIFFSize(tiff) * 3);
	    } else {
		buffer = data;
	    }

	    /* Take care of the palette case... Not too efficient here. */

	    if (td->td_bitspersample != 8 || !red || !green || !blue) {
		err = -1;
	    } else {
		unsigned char *tmpLine, *outLoc = buffer;

		NX_ZONEMALLOC(NXDefaultMallocZone(),
					tmpLine, unsigned char, lineSize);
    
		for (row = 0; row < len; row++) {
		    if ((err = _NXTIFFReadScanline(tiff, tmpLine, row, 0)) == -1) {
			break;
		    } else {
			unsigned char *inLoc = tmpLine;
			unsigned int cnt, pixel;
			for (cnt = 0; cnt < lineSize; cnt++, inLoc++) {
			    pixel = (unsigned int)(*inLoc);
			    *outLoc++ = (unsigned char)(red[pixel] >> 8);
			    *outLoc++ = (unsigned char)(green[pixel] >> 8);
			    *outLoc++ = (unsigned char)(blue[pixel] >> 8);
			}
		    }
		}
		NX_FREE(tmpLine);
	    }

 	} else {

	    if (!data) {
		NX_ZONEMALLOC(zone, buffer, unsigned char, _NXTIFFSize(tiff));
	    } else {
		buffer = data;
	    }

	    if (td->td_planarconfig == PLANARCONFIG_CONTIG) {
		numLines = len;
	    } else {
		numLines = len * td->td_samplesperpixel;
	    }
	    
	    for (line = buffer, row = 0; row < numLines; row++, line += lineSize) {
		if ((err = _NXTIFFReadScanline(tiff, line, row % len, row / len)) == -1) 
		    break;
	    }
	}

	if ((err == -1) && !data) {
	    free (buffer);
	} else {
	    data = buffer;
	}
    }

    return (err == -1) ? NULL : data;
}

/*
 * NXReadTIFF will read the image data for image imageNumber in a TIFF
 * format file.  The info parameter is a pointer to an unitialized
 * NXTIFFInfo, which you should allocate on the stack. If data is equal
 * to NULL, the image data will be stored into a malloced region of
 * memory, otherwise it will be copied to the memory pointed to by data.
 * If there is an error, the error field of the info struct will be
 * non-zero and NXReadTIFF will return NULL.
 *
 * If the photometric indicates that the data is in "min-is-white" form, the
 * data is complemented and the photometric is changed to "min-is-black."
 *
 * Palette data (photometric 3) is returned as is; this should probably be
 * expanded. 
 */
void *NXReadTIFF(int imageNumber, NXStream *s, NXTIFFInfo *info, void *data)
{
    TIFF *tiff;
    int size;			// Size, in bytes, of whole image data
    unsigned char *buffer;
    
    if ((size = _NXGetTIFFInfoAux (imageNumber, s, info, &tiff)) == 0) {
	// In case of error NNXGetTIFFInfo deallocates the TIFF structure.
	return NULL;
    }
    
    buffer = _NXTIFFReadFromZone(tiff, data, NXDefaultMallocZone());
    
    _NXTIFFClose (tiff);
    
    if (!buffer) {
	info->error = NX_BAD_TIFF_FORMAT;	
	return NULL;
    } else {
#ifdef BECOMPATIBLEWITHOLDNXREADTIFF
	if (info->image.photoInterp == 0) {
	    _NXComplement(buffer, buffer, size);
	    info->image.photoInterp = 1;
	}
#endif
	return buffer;
    }
}

/*
 * Low level TIFF writing function; call on an open TIFF.
 * Returns 0 if no error.
 */
int _NXTIFFWrite (TIFF *tiff, unsigned char *data[5], int width, int height, BOOL isPlanar, BOOL hasAlpha, NXColorSpaceType colorSpace, int spp, int bps, int compression, float xRes, float yRes, int resUnit, char *software, float compressionFactor)
{
    int err = 0;

    if (_NXTIFFSetField(tiff, TIFFTAG_COMPRESSION, compression) != 1) {
	return NX_COMPRESSION_NOT_YET_SUPPORTED;
    }	

    _NXTIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, colorSpace);
    if (hasAlpha) {
	_NXTIFFSetField(tiff, TIFFTAG_MATTEING, 1);
    }						

    _NXTIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, width);
    _NXTIFFSetField(tiff, TIFFTAG_IMAGELENGTH, height);
    _NXTIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, bps);
    _NXTIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, spp);
    _NXTIFFSetField(tiff, TIFFTAG_PLANARCONFIG,
		    (isPlanar ? PLANARCONFIG_SEPARATE : PLANARCONFIG_CONTIG ));

    if (resUnit > 0 && resUnit < 3) {
	_NXTIFFSetField(tiff, TIFFTAG_XRESOLUTION, xRes);
	_NXTIFFSetField(tiff, TIFFTAG_YRESOLUTION, yRes);
	_NXTIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, resUnit);
    }
    
    if (software) {
	_NXTIFFSetField(tiff, TIFFTAG_SOFTWARE, software);
    }
    
#ifdef LIMITLZWSTRIPSIZE
    /*
	* Prevent huge strips with LZW; otherwise readers on wimpy machines
	* will choke. Also there seems to be a bug in creating huge strips
	* in LZW mode, anyway. 
	*/
    if (compression == COMPRESSION_LZW) {
	int rowsperstrip = (32*1024)/_NXTIFFScanlineSize(tiff);
	if (rowsperstrip == 0) rowsperstrip = 1;
	_NXTIFFSetField(tiff, TIFFTAG_ROWSPERSTRIP, rowsperstrip);
    }
#endif

#ifdef JPEG_SUPPORT
    if (compression == COMPRESSION_JPEG) {

	if ((err = _NXTIFFWriteJPEG (tiff, data, compressionFactor)) == -1) {
	    return NX_COMPRESSION_NOT_YET_SUPPORTED;
	}

    } else
#endif
    {
	int lineSize = _NXTIFFScanlineSize(tiff);
	unsigned char *line;
	int row, plane;

	for (plane = 0; plane < (isPlanar ? spp : 1); plane++) {
	    for (line = data[plane], row = 0;
		row < height; row++, line += lineSize) {
		if ((err = _NXTIFFWriteScanline(tiff, line, row, plane)) == -1) break;
	    }
	}
    }

    return (err == -1 ? NX_FILE_IO_ERROR : 0);
}


/*
 * This is a temporary backdoor to allow us to write TIFF files in various
 * formats until we come up with a decent scheme for specifying values
 * for different tags.  
 *
 * Call with resUnit == 0 if you don't want to specify resolution.  Pass in
 * resolution values defined in tiff.h for compression (1=none, 5=LZW).
 *
 * If you don't care about the software argument, simply
 * pass in NULL.
 *
 * This function returns 0 on no error or returns one of the error codes
 * defined in tiff.h.
 */
void NXWriteTIFF (NXStream *s, NXImageInfo *image, void *data)
{
    int err;
    int actualPhotoInterp;
    TIFF *tiff;
    
    if (!(tiff = _NXTIFFOpenStream (s, NO, &err))) {
	return; /* err */
    }
    
    switch (image->photoInterp & ~NX_ALPHAMASK) {
	case 0:
	case 1:
	case 2:
	    actualPhotoInterp = image->photoInterp & ~NX_ALPHAMASK;
	    break;
	case 3:
	    if (image->photoInterp & NX_ALPHAMASK) {
		NXLogError ("Can't write CMYKA to TIFF.\n");
		_NXTIFFClose (tiff);
		return; /* NX_FORMAT_NOT_YET_SUPPORTED */
	    }
	    actualPhotoInterp = 5;
	    break;
	default:
	    NXLogError ("Can't write photoInterp %d to TIFF.\n", 
					    image->photoInterp);
	    _NXTIFFClose (tiff);
	    return; /* NX_FORMAT_NOT_YET_SUPPORTED */
    }
    _NXTIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, actualPhotoInterp);

    /*
	* If the image has alpha, signal it with the MATTEing tag.
	*/
    if (image->photoInterp & NX_ALPHAMASK) {
	_NXTIFFSetField(tiff, TIFFTAG_MATTEING, 1);
    }						
    
    _NXTIFFSetField(tiff, TIFFTAG_COMPRESSION, 1);
    _NXTIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, image->width);
    _NXTIFFSetField(tiff, TIFFTAG_IMAGELENGTH, image->height);
    _NXTIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, image->bitsPerSample);
    _NXTIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, image->samplesPerPixel);
    _NXTIFFSetField(tiff, TIFFTAG_PLANARCONFIG, image->planarConfig);
    
    {
	int scanLineSize = _NXTIFFScanlineSize(tiff);
	u_char *scanLine;
	int row;
	int imageLength = image->height;
	int numLines;	// Num of scanlines to read; either imagelength
			// or imagelength * spp, depending on planarconfig.
	
	if (image->planarConfig == NX_MESHED) {
	    numLines = imageLength;
	} else {
	    numLines = imageLength * image->samplesPerPixel;
	}
	
	for (scanLine = data, row = 0; row < numLines; row++, scanLine += scanLineSize) {
	    if ((err = _NXTIFFWriteScanline(tiff, scanLine, row % imageLength, row / imageLength)) == -1) break;
	}
    }
    
    // At this point, if err = -1 we have an error. 
    
    _NXTIFFClose (tiff);	
}

#ifdef GETANDSETRESOLUTIONKLUDGE
 
int _NXTIFFGetWithExtraInfo(int imageNumber, NXStream *s, NXTIFFInfo *info, float *xRes, float *yRes, int *resolutionUnit)
{
    TIFF *tiff;
    int size = _NXGetTIFFInfoAux(imageNumber, s, info, &tiff);
    if (size) {
	/*
	 * The next line emulates the (undocumented but used in Mail and
	 * Bitmap) NXGetTIFFInfo behaviour of leaving the stream pointer
	 * pointing at the image.
	 */
	if (tiff->tif_dir.td_nstrips == 1) { 
            NXSeek (s, info->stripOffsets[0], NX_FROMSTART);
	}
	*xRes = tiff->tif_dir.td_xresolution;
	*yRes = tiff->tif_dir.td_yresolution;
	*resolutionUnit = tiff->tif_dir.td_resolutionunit;
	_NXTIFFClose(tiff);
    }
    return size;
}

#endif

#endif

/* End of original tif_open.c */

/* Start of original tif_close.c */

/*
 * TIFF Library.
 */
#ifndef NeXT_MOD
#include "tiffio.h"

void
TIFFClose(tif)
	TIFF *tif;
{
	if (tif->tif_mode != O_RDONLY)
		/*
		 * Flush buffered data and directory (if dirty).
		 */
		TIFFFlush(tif);
	if (tif->tif_cleanup)
		(*tif->tif_cleanup)(tif);
	TIFFFreeDirectory(tif);
	if (tif->tif_rawdata)
		free(tif->tif_rawdata);
	(void) close(tif->tif_fd);
	free((char *)tif);
}
#else
void _NXTIFFClose (TIFF *tif)
{
    if (tif->tif_mode != O_RDONLY)
	/*
	 * Flush buffered data and directory (if dirty).
	 */
	_NXTIFFFlush(tif);
    if (tif->tif_cleanup)
	(*tif->tif_cleanup)(tif);
    TIFFFreeDirectory(tif);
    if (tif->tif_rawdata)
	free(tif->tif_rawdata);
    free((char *)tif);
}
#endif

/* End of original tif_close.c */

/* Start of original tif_error.c */

/*
 * TIFF Library.
 */
#ifndef NeXT_MOD
#include <stdio.h>
#include "tiffio.h"
#endif

void
#if USE_PROTOTYPES
#ifdef NeXT_MOD
_NXTIFFError(char *module, char *fmt, ...)
#else
TIFFError(char *module, char *fmt, ...)
#endif
#else
/*VARARGS2*/
#ifdef NeXT_MOD
_NXTIFFError(module, fmt, va_alist)
	char *module;
	char *fmt;
	va_dcl
#else
TIFFError(module, fmt, va_alist)
	char *module;
	char *fmt;
	va_dcl
#endif
#endif
{
	va_list ap;

#ifndef NeXT_MOD
/*
 * ??? Should really make this thing call NXLogError.
 */
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
#else
	fprintf(stderr, "TIFF Error: ");
#endif
	VA_START(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ".\n");
}

/* End of original tif_error.c */

/* Start of original tif_warning.c */

/*
 * TIFF Library.
 */
#ifndef NeXT_MOD
#include <stdio.h>
#include "tiffio.h"
#endif

void
#if USE_PROTOTYPES
#ifdef NeXT_MOD
_NXTIFFWarning(char *module, char *fmt, ...)
#else
TIFFWarning(char *module, char *fmt, ...)
#endif
#else
/*VARARGS2*/
#ifdef NeXT_MOD
_NXTIFFWarning(module, fmt, va_alist)
	char *module;
	char *fmt;
	va_dcl
#else
TIFFWarning(module, fmt, va_alist)
	char *module;
	char *fmt;
	va_dcl
#endif
#endif
{
#ifdef AKDEBUG
#if 0
	va_list ap;

#ifdef NeXT_MOD
	fprintf(stderr, "TIFF Warning: ");
#else
	if (module != NULL)
		fprintf(stderr, "%s: ", module);
	fprintf(stderr, "Warning, ");
#endif
	VA_START(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fprintf(stderr, ".\n");
#endif
#endif
}

/* End of original tif_warning.c */

/*
  
Modifications (since 77):

77
--
2/19/90 aozer	Created from Sam Leffler's freely distributable library
2/21/90 aozer	Made NXGetTIFFInfo leave stream pointer at data (as some
		old stuff (Bitmap, Mail) depended on this feature)
 3/4/90 aozer	NXReadTIFF does not complement data anymore if photoMetric=0
 3/8/90 aozer	Frame depends on above; so NXReadTIFF does complement data

80
--
 4/11/90 aozer	Added some extern (but private) functions to allow access to
		the Leffler library. (Functions include _NXTIFFRead, etc)

85
--
 5/21/90 aozer	_NXTIFFOpen()'s err argument can now be NULL.

86
--
 6/9/90 aozer	Integrated changes for Sam Leffler's TIFF Library v2.2.
 6/11/90 aozer	Added ability to append to TIFF files through the normal
		writeTIFF: methods.

89
--
 7/31/90 aozer	Made _NXGetTIFFInfoAux() set info->error to zero. (This makes
		NXReadTIFF() etc all return zero in case of no error...)

91
--
 8/10/90 aozer	Added _NXTIFFReadFromZone().
 8/11/90 aozer	Added _NXTIFFWriteJPEG() and _NXTIFFReadJPEG().

92
--
 8/20/90 aozer	Added support for palette-based TIFF files; we read them in
		and make them true color...

*/

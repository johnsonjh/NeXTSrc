/*
	tiffPrivate.h
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#define NeXT_MOD

/*
 * This file contains the original tiff.h, tiffcompat.h,
 * tiffio.h, and machdep.h, 
 *
 * Original copyright:
 */

/*	tiff.h	1.18	90/05/22	*/

/*
 * Copyright (c) 1988, 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

#ifndef _TIFF_
#define	_TIFF_

#ifdef NeXT_MOD

#import <objc/objc.h>
#import <streams/streams.h>
#import "graphics.h"

/* 
 * Various flags...
 */
#define NEXT_SUPPORT
#define FAX3_SUPPORT
#define JPEG_SUPPORT
#define USE_VARARGS 0
#define USE_PROTOTYPES 1

// #define THUNDER_SUPPORT
// #define PICIO_SUPPORT
// #define CCITT_SUPPORT
// #define FAX4_SUPPORT
// #define SGI_SUPPORT

/*
 * Temporary kludgery; see tiff.m and tiffDir.m for actual use.
 */
#define LIMITLZWSTRIPSIZE
#define GETANDSETRESOLUTIONKLUDGE
#define DEALWITHOLDBUGGYALPHAIMAGES
#define BECOMPATIBLEWITHOLDNXREADTIFF
#define ACCEPTWRONGNUMBEROFBPSVALUES

#endif
/*
 * Tag Image File Format (TIFF)
 *
 * Based on Rev 5.0 from:
 *    Developer's Desk		Window Marketing Group
 *    Aldus Corporation		Microsoft Corporation
 *    411 First Ave. South	16011 NE 36th Way
 *    Suite 200			Box 97017
 *    Seattle, WA  98104	Redmond, WA  98073-9717
 *    206-622-5500		206-882-8080
 */
#define	TIFF_VERSION	42

#define	TIFF_BIGENDIAN		0x4d4d
#define	TIFF_LITTLEENDIAN	0x4949

typedef	struct {
	unsigned short tiff_magic;	/* magic number (defines byte order) */
	unsigned short tiff_version;	/* TIFF version number */
	unsigned long  tiff_diroff;	/* byte offset to first directory */
} TIFFHeader;

/*
 * TIFF Image File Directories are comprised of
 * a table of field descriptors of the form shown
 * below.  The table is sorted in ascending order
 * by tag.  The values associated with each entry
 * are disjoint and may appear anywhere in the file
 * (so long as they are placed on a word boundary).
 *
 * If the value is 4 bytes or less, then it is placed
 * in the offset field to save space.  If the value
 * is less than 4 bytes, it is left-justified in the
 * offset field.
 */
typedef	struct {
	unsigned short tdir_tag;	/* see below */
	unsigned short tdir_type;	/* data type; see below */
	unsigned long  tdir_count;	/* number of items; length in spec */
	unsigned long  tdir_offset;	/* byte offset to field data */
} TIFFDirEntry;

typedef	enum {
	TIFF_BYTE = 1,		/* 8-bit unsigned integer */
	TIFF_ASCII = 2,		/* 8-bit bytes w/ last byte null */
	TIFF_SHORT = 3,		/* 16-bit unsigned integer */
	TIFF_LONG = 4,		/* 32-bit unsigned integer */
	TIFF_RATIONAL = 5	/* 64-bit fractional (numerator+denominator) */
} TIFFDataType;

/*
 * TIFF Tag Definitions.
 *
 * Those marked with a + are obsoleted by revision 5.0
 */
#define	TIFFTAG_SUBFILETYPE		254	/* subfile data descriptor */
#define	    FILETYPE_REDUCEDIMAGE	0x1	/* reduced resolution version */
#define	    FILETYPE_PAGE		0x2	/* one page of many */
#define	    FILETYPE_MASK		0x4	/* transparency mask */
#define	TIFFTAG_OSUBFILETYPE		255	/* +kind of data in subfile */
#define	    OFILETYPE_IMAGE		1	/* full resolution image data */
#define	    OFILETYPE_REDUCEDIMAGE	2	/* reduced size image data */
#define	    OFILETYPE_PAGE		3	/* one page of many */
#define	TIFFTAG_IMAGEWIDTH		256	/* image width in pixels */
#define	TIFFTAG_IMAGELENGTH		257	/* image height in pixels */
#define	TIFFTAG_BITSPERSAMPLE		258	/* bits per channel (sample) */
#define	TIFFTAG_COMPRESSION		259	/* data compression technique */
#define	    COMPRESSION_NONE		1	/* dump mode */
#define	    COMPRESSION_CCITTRLE	2	/* CCITT modified Huffman RLE */
#define	    COMPRESSION_CCITTFAX3	3	/* CCITT Group 3 fax encoding */
#define	    COMPRESSION_CCITTFAX4	4	/* CCITT Group 4 fax encoding */
#define	    COMPRESSION_LZW		5	/* Lempel-Ziv  & Welch */
#define	    COMPRESSION_NEXT		32766	/* NeXT 2-bit RLE */
#define	    COMPRESSION_CCITTRLEW	32771	/* #1 w/ word alignment */
#define	    COMPRESSION_PACKBITS	32773	/* Macintosh RLE */
#define	    COMPRESSION_THUNDERSCAN	32809	/* ThunderScan RLE */
#define	    COMPRESSION_PICIO		32900	/* old Pixar picio RLE */
#define	    COMPRESSION_SGIRLE		32901	/* Silicon Graphics RLE */
#ifdef NeXT_MOD
#define	    COMPRESSION_JPEG		32865	/* JPEG */
#endif
#define	TIFFTAG_PHOTOMETRIC		262	/* photometric interpretation */
#define	    PHOTOMETRIC_MINISWHITE	0	/* min value is white */
#define	    PHOTOMETRIC_MINISBLACK	1	/* min value is black */
#define	    PHOTOMETRIC_RGB		2	/* RGB color model */
#define	    PHOTOMETRIC_PALETTE		3	/* color map indexed */
#define	    PHOTOMETRIC_MASK		4	/* holdout mask */
#define	    PHOTOMETRIC_DEPTH		32768	/* z-depth data */
#define	TIFFTAG_THRESHHOLDING		263	/* +thresholding used on data */
#define	    THRESHHOLD_BILEVEL		1	/* b&w art scan */
#define	    THRESHHOLD_HALFTONE		2	/* or dithered scan */
#define	    THRESHHOLD_ERRORDIFFUSE	3	/* usually floyd-steinberg */
#define	TIFFTAG_CELLWIDTH		264	/* +dithering matrix width */
#define	TIFFTAG_CELLLENGTH		265	/* +dithering matrix height */
#define	TIFFTAG_FILLORDER		266	/* +data order within a byte */
#define	    FILLORDER_MSB2LSB		1	/* most significant -> least */
#define	    FILLORDER_LSB2MSB		2	/* least significant -> most */
#define	TIFFTAG_DOCUMENTNAME		269	/* name of doc. image is from */
#define	TIFFTAG_IMAGEDESCRIPTION	270	/* info about image */
#define	TIFFTAG_MAKE			271	/* scanner manufacturer name */
#define	TIFFTAG_MODEL			272	/* scanner model name/number */
#define	TIFFTAG_STRIPOFFSETS		273	/* offsets to data strips */
#define	TIFFTAG_ORIENTATION		274	/* +image orientation */
#define	    ORIENTATION_TOPLEFT		1	/* row 0 top, col 0 lhs */
#define	    ORIENTATION_TOPRIGHT	2	/* row 0 top, col 0 rhs */
#define	    ORIENTATION_BOTRIGHT	3	/* row 0 bottom, col 0 rhs */
#define	    ORIENTATION_BOTLEFT		4	/* row 0 bottom, col 0 lhs */
#define	    ORIENTATION_LEFTTOP		5	/* row 0 lhs, col 0 top */
#define	    ORIENTATION_RIGHTTOP	6	/* row 0 rhs, col 0 top */
#define	    ORIENTATION_RIGHTBOT	7	/* row 0 rhs, col 0 bottom */
#define	    ORIENTATION_LEFTBOT		8	/* row 0 lhs, col 0 bottom */
#define	TIFFTAG_SAMPLESPERPIXEL		277	/* samples per pixel */
#define	TIFFTAG_ROWSPERSTRIP		278	/* rows per strip of data */
#define	TIFFTAG_STRIPBYTECOUNTS		279	/* bytes counts for strips */
#define	TIFFTAG_MINSAMPLEVALUE		280	/* +minimum sample value */
#define	TIFFTAG_MAXSAMPLEVALUE		281	/* maximum sample value */
#define	TIFFTAG_XRESOLUTION		282	/* pixels/resolution in x */
#define	TIFFTAG_YRESOLUTION		283	/* pixels/resolution in y */
#define	TIFFTAG_PLANARCONFIG		284	/* storage organization */
#define	    PLANARCONFIG_CONTIG		1	/* single image plane */
#define	    PLANARCONFIG_SEPARATE	2	/* separate planes of data */
#define	TIFFTAG_PAGENAME		285	/* page name image is from */
#define	TIFFTAG_XPOSITION		286	/* x page offset of image lhs */
#define	TIFFTAG_YPOSITION		287	/* y page offset of image lhs */
#define	TIFFTAG_FREEOFFSETS		288	/* +byte offset to free block */
#define	TIFFTAG_FREEBYTECOUNTS		289	/* +sizes of free blocks */
#define	TIFFTAG_GRAYRESPONSEUNIT	290	/* gray scale curve accuracy */
#define	    GRAYRESPONSEUNIT_10S	1	/* tenths of a unit */
#define	    GRAYRESPONSEUNIT_100S	2	/* hundredths of a unit */
#define	    GRAYRESPONSEUNIT_1000S	3	/* thousandths of a unit */
#define	    GRAYRESPONSEUNIT_10000S	4	/* ten-thousandths of a unit */
#define	    GRAYRESPONSEUNIT_100000S	5	/* hundred-thousandths */
#define	TIFFTAG_GRAYRESPONSECURVE	291	/* gray scale response curve */
#define	TIFFTAG_GROUP3OPTIONS		292	/* 32 flag bits */
#define	    GROUP3OPT_2DENCODING	0x1	/* 2-dimensional coding */
#define	    GROUP3OPT_UNCOMPRESSED	0x2	/* data not compressed */
#define	    GROUP3OPT_FILLBITS		0x4	/* fill to byte boundary */
#define	TIFFTAG_GROUP4OPTIONS		293	/* 32 flag bits */
#define	    GROUP4OPT_UNCOMPRESSED	0x2	/* data not compressed */
#define	TIFFTAG_RESOLUTIONUNIT		296	/* units of resolutions */
#define	    RESUNIT_NONE		1	/* no meaningful units */
#define	    RESUNIT_INCH		2	/* english */
#define	    RESUNIT_CENTIMETER		3	/* metric */
#define	TIFFTAG_PAGENUMBER		297	/* page numbers of multi-page */
#define	TIFFTAG_COLORRESPONSEUNIT	300	/* color scale curve accuracy */
#define	    COLORRESPONSEUNIT_10S	1	/* tenths of a unit */
#define	    COLORRESPONSEUNIT_100S	2	/* hundredths of a unit */
#define	    COLORRESPONSEUNIT_1000S	3	/* thousandths of a unit */
#define	    COLORRESPONSEUNIT_10000S	4	/* ten-thousandths of a unit */
#define	    COLORRESPONSEUNIT_100000S	5	/* hundred-thousandths */
#define	TIFFTAG_COLORRESPONSECURVE	301	/* RGB response curve */
#define	TIFFTAG_SOFTWARE		305	/* name & release */
#define	TIFFTAG_DATETIME		306	/* creation date and time */
#define	TIFFTAG_ARTIST			315	/* creator of image */
#define	TIFFTAG_HOSTCOMPUTER		316	/* machine where created */
#define	TIFFTAG_PREDICTOR		317	/* prediction scheme w/ LZW */
#define	TIFFTAG_WHITEPOINT		318	/* image white point */
#define	TIFFTAG_PRIMARYCHROMATICITIES	319	/* primary chromaticities */
#define	TIFFTAG_COLORMAP		320	/* RGB map for pallette image */
#define	TIFFTAG_BADFAXLINES		326	/* lines w/ wrong pixel count */
#define	TIFFTAG_CLEANFAXDATA		327	/* regenerated line info */
#define	     CLEANFAXDATA_CLEAN		0	/* no errors detected */
#define	     CLEANFAXDATA_REGENERATED	1	/* receiver regenerated lines */
#define	     CLEANFAXDATA_UNCLEAN	2	/* uncorrected errors exist */
#define	TIFFTAG_CONSECUTIVEBADFAXLINES	328	/* max consecutive bad lines */
/* tags 32995-32999 are private tags registered to SGI */
#define	TIFFTAG_MATTEING		32995	/* alpha channel is present */

#ifdef NeXT_MOD
#define	TIFFTAG_TILEWIDTH		322
#define	TIFFTAG_TILELENGTH		323
#define	TIFFTAG_TILEOFFSETS		324
#define	TIFFTAG_TILEBYTECOUNTS		325

#define TIFFTAG_JPEGMODE		33603
#define TIFFTAG_JPEGQFACTOR		33604
#define TIFFTAG_JPEGQMATRIXPRECISION	33605
#define TIFFTAG_JPEGQMATRIXDATA		33606
#define TIFFTAG_JPEGDCTABLES		33607
#define TIFFTAG_JPEGACTABLES		33608
#define TIFFTAG_NEWCOMPONENTSUBSAMPLE	33609
#endif

#endif /* _TIFF_ */

/* End of original tiff.h */

/* Start of original tiffcompat.h */

/*	tiffcompat.h	1.8	90/05/18	*/

#ifndef _COMPAT_
#define	_COMPAT_

/*
 * Copyright (c) 1990 by Sam Leffler.
 * All rights reserved.
 *
 * This file is provided for unrestricted use provided that this
 * legend is included on all tape media and as a part of the
 * software program in whole or part.  Users may copy, modify or
 * distribute this file at will.
 */

/*
 * This file contains a hodgepodge of definitions and
 * declarations that are needed to provide compatibility
 * between the native system and the base UNIX implementation
 * that the library assumes (~4BSD).  In particular, you
 * can override the standard i/o interface (read/write/lseek)
 * by redefining the ReadOK/WriteOK/SeekOK macros to your
 * liking.
 *
 * NB: This file is a mess.
 */
#if defined(__STDC__) || USE_PROTOTYPES
#include <stdio.h>
#endif
#include <sys/types.h>
#include <fcntl.h>
#ifdef THINK_C
#include <stdlib.h>
#endif

#ifdef SYSV
#include <unistd.h>

#define	L_SET	SEEK_SET
#define	L_INCR	SEEK_CUR
#define	L_XTND	SEEK_END

#define	bzero(dst,len)		memset(dst, 0, len)
#define	bcopy(src,dst,len)	memcpy(dst, src, len)
#define	bcmp(src, dst, len)	memcmp(dst, src, len)
#endif

#ifdef BSDTYPES
typedef	unsigned char u_char;
typedef	unsigned short u_short;
typedef	unsigned int u_int;
typedef	unsigned long u_long;
#endif

/*
 * Return an open file descriptor or -1.
 */
#if defined(applec) || defined(THINK_C)
#define	TIFFOpenFile(name, mode, prot)	open(name, mode)
#else
#if defined(MSDOS)
#define	TIFFOpenFile(name, mode, prot)	open(name, mode|O_BINARY, prot)
#else
#define	TIFFOpenFile(name, mode, prot)	open(name, mode, prot)
#endif
#endif

/*
 * Return the size in bytes of the file
 * associated with the supplied file descriptor.
 */
extern	long TIFFGetFileSize();

#ifndef L_SET
#define L_SET	0
#define L_INCR	1
#define L_XTND	2
#endif

#ifdef NeXT_MOD

#undef L_SET
#undef L_INCR
#undef L_XTND

#ifndef ReadOK
#define	ReadOK(s, buf, size)	(NXRead(s, (char *)buf, size) == size)
#endif
#ifndef SeekOK
#define	SeekOK(s, off)			(NXSeek(s, (long)off, NX_FROMSTART), 1)
#endif
#ifndef WriteOK
#define	WriteOK(s, buf, size)	(NXWrite(s, (char *)buf, size) == size)
#endif

#else
#ifdef notdef
#define lseek unix_lseek	/* Mac's Standard 'lseek' won't extend file */
#endif
extern	long lseek();

#ifndef ReadOK
#define	ReadOK(fd, buf, size)	(read(fd, (char *)buf, size) == size)
#endif
#ifndef SeekOK
#define	SeekOK(fd, off)	(lseek(fd, (long)off, L_SET) == (long)off)
#endif
#ifndef WriteOK
#define	WriteOK(fd, buf, size)	(write(fd, (char *)buf, size) == size)
#endif
#endif

#if defined(__MACH__) || defined(THINK_C)
extern	void *malloc(size_t size);
extern	void *realloc(void *ptr, size_t size);
#else
#if defined(MSDOS)
#include <malloc.h>
#else
extern	char *malloc();
extern	char *realloc();
#endif
#endif

/*
 * dblparam_t is the type that a double precision
 * floating point value will have on the parameter
 * stack (when coerced by the compiler).
 */
#ifdef applec
typedef extended dblparam_t;
#else
typedef double dblparam_t;
#endif

/*
 * Varargs parameter list handling...YECH!!!!
 */
#if defined(__STDC__) && !defined(USE_VARARGS)
#define	USE_VARARGS	0
#endif

#if defined(USE_VARARGS)
#if USE_VARARGS
#include <varargs.h>
#define	VA_START(ap, parmN)	va_start(ap)
#else
#include <stdarg.h>
#define	VA_START(ap, parmN)	va_start(ap, parmN)
#endif
#endif /* defined(USE_VARARGS) */

/*
 * How to concatenate lexical tokens with
 * the C preprocessor -- more YECH!
 */
#if defined(__STDC__) && !defined(apollo)
#define	CAT(a,b)	a##b
#else
#define	IDENT(x)	x
#define	CAT(a,b)	IDENT(a)b
#endif

#if defined(__STDC__) && !defined(USE_PROTOTYPES)
#define	USE_PROTOTYPES	1
#endif
#endif /* _COMPAT_ */

/* End of origin tiffcompat.h */

/* Start of original tiffio.h */

/*	tiffio.h	1.27	90/05/22	*/

#ifndef _TIFFIO_
#define	_TIFFIO_

#ifdef NeXT_MOD
#define TIFFWarning 			_NXTIFFWarning
#define TIFFError 			_NXTIFFError
#define TIFFReadDirectory 		_NXTIFFReadDirectory 
#define	TIFFFreeDirectory 		_NXTIFFFreeDirectory
#define	TIFFDefaultDirectory		_NXTIFFDefaultDirectory
#define	TIFFGetField 			_NXTIFFGetField
#define	TIFFSetField 			_NXTIFFSetField
#define	TIFFWriteDirectory 		_NXTIFFWriteDirectory
#define	TIFFSetDirectory 		_NXTIFFSetDirectory
#define	TIFFScanlineSize		_NXTIFFScanlineSize
#define TIFFFlushData1			_NXTIFFFlushData1
#define TIFFWriteCheck			_NXTIFFWriteCheck
#endif

/*
 * TIFF I/O Library Definitions.
 */
#ifndef NeXT_MOD
#include "tiffcompat.h"
#include "tiff.h"
#endif

/*
 * Internal format of a TIFF directory entry.
 */
typedef	struct {
	u_short	td_subfiletype;
	u_short	td_imagewidth, td_imagelength;
	u_short	td_bitspersample;
	u_short	td_compression;
	u_short	td_photometric;
	u_short	td_threshholding;
	u_short	td_fillorder;
	u_short	td_orientation;
	u_short	td_samplesperpixel;
	u_short	td_predictor;
	u_long	td_rowsperstrip;
	u_long	td_minsamplevalue, td_maxsamplevalue;	/* maybe float? */
	float	td_xresolution, td_yresolution;
	u_short	td_resolutionunit;
	u_short	td_planarconfig;
	float	td_xposition, td_yposition;
	u_long	td_group3options;
	u_long	td_group4options;
	u_short	td_pagenumber[2];
	u_short	td_grayresponseunit;
	u_short	td_colorresponseunit;
	u_short	td_matteing;
	u_short	td_cleanfaxdata;
	u_short	td_badfaxrun;
	u_long	td_badfaxlines;
	u_short	*td_grayresponsecurve;	/* u_short for now (maybe float?) */
	u_short	*td_redresponsecurve;	/* u_short for now (maybe float?) */
	u_short	*td_greenresponsecurve;	/* u_short for now (maybe float?) */
	u_short	*td_blueresponsecurve;	/* u_short for now (maybe float?) */
	u_short	*td_redcolormap;
	u_short	*td_greencolormap;
	u_short	*td_bluecolormap;
#ifdef notdef
/* not yet used/supported */
	float	td_whitepoint[2];
	float	td_primarychromaticities[6];
#endif
	char	*td_documentname;
	char	*td_artist;
	char	*td_datetime;
	char	*td_hostcomputer;
	char	*td_imagedescription;
	char	*td_make;
	char	*td_model;
	char	*td_software;
	char	*td_pagename;
	u_long	td_fieldsset[2];	/* bit vector of fields that are set */
	u_long	td_stripsperimage;
	u_long	td_nstrips;		/* size of offset & bytecount arrays */
	u_long	*td_stripoffset;
	u_long	*td_stripbytecount;
#ifdef NeXT_MOD
	u_long  td_tilesacross;
	u_long  td_tilesdown;
	u_short td_tilewidth;
	u_short td_tilelength;
	/* currently we treat tiles as strips; thus tilesacross must be 1.
	 * stripbytecount and stripoffset arrays store the tilebytecount
	 * and tileoffset values.
	 */
	u_short td_usetiles;
	/* the jpeg stuff */
	u_short	td_jpegmode;
	u_short	td_jpegqfactor;
	u_short	td_jpegqmatrixprecision;
	u_short	td_newcomponentsubsample[10];
	/* as many as we can have channels */
	u_long	td_jpegqmatrixdata[5];
	u_long	td_jpegdctables[5];
	u_long	td_jpegactables[5];
#endif
} TIFFDirectory;

/*
 * Field flags used to indicate fields that have
 * been set in a directory, and to reference fields
 * when manipulating a directory.
 */
/* multi-entry fields */
#define	FIELD_IMAGEDIMENSIONS		0
#define	FIELD_CELLDIMENSIONS		1		/* XXX */
#define	FIELD_RESOLUTION		2
#define	FIELD_POSITION			3
/* single-entry fields */
#define	FIELD_SUBFILETYPE		4
#define	FIELD_BITSPERSAMPLE		5
#define	FIELD_COMPRESSION		6
#define	FIELD_PHOTOMETRIC		7
#define	FIELD_THRESHHOLDING		8
#define	FIELD_FILLORDER			9		/* XXX */
#define	FIELD_DOCUMENTNAME		10
#define	FIELD_IMAGEDESCRIPTION		11
#define	FIELD_MAKE			12
#define	FIELD_MODEL			13
#define	FIELD_ORIENTATION		14
#define	FIELD_SAMPLESPERPIXEL		15
#define	FIELD_ROWSPERSTRIP		16
#define	FIELD_MINSAMPLEVALUE		17
#define	FIELD_MAXSAMPLEVALUE		18
#define	FIELD_PLANARCONFIG		19
#define	FIELD_PAGENAME			20
#define	FIELD_GRAYRESPONSEUNIT		21
#define	FIELD_GRAYRESPONSECURVE		22
#define	FIELD_GROUP3OPTIONS		23
#define	FIELD_GROUP4OPTIONS		24
#define	FIELD_RESOLUTIONUNIT		25
#define	FIELD_PAGENUMBER		26
#define	FIELD_COLORRESPONSEUNIT		27
#define	FIELD_COLORRESPONSECURVE	28
#define	FIELD_STRIPBYTECOUNTS		29
#define	FIELD_STRIPOFFSETS		31
#define	FIELD_COLORMAP			32
#define FIELD_PREDICTOR			33
#define FIELD_ARTIST			34
#define FIELD_DATETIME			35
#define FIELD_HOSTCOMPUTER		36
#define FIELD_SOFTWARE			37
#define	FIELD_MATTEING			38
#define	FIELD_BADFAXLINES		39
#define	FIELD_CLEANFAXDATA		40
#define	FIELD_BADFAXRUN			41
#ifdef NeXT_MOD
#define	FIELD_TILEWIDTH			42
#define	FIELD_TILELENGTH		43
#define	FIELD_TILEOFFSETS		44
#define	FIELD_TILEBYTECOUNTS		45
#define FIELD_JPEGMODE			46
#define FIELD_JPEGQFACTOR		47
#define FIELD_JPEGQMATRIXPRECISION	48
#define FIELD_JPEGQMATRIXDATA		49
#define FIELD_JPEGDCTABLES		50
#define FIELD_JPEGACTABLES		51
#define FIELD_NEWCOMPONENTSUBSAMPLE	52
#define	FIELD_LAST			FIELD_NEWCOMPONENTSUBSAMPLE
#else
#define	FIELD_LAST			FIELD_BADFAXRUN
#endif

#define	TIFFFieldSet(tif, field) \
    ((tif)->tif_dir.td_fieldsset[field/32] & (1L<<(field&0x1f)))
#define	TIFFSetFieldBit(tif, field) \
    ((tif)->tif_dir.td_fieldsset[field/32] |= (1L<<(field&0x1f)))

typedef	struct {
	char	*tif_name;		/* name of open file */
#ifdef NeXT_MOD
	NXStream *tif_fd;		/* open stream descriptor */
#else
	short	tif_fd;			/* open file descriptor */
#endif
	short	tif_mode;		/* open mode (O_*) */
	char	tif_fillorder;		/* natural bit fill order for machine */
	char	tif_options;		/* compression-specific options */
	short	tif_flags;
#define	TIFF_DIRTYHEADER	0x1	/* header must be written on close */
#define	TIFF_DIRTYDIRECT	0x2	/* current directory must be written */
#define	TIFF_BUFFERSETUP	0x4	/* data buffers setup */
#define	TIFF_BEENWRITING	0x8	/* written 1+ scanlines to file */
#define	TIFF_SWAB		0x10	/* byte swap file information */
#define	TIFF_NOBITREV		0x20	/* inhibit bit reversal logic */
	long	tif_diroff;		/* file offset of current directory */
	long	tif_nextdiroff;		/* file offset of following directory */
	TIFFDirectory tif_dir;		/* internal rep of current directory */
	TIFFHeader tif_header;		/* file's header block */
	int	tif_typeshift[6];	/* data type shift counts */
	long	tif_typemask[6];	/* data type masks */
	long	tif_row;		/* current scanline */
	int	tif_curstrip;		/* current strip for read/write */
	long	tif_curoff;		/* current offset for read/write */
/* compression scheme hooks */
	int	(*tif_stripdecode)();	/* strip decoding routine (pre) */
	int	(*tif_decoderow)();	/* scanline decoding routine */
	int	(*tif_stripencode)();	/* strip encoding routine (pre) */
	int	(*tif_encoderow)();	/* scanline encoding routine */
	int	(*tif_encodestrip)();	/* strip encoding routine (post) */
	int	(*tif_close)();		/* cleanup-on-close routine */
	int	(*tif_seek)();		/* position within a strip routine */
	int	(*tif_cleanup)();	/* routine called to cleanup state */
	char	*tif_data;		/* compression scheme private data */
/* input/output buffering */
	int	tif_scanlinesize;	/* # of bytes in a scanline */
	char	*tif_rawdata;		/* raw data buffer */
	long	tif_rawdatasize;	/* # of bytes in raw data buffer */
	char	*tif_rawcp;		/* current spot in raw buffer */
	long	tif_rawcc;		/* bytes unread from raw buffer */
#ifdef NeXT_MOD
	long	tif_dataoff;		/* used to be static in tif_dir.m */
	int	tif_freeprev;		/* indicates that prev dir should be freed */
#endif		
} TIFF;

/* generic option bit names */
#define	TIFF_OPT0	0x1
#define	TIFF_OPT1	0x2
#define	TIFF_OPT2	0x4
#define	TIFF_OPT3	0x8
#define	TIFF_OPT4	0x10
#define	TIFF_OPT5	0x20
#define	TIFF_OPT6	0x40
#define	TIFF_OPT7	0x80

#ifndef NULL
#define	NULL	0
#endif

#ifndef NeXT_MOD
extern u_char TIFFBitRevTable[256];
extern u_char TIFFNoBitRevTable[256];
#endif

#ifdef NeXT_MOD
/* 
 * The following are the symbols that are private to the TIFF package
 * (and hence the whole AppKit).
 */
extern	const unsigned char *_NXTIFFBitTable(BOOL reverse);
extern	void _NXTIFFError(char*, char*, ...);
extern	void _NXTIFFWarning(char*, char*, ...);
extern	int  _NXTIFFFlush(TIFF*);
extern	int  _NXTIFFFlushData(TIFF*);
extern	int  _NXTIFFFlushData1(TIFF*);
extern	int  _NXTIFFGetField(TIFF*, int, ...);
extern	int  _NXTIFFReadDirectory(TIFF*);
extern	int  _NXTIFFScanlineSize(TIFF*);
extern	int  _NXTIFFSetDirectory(TIFF*, int);
extern	int  _NXTIFFSetField(TIFF*, int, ...);
extern	int  _NXTIFFWriteDirectory(TIFF *);
extern	int  _NXTIFFDefaultDirectory(TIFF *);
extern	void _NXTIFFFreeDirectory(TIFF *);
extern	int  _NXTIFFReadScanline(TIFF*, u_char*, u_int, u_int);
extern	int  _NXTIFFWriteScanline(TIFF*, u_char*, u_int, u_int);
extern  int  _NXTIFFSetCompressionScheme(TIFF *, int);
extern	int  _NXTIFFNumDirectories(TIFF *);
extern  int  _NXTIFFWrite (TIFF *tiff, unsigned char *data[5], int width, int height, BOOL isPlanar, BOOL hasAlpha, NXColorSpaceType colorSpace, int spp, int bps, int compression, float xRes, float yRes, int resUnit, char *software, float compressionFactor);
extern  TIFF *_NXTIFFOpenStream(NXStream *s, BOOL readMode, int *err);
extern  TIFF *_NXTIFFOpenStreamWithMode(NXStream *s, int mode, int *err);
extern  void _NXTIFFClose (TIFF *tif);
extern  unsigned char *_NXTIFFReadFromZone(TIFF *tiff, unsigned char *data, NXZone *zone);
extern  int _NXTIFFRead(TIFF *tiff, unsigned char *data);
extern  int _NXTIFFSize(TIFF *tiff);
extern  int _NXTIFFWriteRawStrip(TIFF *tif, u_int strip, u_char *data, u_int cc);
extern  int _NXTIFFReadRawStrip(TIFF *tif, u_int strip, u_char *data, u_int cc);
extern  int _NXTIFFReadJPEG (TIFF *tiff, unsigned char **data); 
extern  int _NXTIFFWriteJPEG(register TIFF *tif, u_char *planes[5], float compression);
extern  int _NXTIFFWriteCheck(TIFF *, char *);

#define NX_TIFFWRITEMODE	O_WRONLY 
#define NX_TIFFREADMODE		O_RDONLY
#define NX_TIFFAPPENDMODE 	O_RDWR

#define _NXTIFFFieldSet(tif,field) TIFFFieldSet(tif,field)
#define _NXTIFFOpenForRead(stream) _NXTIFFOpenStream(stream, YES, NULL)
/*
 * We assume 16-bit shorts and 32-bit longs.
 */
#define _NXTIFFSwabShort(wp) \
	{register unsigned char *cp = (unsigned char *)(wp); \
	 int t = cp[1]; cp[1] = cp[0]; cp[0] = t;}

#define _NXTIFFSwabLong(lp) \
	{register unsigned char *cp = (unsigned char *)(lp); \
	 int t = cp[3]; cp[3] = cp[0]; cp[0] = t; \
	 t = cp[2]; cp[2] = cp[1]; cp[1] = t;}

#ifndef _NXTIFFSwabShort
extern void _NXTIFFSwabShort(unsigned short *);
#endif
#ifndef _NXTIFFSwabLong
extern void _NXTIFFSwabLong(unsigned long *);
#endif
#ifndef _NXTIFFSwabArrayOfShort
extern void _NXTIFFSwabArrayOfShort(register unsigned short *, register int);
#endif
#ifndef _NXTIFFSwabArrayOfLong
extern void _NXTIFFSwabArrayOfLong(register unsigned long *, register int);
#endif

#else
#if defined(c_plusplus) || defined(__cplusplus) || defined(__STDC__) || USE_PROTOTYPES
#if defined(__cplusplus)
extern "C" {
#endif
extern	void TIFFClose(TIFF*);
extern	int TIFFFlush(TIFF*);
extern	int TIFFFlushData(TIFF*);
extern	int TIFFGetField(TIFF*, int, ...);
extern	int TIFFReadDirectory(TIFF*);
extern	int TIFFScanlineSize(TIFF*);
extern	int TIFFSetDirectory(TIFF*, int);
extern	int TIFFSetField(TIFF*, int, ...);
extern	int TIFFWriteDirectory(TIFF *);
#if defined(c_plusplus) || defined(__cplusplus)
extern	TIFF* TIFFOpen(const char*, const char*);
extern	void TIFFError(const char*, const char*, ...);
extern	void TIFFWarning(const char*, const char*, ...);
extern	void TIFFPrintDirectory(TIFF*, FILE*, int = 0, int = 0, int = 0);
extern	int TIFFReadScanline(TIFF*, u_char*, u_int, u_int = 0);
extern	int TIFFWriteScanline(TIFF*, u_char*, u_int, u_int = 0);
#else
extern	TIFF* TIFFOpen(char*, char*);
extern	void TIFFError(char*, char*, ...);
extern	void TIFFWarning(char*, char*, ...);
extern	void TIFFPrintDirectory(TIFF*, FILE*, int, int, int);
extern	int TIFFReadScanline(TIFF*, u_char*, u_int, u_int);
extern	int TIFFWriteScanline(TIFF*, u_char*, u_int, u_int);
#endif
extern	int TIFFReadEncodedStrip(TIFF*, u_int, u_char *, u_int);
extern	int TIFFReadRawStrip(TIFF*, u_int, u_char *, u_int);
extern	int TIFFWriteEncodedStrip(TIFF*, u_int, u_char *, u_int);
extern	int TIFFWriteRawStrip(TIFF*, u_int, u_char *, u_int);
#if defined(__cplusplus)
}
#endif
#else
extern	void TIFFClose();
extern	TIFF *TIFFOpen();
extern	void TIFFError();
extern	int TIFFFlush();
extern	int TIFFFlushData();
extern	int TIFFGetField();
extern	void TIFFPrintDirectory();
extern	int TIFFReadDirectory();
extern	int TIFFReadScanline();
extern	int TIFFScanlineSize();
extern	int TIFFSetDirectory();
extern	int TIFFSetField();
extern	void TIFFWarning();
extern	int TIFFWriteDirectory();
extern	int TIFFWriteScanline();
#endif
#endif

#endif /* _TIFFIO_ */

/* End of original tiffio.h */

/* Start of origin machdep.h */

/*	machdep.h	1.11	90/05/22	*/

#ifndef _MACHDEP_
#define	_MACHDEP_
/*
 * Machine dependent definitions:
 *   o floating point formats
 *   o byte ordering
 *   o signed/unsigned chars
 *
 * NB, there are lots of assumptions here:
 *   - 32-bit natural integers	(sign extension code)
 *   - native float is 4 bytes	(floating point conversion)
 *   - native double is 8 bytes	(floating point conversion)
 */

#if defined(sun) || defined(sparc) || defined(stellar) || defined(MIPSEB) || defined(hpux) || defined(apollo) || defined(NeXT)
#define	BIGENDIAN	1
#define	IEEEFP
#endif /* sun || sparc || stellar || MIPSEB || hpux || apollo || NeXT */

/* MIPSEL = MIPS w/ Little Endian byte ordering (e.g. DEC 3100) */
#if defined(MIPSEL)
#define	BIGENDIAN	0
#define	IEEEFP
#endif /* MIPSEL */

#ifdef transputer
#define	BIGENDIAN	0
#define	UCHARS			/* yech! unsigned characters */
#define	IEEEFP			/* IEEE floating point supported */
#endif /* transputer */

#if defined(m386) || defined(M_I86) || defined(i386)
#define BIGENDIAN	0
#define IEEEFP			/* IEEE floating point supported */
#endif /* m386 || M_I86 || i386 */

#ifdef mips
#define	UCHARS			/* unsigned characters */
#endif /* mips */

#ifdef IEEEFP
typedef	struct ieeedouble nativedouble;
typedef	struct ieeefloat nativefloat;
#define	ISFRACTION(e)	(1022 - 4 <= (e) && (e) <= 1022 + 15)
#define	EXTRACTFRACTION(dp, fract) \
   ((fract) = ((unsigned long)(1<<31)|((dp)->native.mant<<11)|\
      ((dp)->native.mant2>>21)) >> (1022+16-(dp)->native.exp))
#define	EXTRACTEXPONENT(dp, exponent) ((exponent) = (dp)->native.exp)
#define	NATIVE2IEEEFLOAT(fp)
#define	IEEEFLOAT2NATIVE(fp)
#define	IEEEDOUBLE2NATIVE(dp)

#ifdef NeXT_MOD
#define	_NXTIFFSwabArrayOfFloat(fp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)fp,n)
#define	_NXTIFFSwabArrayOfDouble(dp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)dp,2*n)
#else
#define	TIFFSwabArrayOfFloat(fp,n)  TIFFSwabArrayOfLong((unsigned long *)fp,n)
#define	TIFFSwabArrayOfDouble(dp,n) TIFFSwabArrayOfLong((unsigned long *)dp,2*n)
#endif
#endif /* IEEEFP */

#ifdef tahoe
#define	BIGENDIAN	1

typedef	struct {
	unsigned	sign:1;
	unsigned	exp:8;
	unsigned	mant:23;
	unsigned	mant2;
} nativedouble;
typedef	struct {
	unsigned	sign:1;
	unsigned	exp:8;
	unsigned	mant:23;
} nativefloat;
#define	ISFRACTION(e)	(128 - 4 <= (e) && (e) <= 128 + 15)
#define	EXTRACTFRACTION(dp, fract) \
    ((fract) = ((1<<31)|((dp)->native.mant<<8)|((dp)->native.mant2>>15)) >> \
	(128+16-(dp)->native.exp))
#define	EXTRACTEXPONENT(dp, exponent) ((exponent) = (dp)->native.exp - 2)
/*
 * Beware, over/under-flow in conversions will
 * result in garbage values -- handling it would
 * require a subroutine call or lots more code.
 */
#define	NATIVE2IEEEFLOAT(fp) { \
    if ((fp)->native.exp) \
        (fp)->ieee.exp = (fp)->native.exp - 129 + 127;	/* alter bias */\
}
#define	IEEEFLOAT2NATIVE(fp) { \
    if ((fp)->ieee.exp) \
        (fp)->native.exp = (fp)->ieee.exp - 127 + 129; 	/* alter bias */\
}
#define	IEEEDOUBLE2NATIVE(dp) { \
    if ((dp)->native.exp = (dp)->ieee.exp) \
	(dp)->native.exp += -1023 + 129; \
    (dp)->native.mant = ((dp)->ieee.mant<<3)|((dp)->native.mant2>>29); \
    (dp)->native.mant2 <<= 3; \
}
/* the following is to work around a compiler bug... */
#define	SIGNEXTEND(a,b)	{ char ch; ch = (a); (b) = ch; }

#ifdef NeXT_MOD
#define	_NXTIFFSwabArrayOfFloat(fp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)fp,n)
#define	_NXTIFFSwabArrayOfDouble(dp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)dp,2*n)
#else
#define	TIFFSwabArrayOfFloat(fp,n)  TIFFSwabArrayOfLong((unsigned long *)fp,n)
#define	TIFFSwabArrayOfDouble(dp,n) TIFFSwabArrayOfLong((unsigned long *)dp,2*n)
#endif
#endif /* tahoe */

#ifdef vax
#define	BIGENDIAN	0

typedef	struct {
	unsigned	mant1:7;
	unsigned	exp:8;
	unsigned	sign:1;
	unsigned	mant2:16;
	unsigned	mant3;
} nativedouble;
typedef	struct {
	unsigned	mant1:7;
	unsigned	exp:8;
	unsigned	sign:1;
	unsigned	mant2:16;
} nativefloat;
#define	ISFRACTION(e)	(128 - 4 <= (e) && (e) <= 128 + 15)
#define	EXTRACTFRACTION(dp, fract) \
    ((fract) = ((1<<31)|((dp)->native.mant1<<16)|(dp)->native.mant2)) >> \
	(128+16-(dp)->native.exp))
#define	EXTRACTEXPONENT(dp, exponent) ((exponent) = (dp)->native.exp - 2)
/*
 * Beware, these do not handle over/under-flow
 * during conversion from ieee to native format.
 */
#define	NATIVE2IEEEFLOAT(fp) { \
    float_t t; \
    if (t.ieee.exp = (fp)->native.exp) \
	t.ieee.exp += -129 + 127; \
    t.ieee.sign = (fp)->native.sign; \
    t.ieee.mant = ((fp)->native.mant1<<16)|(fp)->native.mant2; \
    *(fp) = t; \
}
#define	IEEEFLOAT2NATIVE(fp) { \
    float_t t; int v = (fp)->ieee.exp; \
    if (v) v += -127 + 129;		/* alter bias of exponent */\
    t.native.exp = v;			/* implicit truncation of exponent */\
    t.native.sign = (fp)->ieee.sign; \
    v = (fp)->ieee.mant; \
    t.native.mant1 = v >> 16; \
    t.native.mant2 = v;\
    *(fp) = v; \
}
#define	IEEEDOUBLE2NATIVE(dp) { \
    double_t t; int v = (dp)->ieee.exp; \
    if (v) v += -1023 + 129; 		/* if can alter bias of exponent */\
    t.native.exp = v;			/* implicit truncation of exponent */\
    v = (dp)->ieee.mant; \
    t.native.sign = (dp)->ieee.sign; \
    t.native.mant1 = v >> 16; \
    t.native.mant2 = v;\
    t.native.mant3 = (dp)->mant2; \
    *(dp) = t; \
}

#ifdef NeXT_MOD
#define	_NXTIFFSwabArrayOfFloat(fp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)fp,n)
#define	_NXTIFFSwabArrayOfDouble(dp,n) \
		_NXTIFFSwabArrayLong((unsigned long *)dp,2*n)
#else
#define	TIFFSwabArrayOfFloat(fp,n)  TIFFSwabArrayOfLong((unsigned long *)fp,n)
#define	TIFFSwabArrayOfDouble(dp,n) TIFFSwabArrayOfLong((unsigned long *)dp,2*n)
#endif
#endif /* vax */

/*
 * These unions are used during floating point
 * conversions.  The macros given above define
 * the conversion operations.
 */

typedef	struct ieeedouble {
#if BIGENDIAN == 1
	unsigned	sign:1;
	unsigned	exp:11;
	unsigned long	mant:20;
	unsigned	mant2;
#else
#if !defined(vax)
#ifdef INT_16_BIT	/* MSDOS C compilers */
	unsigned long	mant2;
	unsigned long	mant:20;
	unsigned long	exp:11;
	unsigned long	sign:1;
#else /* 32 bit ints */
	unsigned	mant2;
	unsigned long	mant:20;
	unsigned	exp:11;
	unsigned	sign:1;
#endif /* 32 bit ints */
#else
	unsigned long	mant:20;
	unsigned	exp:11;
	unsigned	sign:1;
	unsigned	mant2;
#endif /* !vax */
#endif
} ieeedouble;
typedef	struct ieeefloat {
#if BIGENDIAN == 1
	unsigned	sign:1;
	unsigned	exp:8;
	unsigned long	mant:23;
#else
#ifdef INT_16_BIT	/* MSDOS C compilers */
	unsigned long	mant:23;
	unsigned long	exp:8;
	unsigned long	sign:1;
#else /* 32 bit ints */
	unsigned long	mant:23;
	unsigned	exp:8;
	unsigned	sign:1;
#endif /* 32 bit ints */
#endif
} ieeefloat;

typedef	union {
	ieeedouble	ieee;
	nativedouble	native;
	char		b[8];
	double		d;
} double_t;

typedef	union {
	ieeefloat	ieee;
	nativefloat	native;
	char		b[4];
	float		f;
} float_t;

/*
 * Sign extend the low order byte of one
 * integer to form a 32-bit value that's
 * placed in the second.
 */
#if defined(UCHARS) && !defined(SIGNEXTEND)
/* there may be a better way on some machines... */
#define	SIGNEXTEND(a,b)	{ if (((b) = (a)) & 0x80) (b) |= 0xffffff00; }
#else
#if !defined(SIGNEXTEND)
#define	SIGNEXTEND(a,b)	((b) = (char)(a))
#endif
#endif /* UCHARS */
#endif /* _MACHDEP_ */



/*
  
Modifications (since 77):

77
--
2/18/90 aozer	Created from Sam Leffler's freely distributable library

86
--
 6/9/90 aozer	Integrated changes for Sam Leffler's TIFF Library v2.2.

*/




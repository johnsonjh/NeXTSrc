/*
	tiffDir.m
	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#define NeXT_MOD

/*
 * This file contains code from the original tif_dir.c
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
 *
 * Directory Support Routines.
 *
 * NB: Beware of the varargs declarations for routines in
 *     this file.  The names and types of variables has been
 *     carefully chosen to make things work with compilers that
 *     are busted in one way or another (e.g. SGI/MIPS).
 */

#ifndef NeXT_MOD
#include "tiffio.h"
#else
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <libc.h>

#import "tiffPrivate.h"
#import "nextstd.h"
#endif

#ifndef TRUE
#define	TRUE	1
#define	FALSE	0
#endif

#define	howmany(x, y)	(((x)+((y)-1))/(y))
#define	roundup(x, y)	((((x)+((y)-1))/(y))*(y))

#define	FieldSet(fields, f)		(fields[f/32] & (1L<<(f&0x1f)))
#define	ResetFieldBit(fields, f)	(fields[f/32] &= ~(1L<<(f&0x1f)))
#ifdef NeXT_MOD
#define	SetFieldBit(fields, f)		fields[f/32] |= (1L<<(f&0x1f))
#endif

#define	TIFFExtractData(tif, type, v) \
    ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) >> (tif)->tif_typeshift[type]) & (tif)->tif_typemask[type] : \
	(v) & (tif)->tif_typemask[type])
#define	TIFFInsertData(tif, type, v) \
    ((tif)->tif_header.tiff_magic == TIFF_BIGENDIAN ? \
        ((v) & (tif)->tif_typemask[type]) << (tif)->tif_typeshift[type] : \
	(v) & (tif)->tif_typemask[type])

typedef	struct {
	u_short	field_tag;		/* field's tag */
	short	field_count;		/* write count (-1 for unknown) */
	TIFFDataType field_type;	/* type of associated data */
	u_short	field_bit;		/* bit in fieldsset bit vector */
	u_short	field_oktochange;	/* if true, can change while writing */
	char	*field_name;		/* ASCII name */
} TIFFFieldInfo;

/*
 * NB: This array is assumed to be sorted by tag.
 */
#ifdef NeXT_MOD
static const TIFFFieldInfo FieldInfo[] = {
#else
static TIFFFieldInfo FieldInfo[] = {
#endif
    { TIFFTAG_SUBFILETYPE,	 1, TIFF_LONG,	FIELD_SUBFILETYPE,
      TRUE,	"SubfileType" },
#ifdef SUBFILE_COMPAT
    { TIFFTAG_SUBFILETYPE,	 1, TIFF_SHORT,	FIELD_SUBFILETYPE,
      FALSE,	"SubfileType" },
#endif
    { TIFFTAG_OSUBFILETYPE,	 1, TIFF_SHORT,	FIELD_SUBFILETYPE,
      FALSE,	"OldSubfileType" },
    { TIFFTAG_IMAGEWIDTH,	 1, TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      FALSE,	"ImageWidth" },
    { TIFFTAG_IMAGEWIDTH,	 1, TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      FALSE,	"ImageWidth" },
    { TIFFTAG_IMAGELENGTH,	 1, TIFF_SHORT,	FIELD_IMAGEDIMENSIONS,
      TRUE,	"ImageLength" },
    { TIFFTAG_IMAGELENGTH,	 1, TIFF_LONG,	FIELD_IMAGEDIMENSIONS,
      TRUE,	"ImageLength" },
    { TIFFTAG_BITSPERSAMPLE,	-1, TIFF_SHORT,	FIELD_BITSPERSAMPLE,
      FALSE,	"BitsPerSample" },
    { TIFFTAG_COMPRESSION,	 1, TIFF_SHORT,	FIELD_COMPRESSION,
      FALSE,	"Compression" },
    { TIFFTAG_PHOTOMETRIC,	 1, TIFF_SHORT,	FIELD_PHOTOMETRIC,
      TRUE,	"PhotometricInterpretation" },
    { TIFFTAG_THRESHHOLDING,	 1, TIFF_SHORT,	FIELD_THRESHHOLDING,
      TRUE,	"Threshholding" },
    { TIFFTAG_CELLWIDTH,	 1, TIFF_SHORT,	(u_short)-1,
      TRUE,	"CellWidth" },
    { TIFFTAG_CELLLENGTH,	 1, TIFF_SHORT,	(u_short)-1,
      TRUE,	"CellLength" },
    { TIFFTAG_FILLORDER,	 1, TIFF_SHORT,	FIELD_FILLORDER,
      FALSE,	"FillOrder" },
    { TIFFTAG_DOCUMENTNAME,	-1, TIFF_ASCII,	FIELD_DOCUMENTNAME,
      TRUE,	"DocumentName" },
    { TIFFTAG_IMAGEDESCRIPTION,	-1, TIFF_ASCII,	FIELD_IMAGEDESCRIPTION,
      TRUE,	"ImageDescription" },
    { TIFFTAG_MAKE,		-1, TIFF_ASCII,	FIELD_MAKE,
      TRUE,	"Make" },
    { TIFFTAG_MODEL,		-1, TIFF_ASCII,	FIELD_MODEL,
      TRUE,	"Model" },
    { TIFFTAG_STRIPOFFSETS,	-1, TIFF_LONG,	FIELD_STRIPOFFSETS,
      FALSE,	"StripOffsets" },
    { TIFFTAG_STRIPOFFSETS,	-1, TIFF_SHORT,	FIELD_STRIPOFFSETS,
      FALSE,	"StripOffsets" },
    { TIFFTAG_ORIENTATION,	 1, TIFF_SHORT,	FIELD_ORIENTATION,
      TRUE,	"Orientation" },
    { TIFFTAG_SAMPLESPERPIXEL,	 1, TIFF_SHORT,	FIELD_SAMPLESPERPIXEL,
      FALSE,	"SamplesPerPixel" },
    { TIFFTAG_ROWSPERSTRIP,	 1, TIFF_LONG,	FIELD_ROWSPERSTRIP,
      FALSE,	"RowsPerStrip" },
    { TIFFTAG_ROWSPERSTRIP,	 1, TIFF_SHORT,	FIELD_ROWSPERSTRIP,
      FALSE,	"RowsPerStrip" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1, TIFF_LONG,	FIELD_STRIPBYTECOUNTS,
      FALSE,	"StripByteCounts" },
    { TIFFTAG_STRIPBYTECOUNTS,	-1, TIFF_SHORT,	FIELD_STRIPBYTECOUNTS,
      FALSE,	"StripByteCounts" },
    { TIFFTAG_MINSAMPLEVALUE,	-1, TIFF_SHORT,	FIELD_MINSAMPLEVALUE,
      TRUE,	"MinSampleValue" },
    { TIFFTAG_MAXSAMPLEVALUE,	-1, TIFF_SHORT,	FIELD_MAXSAMPLEVALUE,
      TRUE,	"MaxSampleValue" },
    { TIFFTAG_XRESOLUTION,	 1, TIFF_RATIONAL,FIELD_RESOLUTION,
      TRUE,	"XResolution" },
    { TIFFTAG_YRESOLUTION,	 1, TIFF_RATIONAL,FIELD_RESOLUTION,
      TRUE,	"YResolution" },
    { TIFFTAG_PLANARCONFIG,	 1, TIFF_SHORT,	FIELD_PLANARCONFIG,
      FALSE,	"PlanarConfiguration" },
    { TIFFTAG_PAGENAME,		-1, TIFF_ASCII,	FIELD_PAGENAME,
      TRUE,	"PageName" },
    { TIFFTAG_XPOSITION,	 1, TIFF_RATIONAL,FIELD_POSITION,
      TRUE,	"XPosition" },
    { TIFFTAG_YPOSITION,	 1, TIFF_RATIONAL,FIELD_POSITION,
      TRUE,	"YPosition" },
    { TIFFTAG_FREEOFFSETS,	-1, TIFF_LONG,	(u_short)-1,
      FALSE,	"FreeOffsets" },
    { TIFFTAG_FREEBYTECOUNTS,	-1, TIFF_LONG,	(u_short)-1,
      FALSE,	"FreeByteCounts" },
    { TIFFTAG_GRAYRESPONSEUNIT,	 1, TIFF_SHORT,	FIELD_GRAYRESPONSEUNIT,
      TRUE,	"GrayResponseUnit" },
    { TIFFTAG_GRAYRESPONSECURVE,-1, TIFF_SHORT,	FIELD_GRAYRESPONSECURVE,
      TRUE,	"GrayResponseCurve" },
    { TIFFTAG_GROUP3OPTIONS,	 1, TIFF_LONG,	FIELD_GROUP3OPTIONS,
      FALSE,	"Group3Options" },
    { TIFFTAG_GROUP4OPTIONS,	 1, TIFF_LONG,	FIELD_GROUP4OPTIONS,
      FALSE,	"Group4Options" },
    { TIFFTAG_RESOLUTIONUNIT,	 1, TIFF_SHORT,	FIELD_RESOLUTIONUNIT,
      TRUE,	"ResolutionUnit" },
    { TIFFTAG_PAGENUMBER,	 2, TIFF_SHORT,	FIELD_PAGENUMBER,
      TRUE,	"PageNumber" },
    { TIFFTAG_COLORRESPONSEUNIT, 1, TIFF_SHORT,	FIELD_COLORRESPONSEUNIT,
      TRUE,	"ColorResponseUnit" },
    { TIFFTAG_COLORRESPONSECURVE,-1,TIFF_SHORT,	FIELD_COLORRESPONSECURVE,
      TRUE,	"ColorResponseCurve" },
    { TIFFTAG_SOFTWARE,		-1, TIFF_ASCII,	FIELD_SOFTWARE,
      TRUE,	"Software" },
    { TIFFTAG_DATETIME,		-1, TIFF_ASCII,	FIELD_DATETIME,
      TRUE,	"DateTime" },
    { TIFFTAG_ARTIST,		-1, TIFF_ASCII,	FIELD_ARTIST,
      TRUE,	"Artist" },
    { TIFFTAG_HOSTCOMPUTER,	-1, TIFF_ASCII,	FIELD_HOSTCOMPUTER,
      TRUE,	"HostComputer" },
    { TIFFTAG_PREDICTOR,	 1, TIFF_SHORT,	FIELD_PREDICTOR,
      FALSE,	"Predictor" },
    { TIFFTAG_WHITEPOINT,	 2, TIFF_RATIONAL,(u_short)-1,
      TRUE,	"WhitePoint" },
    { TIFFTAG_PRIMARYCHROMATICITIES,6, TIFF_RATIONAL,(u_short)-1,
      TRUE,	"PrimaryChromaticities" },
    { TIFFTAG_COLORMAP,		 1, TIFF_SHORT,	FIELD_COLORMAP,
      TRUE,	"ColorMap" },
#ifdef NeXT_MOD
    { TIFFTAG_TILEWIDTH,	 1, TIFF_SHORT,	FIELD_TILEWIDTH,
      FALSE,	"TileWidth" },
    { TIFFTAG_TILEWIDTH,	 1, TIFF_LONG,	FIELD_TILEWIDTH,
      FALSE,	"TileWidth" },
    { TIFFTAG_TILELENGTH,	 1, TIFF_SHORT,	FIELD_TILELENGTH,
      TRUE,	"TileLength" },
    { TIFFTAG_TILELENGTH,	 1, TIFF_LONG,	FIELD_TILELENGTH,
      TRUE,	"TileLength" },
    { TIFFTAG_TILEOFFSETS,	-1, TIFF_LONG,	FIELD_TILEOFFSETS,
      FALSE,	"TileOffsets" },
    { TIFFTAG_TILEOFFSETS,	-1, TIFF_SHORT,	FIELD_TILEOFFSETS,
      FALSE,	"TileOffsets" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, TIFF_LONG,	FIELD_TILEBYTECOUNTS,
      FALSE,	"TileByteCounts" },
    { TIFFTAG_TILEBYTECOUNTS,	-1, TIFF_SHORT,	FIELD_TILEBYTECOUNTS,
      FALSE,	"TileByteCounts" },
#endif
    { TIFFTAG_BADFAXLINES,	 1, TIFF_LONG,	FIELD_BADFAXLINES,
      TRUE,	"BadFaxLines" },
    { TIFFTAG_BADFAXLINES,	 1, TIFF_SHORT,	FIELD_BADFAXLINES,
      TRUE,	"BadFaxLines" },
    { TIFFTAG_CLEANFAXDATA,	 1, TIFF_SHORT,	FIELD_CLEANFAXDATA,
      TRUE,	"CleanFaxData" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES,1, TIFF_LONG,FIELD_BADFAXRUN,
      TRUE,	"ConsecutiveBadFaxLines" },
    { TIFFTAG_CONSECUTIVEBADFAXLINES,1, TIFF_SHORT,FIELD_BADFAXRUN,
      TRUE,	"ConsecutiveBadFaxLines" },
/* begin nonstandard tags */
    { TIFFTAG_MATTEING,		 1, TIFF_SHORT,	FIELD_MATTEING,
      FALSE,	"Matteing" },
#ifdef NeXT_MOD
    { TIFFTAG_JPEGMODE,		 1, TIFF_SHORT,	FIELD_JPEGMODE,
      TRUE,	"JPEG Mode" },
    { TIFFTAG_JPEGQFACTOR,	 1, TIFF_SHORT,	FIELD_JPEGQFACTOR,
      TRUE,	"JPEG Q Factor" },
    { TIFFTAG_JPEGQMATRIXPRECISION, 1, TIFF_SHORT, FIELD_JPEGQMATRIXPRECISION,
      TRUE,	"JPEG Q Matrix Precision" },
    { TIFFTAG_JPEGQMATRIXDATA,	 -1, TIFF_LONG,	FIELD_JPEGQMATRIXDATA,
      TRUE,	"JPEG Q Matrix Data" },
    { TIFFTAG_JPEGDCTABLES,	 -1, TIFF_LONG,	FIELD_JPEGDCTABLES,
      TRUE,	"JPEG DC Tables" },
    { TIFFTAG_JPEGACTABLES,	 -1, TIFF_LONG,	FIELD_JPEGACTABLES,
      TRUE,	"JPEG AC Tables" },
    { TIFFTAG_NEWCOMPONENTSUBSAMPLE, -1, TIFF_SHORT, 
						FIELD_NEWCOMPONENTSUBSAMPLE,
      TRUE,	"New Component Subsample" },
#endif
};
#define	NFIELDINFO	(sizeof (FieldInfo) / sizeof (FieldInfo[0]))

#define	IGNORE	0		/* tag placeholder used below */

#ifdef NeXT_MOD
static const int datawidth[] = {
#else
static int datawidth[] = {
#endif
    1,	/* nothing */
    1,	/* TIFF_BYTE */
    1,	/* TIFF_ASCII */
    2,	/* TIFF_SHORT */
    4,	/* TIFF_LONG */
    8,	/* TIFF_RATIONAL */
};

#if USE_PROTOTYPES
#define	DECLARE1(f,t1,a1)		f(t1 a1)
#define	DECLARE2(f,t1,a1,t2,a2)		f(t1 a1, t2 a2)
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3)
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4)	f(t1 a1, t2 a2, t3 a3, t4 a4)
#define	DECLARE1V(f,t1,a1)		f(t1 a1 ...)
#define	DECLARE2V(f,t1,a1,t2,a2)	f(t1 a1, t2 a2, ...)
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3)	f(t1 a1, t2 a2, t3 a3, ...)

#ifdef NeXT_MOD
static void MissingRequired(TIFF *, char *);
static int TIFFFetchData(TIFF *, TIFFDirEntry *, char *);
static int TIFFFetchShorts(TIFF *, TIFFDirEntry *);
static int TIFFFetchLongs(TIFF *, TIFFDirEntry *);
static int TIFFFetchString(TIFF *, TIFFDirEntry *, char *);
static double TIFFFetchRational(TIFF *, TIFFDirEntry *);
static void TIFFUnpackShorts(TIFF *, u_short [], TIFFDirEntry *);
static int TIFFFetchPerSampleShorts(TIFF *, TIFFDirEntry *, long *);
static int TIFFWriteSampleLongs(TIFF* tif, u_short tag, TIFFDirEntry* dir, u_long* lp);
static int TIFFWriteShorts(TIFF* tif, u_short tag, TIFFDirEntry* dir, u_short* lp, int n);
static int TIFFFetchStripThing(TIFF *, TIFFDirEntry *, long, u_long **);
static const TIFFFieldInfo *FindFieldInfo(u_short, TIFFDataType);
static const TIFFFieldInfo *FieldWithTag(u_short);
static void TIFFGetField1(TIFFDirectory *, int, va_list);
#else
static	MissingRequired(TIFF *, char *);
static	TIFFFetchData(TIFF *, TIFFDirEntry *, char *);
static	TIFFFetchString(TIFF *, TIFFDirEntry *, char *);
static	double TIFFFetchRational(TIFF *, TIFFDirEntry *);
static	TIFFUnpackShorts(TIFF *, u_short [], TIFFDirEntry *);
static	TIFFFetchPerSampleShorts(TIFF *, TIFFDirEntry *, long *);
static	TIFFFetchStripThing(TIFF *, TIFFDirEntry *, long, u_long **);
static	TIFFFieldInfo *FindFieldInfo(u_short, TIFFDataType);
static	TIFFFieldInfo *FieldWithTag(u_short);
static	TIFFGetField1(TIFFDirectory *, int, va_list);
#endif
#else
#define	DECLARE1(f,t1,a1)		f(a1) t1 a1;
#define	DECLARE2(f,t1,a1,t2,a2)		f(a1,a2) t1 a1; t2 a2;
#define	DECLARE3(f,t1,a1,t2,a2,t3,a3)	f(a1, a2, a3) t1 a1; t2 a2; t3 a3;
#define	DECLARE4(f,t1,a1,t2,a2,t3,a3,t4,a4) \
	f(a1, a2, a3, a4) t1 a1; t2 a2; t3 a3; t4 a4;
#define	DECLARE1V(f,t1,a1) \
	f(a1, va_alist) t1 a1; va_dcl
#define	DECLARE2V(f,t1,a1,t2,a2) \
	f(a1, a2, va_alist) t1 a1; t2 a2; va_dcl
#define	DECLARE3V(f,t1,a1,t2,a2,t3,a3) \
	f(a1, a2, a3, va_alist) t1 a1; t2 a2; t3 a3; va_dcl

static	MissingRequired();
static	TIFFFetchData();
static	TIFFFetchString();
static	double TIFFFetchRational();
static	TIFFUnpackShorts();
static	TIFFFetchPerSampleShorts();
static	TIFFFetchStripThing();
static	TIFFFieldInfo *FindFieldInfo();
static	TIFFFieldInfo *FieldWithTag();
static	TIFFGetField1();
#endif

/*
 * Read the next TIFF directory from a file
 * and convert it to the internal format.
 * We read directories sequentially.
 */
#ifdef NeXT_MOD
int _NXTIFFReadDirectory(TIFF *tif)
#else
TIFFReadDirectory(tif)
	TIFF *tif;
#endif
{
	register TIFFDirEntry *dp;
	register int n;
	register TIFFDirectory *td = NULL;
	TIFFDirEntry *dir = NULL;
	long v;
#ifdef NeXT_MOD
	const
#endif
	TIFFFieldInfo *fip;
	u_short dircount;
	char *cp;
	int diroutoforderwarning = 0, ok;

	tif->tif_diroff = tif->tif_nextdiroff;
	if (tif->tif_diroff == 0)		/* no more directories */
		return (0);
	if (!SeekOK(tif->tif_fd, tif->tif_diroff)) {
		TIFFError(tif->tif_name, "Seek error accessing TIFF directory");
		return (0);
	}
	if (!ReadOK(tif->tif_fd, &dircount, sizeof (short))) {
		TIFFError(tif->tif_name, "Can not read TIFF directory count");
		return (0);
	}
	if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
		_NXTIFFSwabShort(&dircount);
#else
		TIFFSwabShort(&dircount);
#endif
	dir = (TIFFDirEntry *)malloc(dircount * sizeof (TIFFDirEntry));
	if (dir == NULL) {
		TIFFError(tif->tif_name, "No space to read TIFF directory");
		return (0);
	}
	if (!ReadOK(tif->tif_fd, dir, dircount * sizeof (TIFFDirEntry))) {
		TIFFError(tif->tif_name, "Can not read TIFF directory");
		goto bad;
	}
	/*
	 * Read offset to next directory for sequential scans.
	 */
	if (!ReadOK(tif->tif_fd, &tif->tif_nextdiroff, sizeof (long)))
		tif->tif_nextdiroff = 0;
	if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
		_NXTIFFSwabLong(&tif->tif_nextdiroff);
#else
		TIFFSwabLong(&tif->tif_nextdiroff);
#endif

	tif->tif_flags &= ~TIFF_BEENWRITING;	/* reset before new dir */
	/*
	 * Setup default value and then make a pass over
	 * the fields to check type and tag information,
	 * and to extract info required to size data
	 * structures.  A second pass is made afterwards
	 * to read in everthing not taken in the first pass.
	 */
	td = &tif->tif_dir;
#ifdef NeXT_MOD
	if ((tif->tif_freeprev) || (tif->tif_diroff != tif->tif_header.tiff_diroff)) {
		tif->tif_freeprev = 0;
#else
	if (tif->tif_diroff != tif->tif_header.tiff_diroff) {
#endif
		/* free any old stuff and reinit */
		TIFFFreeDirectory(tif);
	}
	TIFFDefaultDirectory(tif);
	/*
	 * Electronic Arts writes gray-scale TIFF files
	 * without a PlanarConfiguration directory entry.
	 * Thus we setup a default value here, even though
	 * the TIFF spec says there is no default value.
	 */
	TIFFSetField(tif, TIFFTAG_PLANARCONFIG, PLANARCONFIG_CONTIG);
	for (fip = FieldInfo, dp = dir, n = dircount; n > 0; n--, dp++) {
		if (tif->tif_flags & TIFF_SWAB) {
#ifdef NeXT_MOD
			_NXTIFFSwabArrayOfShort(&dp->tdir_tag, 2);
			_NXTIFFSwabArrayOfLong(&dp->tdir_count, 2);
#else
			TIFFSwabArrayOfShort(&dp->tdir_tag, 2);
			TIFFSwabArrayOfLong(&dp->tdir_count, 2);
#endif
		}
		/*
		 * Find the field information entry for this tag.
		 */
		/*
		 * Silicon Beach (at least) writes unordered
		 * directory tags (violating the spec).  Handle
		 * it here, but be obnoxious (maybe they'll
		 * fix it?).
		 */
		if (dp->tdir_tag < fip->field_tag) {
			if (!diroutoforderwarning) {
				TIFFWarning(tif->tif_name,
	"Invalid TIFF directory; tags are not sorted in ascending order");
				diroutoforderwarning = 1;
			}
			fip = FieldInfo;	/* O(n^2) */
		}
		while (fip < &FieldInfo[NFIELDINFO] &&
		    fip->field_tag < dp->tdir_tag)
			fip++;
		if (fip >= &FieldInfo[NFIELDINFO] ||
		    fip->field_tag != dp->tdir_tag) {
			TIFFWarning(tif->tif_name,
			    "ignoring unknown field with tag %d (0x%x)",
			    dp->tdir_tag,  dp->tdir_tag);
			dp->tdir_tag = IGNORE;
			fip = FieldInfo;	/* restart search */
			continue;
		}
		/*
		 * Null out old tags that we ignore.
		 */
		if (fip->field_bit == (u_short)-1) {
			dp->tdir_tag = IGNORE;
			continue;
		}
		/*
		 * Check data type.
		 */
		while ((TIFFDataType)dp->tdir_type != fip->field_type) {
			fip++;
			if (fip >= &FieldInfo[NFIELDINFO] ||
			    fip->field_tag != dp->tdir_tag) {
				TIFFError(tif->tif_name,
				    "Wrong data type %d for field \"%s\"",
				    dp->tdir_type, fip[-1].field_name);
				goto bad;
			}
		}
		switch (dp->tdir_tag) {
		case TIFFTAG_STRIPOFFSETS:
		case TIFFTAG_STRIPBYTECOUNTS:
			TIFFSetFieldBit(tif, fip->field_bit);
			break;
#ifdef NeXT_MOD
		case TIFFTAG_TILEOFFSETS:
		case TIFFTAG_TILEBYTECOUNTS:
			TIFFSetFieldBit(tif, fip->field_bit);
			break;
#endif
		case TIFFTAG_IMAGELENGTH:
#ifdef NeXT_MOD
		case TIFFTAG_IMAGEWIDTH:
		case TIFFTAG_TILELENGTH:
		case TIFFTAG_TILEWIDTH:
#endif
		case TIFFTAG_PLANARCONFIG:
		case TIFFTAG_SAMPLESPERPIXEL:
			if (!TIFFSetField(tif, dp->tdir_tag, (int)
			  TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset)))
				goto bad;
			break;
		case TIFFTAG_ROWSPERSTRIP:
			if (!TIFFSetField(tif, dp->tdir_tag, (long)
			  TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset)))
				goto bad;
			break;
		}
	}

	/*
	 * Allocate directory structure and setup defaults.
	 */
	if (!TIFFFieldSet(tif, FIELD_IMAGEDIMENSIONS)) {
		MissingRequired(tif, "ImageLength");
		goto bad;
	}
	if (!TIFFFieldSet(tif, FIELD_PLANARCONFIG)) {
		MissingRequired(tif, "PlanarConfiguration");
		goto bad;
	}
#ifdef NeXT_MOD
	if (TIFFFieldSet(tif, FIELD_STRIPOFFSETS)) {
	    td->td_stripsperimage = (td->td_rowsperstrip == 0xffffffff ?
		1 : howmany(td->td_imagelength, td->td_rowsperstrip));
	    td->td_nstrips = td->td_stripsperimage;
	    if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		    td->td_nstrips *= td->td_samplesperpixel;
	    td->td_usetiles = 0;
	} else if (TIFFFieldSet(tif, FIELD_TILEOFFSETS)) {
	    td->td_tilesacross = (td->td_imagewidth + td->td_tilewidth - 1) / td->td_tilewidth;
	    td->td_tilesdown = (td->td_imagelength + td->td_tilelength - 1) / td->td_tilelength;
	    if (td->td_tilesacross != 1) {
		TIFFError(NULL, "File contains %d tiles across; can't deal",
		    		td->td_tilesacross);
		goto bad;
	    }

	    td->td_stripsperimage = td->td_tilesdown;
	    td->td_rowsperstrip = td->td_tilelength;
	    td->td_nstrips = td->td_stripsperimage;
	    if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		    td->td_nstrips *= td->td_samplesperpixel;
	    td->td_usetiles = 1;
#ifdef AKDEBUG
	printf ("Tiles, %d across, %d down, %d per image, %d total\n",
	    td->td_tilesacross,
	    td->td_tilesdown,
	    td->td_stripsperimage,
	    td->td_nstrips);
#endif
	} else {
	    MissingRequired(tif, "StripOffsets");
	    goto bad;
	}
#else
	if (!TIFFFieldSet(tif, FIELD_STRIPOFFSETS)) {
		MissingRequired(tif, "StripOffsets");
		goto bad;
	}
	td->td_stripsperimage = (td->td_rowsperstrip == 0xffffffff ?
	    1 : howmany(td->td_imagelength, td->td_rowsperstrip));
	td->td_nstrips = td->td_stripsperimage;
	if (td->td_planarconfig == PLANARCONFIG_SEPARATE)
		td->td_nstrips *= td->td_samplesperpixel;
#endif

	/*
	 * Second pass: extract other information.
	 *
	 * Should do a better job of verifying values.
	 */
	for (dp = dir, n = dircount; n > 0; n--, dp++) {
		if (dp->tdir_tag == IGNORE)
			continue;
		if (dp->tdir_type == (short)TIFF_ASCII) {
			if (dp->tdir_count > 0) {
				cp = malloc(dp->tdir_count);
				if (cp == NULL)
					goto bad;
				ok = (TIFFFetchString(tif, dp, cp) &&
				    TIFFSetField(tif, dp->tdir_tag, cp));
				free(cp);
				if (!ok)
					goto bad;
			}
			continue;
		}
		if (dp->tdir_type == (short)TIFF_RATIONAL) {
			if (!TIFFSetField(tif, dp->tdir_tag,
			    TIFFFetchRational(tif, dp)))
				goto bad;
			continue;
		}
		switch (dp->tdir_tag) {
		case TIFFTAG_COMPRESSION:
			/*
			 * The 5.0 spec says the compression tag has
			 * one value, while earlier specs say it has
			 * one value per sample.  Because of this, we
			 * accept the tag if one value is supplied.
			 */
			if (dp->tdir_count == 1) {
				v = TIFFExtractData(tif,
				    dp->tdir_type, dp->tdir_offset);
				if (!TIFFSetField(tif,
				     TIFFTAG_COMPRESSION, (int)v))
					goto bad;
				break;
			}
			/* fall thru... */
		case TIFFTAG_MINSAMPLEVALUE:
		case TIFFTAG_MAXSAMPLEVALUE:
		case TIFFTAG_BITSPERSAMPLE:
			if (!TIFFFetchPerSampleShorts(tif, dp, &v) ||
			    !TIFFSetField(tif, dp->tdir_tag, (int)v))
				goto bad;
			break;
		case TIFFTAG_STRIPOFFSETS:
#ifdef NeXT_MOD
		case TIFFTAG_TILEOFFSETS:
#ifdef DEALWITHOLDBUGGYALPHAIMAGES
			if ((td->td_nstrips != dp->tdir_count) &&
				(td->td_photometric == 5) &&
				(dp->tdir_count == 1)) {
				TIFFError(NULL,
"Illegal StripOffset and StripByteCount counts. Will try to workaround the problem but please fix the file",
		    		td->td_nstrips);
				dp->tdir_count = td->td_nstrips;
			}			
#endif
#endif
			if (!TIFFFetchStripThing(tif, dp,
			    td->td_nstrips, &td->td_stripoffset))
				goto bad;
#ifdef NeXT_MOD
			/*
			 * We set the stripoffsets bit here but
			 * later change it to tileoffsets. Kludge.
			 */
#endif
			TIFFSetFieldBit(tif, FIELD_STRIPOFFSETS);
			break;
		case TIFFTAG_STRIPBYTECOUNTS:
#ifdef NeXT_MOD
		case TIFFTAG_TILEBYTECOUNTS:
#ifdef DEALWITHOLDBUGGYALPHAIMAGES
			if ((td->td_nstrips != dp->tdir_count) &&
				(td->td_photometric == 5) &&
				(dp->tdir_count == 0)) {
				dp->tdir_count = 2;
			}			
#endif
#endif
			if (!TIFFFetchStripThing(tif, dp,
			    td->td_nstrips, &td->td_stripbytecount))
				goto bad;
#ifdef NeXT_MOD
			/*
			 * We set the stripbytecounts bit here but
			 * later change it to tilebytecounts. Kludge.
			 */
#endif
			TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
			break;
#ifdef NeXT_MOD
		case TIFFTAG_JPEGQMATRIXDATA:
		case TIFFTAG_JPEGACTABLES:
		case TIFFTAG_JPEGDCTABLES:
			if (!TIFFFetchLongs(tif, dp)) goto bad;
			break;
		case TIFFTAG_NEWCOMPONENTSUBSAMPLE:
			if (!TIFFFetchShorts(tif, dp)) goto bad;
			break;
		case TIFFTAG_TILELENGTH:
		case TIFFTAG_TILEWIDTH:
		case TIFFTAG_IMAGEWIDTH:
#endif
		case TIFFTAG_IMAGELENGTH:
		case TIFFTAG_ROWSPERSTRIP:
		case TIFFTAG_SAMPLESPERPIXEL:
		case TIFFTAG_PLANARCONFIG:
			/* handled in first pass above */
			break;
		case TIFFTAG_GRAYRESPONSECURVE:
		case TIFFTAG_COLORRESPONSECURVE:
		case TIFFTAG_COLORMAP:
			v = (1L<<td->td_bitspersample) * sizeof (u_short);
			cp = malloc(dp->tdir_tag == TIFFTAG_GRAYRESPONSECURVE ? 
				v : 3*v);
			if (cp == NULL)
				goto bad;
			ok = (TIFFFetchData(tif, dp, cp) &&
			    TIFFSetField(tif, dp->tdir_tag, cp,cp+v,cp+2*v));
			free(cp);
			if (!ok)
				goto bad;
			break;
		case TIFFTAG_PAGENUMBER:
			TIFFUnpackShorts(tif, td->td_pagenumber, dp);
			TIFFSetFieldBit(tif, FIELD_PAGENUMBER);
			break;
/* BEGIN REV 4.0 COMPATIBILITY */
		case TIFFTAG_OSUBFILETYPE:
			v = 0;
			switch (TIFFExtractData(tif, dp->tdir_type,
			    dp->tdir_offset)) {
			case OFILETYPE_REDUCEDIMAGE:
				v = FILETYPE_REDUCEDIMAGE;
				break;
			case OFILETYPE_PAGE:
				v = FILETYPE_PAGE;
				break;
			}
			if (!TIFFSetField(tif, dp->tdir_tag, (int)v))
				goto bad;
			break;
/* END REV 4.0 COMPATIBILITY */
		default:
			if (!TIFFSetField(tif, dp->tdir_tag, (int)
			  TIFFExtractData(tif, dp->tdir_type, dp->tdir_offset)))
				goto bad;
			break;
		}
	}

	if (!TIFFFieldSet(tif, FIELD_STRIPBYTECOUNTS)) {
		int space;
		/*
		 * Some manufacturers violate the spec by not giving
		 * the size of the strips.  In this case, assume there
		 * is one uncompressed strip of data.
		 */
		if (td->td_nstrips > 1) {
			MissingRequired(tif, "StripByteCounts");
			goto bad;
		}
		TIFFWarning(tif->tif_name,
"TIFF directory is missing required \"%s\" field, calculating from imagelength",
		    FieldWithTag(TIFFTAG_STRIPBYTECOUNTS)->field_name);
		td->td_stripbytecount = (u_long *)malloc(sizeof (u_long));
		space = sizeof (TIFFHeader)
		    + sizeof (short)
		    + (dircount * sizeof (TIFFDirEntry))
		    + sizeof (long);
		/* calculate amount of space used by indirect values */
		for (dp = dir, n = dircount; n > 0; n--, dp++) {
			int cc = dp->tdir_count * datawidth[dp->tdir_type];
			if (cc > sizeof (long))
				space += cc;
		}
#ifdef NeXT_MOD
		{
			int streamSize, curLoc;
			curLoc = NXTell(tif->tif_fd);
			NXSeek(tif->tif_fd, 0, NX_FROMEND);
			streamSize = NXTell(tif->tif_fd);
			NXSeek(tif->tif_fd, curLoc, NX_FROMSTART);
			td->td_stripbytecount[0] = streamSize - space;
	    }	
#else
		td->td_stripbytecount[0] = TIFFGetFileSize(tif->tif_fd) - space;
#endif
		TIFFSetFieldBit(tif, FIELD_STRIPBYTECOUNTS);
		if (!TIFFFieldSet(tif, FIELD_ROWSPERSTRIP))
			td->td_rowsperstrip = td->td_imagelength;
	}
	if (dir)
		free((char *)dir);
	if (!TIFFFieldSet(tif, FIELD_MAXSAMPLEVALUE))
		td->td_maxsamplevalue = (1L<<td->td_bitspersample)-1;
	/*
	 * Setup default compression scheme.
	 */
	if (!TIFFFieldSet(tif, FIELD_COMPRESSION))
		TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	/*
	 * Reinitialize i/o since we are starting on a new directory.
	 */
	tif->tif_row = -1;
	tif->tif_curstrip = -1;
	tif->tif_scanlinesize = TIFFScanlineSize(tif);
#ifdef NeXT_MOD
	/* Take care of old images with photometric 5 & images w/out matte */
	if (((td->td_photometric == 5) && (td->td_samplesperpixel == 2)) ||
	    ((td->td_photometric == 2) && (td->td_samplesperpixel == 4)) ||
	    ((td->td_photometric < 2)  && (td->td_samplesperpixel == 2))) {  
	    if (td->td_photometric == 5) td->td_photometric = 1;
	    td->td_matteing = 1;
	}
	/* Take care of JPEG images which claim they're YUV when they aren't */
	if ((td->td_photometric == 6) && (td->td_compression == COMPRESSION_JPEG)) {
	    td->td_photometric = 2;
	}
#endif
	return (1);
bad:
	if (dir)
		free((char *)dir);
	return (0);
}

static
#ifdef NeXT_MOD
void
#endif
MissingRequired(tif, tagname)
	TIFF *tif;
	char *tagname;
{
	TIFFError(tif->tif_name,
	    "TIFF directory is missing required \"%s\" field", tagname);
}

/*
 * Fetch a contiguous directory item.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFFetchData(tif, dir, cp)
	TIFF *tif;
	TIFFDirEntry *dir;
	char *cp;
{
	int cc, w;

	w = datawidth[dir->tdir_type];
	cc = dir->tdir_count * w;
	if (SeekOK(tif->tif_fd, dir->tdir_offset) &&
	    ReadOK(tif->tif_fd, cp, cc)) {
		if (tif->tif_flags & TIFF_SWAB) {
			switch (dir->tdir_type) {
#ifdef NeXT_MOD
			case TIFF_SHORT:
				_NXTIFFSwabArrayOfShort((unsigned short *)cp, 
							dir->tdir_count);
				break;
			case TIFF_LONG:
				_NXTIFFSwabArrayOfLong((unsigned long *)cp, 
							dir->tdir_count);
				break;
			case TIFF_RATIONAL:
				_NXTIFFSwabArrayOfLong((unsigned long *)cp, 
							2*dir->tdir_count);
				break;
#else
			case TIFF_SHORT:
				TIFFSwabArrayOfShort(cp, dir->tdir_count);
				break;
			case TIFF_LONG:
				TIFFSwabArrayOfLong(cp, dir->tdir_count);
				break;
			case TIFF_RATIONAL:
				TIFFSwabArrayOfLong(cp, 2*dir->tdir_count);
				break;
#endif
			}
		}
		return (cc);
	}
	TIFFError(tif->tif_name, "Error fetching data for field \"%s\"",
	    FieldWithTag(dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Fetch an ASCII item from the file.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFFetchString(tif, dir, cp)
	TIFF *tif;
	TIFFDirEntry *dir;
	char *cp;
{
	if (dir->tdir_count <= 4) {
		u_long l = dir->tdir_offset;
		if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
			_NXTIFFSwabLong(&l);
#else
			TIFFSwabLong(&l);
#endif
		bcopy(&l, cp, dir->tdir_count);
		return (1);
	}
	return (TIFFFetchData(tif, dir, cp));
}

/*
 * Fetch a rational item from the file
 * at offset off.  We return the value
 * as floating point number.
 */
static double
TIFFFetchRational(tif, dir)
	TIFF *tif;
	TIFFDirEntry *dir;
{
	long l[2];

	if (!TIFFFetchData(tif, dir, (char *)l))
		return (1.);
	if (l[1] == 0) {
		TIFFError(tif->tif_name, "%s: Rational with zero denominator",
		    FieldWithTag(dir->tdir_tag)->field_name);
		return (1.);
	}
	return ((double)l[0] / (double)l[1]);
}

static
#ifdef NeXT_MOD
void
#endif
TIFFUnpackShorts(tif, v, dir)
	TIFF *tif;
	u_short v[];
	TIFFDirEntry *dir;
{
	if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
		v[0] = dir->tdir_offset >> 16;
		v[1] = dir->tdir_offset & 0xffff;
	} else {
		v[0] = dir->tdir_offset & 0xffff;
		v[1] = dir->tdir_offset >> 16;
	}
}

#ifdef NeXT_MOD
static int TIFFFetchShorts(TIFF *tif, TIFFDirEntry *dir)
{
    u_short *v = malloc(sizeof(u_long) * dir->tdir_count);
    int result = 1;

    if (dir->tdir_count == 1) {
	*v = TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
    } else if (dir->tdir_count == 2) {
	TIFFUnpackShorts(tif, v, dir);
    } else {
	result = TIFFFetchData(tif, dir, (char *)v);
    }
    if (result) {
	TIFFSetField(tif, dir->tdir_tag, v);
    }
    free(v);
    return result;
}

static int TIFFFetchLongs(TIFF *tif, TIFFDirEntry *dir)
{
    u_long *v = malloc(sizeof(u_long) * dir->tdir_count);
    int result = 1;

    if (dir->tdir_count == 1) {
	*v = TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
    } else {
	result = TIFFFetchData(tif, dir, (char *)v);
    }
    if (result) {
	TIFFSetField(tif, dir->tdir_tag, v);
    }
    free(v);
    return result;
}
#endif

static
#ifdef NeXT_MOD
int
#endif
TIFFFetchPerSampleShorts(tif, dir, pl)
	TIFF *tif;
	TIFFDirEntry *dir;
	long *pl;
{
	u_short v[4];
	int i;

	switch (dir->tdir_count) {
	case 1:
		*pl = TIFFExtractData(tif, dir->tdir_type, dir->tdir_offset);
		return (1);		/* XXX should check samplesperpixel */
	case 2:
		TIFFUnpackShorts(tif, v, dir);
		break;
	default:
		if (!TIFFFetchData(tif, dir, (char *)v))
			return (0);
		break;
	}
#ifdef NeXT_MOD
#ifndef ACCEPTWRONGNUMBEROFBPSVALUES
	if (tif->tif_dir.td_samplesperpixel != dir->tdir_count) {
		TIFFError(tif->tif_name, "Incorrect count %d for field \"%s\"",
		    dir->tdir_count, FieldWithTag(dir->tdir_tag)->field_name);
		return (0);
	}
#endif
#else
	if (tif->tif_dir.td_samplesperpixel != dir->tdir_count) {
		TIFFError(tif->tif_name, "Incorrect count %d for field \"%s\"",
		    dir->tdir_count, FieldWithTag(dir->tdir_tag)->field_name);
		return (0);
	}
#endif
	for (i = 1; i < dir->tdir_count; i++)
		if (v[i] != v[0]) {
			TIFFError(tif->tif_name,
	"Cannot handle different per-sample values for field \"%s\"",
			     FieldWithTag(dir->tdir_tag)->field_name);
			return (0);
		}
	*pl = v[0];
	return (1);
}


static
#ifdef NeXT_MOD
int
#endif
TIFFFetchStripThing(tif, dir, nstrips, lpp)
	TIFF *tif;
	TIFFDirEntry *dir;
	long nstrips;
	u_long **lpp;
{
	register u_long *lp;
	int status;

	if (nstrips != dir->tdir_count) {
		TIFFError(tif->tif_name,
		    "Count mismatch for field \"%s\"; expecting %d, got %d",
		    FieldWithTag(dir->tdir_tag)->field_name, nstrips,
		    dir->tdir_count);
		return (0);
	}
	/*
	 * Allocate space for strip information.
	 */
	if (*lpp == NULL &&
	    (*lpp = (u_long *)malloc(nstrips * sizeof (u_long))) == NULL) {
		TIFFError(tif->tif_name, "No space for \"%s\" array",
		    FieldWithTag(dir->tdir_tag)->field_name);
		return (0);
	}
	lp = *lpp;
	status = 1;
	if (dir->tdir_type == (int)TIFF_SHORT) {
		/*
		 * Handle short->long expansion.
		 */
		if (dir->tdir_count > 2) {
			char *dp =
			    malloc(dir->tdir_count * datawidth[(int)TIFF_SHORT]);
			if (dp == NULL) {
				TIFFError(tif->tif_name,
				    "No memory to fetch field \"%s\"",
				    FieldWithTag(dir->tdir_tag)->field_name);
				return (0);
			}
			if (status = TIFFFetchData(tif, dir, dp)) {
				register u_short *wp = (u_short *)dp;
				while (nstrips-- > 0)
					*lp++ = *wp++;
			}
			free(dp);
		} else {
			/*
			 * Extract data from offset field.
			 */
			if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
				*lp++ = dir->tdir_offset >> 16;
				*lp = dir->tdir_offset & 0xffff;
			} else {
				*lp++ = dir->tdir_offset & 0xffff;
				*lp = dir->tdir_offset >> 16;
			}
		}
	} else {
		if (dir->tdir_count > 1)
			status = TIFFFetchData(tif, dir, (char *)lp);
		else
			*lp = dir->tdir_offset;
	}
	return (status);
}

#ifdef NeXT_MOD
static const TIFFFieldInfo *
#else
static TIFFFieldInfo *
#endif
DECLARE2(FindFieldInfo, u_short, tag, TIFFDataType, dt)
{
#ifdef NeXT_MOD
	register const TIFFFieldInfo *fip;
#else
	register TIFFFieldInfo *fip;
#endif

	/* NB: if table gets big, can use sorted search (e.g. binary search) */
	for (fip = FieldInfo; fip < &FieldInfo[NFIELDINFO]; fip++)
		if (fip->field_tag == tag &&
		    (dt == (TIFFDataType)0 || fip->field_type == dt))
			return (fip);
	return ((TIFFFieldInfo *)0);
}

#ifdef NeXT_MOD
static const TIFFFieldInfo *
#else
static TIFFFieldInfo *
#endif
DECLARE1(FieldWithTag, u_short, tag)
{
#ifdef NeXT_MOD
	const TIFFFieldInfo *fip = FindFieldInfo(tag, (TIFFDataType)0);
#else
	TIFFFieldInfo *fip = FindFieldInfo(tag, (TIFFDataType)0);
#endif
	if (fip)
		return (fip);
	TIFFError("FieldWithTag", "Internal error, unknown tag 0x%x", tag);
#ifdef NeXT_MOD
	NXLogError ("Internal TIFF error, unknown tag 0x%x", tag);
	return NULL;
#else
	exit(-1);
	/*NOTREACHED*/
#endif
}

#define	CleanupField(field, member) {		\
    if (TIFFFieldSet(tif, CAT(FIELD_,field))) {	\
	free((char *)td->member);		\
	td->member = 0;				\
    }						\
}
#ifdef NeXT_MOD
void
#endif
TIFFFreeDirectory(tif)
	TIFF *tif;
{
	register TIFFDirectory *td = &tif->tif_dir;

	CleanupField(GRAYRESPONSECURVE, td_grayresponsecurve);
	if (TIFFFieldSet(tif, FIELD_COLORRESPONSECURVE)) {
		free(td->td_redresponsecurve), td->td_redresponsecurve = 0;
		free(td->td_greenresponsecurve), td->td_greenresponsecurve = 0;
		free(td->td_blueresponsecurve), td->td_blueresponsecurve = 0;
	}
	if (TIFFFieldSet(tif, FIELD_COLORMAP)) {
		free(td->td_redcolormap), td->td_redcolormap = 0;
		free(td->td_greencolormap), td->td_greencolormap = 0;
		free(td->td_bluecolormap), td->td_bluecolormap = 0;
	}
	CleanupField(DOCUMENTNAME, td_documentname);
	CleanupField(ARTIST, td_artist);
	CleanupField(DATETIME, td_datetime);
	CleanupField(HOSTCOMPUTER, td_hostcomputer);
	CleanupField(IMAGEDESCRIPTION, td_imagedescription);
	CleanupField(MAKE, td_make);
	CleanupField(MODEL, td_model);
	CleanupField(SOFTWARE, td_software);
	CleanupField(PAGENAME, td_pagename);
	if  (td->td_stripoffset) {
		free((char *)td->td_stripoffset);
		td->td_stripoffset = NULL;
	}
	if  (td->td_stripbytecount) {
		free((char *)td->td_stripbytecount);
		td->td_stripbytecount = NULL;
	}
}
#undef CleanupField

/*
 * Setup a default directory structure.
 */
#ifdef NeXT_MOD
int _NXTIFFDefaultDirectory(TIFF *tif)
#else
TIFFDefaultDirectory(tif)
	TIFF *tif;
#endif
{
	register TIFFDirectory *td = &tif->tif_dir;

	bzero((char *)td, sizeof (*td));
	td->td_fillorder = FILLORDER_MSB2LSB;
	td->td_bitspersample = 1;
	td->td_threshholding = THRESHHOLD_BILEVEL;
	td->td_orientation = ORIENTATION_TOPLEFT;
	td->td_samplesperpixel = 1;
	td->td_predictor = 1;
	td->td_rowsperstrip = 0xffffffff;
	td->td_grayresponseunit = GRAYRESPONSEUNIT_100S;
	td->td_resolutionunit = RESUNIT_INCH;
	td->td_colorresponseunit = COLORRESPONSEUNIT_100S;
#ifdef NeXT_MOD
	td->td_usetiles = 0;
#endif
	(void) TIFFSetField(tif, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
	/*
	 * NB: The directory is marked dirty as a result of setting
	 * up the default compression scheme.  However, this really
	 * isn't correct -- we want TIFF_DIRTYDIRECT to be set only
	 * if the user does something.  We could just do the setup
	 * by hand, but it seems better to use the normal mechanism
	 * (i.e. TIFFSetField).
	 */
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;
#ifdef NeXT_MOD
	return 1;
#endif
}

static
#ifdef NeXT_MOD
void
#endif
setString(cpp, cp)
	char **cpp, *cp;
{
	if (*cpp)
		free(*cpp), *cpp = 0;
	if (cp) {
		int len = strlen(cp)+1;
		if (*cpp = malloc(len))
			bcopy(cp, *cpp, len);
	}
}

static
#ifdef NeXT_MOD
void
#endif
setCurve(wpp, wp, nw)
	u_short **wpp, *wp;
	long nw;
{
	if (*wpp)
		free((char *)*wpp), *wpp = 0;
	nw *= sizeof (u_short);
	if (wp && (*wpp = (u_short *)malloc(nw)))
		bcopy(wp, *wpp, nw);
}

/*
 * Record the value of a field in the
 * internal directory structure.  The
 * field will be written to the file
 * when/if the directory structure is
 * updated.
 */
/*VARARGS2*/
#ifdef NeXT_MOD
int
DECLARE2V(_NXTIFFSetField, TIFF*, tif, int, tag)
#else
DECLARE2V(TIFFSetField, TIFF*, tif, int, tag)
#endif
{
#ifdef NeXT_MOD
	char module[] = "TIFFSetField";
#else
	static char module[] = "TIFFSetField";
#endif
	TIFFDirectory *td = &tif->tif_dir;
	va_list ap;
	long v;
	int field = -1, status = 1;
#ifdef NeXT_MOD
	int cnt;
	u_long *lp;
	u_short *sp;
#endif

	if (tag != TIFFTAG_IMAGELENGTH && (tif->tif_flags & TIFF_BEENWRITING)) {
#ifdef NeXT_MOD
	        const
#endif
		TIFFFieldInfo *fip = FindFieldInfo(tag, (TIFFDataType)0);

		/*
		 * Consult info table to see if tag can be changed
		 * after we've started writing.  We only allow changes
		 * to those tags that don't/shouldn't affect the
		 * compression and/or format of the data.
		 */
		if (fip && !fip->field_oktochange) {
			TIFFError(module,
			    "%s: Cannot modify tag \"%s\" while writing",
			    tif->tif_name, fip->field_name);
			return (0);
		}
	}
	VA_START(ap, tag);
	switch (tag) {
	case TIFFTAG_SUBFILETYPE:
		td->td_subfiletype = va_arg(ap, int);
		field = FIELD_SUBFILETYPE;
		break;
	case TIFFTAG_IMAGEWIDTH:
		td->td_imagewidth = va_arg(ap, int);
		field = FIELD_IMAGEDIMENSIONS;
		break;
	case TIFFTAG_IMAGELENGTH:
		td->td_imagelength = va_arg(ap, int);
		field = FIELD_IMAGEDIMENSIONS;
		break;
#ifdef NeXT_MOD
	case TIFFTAG_TILEWIDTH:
		td->td_tilewidth = va_arg(ap, int);
		field = FIELD_TILEWIDTH;
		break;
	case TIFFTAG_TILELENGTH:
		td->td_tilelength = va_arg(ap, int);
		field = FIELD_TILELENGTH;
		break;
#endif
	case TIFFTAG_BITSPERSAMPLE:
		td->td_bitspersample = va_arg(ap, int);
		field = FIELD_BITSPERSAMPLE;
		break;
	case TIFFTAG_COMPRESSION:
		v = va_arg(ap, int) & 0xffff;
		/*
		 * If we're changing the compression scheme,
		 * the notify the previous module so that it
		 * can cleanup any state it's setup.
		 */
		if (TIFFFieldSet(tif, FIELD_COMPRESSION)) {
			if (td->td_compression == v)
				break;
			if (tif->tif_cleanup)
				(*tif->tif_cleanup)(tif);
		}
		/*
		 * Setup new compression routine state.
		 */
#ifdef NeXT_MOD
		if (status = _NXTIFFSetCompressionScheme(tif, v)) {
#else
		if (status = TIFFSetCompressionScheme(tif, v)) {
#endif
			td->td_compression = v;
			field = FIELD_COMPRESSION;
		}
		break;
	case TIFFTAG_PHOTOMETRIC:
		td->td_photometric = va_arg(ap, int);
		field = FIELD_PHOTOMETRIC;
		break;
	case TIFFTAG_THRESHHOLDING:
		td->td_threshholding = va_arg(ap, int);
		field = FIELD_THRESHHOLDING;
		break;
	case TIFFTAG_FILLORDER:
		td->td_fillorder = va_arg(ap, int);
		field = FIELD_FILLORDER;
		break;
	case TIFFTAG_DOCUMENTNAME:
		setString(&td->td_documentname, va_arg(ap, char *));
		field = FIELD_DOCUMENTNAME;
		break;
	case TIFFTAG_ARTIST:
		setString(&td->td_artist, va_arg(ap, char *));
		field = FIELD_ARTIST;
		break;
	case TIFFTAG_DATETIME:
		setString(&td->td_datetime, va_arg(ap, char *));
		field = FIELD_DATETIME;
		break;
	case TIFFTAG_HOSTCOMPUTER:
		setString(&td->td_hostcomputer, va_arg(ap, char *));
		field = FIELD_HOSTCOMPUTER;
		break;
	case TIFFTAG_IMAGEDESCRIPTION:
		setString(&td->td_imagedescription, va_arg(ap, char *));
		field = FIELD_IMAGEDESCRIPTION;
		break;
	case TIFFTAG_MAKE:
		setString(&td->td_make, va_arg(ap, char *));
		field = FIELD_MAKE;
		break;
	case TIFFTAG_MODEL:
		setString(&td->td_model, va_arg(ap, char *));
		field = FIELD_MODEL;
		break;
	case TIFFTAG_SOFTWARE:
		setString(&td->td_software, va_arg(ap, char *));
		field = FIELD_SOFTWARE;
		break;
	case TIFFTAG_ORIENTATION:
		td->td_orientation = va_arg(ap, int);
		field = FIELD_ORIENTATION;
		break;
	case TIFFTAG_SAMPLESPERPIXEL:
		v = va_arg(ap, int);
		if (v == 0)
			goto badvalue;
		if (v > 4) {
			TIFFError(tif->tif_name,
			    "Cannot handle %d-channel data", v); 
			goto bad;
		}
		td->td_samplesperpixel = v;
		field = FIELD_SAMPLESPERPIXEL;
		break;
	case TIFFTAG_ROWSPERSTRIP:
		v = va_arg(ap, long);
		if (v == 0)
			goto badvalue;
		td->td_rowsperstrip = v;
		field = FIELD_ROWSPERSTRIP;
		break;
	case TIFFTAG_MINSAMPLEVALUE:
		td->td_minsamplevalue = va_arg(ap, int) & 0xffff;
		field = FIELD_MINSAMPLEVALUE;
		break;
	case TIFFTAG_MAXSAMPLEVALUE:
		td->td_maxsamplevalue = va_arg(ap, int) & 0xffff;
		field = FIELD_MAXSAMPLEVALUE;
		break;
	case TIFFTAG_XRESOLUTION:
		td->td_xresolution = va_arg(ap, dblparam_t);
		field = FIELD_RESOLUTION;
		break;
	case TIFFTAG_YRESOLUTION:
		td->td_yresolution = va_arg(ap, dblparam_t);
		field = FIELD_RESOLUTION;
		break;
	case TIFFTAG_PLANARCONFIG:
		v = va_arg(ap, int);
		td->td_planarconfig = v;
		field = FIELD_PLANARCONFIG;
		break;
	case TIFFTAG_PAGENAME:
		setString(&td->td_pagename, va_arg(ap, char *));
		field = FIELD_PAGENAME;
		break;
	case TIFFTAG_XPOSITION:
		td->td_xposition = va_arg(ap, dblparam_t);
		field = FIELD_POSITION;
		break;
	case TIFFTAG_YPOSITION:
		td->td_yposition = va_arg(ap, dblparam_t);
		field = FIELD_POSITION;
		break;
	case TIFFTAG_GRAYRESPONSEUNIT:
		td->td_grayresponseunit = va_arg(ap, int);
		field = FIELD_GRAYRESPONSEUNIT;
		break;
	case TIFFTAG_GRAYRESPONSECURVE:
		setCurve(&td->td_grayresponsecurve,
		    va_arg(ap, u_short *), 1L<<td->td_bitspersample);
		field = FIELD_GRAYRESPONSECURVE;
		break;
	case TIFFTAG_GROUP3OPTIONS:
		td->td_group3options = va_arg(ap, long);
		field = FIELD_GROUP3OPTIONS;
		break;
	case TIFFTAG_GROUP4OPTIONS:
		td->td_group4options = va_arg(ap, long);
		field = FIELD_GROUP4OPTIONS;
		break;
	case TIFFTAG_RESOLUTIONUNIT:
		td->td_resolutionunit = va_arg(ap, int);
		field = FIELD_RESOLUTIONUNIT;
		break;
	case TIFFTAG_PAGENUMBER:
		td->td_pagenumber[0] = va_arg(ap, int);
		td->td_pagenumber[1] = va_arg(ap, int);
		field = FIELD_PAGENUMBER;
		break;
	case TIFFTAG_COLORRESPONSEUNIT:
		td->td_colorresponseunit = va_arg(ap, int);
		field = FIELD_COLORRESPONSEUNIT;
		break;
	case TIFFTAG_COLORRESPONSECURVE:
		v = 1L<<td->td_bitspersample;
		setCurve(&td->td_redresponsecurve, va_arg(ap, u_short *), v);
		setCurve(&td->td_greenresponsecurve, va_arg(ap, u_short *), v);
		setCurve(&td->td_blueresponsecurve, va_arg(ap, u_short *), v);
		field = FIELD_COLORRESPONSECURVE;
		break;
	case TIFFTAG_COLORMAP:
		v = 1L<<td->td_bitspersample;
		setCurve(&td->td_redcolormap, va_arg(ap, u_short *), v);
		setCurve(&td->td_greencolormap, va_arg(ap, u_short *), v);
		setCurve(&td->td_bluecolormap, va_arg(ap, u_short *), v);
		field = FIELD_COLORMAP;
		break;
	case TIFFTAG_PREDICTOR:
		td->td_predictor = va_arg(ap, int);
		field = FIELD_PREDICTOR;
		break;
	case TIFFTAG_MATTEING:
		td->td_matteing = va_arg(ap, int);
		field = FIELD_MATTEING;
		break;
#ifdef NeXT_MOD
	case TIFFTAG_JPEGMODE:
		td->td_jpegmode = va_arg(ap, int);
		field = FIELD_JPEGMODE;
		break;
	case TIFFTAG_JPEGQFACTOR:
		td->td_jpegqfactor = va_arg(ap, int);
		field = FIELD_JPEGQFACTOR;
		break;
	case TIFFTAG_JPEGQMATRIXPRECISION:
		td->td_jpegqmatrixprecision = va_arg(ap, int);
		field = FIELD_JPEGQMATRIXPRECISION;
		break;
	case TIFFTAG_JPEGQMATRIXDATA:
		lp = va_arg(ap, u_long *);
		for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		    td->td_jpegqmatrixdata[cnt] = *lp++;
		}
		field = FIELD_JPEGQMATRIXDATA;
		break;
	case TIFFTAG_JPEGACTABLES:
		lp = va_arg(ap, u_long *);
		for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		    td->td_jpegactables[cnt] = *lp++;
		}
		field = FIELD_JPEGACTABLES;
		break;
	case TIFFTAG_JPEGDCTABLES:
		lp = va_arg(ap, u_long *);
		for (cnt = 0; cnt < td->td_samplesperpixel; cnt++) {
		    td->td_jpegdctables[cnt] = *lp++;
		}
		field = FIELD_JPEGDCTABLES;
		break;
	case TIFFTAG_NEWCOMPONENTSUBSAMPLE:
		sp = va_arg(ap, u_short *);
		for (cnt = 0; cnt < td->td_samplesperpixel * 2; cnt++) {
		    td->td_newcomponentsubsample[cnt] = *sp++;
		}
		field = FIELD_NEWCOMPONENTSUBSAMPLE;
		break;
#endif
	case TIFFTAG_BADFAXLINES:
		td->td_badfaxlines = va_arg(ap, long);
		field = FIELD_BADFAXLINES;
		break;
	case TIFFTAG_CLEANFAXDATA:
		td->td_cleanfaxdata = va_arg(ap, int);
		field = FIELD_CLEANFAXDATA;
		break;
	case TIFFTAG_CONSECUTIVEBADFAXLINES:
		td->td_badfaxrun = va_arg(ap, int);
		field = FIELD_BADFAXRUN;
		break;
	}
	if (field >= 0) {
		TIFFSetFieldBit(tif, field);
		tif->tif_flags |= TIFF_DIRTYDIRECT;
	}
	va_end(ap);
	return (status);
badvalue:
	TIFFError(tif->tif_name, "%d: Bad value for \"%s\"", v,
	    FieldWithTag(tag)->field_name);
bad:
	va_end(ap);
	return (0);
}

/*
 * Return the value of a field in the
 * internal directory structure.
 */
/*VARARGS2*/
#ifdef NeXT_MOD
int
DECLARE2V(_NXTIFFGetField, TIFF*, tif, int, tag)
#else
DECLARE2V(TIFFGetField, TIFF*, tif, int, tag)
#endif
{
#ifdef NeXT_MOD
        const
#endif
	TIFFFieldInfo *fip = FindFieldInfo(tag, (TIFFDataType)0);

	if (!fip)
		TIFFError("TIFFGetField", "Unknown field, tag 0x%x", tag);
	else if (TIFFFieldSet(tif, fip->field_bit)) {
		va_list ap;

		VA_START(ap, tag);
		(void) TIFFGetField1(&tif->tif_dir, tag, ap);
		va_end(ap);
		return (1);
	}
	return (0);
}

static
#ifdef NeXT_MOD
void
#endif
TIFFGetField1(td, tag, ap)
	TIFFDirectory *td;
	int tag;
	va_list ap;
{

	switch (tag) {
	case TIFFTAG_SUBFILETYPE:
		*va_arg(ap, u_long *) = td->td_subfiletype;
		break;
	case TIFFTAG_IMAGEWIDTH:
		*va_arg(ap, u_short *) = td->td_imagewidth;
		break;
	case TIFFTAG_IMAGELENGTH:
		*va_arg(ap, u_short *) = td->td_imagelength;
		break;
	case TIFFTAG_BITSPERSAMPLE:
		*va_arg(ap, u_short *) = td->td_bitspersample;
		break;
	case TIFFTAG_COMPRESSION:
		*va_arg(ap, u_short *) = td->td_compression;
		break;
	case TIFFTAG_PHOTOMETRIC:
		*va_arg(ap, u_short *) = td->td_photometric;
		break;
	case TIFFTAG_THRESHHOLDING:
		*va_arg(ap, u_short *) = td->td_threshholding;
		break;
	case TIFFTAG_FILLORDER:
		*va_arg(ap, u_short *) = td->td_fillorder;
		break;
	case TIFFTAG_DOCUMENTNAME:
		*va_arg(ap, char **) = td->td_documentname;
		break;
	case TIFFTAG_ARTIST:
		*va_arg(ap, char **) = td->td_artist;
		break;
	case TIFFTAG_DATETIME:
		*va_arg(ap, char **) = td->td_datetime;
		break;
	case TIFFTAG_HOSTCOMPUTER:
		*va_arg(ap, char **) = td->td_hostcomputer;
		break;
	case TIFFTAG_IMAGEDESCRIPTION:
		*va_arg(ap, char **) = td->td_imagedescription;
		break;
	case TIFFTAG_MAKE:
		*va_arg(ap, char **) = td->td_make;
		break;
	case TIFFTAG_MODEL:
		*va_arg(ap, char **) = td->td_model;
		break;
	case TIFFTAG_SOFTWARE:
		*va_arg(ap, char **) = td->td_software;
		break;
	case TIFFTAG_ORIENTATION:
		*va_arg(ap, u_short *) = td->td_orientation;
		break;
	case TIFFTAG_SAMPLESPERPIXEL:
		*va_arg(ap, u_short *) = td->td_samplesperpixel;
		break;
	case TIFFTAG_ROWSPERSTRIP:
		*va_arg(ap, u_long *) = td->td_rowsperstrip;
		break;
	case TIFFTAG_MINSAMPLEVALUE:
		*va_arg(ap, u_short *) = td->td_minsamplevalue;
		break;
	case TIFFTAG_MAXSAMPLEVALUE:
		*va_arg(ap, u_short *) = td->td_maxsamplevalue;
		break;
	case TIFFTAG_XRESOLUTION:
		*va_arg(ap, float *) = td->td_xresolution;
		break;
	case TIFFTAG_YRESOLUTION:
		*va_arg(ap, float *) = td->td_yresolution;
		break;
	case TIFFTAG_PLANARCONFIG:
		*va_arg(ap, u_short *) = td->td_planarconfig;
		break;
	case TIFFTAG_XPOSITION:
		*va_arg(ap, float *) = td->td_xposition;
		break;
	case TIFFTAG_YPOSITION:
		*va_arg(ap, float *) = td->td_yposition;
		break;
	case TIFFTAG_PAGENAME:
		*va_arg(ap, char **) = td->td_pagename;
		break;
	case TIFFTAG_GRAYRESPONSEUNIT:
		*va_arg(ap, u_short *) = td->td_grayresponseunit;
		break;
	case TIFFTAG_GRAYRESPONSECURVE:
		*va_arg(ap, u_short **) = td->td_grayresponsecurve;
		break;
	case TIFFTAG_GROUP3OPTIONS:
		*va_arg(ap, u_long *) = td->td_group3options;
		break;
	case TIFFTAG_GROUP4OPTIONS:
		*va_arg(ap, u_long *) = td->td_group4options;
		break;
	case TIFFTAG_RESOLUTIONUNIT:
		*va_arg(ap, u_short *) = td->td_resolutionunit;
		break;
	case TIFFTAG_PAGENUMBER:
		*va_arg(ap, u_short *) = td->td_pagenumber[0];
		*va_arg(ap, u_short *) = td->td_pagenumber[1];
		break;
	case TIFFTAG_COLORRESPONSEUNIT:
		*va_arg(ap, u_short *) = td->td_colorresponseunit;
		break;
	case TIFFTAG_COLORRESPONSECURVE:
		*va_arg(ap, u_short **) = td->td_redresponsecurve;
		*va_arg(ap, u_short **) = td->td_greenresponsecurve;
		*va_arg(ap, u_short **) = td->td_blueresponsecurve;
		break;
	case TIFFTAG_COLORMAP:
		*va_arg(ap, u_short **) = td->td_redcolormap;
		*va_arg(ap, u_short **) = td->td_greencolormap;
		*va_arg(ap, u_short **) = td->td_bluecolormap;
		break;
	case TIFFTAG_PREDICTOR:
		*va_arg(ap, u_short *) = td->td_predictor;
		break;
	case TIFFTAG_STRIPOFFSETS:
		*va_arg(ap, u_long **) = td->td_stripoffset;
		break;
	case TIFFTAG_STRIPBYTECOUNTS:
		*va_arg(ap, u_long **) = td->td_stripbytecount;
		break;
	case TIFFTAG_MATTEING:
		*va_arg(ap, u_short *) = td->td_matteing;
		break;
#ifdef NeXT_MOD
	case TIFFTAG_JPEGMODE:
		*va_arg(ap, u_short *) = td->td_jpegmode;
		break;
	case TIFFTAG_JPEGQFACTOR:
		*va_arg(ap, u_short *) = td->td_jpegqfactor;
		break;
	case TIFFTAG_JPEGQMATRIXPRECISION:
		*va_arg(ap, u_short *) = td->td_jpegqmatrixprecision;
		break;
#endif
	case TIFFTAG_BADFAXLINES:
		*va_arg(ap, u_long *) = td->td_badfaxlines;
		break;
	case TIFFTAG_CLEANFAXDATA:
		*va_arg(ap, u_short *) = td->td_cleanfaxdata;
		break;
	case TIFFTAG_CONSECUTIVEBADFAXLINES:
		*va_arg(ap, u_long *) = td->td_badfaxrun;
		break;
	}
	va_end(ap);
}

/*
 * Internal interface to TIFFGetField...
 */
static
#ifdef NeXT_MOD
void
#endif
/*VARARGS2*/
DECLARE2V(TIFFgetfield, TIFFDirectory*, td, int, tag)
{
	va_list ap;

	VA_START(ap, tag);
	(void) TIFFGetField1(td, tag, ap);
	va_end(ap);
}

#if USE_PROTOTYPES
static	TIFFWriteRational(TIFF *, u_short, TIFFDirEntry *, float);
static	TIFFWritePerSampleShorts(TIFF *, u_short, TIFFDirEntry *, short);
static	TIFFWriteStripThing(TIFF *, u_short, TIFFDirEntry *, u_long *);
static	TIFFWriteData(TIFF *, TIFFDirEntry *, char *);
static	TIFFLinkDirectory(TIFF *);
#else
static	TIFFWriteRational();
static	TIFFWritePerSampleShorts();
static	TIFFWriteStripThing();
static	TIFFWriteData();
static	TIFFLinkDirectory();
#endif

/* shorthands for setting up and writing directory... */
#define	MakeShortDirent(tag, v) \
	dir->tdir_tag = tag; \
	dir->tdir_type = (short)TIFF_SHORT; \
	dir->tdir_count = 1; \
	dir->tdir_offset = TIFFInsertData(tif, (int)TIFF_SHORT, v); \
	dir++
#define	WriteRationalPair(tag1, v1, tag2, v2) \
	(TIFFWriteRational(tif, tag1, dir++, v1) && \
	 TIFFWriteRational(tif, tag2, dir++, v2))

#ifndef NeXT_MOD
static	long dataoff;
#endif
/*
 * Write the contents of the current directory
 * to the specified file.  This routine doesn't
 * handle overwriting a directory with auxiliary
 * storage that's been changed.
 */
#ifdef NeXT_MOD
int _NXTIFFWriteDirectory(TIFF *tif)
#else
TIFFWriteDirectory(tif)
	TIFF *tif;
#endif
{
	short dircount, v;
	int nfields, dirsize;
	char *data, *cp;
#ifdef NeXT_MOD
	const
#endif
	TIFFFieldInfo *fip;
	TIFFDirEntry *dir;
	TIFFDirectory *td;
	u_long b, off, fields[sizeof (td->td_fieldsset) / sizeof (u_long)];

	if (tif->tif_mode == O_RDONLY)
		return (1);
#ifdef NeXT_MOD
	if (tif->tif_rawcc > 0 && !_NXTIFFFlushData(tif)) {
#else
	if (tif->tif_rawcc > 0 && !TIFFFlushData(tif)) {
#endif
		TIFFError(tif->tif_name,
		    "Error flushing data before directory write");
		return (0);
	}
	/*
	 * Clear write state so that subsequent images with
	 * different characteristics get the right buffers
	 * setup for them.
	 */
	if (tif->tif_close)
		(*tif->tif_close)(tif);
	if (tif->tif_cleanup)
		(*tif->tif_cleanup)(tif);
	if (tif->tif_rawdata) {
		free(tif->tif_rawdata);
		tif->tif_rawdata = NULL;
		tif->tif_rawcc = 0;
	}
	tif->tif_flags &= ~(TIFF_BEENWRITING|TIFF_BUFFERSETUP);

	td = &tif->tif_dir;
	/*
	 * Size the directory so that we can calculate
	 * offsets for the data items that aren't kept
	 * in-place in each field.
	 */
	nfields = 0;
	for (b = 0; b <= FIELD_LAST; b++)
		if (TIFFFieldSet(tif, b))
			nfields += (b < FIELD_SUBFILETYPE ? 2 : 1);
	dirsize = nfields * sizeof (TIFFDirEntry);
	data = malloc(dirsize);
	if (data == NULL) {
		TIFFError(tif->tif_name,
		    "Cannot write directory, out of space");
		return (0);
	}
	/*
	 * Directory hasn't been placed yet, put
	 * it at the end of the file and link it
	 * into the existing directory structure.
	 */
	if (tif->tif_diroff == 0 && !TIFFLinkDirectory(tif))
		return (0);

#ifdef NeXT_MOD
	tif->tif_dataoff =
		tif->tif_diroff + sizeof (short) + dirsize + sizeof (long);
	if (tif->tif_dataoff & 1)
		tif->tif_dataoff++;
	(void) SeekOK(tif->tif_fd, tif->tif_dataoff);
#else
	dataoff = 
			tif->tif_diroff + sizeof (short) + dirsize + sizeof (long);
	if (dataoff & 1)
		dataoff++;
	(void) lseek(tif->tif_fd, dataoff, L_SET);
#endif
	dir = (TIFFDirEntry *)data;
	/*
	 * Setup external form of directory
	 * entries and write data items.
	 */
	bcopy(td->td_fieldsset, fields, sizeof (fields));
#ifdef NeXT_MOD
	if (td->td_usetiles) {
	    if (FieldSet(fields, FIELD_STRIPOFFSETS)) {	
		ResetFieldBit(fields, FIELD_STRIPOFFSETS);
		SetFieldBit(fields, FIELD_TILEOFFSETS);
	    }
	    if (FieldSet(fields, FIELD_STRIPBYTECOUNTS)) {
		ResetFieldBit(fields, FIELD_STRIPBYTECOUNTS);
		SetFieldBit(fields, FIELD_TILEBYTECOUNTS);
	    }
	}	
#endif
	for (fip = FieldInfo; fip < &FieldInfo[NFIELDINFO]; fip++) {
		if (fip->field_bit == (u_short)-1 ||
		    !FieldSet(fields, fip->field_bit))
			continue;
		if (fip->field_type == TIFF_ASCII) {
			TIFFgetfield(td, fip->field_tag, &cp);
			dir->tdir_tag = fip->field_tag;
			dir->tdir_type = (short)TIFF_ASCII;
			dir->tdir_count = strlen(cp) + 1;
			if (!TIFFWriteData(tif, dir++, cp))
				goto bad;
			ResetFieldBit(fields, fip->field_bit);
			continue;
		}
		switch (fip->field_bit) {
#ifdef NeXT_MOD
		case FIELD_TILEOFFSETS:
		case FIELD_TILEBYTECOUNTS:
			if (!TIFFWriteStripThing(tif,
			    fip->field_tag, dir++,
			    fip->field_bit == FIELD_TILEOFFSETS ?
			      td->td_stripoffset : td->td_stripbytecount))
				goto bad;
			break;
#endif
		case FIELD_STRIPOFFSETS:
		case FIELD_STRIPBYTECOUNTS:
			if (!TIFFWriteStripThing(tif,
			    fip->field_tag, dir++,
			    fip->field_bit == FIELD_STRIPOFFSETS ?
			      td->td_stripoffset : td->td_stripbytecount))
				goto bad;
			break;
		case FIELD_GRAYRESPONSECURVE:
			dir->tdir_tag = fip->field_tag;
			dir->tdir_type = (short)TIFF_SHORT;
			dir->tdir_count = 1L<<td->td_bitspersample;
#define	WRITE(x)	TIFFWriteData(tif, dir, (char*) x)
			if (!WRITE(td->td_grayresponsecurve))
				goto bad;
			dir++;
			break;
		case FIELD_COLORRESPONSECURVE:
		case FIELD_COLORMAP:
			dir->tdir_tag = fip->field_tag;
			dir->tdir_type = (short)TIFF_SHORT;
			/* XXX -- yech, fool TIFFWriteData */
			dir->tdir_count = 1L<<td->td_bitspersample;
#ifdef NeXT_MOD
			off = tif->tif_dataoff;
#else
			off = dataoff;
#endif
			if (fip->field_tag == TIFFTAG_COLORMAP) {
				if (!WRITE(td->td_redcolormap) ||
				    !WRITE(td->td_greencolormap) ||
				    !WRITE(td->td_bluecolormap))
					goto bad;
			} else {
				if (!WRITE(td->td_redresponsecurve) ||
				    !WRITE(td->td_greenresponsecurve) ||
				    !WRITE(td->td_blueresponsecurve))
					goto bad;
			}
#undef WRITE
			dir->tdir_count *= 3;
			dir->tdir_offset = off;
			dir++;
			break;
		case FIELD_IMAGEDIMENSIONS:
			MakeShortDirent(TIFFTAG_IMAGEWIDTH, td->td_imagewidth);
			MakeShortDirent(TIFFTAG_IMAGELENGTH,td->td_imagelength);
			break;
#ifdef NeXT_MOD
		// ??? We might not need these two! -Ali
		case FIELD_TILEWIDTH:
			MakeShortDirent(TIFFTAG_TILEWIDTH, td->td_tilewidth);
			break;
		case FIELD_TILELENGTH:
			MakeShortDirent(TIFFTAG_TILELENGTH,td->td_tilelength);
			break;
#endif
		case FIELD_POSITION:
			if (!WriteRationalPair(
			    TIFFTAG_XPOSITION, td->td_xposition,
			    TIFFTAG_YPOSITION, td->td_yposition))
				goto bad;
			break;
		case FIELD_RESOLUTION:
			if (!WriteRationalPair(
			    TIFFTAG_XRESOLUTION, td->td_xresolution,
			    TIFFTAG_YRESOLUTION, td->td_yresolution))
				goto bad;
			break;
#ifdef NeXT_MOD
		case FIELD_JPEGQMATRIXDATA:
			if (!TIFFWriteSampleLongs(tif, fip->field_tag,
			    dir++, td->td_jpegqmatrixdata))
				goto bad;
			break;
		case FIELD_JPEGDCTABLES:
			if (!TIFFWriteSampleLongs(tif, fip->field_tag,
			    dir++, td->td_jpegdctables))
				goto bad;
			break;
		case FIELD_JPEGACTABLES:
			if (!TIFFWriteSampleLongs(tif, fip->field_tag,
			    dir++, td->td_jpegactables))
				goto bad;
			break;
		case FIELD_NEWCOMPONENTSUBSAMPLE:
			if (!TIFFWriteShorts(tif, fip->field_tag,
			    dir++, td->td_newcomponentsubsample, 
			    td->td_samplesperpixel * 2))
				goto bad;
			break;
#endif
		case FIELD_BITSPERSAMPLE:
		case FIELD_MINSAMPLEVALUE:
		case FIELD_MAXSAMPLEVALUE:
			TIFFgetfield(td, fip->field_tag, &v);
			if (!TIFFWritePerSampleShorts(tif, fip->field_tag,
			    dir++, v))
				goto bad;
			break;
		case FIELD_PAGENUMBER: {
			short v1, v2;
			dir->tdir_tag = TIFFTAG_PAGENUMBER;
			dir->tdir_type = (short)TIFF_SHORT;
			dir->tdir_count = 2;
			TIFFgetfield(td, TIFFTAG_PAGENUMBER, &v1, &v2);
			if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN)
				dir->tdir_offset = (v1 << 16) | (v2 & 0xffff);
			else
				dir->tdir_offset = (v2 << 16) | (v1 & 0xffff);
			dir++;
			break;
		}
		default:
			dir->tdir_tag = fip->field_tag;
			dir->tdir_type = (short)fip->field_type;
			dir->tdir_count = fip->field_count;
			if (fip->field_type == TIFF_SHORT) {
				TIFFgetfield(td, fip->field_tag, &v);
				dir->tdir_offset =
				    TIFFInsertData(tif, dir->tdir_type, v);
			} else
				TIFFgetfield(td, fip->field_tag,
				    &dir->tdir_offset);
			dir++;
			break;
		}
		ResetFieldBit(fields, fip->field_bit);
	}
	/*
	 * Write directory.
	 */
#ifdef NeXT_MOD
	(void) SeekOK(tif->tif_fd, tif->tif_diroff);
#else
	(void) lseek(tif->tif_fd, tif->tif_diroff, L_SET);
#endif
	dircount = nfields;
	if (!WriteOK(tif->tif_fd, &dircount, sizeof (short))) {
		TIFFError(tif->tif_name, "Error writing directory count");
		goto bad;
	}
	if (!WriteOK(tif->tif_fd, data, dirsize)) {
		TIFFError(tif->tif_name, "Error writing directory contents");
		goto bad;
	}
	if (!WriteOK(tif->tif_fd, &tif->tif_nextdiroff, sizeof (long))) {
		TIFFError(tif->tif_name, "Error writing directory link");
		goto bad;
	}
	TIFFFreeDirectory(tif);
	free(data);
	tif->tif_flags &= ~TIFF_DIRTYDIRECT;

	/*
	 * Reset directory-related state for subsequent
	 * directories.
	 */
	TIFFDefaultDirectory(tif);
	tif->tif_diroff = 0;
	tif->tif_curoff = 0;
	tif->tif_row = -1;
	tif->tif_curstrip = -1;
	return (1);
bad:
	free(data);
	return (0);
}
#undef MakeShortDirent
#undef WriteRationalPair

static
#ifdef NeXT_MOD
int
#endif
DECLARE4(TIFFWriteRational,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, float, v)
{
	long t[2];

	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_RATIONAL;
	dir->tdir_count = 1;
	/* need algorithm to convert ... XXX */
	t[0] = v * 10000.0;
	t[1] = 10000;
	return (TIFFWriteData(tif, dir, (char *)t));
}

static
#ifdef NeXT_MOD
int
#endif
DECLARE4(TIFFWritePerSampleShorts,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, short, v)
{
	short w[4];
	int i, samplesperpixel = tif->tif_dir.td_samplesperpixel;

	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_SHORT;
	dir->tdir_count = samplesperpixel;
	if (samplesperpixel <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (long)v << 16;
			if (samplesperpixel == 2)
				dir->tdir_offset |= v & 0xffff;
		} else {
			dir->tdir_offset = v & 0xffff;
			if (samplesperpixel == 2)
				dir->tdir_offset |= (long)v << 16;
		}
		return (1);
	}
	for (i = 0; i < samplesperpixel; i++)
		w[i] = v;
	return (TIFFWriteData(tif, dir, (char *)w));
}

static
#ifdef NeXT_MOD
int
#endif
DECLARE4(TIFFWriteStripThing,
    TIFF*, tif, u_short, tag, TIFFDirEntry*, dir, u_long*, lp)
{

	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_LONG;		/* XXX */
	dir->tdir_count = tif->tif_dir.td_nstrips;
	if (dir->tdir_count > 1)
		return (TIFFWriteData(tif, dir, (char *)lp));
	dir->tdir_offset = *lp;
	return (1);
}

#ifdef NeXT_MOD
/*
 * Writes as many longs from lp[] as there are samples.
 */
static int TIFFWriteSampleLongs(TIFF* tif, u_short tag, TIFFDirEntry* dir, u_long* lp)
{

	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_LONG;
	dir->tdir_count = tif->tif_dir.td_samplesperpixel;
	if (dir->tdir_count > 1)
		return (TIFFWriteData(tif, dir, (char *)lp));
	dir->tdir_offset = *lp;
	return 1;
}

/*
 * Writes n shorts from sp[].
 */
static int TIFFWriteShorts(TIFF* tif, u_short tag, TIFFDirEntry* dir, u_short * sp, int n)
{
	dir->tdir_tag = tag;
	dir->tdir_type = (short)TIFF_SHORT;
	dir->tdir_count = n;
	if (n <= 2) {
		if (tif->tif_header.tiff_magic == TIFF_BIGENDIAN) {
			dir->tdir_offset = (long)sp[0] << 16;
			if (n == 2) dir->tdir_offset |= sp[1] & 0xffff;
		} else {
			dir->tdir_offset = sp[0] & 0xffff;
			if (n == 2) dir->tdir_offset |= (long)(sp[0]) << 16;
		}
		return 1;
	}
	return (TIFFWriteData(tif, dir, (char *)sp));
}
#endif

/*
 * Write a contiguous directory item.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFWriteData(tif, dir, cp)
	TIFF *tif;
	TIFFDirEntry *dir;
	char *cp;
{
	int cc;

#ifdef NeXT_MOD
	dir->tdir_offset = tif->tif_dataoff;
#else
	dir->tdir_offset = dataoff;
#endif
	cc = dir->tdir_count * datawidth[dir->tdir_type];
	if (SeekOK(tif->tif_fd, dir->tdir_offset) &&
	    WriteOK(tif->tif_fd, cp, cc)) {
#ifdef NeXT_MOD
		tif->tif_dataoff += (cc + 1) & ~1;
#else
		dataoff += (cc + 1) & ~1;
#endif
		return (1);
	}
	TIFFError(tif->tif_name, "Error writing data for field \"%s\"",
	    FieldWithTag(dir->tdir_tag)->field_name);
	return (0);
}

/*
 * Link the current directory into the
 * directory chain for the file.
 */
static
#ifdef NeXT_MOD
int
#endif
TIFFLinkDirectory(tif)
	register TIFF *tif;
{
#ifdef NeXT_MOD
	char module[] = "";
#else
	static char module[] = "TIFFLinkDirectory";
#endif
	u_short dircount;
	long nextdir;

#ifdef NeXT_MOD
	NXSeek(tif->tif_fd, 0L, NX_FROMEND);
	tif->tif_diroff = (NXTell(tif->tif_fd)+1) &~ 1L;
#else
	tif->tif_diroff = (lseek(tif->tif_fd, 0L, L_XTND)+1) &~ 1L;
#endif
	if (tif->tif_header.tiff_diroff == 0) {
		/*
		 * First directory, overwrite header.
		 */
		tif->tif_header.tiff_diroff = tif->tif_diroff;
#ifdef NeXT_MOD
		(void) SeekOK(tif->tif_fd, 0L);
#else
		(void) lseek(tif->tif_fd, 0L, L_SET);
#endif
		if (!WriteOK(tif->tif_fd, &tif->tif_header,
		    sizeof (tif->tif_header))) {
			TIFFError(tif->tif_name, "Error writing TIFF header");
			return (0);
		}
		return (1);
	}
	/*
	 * Not the first directory, search to the last and append.
	 */
	nextdir = tif->tif_header.tiff_diroff;
	do {
		if (!SeekOK(tif->tif_fd, nextdir) ||
		    !ReadOK(tif->tif_fd, &dircount, sizeof (dircount))) {
			TIFFError(module, "Error fetching directory count");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
			_NXTIFFSwabShort(&dircount);
#else
			TIFFSwabShort(&dircount);
#endif
#ifdef NeXT_MOD
		NXSeek(tif->tif_fd, dircount * sizeof (TIFFDirEntry), 
		       NX_FROMCURRENT);
#else
		lseek(tif->tif_fd, dircount * sizeof (TIFFDirEntry), L_INCR);
#endif
		if (!ReadOK(tif->tif_fd, &nextdir, sizeof (nextdir))) {
			TIFFError(module, "Error fetching directory link");
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
			_NXTIFFSwabLong(&nextdir);
#else
			TIFFSwabLong(&nextdir);
#endif
	} while (nextdir != 0);
#ifdef NeXT_MOD
	NXSeek(tif->tif_fd, -sizeof (nextdir), NX_FROMCURRENT);
#else
	(void) lseek(tif->tif_fd, -sizeof (nextdir), L_INCR);
#endif
	if (!WriteOK(tif->tif_fd, &tif->tif_diroff, sizeof (tif->tif_diroff))) {
		TIFFError(module, "Error writing directory link");
		return (0);
	}
	return (1);
}

/*
 * Set the n-th directory as the current directory.
 * NB: Directories are numbered starting at 0.
 */
#ifdef NeXT_MOD
int _NXTIFFSetDirectory(register TIFF *tif, int n)
#else
TIFFSetDirectory(tif, n)
	register TIFF *tif;
	int n;
#endif
{
#ifdef NeXT_MOD
	char module[] = "";
#else
	static char module[] = "TIFFSetDirectory";
#endif
	u_short dircount;
	long nextdir;

	nextdir = tif->tif_header.tiff_diroff;
	while (n-- > 0 && nextdir != 0) {
		if (!SeekOK(tif->tif_fd, nextdir) ||
		    !ReadOK(tif->tif_fd, &dircount, sizeof (dircount))) {
			TIFFError(module, "%s: Error fetching directory count",
			    tif->tif_name);
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
			_NXTIFFSwabShort(&dircount);
#else
			TIFFSwabShort(&dircount);
#endif
#ifdef NeXT_MOD
		NXSeek(tif->tif_fd, dircount*sizeof (TIFFDirEntry), NX_FROMCURRENT);
#else
		lseek(tif->tif_fd, dircount*sizeof (TIFFDirEntry), L_INCR);
#endif
		if (!ReadOK(tif->tif_fd, &nextdir, sizeof (nextdir))) {
			TIFFError(module, "%s: Error fetching directory link",
			    tif->tif_name);
			return (0);
		}
		if (tif->tif_flags & TIFF_SWAB)
#ifdef NeXT_MOD
			_NXTIFFSwabLong(&nextdir);
#else
			TIFFSwabLong(&nextdir);
#endif

	}
	tif->tif_nextdiroff = nextdir;
#ifdef NeXT_MOD
	tif->tif_freeprev = 1;
#endif
	return (TIFFReadDirectory(tif));
}

#ifdef NeXT_MOD

#define MAXTIFFDIRECTORIES 10000		// Arbitrary?
/*
 * Return the number of directories.  If the file is somehow screwed up,
 * still return the number of directories that seem OK.
 */
int _NXTIFFNumDirectories(register TIFF *tif)
{
	short dirCount;
	int numDirs;
	long nextdir;
	long originalPos = NXTell (tif->tif_fd);

	numDirs = 0;
	nextdir = tif->tif_header.tiff_diroff;
	while (nextdir != 0 && numDirs < MAXTIFFDIRECTORIES) {
		if (!SeekOK(tif->tif_fd, nextdir) ||
		    !ReadOK(tif->tif_fd, &dirCount, sizeof (dirCount))) break;
		if (tif->tif_flags & TIFF_SWAB) _NXTIFFSwabShort(&dirCount);
		numDirs++;
		NXSeek(tif->tif_fd, dirCount*sizeof (TIFFDirEntry), NX_FROMCURRENT);
		if (!ReadOK(tif->tif_fd, &nextdir, sizeof (nextdir))) break;
		if (tif->tif_flags & TIFF_SWAB) _NXTIFFSwabLong(&nextdir);
	}
	NXSeek(tif->tif_fd, originalPos, NX_FROMSTART);
	return numDirs;
}

#endif

/*
  
Modifications (since 77):

77
--
 2/18/90 aozer  Created from Sam Leffler's freely distributable library
 2/19/90 aozer  Added _NXTIFFNumDirectories()
 2/20/90 aozer  Removed the static dataoff; added a new TIFF field tif_dataoff
 3/5/90 aozer   Added kludgery in _NXReadDirectory to deal with illegal
		alpha TIFF files created by the 1.0 NXWriteTIFF() function.

80
--
 4/10/90 aozer	Allowed parsing files where number of bps values != spp.
		(The TIFF spec seems to allow for this, oh well...)
 4/11/90 aozer	Applied patches to allow multiple images per file

85
--
 5/21/90 aozer	Convert photometric == 5 to photometric == 1 and add matteing
		if spp = 2 (thus deal with old alpha images at lowest level!)

86
--
 6/9/90 aozer	Integrated changes for Sam Leffler's TIFF Library v2.2.

91
--
 8/7/90 aozer	Added code and tif_freeprev to make sure previous directory got 
		freed on SetDirectory(). (Was not happening if the directory 
		set was zero.) Bug 6844.

93
--
 9/4/90 aozer	In _NXTIFFReadDirectory() set matte=1 if spp==2 for gray or
		spp==4 for RGB images.

98
--
 10/16/90 aozer	Made _NXTIFFNumDirectories() swap dirCount & nextdir if needed.

*/

/*
	afm.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#ifndef AFM_H
#define AFM_H

/* font attributes to ask the server for */

#define NX_FONTHEADER		1
#define NX_FONTMETRICS		2
#define NX_FONTWIDTHS		4
#define NX_FONTCHARDATA		8
#define NX_FONTKERNING		16
#define NX_FONTCOMPOSITES	32


typedef struct {	/* per character info */
    short charCode;		/* character code, -1 if unencoded */
    unsigned char numKernPairs;	/* #kern pairs starting with this char */
    unsigned char reserved;
    float xWidth;		/* width in X of this character */
    int name;			/* name - an index into stringTable */
    float bbox[4];		/* character bbox */
    int kernPairIndex;		/* index into NXFontMetrics.kerns array */
} NXCharMetrics;


typedef struct {	/* elements of the ligature array */
	/* all ligatures go here, regardless of if the chars are encoded */
    int firstCharIndex;		/* index into NXFontMetrics.charMetrics */
    int secondCharIndex;	/* index into NXFontMetrics.charMetrics */
    int ligatureIndex;		/* index into NXFontMetrics.charMetrics */
} NXLigature;


typedef struct {	/* elements of the encoded ligature array */
	/* ligatures only go here if all three chars are encoded */
    unsigned char firstChar;		/* char encoding of first char */
    unsigned char secondChar;		/* char encoding of second char */
    unsigned char ligatureChar;		/* char encoding of ligature */
    unsigned char reserved;
} NXEncodedLigature;


typedef struct {	/* elements of the kern pair array */
    int secondCharIndex;	/* index into NXFontMetrics.charMetrics */
    float dx;			/* displacement relative to first char */
    float dy;
} NXKernPair;


typedef struct {	/* elements of the kern X pair array */
    int secondCharIndex;	/* index into NXFontMetrics.charMetrics */
    float dx;			/* X displacement relative to first char */
				/* Y displacement is implicitly 0 for these */
} NXKernXPair;


typedef struct {	/* elements of the track kern array */
    int degree;			/* degree of tightness */
    float minPointSize;		/* parameters for this track */
    float minKernAmount;
    float maxPointSize;
    float maxKernAmount;
} NXTrackKern;


typedef struct {	/* a composite char */
    int compCharIndex;		/* index into NXFontMetrics.charMetrics */
    int numParts;		/* number of parts making up this char */
    int firstPartIndex;		/* index of first part in */
				/* NXFontMetrics.compositeCharParts */
} NXCompositeChar;


typedef struct {	/* elements of the composite char array */
    int partIndex;		/* index into NXFontMetrics.charMetrics */
    float dx;			/* displacement of part */
    float dy;
} NXCompositeCharPart;


/*
 * Font information from Adobe Font Metrics file
 *
 * Do NOT imbed this structure in your data structures, length may change.
 */

typedef struct _NXFontMetrics {
    char *formatVersion;	/* version of afm file format */
    char *name;			/* name of font for findfont */
    char *fullName;		/* full name of font */
    char *familyName;		/* "font family" name */
    char *weight;		/* weight of font */
    float italicAngle;		/* degrees ccw from vertical */
    char isFixedPitch;		/* is the font mono-spaced? */
    char isScreenFont;		/* is the font a screen font? */
    short screenFontSize;	/* If it is, how big is it? */
    float fontBBox[4];		/* bounding box (llx lly urx ury) */
    float underlinePosition;	/* dist from basline for underlines */
    float underlineThickness;	/* thickness of underline stroke */
    char *version;		/* version identifier */
    char *notice;		/* trademark or copyright */
    char *encodingScheme;	/* default encoding vector */
    float capHeight;		/* top of 'H' */
    float xHeight;		/* top of 'x' */
    float ascender;		/* top of 'd' */
    float descender;		/* bottom of 'p' */
    short hasYWidths;		/* do any chars have non-0 y width? */
    float *widths;		/* character widths in x */
    unsigned int widthsLength;
    char *strings;		/* table of strings and other info */
    unsigned int stringsLength;
    char hasXYKerns;		/* Do any of the kern pairs have nonzero dy? */
    char reserved;
    short *encoding;		/* 256 offsets into charMetrics */
    float *yWidths;		/* character widths in y.  NOT in encoding */
		/* order, but a parallel array to the charMetrics array */
    NXCharMetrics *charMetrics;		/* array of NXCharMetrics */
    int numCharMetrics;			/* num elements */
    NXLigature *ligatures;		/* array of NXLigatures */
    int numLigatures;			/* num elements */
    NXEncodedLigature *encLigatures;	/* array of NXEncodedLigatures */
    int numEncLigatures;		/* num elements */
    union {
	NXKernPair *kernPairs;		/* array of NXKernPairs */
	NXKernXPair *kernXPairs;	/* array of NXKernXPairs */
    } kerns;
    int numKernPairs;			/* num elements */
    NXTrackKern *trackKerns;		/* array of NXTrackKerns */
    int numTrackKerns;			/* num elements */
    NXCompositeChar *compositeChars;	/* array of NXCompositeChar */
    int numCompositeChars;		/* num elements */
    NXCompositeCharPart *compositeCharParts; /* array of NXCompositeCharPart */
    int numCompositeCharParts;		/* num elements */
} NXFontMetrics;

#endif AFM_H


/*
	Font.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#ifndef FONT_H
#define FONT_H

#import <objc/Object.h>
#import "afm.h"

#define NX_IDENTITYMATRIX ((float *) 0)
#define NX_FLIPPEDMATRIX  ((float *) -1)

/* 
 * Figure is also known as non-breaking space.
 * NX_EMSPACE, NX_ENSPACE and NX_THINSPACE have been removed as part of
 * the switch to the NextStep encoding.
 */

#define NX_FIGSPACE	((unsigned short)0x80)

typedef struct _NXFaceInfo {
    NXFontMetrics      *fontMetrics;	    /* Information from afm file */
    int                 flags;		    /* Which font info is present */
    struct _fontFlags {			    /* Keep track of font usage
					     * for Conforming PS comments */
	unsigned int        usedInDoc:1;    /* has font been used in doc? */
	unsigned int        usedInPage:1;   /* has font been used in page? */
	unsigned int        usedInSheet:1;  /* has font been used in sheet? */
	unsigned int        _PADDING:13;
    }                   fontFlags;
    struct _NXFaceInfo *nextFInfo;	    /* next faceInfo in the list */
} NXFaceInfo;

extern const char *NXSystemFont;
extern const char *NXBoldSystemFont;

@interface Font : Object
{
    char               *name;
    float               size;
    int                 style;
    float              *matrix;
    int                 fontNum;
    NXFaceInfo         *faceInfo;
    id                  otherFont;
    struct _fFlags {
	unsigned int        usedByWS:1;
	unsigned int        usedByPrinter:1;
	unsigned int        isScreenFont:1;
	unsigned int        _RESERVED:10;
	unsigned int        _matrixIsIdentity:1;
	unsigned int        _matrixIsFlipped:1;
	unsigned int        _hasStyle:1;
    }                   fFlags;
    unsigned short      _reservedFont2;
    unsigned int        _reservedFont3;
}

+ initialize;
+ allocFromZone:(NXZone *)zone;
+ alloc;
+ newFont:(const char *)fontName size:(float)fontSize style:(int)fontStyle matrix:(const float *)fontMatrix;
+ newFont:(const char *)fontName size:(float)fontSize;
+ newFont:(const char *)fontName size:(float)fontSize matrix:(const float *)fontMatrix;
+ useFont:(const char *)fontName;

- awake;
- free;
- (float)pointSize;
- (const char *)name;
- (int)fontNum;
- (int)style;
- setStyle:(int)aStyle;
- (const float *)matrix;
- (NXFontMetrics *)metrics;
- (NXFontMetrics *)readMetrics:(int)flags;
- (BOOL)hasMatrix;
- set;
- (float)getWidthOf:(const char *)string;
- screenFont;
- finishUnarchiving;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
@end

#endif FONT_H

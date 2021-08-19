/*
	Cursor.h
	Application Kit, Release 1.0
	Copyright (c) 1988, 1989, NeXT, Inc.  All rights reserved. 
*/

#import "Bitmap.h"

/* This object is obsolete, use NXCursor instead. */

/* AppKit Cursors */

#define	NXarrow	NXArrow
#define	NXiBeam	NXIBeam

extern id       NXArrow; 	/* Arrow cursor */
extern id       NXIBeam;	/* Text cursor */

@interface Cursor:Bitmap
{
    NXPoint             hotSpot;
    struct _cFlags {
	unsigned int        onMouseExited:1;
	unsigned int        onMouseEntered:1;
	unsigned int        _RESERVED:14;
    }                   cFlags;
}

@end

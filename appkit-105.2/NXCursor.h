/*
	NXCursor.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "NXImage.h"

/* AppKit Cursors */

#define	NXarrow	NXArrow
#define	NXiBeam	NXIBeam

extern id       NXArrow; 	/* Arrow cursor */
extern id       NXIBeam;	/* Text cursor */

@interface NXCursor : Object
{
    NXPoint             hotSpot;
    struct _csrFlags {
	unsigned int        onMouseExited:1;
	unsigned int        onMouseEntered:1;
	unsigned int        _RESERVED:14;
    }                   cFlags;
    id			image;
    unsigned int	_reservedInt;
}

+ pop;
+ currentCursor;

- init;
- initFromImage:newImage;

- image;
- setImage:newImage;
- setHotSpot:(const NXPoint *)spot;
- push;
- pop;
- set;
- setOnMouseExited:(BOOL)flag;
- setOnMouseEntered:(BOOL)flag;
- mouseEntered:(NXEvent *)theEvent;
- mouseExited:(NXEvent *)theEvent;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFromImage:newImage;

@end

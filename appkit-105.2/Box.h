/*
	Box.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "View.h"

/* Box types */

#define NX_NOBORDER	0
#define NX_LINE		1
#define NX_BEZEL	2
#define NX_GROOVE	3
#define NX_RIDGE	3	/* historical - use NX_GROOVE */

/* Box title positions */

#define NX_NOTITLE	0
#define NX_ABOVETOP	1
#define NX_ATTOP	2
#define NX_BELOWTOP	3
#define NX_ABOVEBOTTOM	4
#define NX_ATBOTTOM	5
#define NX_BELOWBOTTOM	6

@interface Box : View
{
    id                  cell;
    id                  contentView;
    NXSize              offsets;
    NXRect              borderRect;
    NXRect              titleRect;
    struct _bFlags {
	unsigned int        borderType:2;
	unsigned int        titlePosition:3;
	unsigned int        transparent:1;
	unsigned int        _RESERVED:10;
    }                   bFlags;
}

- initFrame:(const NXRect *)frameRect;

- awake;
- free;
- (int)borderType;
- (int)titlePosition;
- setBorderType:(int)aType;
- setTitlePosition:(int)aPosition;
- (const char *)title;
- setTitle:(const char *)aString;
- cell;
- font;
- setFont:fontObj;
- getOffsets:(NXSize *)aSize;
- setOffsets:(NXCoord)width :(NXCoord)height;
- sizeTo:(NXCoord)width :(NXCoord)height;
- sizeToFit;
- setFrameFromContentFrame:(const NXRect *)contentFrame;
- contentView;
- setContentView:aView;
- addSubview:aView;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

/*
	Scroller.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

/* Location of scroll arrows within the scroller */

#define	NX_SCROLLARROWSMAXEND	0
#define	NX_SCROLLARROWSMINEND	1
#define	NX_SCROLLARROWSNONE	2

/* Useable parts in the scroller */

#define	NX_SCROLLERNOPARTS	0
#define	NX_SCROLLERONLYARROWS	1
#define	NX_SCROLLERALLPARTS	2

/* Part codes for various parts of the scroller */

#define	NX_NOPART	0
#define NX_DECPAGE	1
#define	NX_KNOB		2
#define	NX_INCPAGE	3
#define NX_DECLINE	4
#define NX_INCLINE	5
#define	NX_KNOBSLOT	6
#define	NX_JUMP		6

#define	NX_SCROLLERWIDTH	(18.0)

@interface Scroller : Control
{
    float               curValue;
    float               perCent;
    NXCoord             _knobSize;
    int                 hitPart;
    id                  target;
    SEL                 action;
    struct _sFlags {
	unsigned int        isHoriz:1;
	unsigned int        arrowsLoc:2;
	unsigned int        partsUsable:2;
	unsigned int        _fine:1;
	unsigned int        _RESERVED:6;
	unsigned int        _needsEnableFlush:1;
	unsigned int        _thumbing:1;
	unsigned int        _slotDrawn:1;
	unsigned int        _knobDrawn:1;
    }                   sFlags;
}

- initFrame:(const NXRect *)frameRect;
- drawParts;
- awake;
- (NXRect *)calcRect:(NXRect *)aRect forPart:(int)partCode;
- checkSpaceForParts;
- target;
- setTarget:anObject;
- (SEL)action;
- setAction:(SEL)aSelector;
- sizeTo:(NXCoord)width :(NXCoord)height;
- setArrowsPosition:(int)where;
- drawArrow:(BOOL)whichButton :(BOOL)flag;
- drawKnob;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- highlight:(BOOL)flag;
- (int)testPart:(const NXPoint *)thePoint;
- trackKnob:(NXEvent *)theEvent;
- trackScrollButtons:(NXEvent *)theEvent;
- mouseDown:(NXEvent *)theEvent;
- (int)hitPart;
- (float)floatValue;
- setFloatValue:(float)aFloat :(float)percent;
- setFloatValue:(float)aFloat;
- (BOOL)acceptsFirstMouse;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

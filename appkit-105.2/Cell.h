/*
	Cell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "graphics.h"

/* Cell Data Types */

#define NX_ANYTYPE 0
#define NX_INTTYPE 1
#define NX_POSINTTYPE 2
#define NX_FLOATTYPE 3
#define NX_POSFLOATTYPE 4
#define NX_DATETYPE 5
#define NX_DOUBLETYPE 6
#define NX_POSDOUBLETYPE 7

/* Cell Types */

#define NX_NULLCELL	0
#define	NX_TEXTCELL	1
#define NX_ICONCELL	2

/* Cell & ButtonCell */

#define NX_CELLDISABLED		0
#define	NX_CELLSTATE		1
#define	NX_CELLEDITABLE		3
#define	NX_CELLHIGHLIGHTED	5
#define NX_LIGHTBYCONTENTS	6
#define NX_LIGHTBYGRAY		7
#define NX_LIGHTBYBACKGROUND	9
#define NX_ICONISKEYEQUIVALENT	10
#define NX_HASALPHA		11
#define NX_BORDERED		12
#define NX_OVERLAPPINGICON	13
#define NX_ICONHORIZONTAL	14
#define NX_ICONONLEFTORBOTTOM	15
#define	NX_CHANGECONTENTS	16

/* ButtonCell icon positions */

#define NX_TITLEONLY		0
#define NX_ICONONLY		1
#define NX_ICONLEFT		2
#define NX_ICONRIGHT		3
#define NX_ICONBELOW		4
#define NX_ICONABOVE		5
#define NX_ICONOVERLAPS		6

/* ButtonCell highlightsBy and showsStateBy mask */

#define NX_NONE			0
#define NX_CONTENTS		1
#define NX_PUSHIN		2
#define NX_CHANGEGRAY		4
#define NX_CHANGEBACKGROUND	8

/* Cell whenActionIsSent mask flag */

#define NX_PERIODICMASK (1 << (NX_LASTEVENT+1))

@interface Cell : Object
{
    char               *contents;
    id                  support;
    struct _cFlags1 {
	unsigned int        state:1;
	unsigned int        highlighted:1;
	unsigned int        disabled:1;
	unsigned int        editable:1;
	unsigned int        type:2;
	unsigned int        freeText:1;
	unsigned int        alignment:2;
	unsigned int        bordered:1;
	unsigned int        bezeled:1;
	unsigned int        selectable:1;
	unsigned int        scrollable:1;
	unsigned int        entryType:3;
    }                   cFlags1;
    struct _cFlags2 {
	unsigned int        continuous:1;
	unsigned int        actOnMouseDown:1;
	unsigned int        _isLeaf:1;
	unsigned int        floatLeft:4;
	unsigned int        floatRight:4;
	unsigned int        autoRange:1;
	unsigned int        actOnMouseDragged:1;
	unsigned int        _isLoaded:1;
	unsigned int        noWrap:1;
	unsigned int        dontActOnMouseUp:1;
    }                   cFlags2;
    struct __cFlags3 {
	unsigned int	    isWhite:1;
	unsigned int	    useUserKeyEquivalent:1;
	unsigned int	    center:1;
	unsigned int	    docEditing:1;
	unsigned int	    docSaved:1;
	unsigned int	    RESERVED:11;
    }			_cFlags3;
    unsigned short	    _reservedCshort;
}


+ (BOOL)prefersTrackingUntilMouseUp;

- init;
- initTextCell:(const char *)aString;
- initIconCell:(const char *)iconName;

- copy;
- copyFromZone:(NXZone *)zone;
- awake;
- free;
- controlView;
- (int)type;
- setType:(int)aType;
- (int)state;
- setState:(int)value;
- incrementState;
- target;
- setTarget:anObject;
- (SEL)action;
- setAction:(SEL)aSelector;
- (int)tag;
- setTag:(int)anInt;
- (BOOL)isOpaque;
- (BOOL)isEnabled;
- setEnabled:(BOOL)flag;
- (int)sendActionOn:(int)mask;
- (BOOL)isContinuous;
- setContinuous:(BOOL)flag;
- (BOOL)isEditable;
- setEditable:(BOOL)flag;
- (BOOL)isSelectable;
- setSelectable:(BOOL)flag;
- (BOOL)isBordered;
- setBordered:(BOOL)flag;
- (BOOL)isBezeled;
- setBezeled:(BOOL)flag;
- (BOOL)isScrollable;
- setScrollable:(BOOL)flag;
- (BOOL)isHighlighted;
- (int)alignment;
- setAlignment:(int)mode;
- setWrap:(BOOL)flag;
- font;
- setFont:fontObj;
- (int)entryType;
- setEntryType:(int)aType;
- (BOOL)isEntryAcceptable:(const char *)aString;
- setFloatingPointFormat:(BOOL)autoRange left:(unsigned)leftDigits right:(unsigned)rightDigits;
- (unsigned short)keyEquivalent;
- (const char *)stringValue;
- setStringValue:(const char *)aString;
- setStringValueNoCopy:(const char *)aString;
- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag;
- (int)intValue;
- setIntValue:(int)anInt;
- (float)floatValue;
- setFloatValue:(float)aFloat;
- (double)doubleValue;
- setDoubleValue:(double)aDouble;
- takeIntValueFrom:sender;
- takeFloatValueFrom:sender;
- takeDoubleValueFrom:sender;
- takeStringValueFrom:sender;
- (const char *)icon;
- setIcon:(const char *)iconName;
- (int)getParameter:(int)aParameter;
- setParameter:(int)aParameter to:(int)value;
- getIconRect:(NXRect *)theRect;
- getTitleRect:(NXRect *)theRect;
- getDrawRect:(NXRect *)theRect;
- calcCellSize:(NXSize *)theSize;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- calcDrawInfo:(const NXRect *)aRect;
- setTextAttributes:textObj;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;
- (int)mouseDownFlags;
- getPeriodicDelay:(float*)delay andInterval:(float*)interval;
- (BOOL)startTrackingAt:(const NXPoint *)startPoint inView:controlView;
- (BOOL)continueTracking:(const NXPoint *)lastPoint at:(const NXPoint *)currentPoint inView:controlView;
- stopTracking:(const NXPoint *)lastPoint at:(const NXPoint *)stopPoint inView:controlView mouseIsUp:(BOOL)flag;
- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView;
- edit:(const NXRect *)aRect inView:controlView editor:textObj delegate:anObject event:(NXEvent *)theEvent;
- select:(const NXRect *)aRect inView:controlView editor:textObj delegate:anObject start:(int)selStart length:(int)selLength;
- endEditing:textObj;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
- resetCursorRect:(const NXRect *)cellFrame inView:controlView;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTextCell;
+ newTextCell:(const char *)aString;
+ newIconCell;
+ newIconCell:(const char *)iconName;

@end

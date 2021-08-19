/*
	Control.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "View.h"

@interface Control : View
{
    int                 tag;
    id                  cell;
    struct _conFlags {
	unsigned int        enabled:1;
	unsigned int        editingValid:1;
	unsigned int        ignoreMultiClick:1;
	unsigned int        calcSize:1;
	unsigned int        _RESERVED:12;
    }                   conFlags;
    unsigned short      _reservedCshort1;
}

+ setCellClass:factoryId;

- initFrame:(const NXRect *)frameRect;

- free;
- sizeToFit;
- sizeTo:(NXCoord)width :(NXCoord)height;
- calcSize;
- cell;
- setCell:aCell;
- selectedCell;
- target;
- setTarget:anObject;
- (SEL)action;
- setAction:(SEL)aSelector;
- (int)tag;
- setTag:(int)anInt;
- (int)selectedTag;
- ignoreMultiClick:(BOOL)flag;
- mouseDown:(NXEvent *)theEvent;
- (int)mouseDownFlags;
- (int)sendActionOn:(int)mask;
- (BOOL)isContinuous;
- setContinuous:(BOOL)flag;
- (BOOL)isEnabled;
- setEnabled:(BOOL)flag;
- setFloatingPointFormat:(BOOL)autoRange left:(unsigned)leftDigits right:(unsigned)rightDigits;
- (int)alignment;
- setAlignment:(int)mode;
- font;
- setFont:fontObj;
- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag;
- setStringValue:(const char *)aString;
- setStringValueNoCopy:(const char *)aString;
- setIntValue:(int)anInt;
- setFloatValue:(float)aFloat;
- setDoubleValue:(double)aDouble;
- (const char *)stringValue;
- (int)intValue;
- (float)floatValue;
- (double)doubleValue;
- update;
- updateCell:aCell;
- updateCellInside:aCell;
- drawCellInside:aCell;
- drawCell:aCell;
- selectCell:aCell;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- sendAction:(SEL)theAction to:theTarget;
- takeIntValueFrom:sender;
- takeFloatValueFrom:sender;
- takeDoubleValueFrom:sender;
- takeStringValueFrom:sender;
- currentEditor;
- abortEditing;
- validateEditing;
- resetCursorRects;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

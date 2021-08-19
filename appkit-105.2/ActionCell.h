/*
	ActionCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Cell.h"

@interface ActionCell : Cell
{
    int             tag;	
    id              target;	
    SEL             action;	
    id		    _view;	
}

- controlView;
- setFont:fontObj;
- setAlignment:(int)mode;
- setBordered:(BOOL)flag;
- setBezeled:(BOOL)flag;
- setEnabled:(BOOL)flag;
- setFloatingPointFormat:(BOOL)autoRange left:(unsigned int)leftDigits right:(unsigned int)rightDigits;
- setIcon:(const char *)iconName;
- target;
- setTarget:anObject;
- (SEL)action;
- setAction:(SEL)aSelector;
- (int)tag;
- setTag:(int)anInt;
- (const char *)stringValue;
- (int)intValue;
- (float)floatValue;
- (double)doubleValue;
- setStringValue:(const char *)aString;
- setStringValueNoCopy:(char *)aString shouldFree:(BOOL)flag;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

@end

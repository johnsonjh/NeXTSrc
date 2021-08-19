/*
	Form.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Matrix.h"

@interface  Form : Matrix
{
}

+ setCellClass:factoryId;

- initFrame:(const NXRect *)frameRect;

- calcSize;
- sizeTo:(NXCoord)width :(NXCoord)height;
- sizeToFit;
- (int)selectedIndex;
- setEntryWidth:(NXCoord)width;
- setInterline:(NXCoord)spacing;
- setBordered:(BOOL)flag;
- setBezeled:(BOOL)flag;
- setTitleAlignment:(int)mode;
- setTextAlignment:(int)mode;
- setFont:fontObj;
- setTitleFont:fontObj;
- setTextFont:fontObj;
- drawCellAt:(int)index;
- (const char *)titleAt:(int)index;
- setTitle:(const char *)aString at:(int)index;
- addEntry:(const char *)title;
- addEntry:(const char *)title tag:(int)anInt target:anObject action:(SEL)aSelector;
- insertEntry:(const char *)title at:(int)index;
- insertEntry:(const char *)title at:(int)index tag:(int)anInt target:anObject action:(SEL)aSelector;
- removeEntryAt:(int)index;
- setTag:(int)anInt at:(int)index;
- setTarget:anObject at:(int)index;
- setAction:(SEL)aSelector at:(int)index;
- (int)findIndexWithTag:(int)aTag;
- (const char *)stringValueAt:(int)index;
- setStringValue:(const char *)aString at:(int)index;
- (int)intValueAt:(int)index;
- setIntValue:(int)anInt at:(int)index;
- (float)floatValueAt:(int)index;
- setFloatValue:(float)aFloat at:(int)index;
- (double)doubleValueAt:(int)index;
- setDoubleValue:(double)aDouble at:(int)index;
- selectTextAt:(int)index;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

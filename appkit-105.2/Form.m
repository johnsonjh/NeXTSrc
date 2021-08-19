/*
	Form.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Form.h"
#import "FormCell.h"
#import <objc/List.h>
#import <math.h>
#import <zone.h>

#define VSPACER	3.0

static id formCellClass = nil;

@implementation Form:Matrix

+ setCellClass:factoryId
{
    formCellClass = factoryId;
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}


- initFrame:(const NXRect *)frameRect
{
    [super initFrame:frameRect];
    if (!formCellClass) formCellClass = [FormCell class];
    if (_NXCanAllocInZone(formCellClass, @selector(new), @selector(init))) {
	protoCell = [[formCellClass allocFromZone:[self zone]] init];
    } else {
	protoCell = [formCellClass new];
    }
    intercell.height = VSPACER;
    [protoCell calcCellSize:&cellSize];
    if (frameRect) cellSize.width = frameRect->size.width;
    conFlags.calcSize = YES;
    [self setMode:NX_TRACKMODE];
    return self;
}


- (NXCoord)_maxWidth
{
    int i = [self cellCount];
    NXCoord width, maxWidth;

    maxWidth = 0.0;
    while (i) {
	width = [[[cellList objectAt:--i] setTitleWidth:-1.0] titleWidth:&cellSize];
	if (width > maxWidth) maxWidth = width;
    }

    return maxWidth;
}


- (BOOL)_resetTitleWidths:aCell
{
    int i;
    NXCoord tWidth, pWidth;

    tWidth = aCell ? [[aCell setTitleWidth : -1.0] titleWidth:&cellSize]:
    [self _maxWidth];
    pWidth = aCell ? [protoCell titleWidth : &cellSize]:- 1.0;
    if (tWidth >= pWidth) {
	i = [self cellCount];
	while (i) [[cellList objectAt:--i] setTitleWidth:tWidth];
	[protoCell setTitleWidth:tWidth];
	return YES;
    } else {
	[aCell setTitleWidth:pWidth];
	return NO;
    }
}


- calcSize
{
    conFlags.calcSize = NO;
    [self _resetTitleWidths:nil];
    return self;
}


- sizeTo:(NXCoord)width :(NXCoord)height
{
    if (cellSize.width != width) {
	cellSize.width = width;
	conFlags.calcSize = YES;
    }
    return[super sizeTo:width :height];
}


- sizeToFit
{
    NXCoord width = cellSize.width ? cellSize.width : bounds.size.width;

    [protoCell calcCellSize:&cellSize];
    cellSize.width = width;
    [self sizeToCells];

    return self;
}


- (int)selectedIndex
{
    return[self selectedRow];
}


- setEntryWidth:(NXCoord)width
{
    cellSize.width = width;
    return self;
}


- setInterline:(NXCoord)spacing
{
    intercell.height = spacing;
    return self;
}


- setBordered:(BOOL)flag
{
    int i = [self cellCount];

    while (i) [[cellList objectAt:--i] setBordered:flag];
    [protoCell setBordered:flag];
    [self update];

    return self;
}


- setBezeled:(BOOL)flag
{
    int i = [self cellCount];

    while (i) [[cellList objectAt:--i] setBezeled:flag];
    [protoCell setBezeled:flag];

    return self;
}


- setTitleAlignment:(int)mode
{
    int i = [self cellCount];

    while (i) [[cellList objectAt:--i] setTitleAlignment:mode];
    [protoCell setTitleAlignment:mode];

    return self;
}


- setTextAlignment:(int)mode
{
    int i = [self cellCount];

    while (i) [[cellList objectAt:--i] setAlignment:mode];
    [protoCell setAlignment:mode];

    return self;
}


- setFont:fontObj
{
    int i = [self cellCount];

    [window disableDisplay];
    while (i) [[[cellList objectAt:--i] setFont:fontObj] setTitleFont:fontObj];
    [[protoCell setFont:fontObj] setTitleFont:fontObj];
    conFlags.calcSize = YES;
    [window reenableDisplay];

    return self;
}


- setTitleFont:fontObj
{
    int i = [self cellCount];

    [window disableDisplay];
    while (i) [[cellList objectAt:--i] setTitleFont:fontObj];
    [protoCell setTitleFont:fontObj];
    conFlags.calcSize = YES;
    [window reenableDisplay];
    [self update];

    return self;
}


- setTextFont:fontObj
{
    int i = [self cellCount];

    [window reenableDisplay];
    while (i) [[cellList objectAt:--i] setFont:fontObj];
    [protoCell setFont:fontObj];
    conFlags.calcSize = YES;
    [window reenableDisplay];
    [self update];

    return self;
}


- drawCellAt:(int)index
{
    return[self drawCellAt:index :0];
}


- (const char *)titleAt:(int)index
{
    return[[cellList objectAt:index] title];
}


- setTitle:(const char *)aString at:(int)index
{
    return[self setTitle:aString at:index :0];
}


- addEntry:(const char *)title
{
    return[self insertEntry:title at:[self cellCount]];
}


- addEntry:(const char *)title tag:(int)anInt target:anObject action:(SEL)aSelector
{
    return[self insertEntry:title at:[self cellCount] tag:anInt target:anObject action:aSelector];
}


- insertEntry:(const char *)title at:(int)index
{
    if (index >= 0 && index <= numRows) {
	[self insertRowAt:index];
	[self setTitle:title at:index];
	return[cellList objectAt:index];
    } else {
	return nil;
    }
}


- insertEntry:(const char *)title at:(int)index tag:(int)anInt target:anObject action:(SEL)aSelector
{
    if (index >= 0 && index <= numRows) {
	[self insertRowAt:index];
	[self setTag:anInt target:anObject action:aSelector at:index :0];
	[self setTitle:title at:index];
	return[cellList objectAt:index];
    } else {
	return nil;
    }
}


- removeEntryAt:(int)index
{
    [super removeRowAt:index andFree:YES];
    return self;
}


- setTag:(int)anInt at:(int)index
{
    return[self setTag:anInt at:index :0];
}


- setTarget:anObject at:(int)index
{
    return[self setTarget:anObject at:index :0];
}


- setAction:(SEL)aSelector at:(int)index
{
    return[self setAction:aSelector at:index :0];
}


- (int)findIndexWithTag:(int)aTag
{
    id aCell;
    int col, row;

    aCell = [self findCellWithTag:aTag];
    [self getRow:&row andCol:&col ofCell:aCell];

    return row;
}


- (const char *)stringValueAt:(int)index
{
    if (index >= 0 && index < numRows) {
	return[[cellList objectAt:index] stringValue];
    } else {
	return (char *)0;
    }
}


- setStringValue:(const char *)aString at:(int)index
{
    if (index >= 0 && index < numRows) {
	[[cellList objectAt:index] setStringValue:aString];
    }
    return self;
}


- (int)intValueAt:(int)index
{
    if (index >= 0 && index < numRows) {
	return[[cellList objectAt:index] intValue];
    } else {
	return 0;
    }
}


- setIntValue:(int)anInt at:(int)index
{
    if (index >= 0 && index < numRows) {
	[[cellList objectAt:index] setIntValue:anInt];
    }
    return self;
}


- (float)floatValueAt:(int)index
{
    if (index >= 0 && index < numRows) {
	return[[cellList objectAt:index] floatValue];
    } else {
	return 0.0;
    }
}


- setFloatValue:(float)aFloat at:(int)index
{
    if (index >= 0 && index < numRows) {
	[[cellList objectAt:index] setFloatValue:aFloat];
    }
    return self;
}


- (double)doubleValueAt:(int)index
{
    if (index >= 0 && index < numRows) {
	return[[cellList objectAt:index] doubleValue];
    } else {
	return 0.0;
    }
}


- setDoubleValue:(double)aDouble at:(int)index
{
    if (index >= 0 && index < numRows) {
	[[cellList objectAt:index] setDoubleValue:aDouble];
    }
    return self;
}


- selectTextAt:(int)index
{
    return[self selectTextAt:index :0];
}


@end

/*
  
Modifications (since 0.8):
  
12/5/88  pah	add factory method setCellClass:
12/13/88 bgy	converted to the List object;
12/14/88 pah	changed matrixdisplay to _NXMatrixDisplay
 1/25/89 trey	setTitle: takes a const param
		made setString* methods take const params
 2/14/89 pah	fixed sizeToFit to use any width set with setEntryWidth:
 2/18/89 pah	more char * -> const char * changes

0.93
----
 6/15/89 pah	eliminate redundant autodisplaying since ActionCells do it now
		make titleAt: and stringValueAt: return const char *

77
--
 2/6/90	aozer	fixed titleAt: to return title and not stringValue

*/

/*
	Matrix.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"
#import "color.h"

/* Matrix Constants */

#define NX_RADIOMODE		0
#define NX_HIGHLIGHTMODE	1
#define NX_LISTMODE		2
#define NX_TRACKMODE		3

@interface Matrix : Control
{
    id                  cellList;
    id                  target;
    SEL                 action;
    id                  selectedCell;
    int                 selectedRow;
    int                 selectedCol;
    int                 numRows;
    int                 numCols;
    NXSize              cellSize;
    NXSize              intercell;
    float               backgroundGray;
    float               cellBackgroundGray;
    id                  font;
    id                  protoCell;
    id                  cellClass;
    id                  nextText;
    id                  previousText;
    SEL                 doubleAction;
    SEL                 errorAction;
    id                  textDelegate;
    struct _mFlags {
	unsigned int        highlightMode:1;
	unsigned int        radioMode:1;
	unsigned int        listMode:1;
	unsigned int        allowEmptySel:1;
	unsigned int        autoscroll:1;
	unsigned int        reaction:1;
	unsigned int        _RESERVED:8;
	unsigned int        _autosizeCells:1;
	unsigned int        _drawingAncestor:1;
    }                   mFlags;
    unsigned short      _reservedMshort1;
    unsigned int        _reservedMint1;
    void	        *_private;
}

+ initialize;
+ setCellClass:factoryId;

- initFrame:(const NXRect *)frameRect;
- initFrame:(const NXRect *)frameRect mode:(int)aMode prototype:aCell numRows:(int)rowsHigh numCols:(int)colsWide;
- initFrame:(const NXRect *)frameRect mode:(int)aMode cellClass:factoryId numRows:(int)rowsHigh numCols:(int)colsWide;

- free;
- setCellClass:factoryId;
- prototype;
- setPrototype:aCell;
- makeCellAt:(int)row :(int)col;
- setMode:(int)aMode;
- allowEmptySel:(BOOL)flag;
- sendAction:(SEL)aSelector to:anObject forAllCells:(BOOL)flag;
- cellList;
- selectedCell;
- (int)selectedRow;
- (int)selectedCol;
- clearSelectedCell;
- selectCellAt:(int)row :(int)col;
- selectAll:sender;
- selectCell:aCell;
- selectCellWithTag:(int)anInt;
- getCellSize:(NXSize *)theSize;
- setCellSize:(const NXSize *)aSize;
- getIntercell:(NXSize *)theSize;
- setIntercell:(const NXSize *)aSize;
- setEnabled:(BOOL)flag;
- setScrollable:(BOOL)flag;
- font;
- setFont:fontObj;
- (float)backgroundGray;
- setBackgroundGray:(float)value;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- setBackgroundTransparent:(BOOL)flag;
- (BOOL)isBackgroundTransparent;
- setCellBackgroundColor:(NXColor)color;
- (NXColor)cellBackgroundColor;
- setCellBackgroundTransparent:(BOOL)flag;
- (BOOL)isCellBackgroundTransparent;
- (float)cellBackgroundGray;
- setCellBackgroundGray:(float)value;
- setState:(int)value at:(int)row :(int)col;
- setIcon:(const char *)iconName at:(int)row :(int)col;
- setTitle:(const char *)aString at:(int)row :(int)col;
- (int)cellCount;
- getNumRows:(int *)rowCount numCols:(int *)colCount;
- cellAt:(int)row :(int)col;
- getCellFrame:(NXRect *)theRect at:(int)row :(int)col;
- getRow:(int *)row andCol:(int *)col ofCell:aCell;
- getRow:(int *)row andCol:(int *)col forPoint:(const NXPoint *)aPoint;
- renewRows:(int)newRows cols:(int)newCols;
- putCell:newCell at:(int)row :(int)col;
- addRow;
- insertRowAt:(int)row;
- removeRowAt:(int)row andFree:(BOOL)flag;
- addCol;
- insertColAt:(int)col;
- removeColAt:(int)col andFree:(BOOL)flag;
- findCellWithTag:(int)anInt;
- setTag:(int)anInt at:(int)row :(int)col;
- target;
- setTarget:anObject;
- setTarget:anObject at:(int)row :(int)col;
- (SEL)action;
- setAction:(SEL)aSelector;
- (SEL)doubleAction;
- setDoubleAction:(SEL)aSelector;
- (SEL)errorAction;
- setErrorAction:(SEL)aSelector;
- setAction:(SEL)aSelector at:(int)row :(int)col;
- setTag:(int)anInt target:anObject action:(SEL)aSelector at:(int)row :(int)col;
- setAutosizeCells:(BOOL)flag;
- (BOOL)doesAutosizeCells;
- sizeTo:(float)width :(float)height;
- sizeToCells;
- sizeToFit;
- validateSize:(BOOL)flag;
- calcSize;
- drawCell:aCell;
- drawCellInside:aCell;
- drawCellAt:(int)row :(int)col;
- highlightCellAt:(int)row :(int)col lit:(BOOL)flag;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- display;
- setAutoscroll:(BOOL)flag;
- scrollCellToVisible:(int)row :(int)col;
- setReaction:(BOOL)flag;
- (int)mouseDownFlags;
- mouseDown:(NXEvent *)theEvent;
- (BOOL)performKeyEquivalent:(NXEvent *)theEvent;
- sendAction:(SEL)theAction to:theTarget;
- sendAction;
- sendDoubleAction;
- textDelegate;
- setTextDelegate:anObject;
- (BOOL)textWillEnd:textObject;
- (BOOL)textWillChange:textObject;
- textDidEnd:textObject endChar:(unsigned short)whyEnd;
- textDidChange:textObject;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- selectText:sender;
- selectTextAt:(int)row :(int)col;
- setPreviousText:anObject;
- setNextText:anObject;
- (BOOL)acceptsFirstMouse;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
- resetCursorRects;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;
+ newFrame:(const NXRect *)frameRect mode:(int)aMode prototype:aCell numRows:(int)rowsHigh numCols:(int)colsWide;
+ newFrame:(const NXRect *)frameRect mode:(int)aMode cellClass:factoryId numRows:(int)rowsHigh numCols:(int)colsWide;


@end

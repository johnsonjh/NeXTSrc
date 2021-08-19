/*
	NXBrowser.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

@interface NXBrowser : Control
{
    id                  target;
    id                  delegate;
    id			_reserved1[3];
    SEL                 action;
    SEL                 doubleAction;
    id                  matrixClass;
    id                  cellPrototype;
    NXSize		_reserved2;
    short		_reserved4[4];
    unsigned short      pathSeparator;
    char		_reserved3[6];
    void               *_private;
}

- initFrame:(const NXRect *)frameRect;

- free;
- (SEL)action;
- setAction:(SEL)aSelector;
- target;
- setTarget:anObject;
- (SEL)doubleAction;
- setDoubleAction:(SEL)aSelector;
- setMatrixClass:factoryId;
- setCellClass:factoryId;
- cellPrototype;
- setCellPrototype:aCell;
- delegate;
- setDelegate:anObject;
- allowMultiSel:(BOOL)flag;
- allowBranchSel:(BOOL)flag;
- reuseColumns:(BOOL)flag;
- hideLeftAndRightScrollButtons:(BOOL)flag;
- separateColumns:(BOOL)flag;
- (BOOL)columnsAreSeparated;
- useScrollButtons:(BOOL)flag;
- useScrollBars:(BOOL)flag;
- acceptArrowKeys:(BOOL)flag;
- getTitleFromPreviousColumn:(BOOL)flag;
- (BOOL)isTitled;
- setTitled:(BOOL)flag;
- setEnabled:(BOOL)flag;
- (NXRect *)getTitleFrame:(NXRect *)theRect ofColumn:(int)column;
- setTitle:(const char *)aString ofColumn:(int)column;
- (const char *)titleOfColumn:(int)column;
- loadColumnZero;
- setPathSeparator:(unsigned short)charCode;
- setPath:(const char *)path;
- (char *)getPath:(char *)thePath toColumn:(int)column;
- displayColumn:(int)column;
- reloadColumn:(int)column;
- validateVisibleColumns;
- displayAllColumns;
- scrollColumnsRightBy:(int)shiftAmount;
- scrollColumnsLeftBy:(int)shiftAmount;
- scrollColumnToVisible:(int)column;
- setLastColumn:(int)column;
- addColumn;
- setMinColumnWidth:(int)columnWidth;
- (int)minColumnWidth;
- setMaxVisibleColumns:(int)columnCount;
- (int)maxVisibleColumns;
- (int)numVisibleColumns;
- (int)firstVisibleColumn;
- (int)lastVisibleColumn;
- (int)lastColumn;
- (int)selectedColumn;
- (BOOL)isLoaded;
- (int)columnOf:matrix;
- matrixInColumn:(int)column;
- getLoadedCellAtRow:(int)row inColumn:(int)col;
- (NXRect *)getFrame:(NXRect *)theRect ofColumn:(int)column;
- (NXRect *)getFrame:(NXRect *)theRect ofInsideOfColumn:(int)column;
- drawTitle:(const char *)title inRect:(const NXRect *)aRect ofColumn:(int)column;
- clearTitleInRect:(const NXRect *)aRect ofColumn:(int)column;
- (NXCoord)titleHeight;
- tile;
- drawSelf:(const NXRect *)rects :(int)rectCount;
- mouseDown:(NXEvent *)theEvent;
- keyDown:(NXEvent *)theEvent;
- (BOOL)acceptsFirstResponder;
- sizeTo:(NXCoord)width :(NXCoord)height;
- sizeToFit;
- selectAll:sender;
- doClick:sender;
- doDoubleClick:sender;
- scrollUpOrDown:sender;
- reflectScroll:clipView;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

@interface Object(BrowserDelegate)
- (int)browser:sender getNumRowsInColumn:(int)column;
- (int)browser:sender fillMatrix:matrix inColumn:(int)column;
- browser:sender loadCell:cell atRow:(int)row inColumn:(int)column;
- (const char *)browser:sender titleOfColumn:(int)column;
- (BOOL)browser:sender selectCell:(const char *)title inColumn:(int)column;
- (BOOL)browser:sender columnIsValid:(int)column;
- browserWillScroll:sender;
- browserDidScroll:sender;
@end

/*
	Matrix.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Matrix_Private.h"
#import "Control_Private.h"
#import "Cell_Private.h"
#import "Menu_Private.h"
#import "Window.h"
#import "ActionCell.h"
#import "Application.h"
#import "ClipView.h"
#import "MenuCell.h"
#import "Text.h"
#import "timer.h"
#import "nextstd.h"
#import "publicWraps.h"
#import <dpsclient/wraps.h>
#import <objc/List.h>
#import <math.h>

typedef struct {
    @defs (Window)
}                  *windowId;

#define DFLTFONTSIZE 		12.0
#define DFLTFONTSTYLE 		0
#define DFLTCELLWIDTH		100.0
#define DFLTCELLHEIGHT		17.0

#define MATRIXTIMERINTERVAL	0.05
#define MATRIXTIMERDELAY	0.05

#define WMAX (numCols * (cellSize.width + intercell.width)) - intercell.width
#define HMAX (numRows * (cellSize.height + intercell.height))-intercell.height
#define GETCELLOFFSET(r,c)	((int)((c) + ((r) * numCols)))

#define NULLPOS (-1)
#define ROW(x) (x / numCols)
#define COL(x) (x % numCols)
#define VALID(r, c) (r >= 0 && r < numRows && c >= 0 && c < numCols)

#define CANDRAW (window && ((windowId)window)->windowNum > 0 && \
		 !(((windowId)window)->_displayDisabled))

static id           matrixCellClass = nil;
static int          mdFlags = 0;
static int drawingRow = -1;
static int drawingCol = -1;

/*
 * The structure that we hang off the end of Matrix.
 * Best way to refer to this is through the private macro below.
 *
 * The private field is allocated lazily.
 * Because each NXColor is 16 bytes, it makes sense
 * to put both in the private field rather than pointers (otherwise we would
 * allocate 3 16-byte chunks). If we extend the private field some day, we
 * might want to make the NXColor items pointers.
 */
typedef struct _MatrixPrivate {
    NXColor cellBackgroundColor;
    NXColor backgroundColor;
} MatrixPrivate;

#define private ((MatrixPrivate *)_private)

#define ALLOCPRIVATE \
	if (!private) { \
	    NXZone *zone = [self zone]; \
	    NX_ZONEMALLOC(zone, private, MatrixPrivate, 1); \
	    private->cellBackgroundColor = private->backgroundColor = _NXNoColor(); \
	}

@implementation Matrix:Control

+ initialize
{
    [self setVersion:2];
    return self;
}

+ setCellClass:factoryId
{
    matrixCellClass = factoryId;
    return self;
}

- _setSelectedCell:aCell
{
    selectedCell = aCell;
    [self getRow:&selectedRow andCol:&selectedCol ofCell:aCell];
    return self;
}


- _initialize:(int)aMode :(int)rowsHigh :(int)colsWide
{
    int row, col;

    [self setFlipped:YES];
    cellList = [[List allocFromZone:[self zone]] initCount:0];
    intercell.width = 1.0;
    intercell.height = 1.0;
    backgroundGray = -1.0;
    cellBackgroundGray = -1.0;
    selectedRow = selectedCol = -1;
    vFlags.opaque = NO;
    [self setMode:aMode];
    numRows = rowsHigh;
    numCols = colsWide;
    for (row = 0; row < numRows; row++) {
	for (col = 0; col < numCols; col++) {
	    [cellList addObject:[self makeCellAt:row :col]];
	}
    }
    if (mFlags.radioMode && rowsHigh > 0 && colsWide > 0) {
	[self selectCellAt:0 :0];
    }
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }
    conFlags.calcSize = YES;

    return self;
}

+ newFrame:(const NXRect *)frameRect
{
    if (!matrixCellClass) matrixCellClass = [ActionCell class];
    return [self newFrame:frameRect
		mode:NX_RADIOMODE
		cellClass:matrixCellClass
		numRows:0
		numCols:0];
}

- initFrame:(const NXRect *)frameRect
{
    if (!matrixCellClass) matrixCellClass = [ActionCell class];
    return [self initFrame:frameRect
		mode:NX_RADIOMODE
		cellClass:matrixCellClass
		numRows:0
		numCols:0];
}


+ newFrame:(const NXRect *)frameRect
    mode:(int)aMode
    prototype:aCell
    numRows:(int)rowsHigh
    numCols:(int)colsWide
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect 
	mode:aMode prototype:aCell numRows:rowsHigh numCols:colsWide];
}

- initFrame:(const NXRect *)frameRect
    mode:(int)aMode
    prototype:aCell
    numRows:(int)rowsHigh
    numCols:(int)colsWide
{
    if (!matrixCellClass) matrixCellClass = [ActionCell class];
    [super initFrame:frameRect];
    if (aCell) {
	protoCell = aCell;
    } else {
	if (_NXCanAllocInZone(matrixCellClass, @selector(new), @selector(init))) {
	    protoCell = [[matrixCellClass allocFromZone:[self zone]] init];
	} else {
	    protoCell = [matrixCellClass new];
	}
    }
    [protoCell calcCellSize:&cellSize];
    [self _initialize:aMode :rowsHigh :colsWide];
    return self;
}


+ newFrame:(const NXRect *)frameRect
    mode:(int)aMode
    cellClass:factoryId
    numRows:(int)rowsHigh
    numCols:(int)colsWide
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect
	mode:aMode cellClass:factoryId numRows:rowsHigh numCols:colsWide];
}

- initFrame:(const NXRect *)frameRect
    mode:(int)aMode
    cellClass:factoryId
    numRows:(int)rowsHigh
    numCols:(int)colsWide
{
    if (!matrixCellClass) {
	matrixCellClass = [ActionCell class];
    }
    [super initFrame:frameRect];
    if (factoryId) {
	cellClass = factoryId;
    } else {
	cellClass = matrixCellClass;
    }
    cellSize.width = DFLTCELLWIDTH;
    cellSize.height = DFLTCELLHEIGHT;
    [self _initialize:aMode :rowsHigh :colsWide];
    return self;
}


- free
{
    if (private) {
	NX_FREE(private);
    }
    [cellList freeObjects];
    [cellList free];
    [protoCell free];
    cell = nil;			/* so Control doesnt free a second time */
    return[super free];
}


- setCellClass:factoryId
{
    cellClass = factoryId;
    return self;
}


- prototype
{
    return protoCell;
}


- setPrototype:aCell
{
    id oldCell;

    oldCell = protoCell;
    protoCell = aCell;
    if (font) [protoCell setFont:font];

    return oldCell;
}


- makeCellAt:(int)row :(int)col
{
    id aCell;

    if (protoCell) {
	aCell = [protoCell copy];
    } else {
	if (_NXCanAllocInZone(cellClass, @selector(new), @selector(init))) {
	    aCell = [[cellClass allocFromZone:[self zone]] init];
	} else {
	    aCell = [cellClass new];
	}
	if (font) [aCell setFont:font];
    }

    return aCell;
}


- setMode:(int)aMode
{
    int row, col;
    BOOL canDraw, updateRadio;

    updateRadio = (!mFlags.radioMode && aMode == NX_RADIOMODE);
    mFlags.radioMode = mFlags.listMode = mFlags.highlightMode = NO;
    switch (aMode) {
    case NX_RADIOMODE:
	mFlags.radioMode = YES;
	if (updateRadio) {
	    canDraw = [self canDraw] && !vFlags.disableAutodisplay;
	    if (canDraw) {
		[self lockFocus];
	    } else {
		vFlags.needsDisplay = YES;
	    }
	    [self _findFirstOne:&row:&col];
	    [self _turnOffAllExcept:row :col andDraw:canDraw];
	    if (canDraw) [self unlockFocus];
	}
	break;
    case NX_LISTMODE:
	mFlags.listMode = YES;
	break;
    case NX_HIGHLIGHTMODE:
	mFlags.highlightMode = YES;
	break;
    }

    return self;
}


- allowEmptySel:(BOOL)flag
{
    mFlags.allowEmptySel = (flag ? YES : NO);
    return self;
}


- sendAction:(SEL)aSelector to:anObject forAllCells:(BOOL)flag
{
    id aCell;
    int i = 0;
    int numEntries = numRows * numCols;

    _NXCheckSelector(aSelector, "[Matrix -sendAction:to:forAllCells:]");
    while (i < numEntries) {
	aCell = [cellList objectAt:i++];
	if (flag || [aCell isHighlighted]) {
	    if (![anObject perform:aSelector with:aCell]) {
		return self;
	    }
	}
    }

    return self;
}


- cellList
{
    return cellList;
}


- selectedCell
{
    return selectedCell;
}
	
- (int)selectedRow
{
    return selectedRow;
}


- (int)selectedCol
{
    return selectedCol;
}


- _clearSelectedCell
{
    selectedCell = nil;
    selectedRow = -1;
    selectedCol = -1;
    return self;
}

- clearSelectedCell
{
    int i;
    id aCell, oldSelectedCell = selectedCell;

    if (!mFlags.radioMode || mFlags.allowEmptySel) {
	selectedCell = nil;
	selectedRow = -1;
	selectedCol = -1;
	for (i = 0; i < numRows * numCols; i++) {
	    aCell = [cellList objectAt:i];
	    if (aCell) {
		[aCell setState:0];
		_NXSetCellParam(aCell, NX_CELLHIGHLIGHTED, 0);
	    }
	}
    }

    return oldSelectedCell;
}


- selectCellAt:(int)row :(int)col
{
    id lastSelectedCell = nil;
    BOOL updateFlag, focusLocked = NO;
    int offset, lastSelectedRow = -1, lastSelectedCol = -1;

    if (VALID(row, col)) {
	offset = GETCELLOFFSET(row, col);
    } else {
	if (row == -1 || col == -1) {
	    offset = -1;
	} else {
	    return self;
	}
    }

    updateFlag = CANDRAW;

    if (offset >= 0 &&
        [[cellList objectAt:offset] isSelectable] &&
	![window makeFirstResponder:self]) {
	return self;
    }
    if (mFlags.radioMode) {
	if (!mFlags.allowEmptySel && offset < 0)
	    return self;
	lastSelectedCell = selectedCell;
	lastSelectedRow = selectedRow;
	lastSelectedCol = selectedCol;
    }
    selectedRow = row;
    selectedCol = col;
    selectedCell = (offset >= 0) ?[cellList objectAt : offset]:nil;

    [window disableFlushWindow];

    if (mFlags.listMode) {
	if (updateFlag) {
	    if (!focusLocked) {
		[self lockFocus];
		focusLocked = YES;
	    }
	} else {
	    vFlags.needsDisplay = YES;
	}
	[self _turnOffAllExcept:row :col andDraw:updateFlag];
	if (updateFlag) [window flushWindow];
	if (selectedCell) {
	    _NXSetCellParam(selectedCell, NX_CELLHIGHLIGHTED,
			    !_NXGetCellParam(selectedCell, NX_CELLEDITABLE));
	}
    } else if (mFlags.radioMode) {
	if (lastSelectedCell && selectedCell != lastSelectedCell &&
	    !_NXGetCellParam(lastSelectedCell, NX_CELLEDITABLE)) {
	    [lastSelectedCell setState:0];
	    _NXSetCellParam(lastSelectedCell, NX_CELLHIGHLIGHTED, 0);
	    if (updateFlag) {
		if (!focusLocked) {
		    [self lockFocus];
		    focusLocked = YES;
		}
		[self drawCellAt:lastSelectedRow :lastSelectedCol];
	    } else {
		vFlags.needsDisplay = YES;
	    }
	}
    }
    if ([selectedCell isSelectable]) {
	[window endEditingFor:self];
	[self selectTextAt:row :col];
    }
    if (selectedCell && selectedCell != lastSelectedCell) {
	[selectedCell setState:1];
	if (updateFlag) {
	    [self drawCellAt:selectedRow :selectedCol];
	} else {
	    vFlags.needsDisplay = YES;
	}
    }

    [window reenableFlushWindow];
    if (updateFlag) [window flushWindow];
    if (focusLocked) [self unlockFocus];

    return self;
}


- selectAll:sender
{
    int i;
    id aCell;

    if (!mFlags.radioMode) {
	for (i = numRows * numCols - 1; i >= 0; i--) {
	    aCell = [cellList objectAt:i];
	    if (![aCell isSelectable]) {
		[aCell setState:1];
		if (aCell) _NXSetCellParam(aCell, NX_CELLHIGHLIGHTED, YES);
	    }
	}
	[self display];
    }

    return self;
}


- selectCell:aCell
{
    int row, col;

    if ([self getRow:&row andCol:&col ofCell:aCell]) {
	[self selectCellAt:row :col];
	return aCell;
    }

    return nil;
}


- selectCellWithTag:(int)anInt
{
    int i;
    id aCell;

    i = (numRows * numCols);
    while (i) {
	aCell = [cellList objectAt:--i];
	if ([aCell tag] == anInt) {
	    return[self selectCellAt:ROW(i) :COL(i)];
	}
    }

    return nil;
}


- getCellSize:(NXSize *)theSize
{
    *theSize = cellSize;
    return self;
}


- setCellSize:(const NXSize *)aSize
{
    cellSize = *aSize;
    conFlags.calcSize = YES;
    return self;
}


- getIntercell:(NXSize *)theSize
{
    *theSize = intercell;
    return self;
}


- setIntercell:(const NXSize *)aSize
{
    intercell = *aSize;
    return self;
}


- setEnabled:(BOOL)flag
{
    int i;
    id aCell;
    BOOL found = NO;

    if (!flag && [selectedCell isSelectable]) {
	[self abortEditing];
    }
    i = (numRows * numCols);
    while (i) {
	aCell = [cellList objectAt:--i];
	if ([aCell isEnabled] != flag) {
	    [aCell setEnabled:flag];
	    found = YES;
	}
    }
    [protoCell setEnabled:flag];
    conFlags.enabled = flag ? YES : NO;

    return self;
}


- setScrollable:(BOOL)flag
{
    int i;
    id aCell;
    BOOL found = NO;

    if (!flag && [selectedCell isSelectable]) {
	[self abortEditing];
    }
    i = (numRows * numCols);
    while (i) {
	aCell = [cellList objectAt:--i];
	if ([aCell isScrollable] != flag) {
	    [aCell setScrollable:flag];
	    found = YES;
	}
    }
    [protoCell setScrollable:flag];

    return self;
}


- font
{
    return font;
}


- setFont:fontObj
{
    int i;
    id aCell;
    BOOL found = NO;

    [window reenableDisplay];
    i = (numRows * numCols);
    while (i) {
	aCell = [cellList objectAt:--i];
	if ([aCell font] != fontObj) {
	    [aCell setFont:fontObj];
	    found = YES;
	}
    }
    [protoCell setFont:fontObj];
    [window reenableDisplay];
    font = fontObj;
    conFlags.calcSize = YES;
    if (found) [self update];

    return self;
}


- (float)backgroundGray
{
    return backgroundGray;
}


- setBackgroundGray:(float)value
{
    value = (value < 0.0 ? -1.0 : MAX(MIN(value,1.0),0.0));
    if (backgroundGray != value) {
	backgroundGray = value;
	if ((value < 0.0) && private) {
	    private->backgroundColor = _NXNoColor();
	}
	if (backgroundGray >= 0.0) {
	    vFlags.opaque = YES;
	} else {
	    vFlags.opaque = NO;
	}
	[self update];
    }
    return self;
}


- (float)cellBackgroundGray
{
    return cellBackgroundGray;
}


- setCellBackgroundGray:(float)value
{
    value = (value < 0.0 ? -1.0 : MAX(MIN(value,1.0),0.0));
    if (cellBackgroundGray != value) {
	cellBackgroundGray = value;
	if ((value < 0.0) && private) {
	    private->cellBackgroundColor = _NXNoColor();
	}
	if (value >= 0.0) {
	    [self update];
	}
    }
    return self;
}

-(BOOL)_backColorSpecified
{
    return (private && _NXIsValidColor(private->backgroundColor));
}

-(BOOL)_cellColorSpecified
{
    return (private && _NXIsValidColor(private->cellBackgroundColor));
}

- setBackgroundColor:(NXColor)color
{
    ALLOCPRIVATE;
    private->backgroundColor = color;
    if (backgroundGray < 0.0) {
	backgroundGray = NXGrayComponent(color); 
    }
    return self;
}

- (NXColor)backgroundColor
{
    if ([self _backColorSpecified]) {
	return private->backgroundColor;
    } else if (backgroundGray < 0.0) {
	return NX_COLORCLEAR;	/* What else can we return? */
    } else {
	return NXConvertGrayToColor(backgroundGray);
    }
}

- setBackgroundTransparent:(BOOL)flag 
{
    if (flag) {
	[self setBackgroundGray:-1.0];
    } else {
	[self setBackgroundGray:NX_WHITE];	/* ??? */
    }
    return self;
}

- (BOOL)isBackgroundTransparent
{
    return (backgroundGray < 0.0);
}

- setCellBackgroundColor:(NXColor)color
{
    ALLOCPRIVATE;
    private->cellBackgroundColor = color;
    if (cellBackgroundGray < 0.0) {
	cellBackgroundGray = NXGrayComponent(color); 
    }
    return self;
}

- (NXColor)cellBackgroundColor
{
    if ([self _cellColorSpecified]) {
	return private->cellBackgroundColor;
    } else if (cellBackgroundGray < 0.0) {
	return NX_COLORCLEAR;	/* What else can we return? */
    } else {
	return NXConvertGrayToColor(cellBackgroundGray);
    }
}

- setCellBackgroundTransparent:(BOOL)flag
{
    if (flag) {
	[self setCellBackgroundGray:-1.0];
    } else {
	[self setCellBackgroundGray:NX_WHITE];
    }
    return self;
}

- (BOOL)isCellBackgroundTransparent
{
    return (cellBackgroundGray < 0.0);
}

- setState:(int)value at:(int)row :(int)col
{
    id theCell;

    if (!VALID(row, col)) {
	return self;
    }
    if (mFlags.radioMode) {
	if (value) {
	    [self selectCellAt:row :col];
	    [selectedCell setState:value];
	} else if (row == selectedRow &&
	    col == selectedCol &&
	    mFlags.allowEmptySel) {
	    [self selectCellAt:-1:-1];
	}
	return self;
    }
    theCell = [cellList objectAt:GETCELLOFFSET(row, col)];
    if ([theCell state] != value) {
	[theCell setState:value];
	[self updateCell:theCell];
    }

    return self;
}


- setIcon:(const char *)iconName at:(int)row :(int)col
{
    id theCell;

    if (VALID(row, col)) {
	theCell = [cellList objectAt:GETCELLOFFSET(row, col)];
	[theCell setIcon:iconName];
    }

    return self;
}


- setTitle:(const char *)aString at:(int)row :(int)col
{
    id theCell;

    if (VALID(row, col)) {
	theCell = [cellList objectAt:GETCELLOFFSET(row, col)];
	[window disableDisplay];
	[theCell setTitle:aString];
	[window reenableDisplay];
	conFlags.calcSize = YES;
	[self update];
    }

    return self;
}


- (int)cellCount
{
    return (numRows * numCols);
}


- getNumRows:(int *)rowCount numCols:(int *)colCount
{
    if (rowCount) *rowCount = numRows;
    if (colCount) *colCount = numCols;
    return self;
}


- cellAt:(int)row :(int)col
{
    return (VALID(row, col) ?[cellList objectAt : GETCELLOFFSET(row, col)] :nil);
}


- getCellFrame:(NXRect *)theRect at:(int)row :(int)col
{
    theRect->size = cellSize;
    theRect->origin.x = bounds.origin.x
      + (col * (intercell.width + cellSize.width));
    theRect->origin.y = bounds.origin.y
      + (row * (intercell.height + cellSize.height));
    return self;
}


- _getDrawingRow:(int *)row andCol:(int *)col
{
    *row = drawingRow;
    *col = drawingCol;
    return self;
}

- getRow:(int *)row andCol:(int *)col ofCell:aCell
{
    int i = numRows * numCols;

    while (i && [cellList objectAt:--i] != aCell);
    if (i >= 0) {
	*row = ROW(i);
	*col = COL(i);
	return aCell;
    } else {
	*row = -1;
	*col = -1;
	return nil;
    }
}


- getRow:(int *)row andCol:(int *)col forPoint:(const NXPoint *)aPoint
{
    if (aPoint->y < 0.0 || aPoint->x < 0.0 ||
	aPoint->y >= HMAX || aPoint->x >= WMAX) {
	return nil;
    }
    *row = aPoint->y / (intercell.height + cellSize.height);
    if ((*row) * (intercell.height + cellSize.height) + cellSize.height <
	aPoint->y) {
	return nil;
    }
    *col = aPoint->x / (intercell.width + cellSize.width);
    if ((*col) * (intercell.width + cellSize.width) + cellSize.width <
	aPoint->x) {
	return nil;
    }
    return[cellList objectAt:GETCELLOFFSET(*row, *col)];
}


- renewRows:(int)newRows cols:(int)newCols
{
    id aCell;
    int oldSize, newSize;

    oldSize = [cellList count];
    numRows = newRows;
    numCols = newCols;
    newSize = numRows * numCols;
    if (newSize > oldSize) {
	for (; oldSize < newSize; oldSize++) {
	    aCell = [self makeCellAt:oldSize/newRows:oldSize%newRows];
	    [cellList insertObject:aCell at:oldSize];
	}
    }
    if (selectedCell) {
	_NXSetCellParam(selectedCell, NX_CELLHIGHLIGHTED, 0);
	[selectedCell setState:0];	
    }
    selectedRow = selectedCol = -1;
    selectedCell = nil;
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }

    return self;
}


- putCell:newCell at:(int)row :(int)col
{
    id theCell = nil;

    if (VALID(row, col)) {
	theCell = [cellList replaceObjectAt:GETCELLOFFSET(row, col) with:newCell];
	conFlags.calcSize = YES;
	[self updateCell:newCell];
	if (row == col &&  row == 0) {
	    cell = newCell;
	}
    }

    return theCell;
}


- addRow
{
    return[self insertRowAt:numRows];
}


- insertRowAt:(int)row
{
    id aCell;
    int offset, startOffset, endOffset, col, count;

    if (!numCols) {
	numCols = 1;
    }
    startOffset = GETCELLOFFSET(row, 0);
    endOffset = startOffset + numCols;
    col = 0;
    count = [cellList count];
    for (offset = startOffset; offset < endOffset; offset++) {
	if (offset < numRows * numCols || offset >= count) {
	    aCell = [self makeCellAt:row :col];
	    [cellList insertObject:aCell at:offset];
	}
	col++;
    }
    conFlags.calcSize = YES;
    numRows++;
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }

    return self;
}


- removeRowAt:(int)row andFree:(BOOL)flag
{
    id aCell;
    int startOffset, i;

    if (row == selectedRow) {
	[self abortEditing];
	selectedCell = nil;
	selectedRow = -1;
	selectedCol = -1;
    } else if (row < selectedRow) {
	selectedRow--;
    }
    startOffset = GETCELLOFFSET(row, 0);
    for (i = 0; i < numCols; i++) {
	aCell = [cellList removeObjectAt:startOffset];
	if (flag) {
	    [aCell free];
	}
    }
    numRows--;
    conFlags.calcSize = YES;
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }

    return self;
}


- addCol
{
    return[self insertColAt:numCols];
}


- insertColAt:(int)col
{
    id aCell;
    int row, offset;

    if (!numRows) numRows = 1;

    offset = GETCELLOFFSET(0, col);
    for (row = 0; row < numRows; row++) {
	aCell = [self makeCellAt:row :col];
	[cellList insertObject:aCell at:offset];
	offset += numCols + 1;
    }
    conFlags.calcSize = YES;
    numCols++;
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }

    return self;
}


- removeColAt:(int)col andFree:(BOOL)flag
{
    id aCell;
    int startOffset, i;

    if (col == selectedCol) {
	[self abortEditing];
	if (mFlags.radioMode && !mFlags.allowEmptySel &&
	    numRows > 0 && numCols > 1) {
	    selectedCell = [cellList objectAt:GETCELLOFFSET(0, 0)];
	    [selectedCell incrementState];
	    selectedRow = 0;
	    selectedCol = 0;
	} else {
	    selectedCell = nil;
	    selectedRow = -1;
	    selectedCol = -1;
	}
    } else if (col < selectedCol) {
	selectedCol--;
    }
    startOffset = GETCELLOFFSET(numRows - 1, col);
    for (i = 0; i < numRows; i++) {
	aCell = [cellList removeObjectAt:startOffset];
	if (flag) {
	    [aCell free];
	}
	startOffset -= numCols;
    }
    numCols--;
    conFlags.calcSize = YES;
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }

    return self;
}


- findCellWithTag:(int)anInt
{
    int i;
    id aCell;

    i = (numRows * numCols);
    while (i) {
	aCell = [cellList objectAt:--i];
	if ([aCell tag] == anInt) {
	    return aCell;
	}
    }

    return nil;
}


- setTag:(int)anInt at:(int)row :(int)col
{
    if (VALID(row, col)) {
	[[cellList objectAt:GETCELLOFFSET(row, col)] setTag:anInt];
    }
    return self;
}


- target
{
    return target;
}


- setTarget:anObject
{
    target = anObject;
    return self;
}


- setTarget:anObject at:(int)row :(int)col
{
    if (VALID(row, col)) {
	[[cellList objectAt:GETCELLOFFSET(row, col)] setTarget:anObject];
    }
    return self;
}


- (SEL)action
{
    return action;
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Matrix -setAction:]");
    action = aSelector;
    return self;
}


- (SEL)doubleAction
{
    return doubleAction;
}


- setDoubleAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Matrix -setDoubleAction:]");
    doubleAction = aSelector;
    if (aSelector) {
	conFlags.ignoreMultiClick = NO;
    }
    return self;
}


- (SEL)errorAction
{
    return errorAction;
}


- setErrorAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Control -setErrorAction:]");
    errorAction = aSelector;
    return self;
}



- setAction:(SEL)aSelector at:(int)row :(int)col
{
    if (VALID(row, col)) {
	[[cellList objectAt:GETCELLOFFSET(row, col)] setAction:aSelector];
    }
    return self;
}


- setTag:(int)anInt
    target:anObject
    action:(SEL)aSelector
    at:(int)row :(int)col
{
    id aCell;

    if (VALID(row, col)) {
	aCell = [cellList objectAt:GETCELLOFFSET(row, col)];
	[aCell setTag:anInt];
	[aCell setTarget:anObject];
	[aCell setAction:aSelector];
    }

    return self;
}


- setAutosizeCells:(BOOL)flag
{
    BOOL update = NO;

    if ((!flag && mFlags._autosizeCells) || (flag && !mFlags._autosizeCells)) {
	mFlags._autosizeCells = flag ? YES : NO;
	if (flag) {
	    if (numCols && cellSize.width != floor((bounds.size.width + intercell.width) / numCols) - intercell.width) {
		cellSize.width = floor((bounds.size.width + intercell.width) / numCols) - intercell.width;
		update = YES;
	    }
	    if (numRows && cellSize.height != floor((bounds.size.height + intercell.height) / numRows) - intercell.height) {
		cellSize.height = floor((bounds.size.height + intercell.height) / numRows) - intercell.height;
		update = YES;
	    }
	    if (update) [self update];
	}
    }

    return self;
}

- (BOOL)doesAutosizeCells
{
    return mFlags._autosizeCells;
}

- sizeTo:(float)width :(float)height
{
    BOOL editingAborted = NO;

    if (selectedCell && [self currentEditor]) {
	editingAborted = YES;
	[self abortEditing];
    }
    [super sizeTo:width :height];
    if (mFlags._autosizeCells) {
    	if (numCols) cellSize.width = floor((bounds.size.width + intercell.width) / numCols) - intercell.width;
	if (numRows) cellSize.height = floor((bounds.size.height + intercell.height) / numRows) - intercell.height;
    }
    if (editingAborted) {
	[self _makeEditable:selectedCell :selectedRow :selectedCol :NULL];
    }

    return self;
}

- sizeToCells
{
    return[self sizeTo:WMAX :HMAX];
}


- sizeToFit
{
    int i;
    NXCoord mx, my;
    NXSize maxSize, curSize;

    maxSize.width = maxSize.height = 0.0;
    i = (numRows * numCols);
    while (i) {
	[[cellList objectAt:--i] calcCellSize:&curSize];
	if (maxSize.width < curSize.width) {
	    maxSize.width = curSize.width;
	}
	if (maxSize.height < curSize.height) {
	    maxSize.height = curSize.height;
	}
    }
    cellSize = maxSize;
    conFlags.calcSize = YES;
    mx = WMAX;
    my = HMAX;
    if (mx < 0.0) mx = 0.0;
    if (my < 0.0) my = 0.0;

    return[self sizeTo:mx :my];
}


- validateSize:(BOOL)flag
{
    conFlags.calcSize = !flag;
    return self;
}


- calcSize
{
    id aCell;
    NXRect cellBounds;
    int row, col, i = 0;

    cellBounds.origin.y = bounds.origin.y;
    cellBounds.size = cellSize;
    for (row = 0; row < numRows; row++) {
	cellBounds.origin.x = bounds.origin.x;
	for (col = 0; col < numCols; col++) {
	    aCell = [cellList objectAt:i];
	    [aCell calcDrawInfo:&cellBounds];
	    cellBounds.origin.x += cellSize.width + intercell.width;
	    i++;
	}
	cellBounds.origin.y += cellSize.height + intercell.height;
    }
    conFlags.calcSize = NO;

    return self;
}


- drawCell:aCell
{
    int row, col;

    [self getRow:&row andCol:&col ofCell:aCell];
    if (VALID(row, col)) {
	return[self drawCellAt:row :col];
    } else {
	return self;
    }
}


- drawCellInside:aCell
{
    int row, col;

    [self getRow:&row andCol:&col ofCell:aCell];
    if (VALID(row, col)) {
	return[self _drawCellAt:row :col insideOnly:YES];
    } else {
	return self;
    }
}


- _drawCellAt:(int)row :(int)col insideOnly:(BOOL)insideOnly
{
    float bgg = -1.0;
    id aCell, fe = nil;
    NXRect cellBounds, editBounds, visibleRect;
    BOOL displayedFromOpaque = NO;

    if (!VALID(row, col)) return self;

    if (!CANDRAW) {
	vFlags.needsDisplay = YES;
	return self;
    }
    if (conFlags.calcSize) {
	[self calcSize];
    }
    if (mFlags.radioMode && !mFlags.allowEmptySel &&
	(selectedCell == nil || !VALID(selectedRow, selectedCol))) {
	[self selectCellAt:0 :0];
    }
    [self getVisibleRect:&visibleRect];
    [self getCellFrame:&cellBounds at:row :col];
    aCell = [cellList objectAt:GETCELLOFFSET(row, col)];
    if (aCell == selectedCell && [aCell isSelectable] && [aCell isEnabled]) {
	fe = [self currentEditor];
	if (fe) {
	    if ((bgg = [fe backgroundGray]) >= 0.0) {
		if ([[fe superview] isKindOf:[ClipView class]]) {
		    [[fe superview] getFrame:&editBounds];
		} else {
		    [fe getFrame:&editBounds];
		}
		if (!NXContainsRect(&cellBounds, &editBounds)) {
		    bgg = -1.0;
		}
	    }
	}
	if (bgg < 0.0) {
	    [self abortEditing];
	}
    }
    if (NXIntersectsRect(&visibleRect, &cellBounds)) {
	[window disableFlushWindow];
	[self lockFocus];
	if (![aCell isOpaque]) {
	    insideOnly = NO;
	    if (cellBackgroundGray >= 0.0) {
		if ([self _cellColorSpecified] && [self shouldDrawColor]) {
		    NXSetColor (private->cellBackgroundColor);
		} else {
		    PSsetgray (cellBackgroundGray);
		}
		NXRectFill(&cellBounds);
	    } else if (backgroundGray >= 0) {
		if ([self _backColorSpecified] && [self shouldDrawColor]) {
		    NXSetColor (private->backgroundColor);
		} else {
		    PSsetgray (backgroundGray);
		}
		NXRectFill(&cellBounds);
	    } else if (!vFlags.opaque) {
		if (!mFlags._drawingAncestor) {
		    displayedFromOpaque = YES;
		    mFlags._drawingAncestor = YES;
		    [self displayFromOpaqueAncestor:&cellBounds:1 :YES];
		    mFlags._drawingAncestor = NO;
		}
	    }
	    [window flushWindow];
	}
	if (!displayedFromOpaque) {
	    drawingRow = row; drawingCol = col;
	    if (insideOnly) {
		[aCell drawInside:&cellBounds inView:self];
	    } else {
		[aCell drawSelf:&cellBounds inView:self];
	    }
	    [window flushWindow];
	    drawingRow = -1; drawingCol = -1;
	}
	if (bgg >= 0.0) {
	    PSsetgray(bgg);
	    NXRectFill(&editBounds);
	    [fe display];
	    if (!fe) [window flushWindow];
	}
	[window reenableFlushWindow];
	[window flushWindow];
	[self unlockFocus];
    }

    return self;
}


- drawCellAt:(int)row :(int)col
{
    return[self _drawCellAt:row :col insideOnly:NO];
}


- _highlightCellAt:(int)row :(int)col lit:(BOOL)flag andDraw:(BOOL)draw
{
    id aCell;
    NXRect cellBounds, visibleRect;

    if (VALID(row, col)) {
	aCell = [cellList objectAt:GETCELLOFFSET(row, col)];
	if (draw) {
	    [self getCellFrame:&cellBounds at:row :col];
	    [self getVisibleRect:&visibleRect];
	    if (NXIntersectsRect(&visibleRect, &cellBounds)) {
		[window disableFlushWindow];
		[aCell highlight:&cellBounds inView:self lit:flag];
		[window reenableFlushWindow];
		[window flushWindow];
	    }
	}
	_NXSetCellParam(aCell, NX_CELLHIGHLIGHTED, flag);
    }

    return self;
}


- highlightCellAt:(int)row :(int)col lit:(BOOL)flag
{
    vFlags.needsDisplay = !CANDRAW;
    return[self _highlightCellAt:row :col lit:flag andDraw:CANDRAW];
}


static const NXRect nullRect = {{0.0, 0.0}, {0.0, 0.0}};

static BOOL mustClip(NXRect *rect, id self)
{
    NXRect              visibleRect;

    [self getVisibleRect:&visibleRect];
    if (NXEqualRect(rect, &visibleRect))
	return NO;
    if (NXIntersectionRect(&visibleRect, rect)) {
	return YES;
    } else {
	*rect = nullRect;
	return NO;
    }
}


- _replicate:(const NXRect *)cellBounds
{
    int i;
    NXRect srcRect;

    srcRect = *cellBounds;
    [self convertRect:&srcRect toView:nil];

    for (i = 2; i <= numRows; i++) {
	PScomposite(srcRect.origin.x, srcRect.origin.y,
		    srcRect.size.width, srcRect.size.height,
		    [window gState],
		    cellBounds->origin.x,
		    cellBounds->origin.y + i * cellBounds->size.height,
		    NX_COPY);
	[[cellList objectAt:i-1] _setView:self];
    }

    return self;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXRect	tRect, inBounds, _editBounds;
    NXRect     *cellBounds = &tRect;
    NXRect     *r = &inBounds;
    NXRect     *editBounds = &_editBounds;
    int		row, col;
    NXCoord	startX, incrX, incrY;
    BOOL	drawCellBack;
    int		firstCol, lastCol, firstRow, lastRow;
    id         *cellPtr;
    BOOL	clippingNecessary, replicate = NO;
    id		fe;
    float	bgg = -1.0;

    if (conFlags.calcSize) {
	[self calcSize];
    }
    if (numCols && numRows && mFlags.radioMode && !mFlags.allowEmptySel &&
	(selectedCell == nil || !VALID(selectedRow, selectedCol))) {
	[self selectCellAt:0 :0];
    }
    rects = rectCount ? rects : &bounds;
    if (!rectCount) {
	rectCount = 1;
    } else if (rectCount == 3) {
	rectCount--;
	rects++;
    }

/*
    if ([window isKindOf:[Menu class]] &&
	(!rects || NXEqualRect(rects, &bounds)) &&
	numCols == 1 && NXDrawingStatus == NX_DRAWING) {
	replicate = YES;
    }
*/

    while (rectCount && rects) {
	rectCount--;
	inBounds = *rects++;
	clippingNecessary = !vFlags.noClip && mustClip(r, self);
	if (clippingNecessary || !NXEqualRect(r, &nullRect)) {
	    if (backgroundGray >= 0.0) {
		if ([self _backColorSpecified] && [self shouldDrawColor]) {
		    NXSetColor (private->backgroundColor);
		} else {
		    PSsetgray (backgroundGray);
		}
		NXRectFill(r);
	    }
	    if (!numRows || !numCols) break;
	    if (clippingNecessary) {
		PSgsave();
		NXRectClip(r);
	    }
	    firstCol =
	      MIN(floor((NX_X(&inBounds) - bounds.origin.x)
			/ (cellSize.width + intercell.width)), numCols - 1);
	    lastCol =
	      MIN(ceil((NX_MAXX(&inBounds) - bounds.origin.x)
		       / (cellSize.width + intercell.width)), numCols);
	    firstRow =
	      MIN(floor((NX_Y(&inBounds) - bounds.origin.y)
		      / (cellSize.height + intercell.height)), numRows - 1);
	    lastRow =
	      MIN(ceil((NX_MAXY(&inBounds) - bounds.origin.y)
		       / (cellSize.height + intercell.height)), numRows);
	    cellBounds->size = cellSize;
	    drawCellBack = cellBackgroundGray >= 0.0 &&
	      ([self shouldDrawColor] ?
		(!NXEqualColor([self backgroundColor], [self cellBackgroundColor])) : 
		(cellBackgroundGray != backgroundGray));
	    incrX = intercell.width + cellSize.width;
	    incrY = intercell.height + cellSize.height;
	    startX = bounds.origin.x + (firstCol * incrX);
	    cellBounds->origin.y = bounds.origin.y + (firstRow * incrY);
	    cellPtr = (id *)NX_ADDRESS(cellList);
	    cellPtr += firstRow * numCols + firstCol;
	    [window disableFlushWindow];
	    for (row = firstRow; row < lastRow; row++) {
		cellBounds->origin.x = startX;
		for (col = firstCol; col < lastCol; col++) {
		    if (selectedCell == *cellPtr && [selectedCell isSelectable] && [selectedCell isEnabled]) {
			fe = [self currentEditor];
			if (fe) {
			    if ((bgg = [fe backgroundGray]) >= 0.0) {
				if ([[fe superview] isKindOf:[ClipView class]]) {
				    [[fe superview] getFrame:editBounds];
				} else {
				    [fe getFrame:editBounds];
				}
				if (!NXContainsRect(cellBounds, editBounds)) {
				    bgg = -1.0;
				}
			    }
			}
			if (bgg < 0.0) {
			    [self abortEditing];
			}
		    }
		    if (drawCellBack) {
			if ([self _cellColorSpecified] && [self shouldDrawColor]) {
			    NXSetColor (private->cellBackgroundColor);
			} else {
			    PSsetgray (cellBackgroundGray);
			}
			NXRectFill(cellBounds);
		    }
		    drawingRow = row; drawingCol = col;
		    if (replicate) {
			if (row == firstRow && col == firstCol) {
			    [*cellPtr drawSelf:cellBounds inView:self];
			    [self _replicate:cellBounds];
			} else {
			    [*cellPtr drawInside:cellBounds inView:self];
			}
		    } else {
			[*cellPtr drawSelf:cellBounds inView:self];
		    }
		    drawingRow = -1; drawingCol = -1;
		    if (bgg >= 0.0) {
			PSsetgray(bgg);
			NXRectFill(editBounds);
		    }
		    bgg = -1.0;
		    cellBounds->origin.x += incrX;
		    cellPtr++;
		}
		cellBounds->origin.y += incrY;
		cellPtr += numCols - (lastCol - firstCol);
	    }
	    [window reenableFlushWindow];
	    if (clippingNecessary)
		PSgrestore();
	}
    }
    return self;
}


static BOOL allCellsAreOpaque(Matrix *self)
{
    int count = [self->cellList count];
    id *cellPtr = (id *)NX_ADDRESS(self->cellList);

    for (; count--; cellPtr++) {
	if (![*cellPtr isOpaque])
	    return NO;
    }

    return YES;
}


- display
{
    if (vFlags.opaque || backgroundGray >= 0.0 ||
	(!intercell.width && !intercell.height &&
	 (cellBackgroundGray >= 0.0 || allCellsAreOpaque(self)))) {
	return[self display:(NXRect *)0 :0 :NO];
    } else {
	return[self displayFromOpaqueAncestor:(NXRect *)0 :0 :NO];
    }
}


- setAutoscroll:(BOOL)flag
{
    mFlags.autoscroll = (flag ? YES : NO);
    return self;
}

- _scrollRowToCenter:(int)row
{
    int visRows;
    NXRect vRect, cFrame;

    [self getCellFrame:&cFrame at:row :0];
    [self getVisibleRect:&vRect];
    if (vRect.origin.y > cFrame.origin.y ||
	vRect.origin.y + vRect.size.height <
	cFrame.origin.y + cFrame.size.height) {
	visRows = vRect.size.height / cFrame.size.height - 1.0;
	visRows >>= 1;
	if (visRows > 1) {
	    if (cFrame.origin.y < vRect.origin.y) {
		[self scrollCellToVisible:MAX(0, row-visRows) :0];
	    } else if (cFrame.origin.y + cFrame.size.height >
			vRect.origin.y + vRect.size.height) {
		[self scrollCellToVisible:MIN(numRows-1, row+visRows) :0];
	    }
	} else {
	    [self scrollCellToVisible:row :0];
	}
    }

    return self;
}

- scrollCellToVisible:(int)row :(int)col
{
    NXRect cellBounds;

    if (VALID(row, col)) {
	[self getCellFrame:&cellBounds at:row :col];
	[self scrollRectToVisible:&cellBounds];
    }

    return self;
}


- (BOOL)_mouseHit:(NXPoint *)basePoint row:(int *)row col:(int *)col
{
    int i;
    float d;
    NXPoint localPoint;

    *col = -1;
    *row = -1;
    localPoint = *basePoint;
    [self convertPoint:&localPoint fromView:nil];
    if (localPoint.x < bounds.origin.x
	|| localPoint.x > bounds.origin.x + WMAX
	|| localPoint.y < bounds.origin.y
	|| localPoint.y > bounds.origin.y + HMAX)
	return NO;
    else {
	d = bounds.origin.x;
	for (i = 0; i < numCols; i++) {
	    if (localPoint.x < d)
		break;
	    d += cellSize.width;
	    if (localPoint.x < d) {
		*col = i;
		break;
	    }
	    d += intercell.width;
	}
	if (*col == -1)
	    return NO;
	d = bounds.origin.y;
	for (i = 0; i < numRows; i++) {
	    if (localPoint.y < d)
		break;
	    d += cellSize.height;
	    if (localPoint.y < d) {
		*row = i;
		break;
	    }
	    d += intercell.height;
	}
	if (*row == -1) {
	    return NO;
	} else {
	    return YES;
	}
    }
}


- (BOOL)_loopHit:(NXPoint *)basePoint row:(int *)row col:(int *)col
{
    int i;
    float d;
    NXPoint localPoint;
    BOOL retVal = YES;

    *col = -1;
    *row = -1;
    localPoint = *basePoint;
    [self convertPoint:&localPoint fromView:nil];
    d = bounds.origin.x;
    for (i = 0; i < numCols; i++) {
	if (localPoint.x < d) {
	    if (*col == -1) {
		*col = 0;
		retVal = NO;
	    }
	    break;
	}
	d += cellSize.width;
	if (localPoint.x < d) {
	    *col = i;
	    break;
	}
	d += intercell.width;
    }
    if (*col == -1)
	*col = numCols - 1;
    d = bounds.origin.y;
    for (i = 0; i < numRows; i++) {
	if (localPoint.y < d) {
	    if (*row == -1) {
		*row = 0;
		retVal = NO;
	    }
	    break;
	}
	d += cellSize.height;
	if (localPoint.y < d) {
	    *row = i;
	    break;
	}
	d += intercell.height;
    }
    if (*row == -1) *row = numRows - 1;

    return retVal;
}


- (BOOL)_radioHit:(NXPoint *)basePoint row:(int *)row col:(int *)col
{
    int i;
    float d;
    BOOL retval = YES;
    NXPoint localPoint;

    *col = -1;
    *row = -1;
    localPoint = *basePoint;
    [self convertPoint:&localPoint fromView:nil];
    d = bounds.origin.x;
    for (i = 0; i < numCols; i++) {
	if (localPoint.x < d) {
	    *col = i;
	    retval = NO;
	    break;
	}
	d += cellSize.width;
	if (localPoint.x < d) {
	    *col = i;
	    break;
	}
	d += intercell.width;
    }
    if (i == numCols) {
	*col = numCols - 1;
	retval = NO;
    }
    d = bounds.origin.y;
    for (i = 0; i < numRows; i++) {
	if (localPoint.y < d) {
	    *row = i;
	    retval = NO;
	    break;
	}
	d += cellSize.height;
	if (localPoint.y < d) {
	    *row = i;
	    break;
	}
	d += intercell.height;
    }
    if (i == numRows) {
	*row = numRows - 1;
	retval = NO;
    }

    return retval;
}


- _findFirstOne:(int *)row :(int *)col
{
    id aCell;
    int i, j, k;

    k = 0;
    for (i = 0; i < numRows; i++) {
	for (j = 0; j < numCols; j++) {
	    aCell = [cellList objectAt:k];
	    if ([aCell state] && [aCell isEnabled] && ![aCell isSelectable]) {
		*row = i;
		*col = j;
		return aCell;
	    }
	    k++;
	}
    }
    *row = *col -= 1;

    return nil;
}


- _turnOffAllExcept:(int)row :(int)col andDraw:(BOOL)flag
{
    id aCell;
    int i, j, k;

    k = 0;
    for (i = 0; i < numRows; i++) {
	for (j = 0; j < numCols; j++) {
	    aCell = [cellList objectAt:k];
	    if ([aCell state] &&
		[aCell isEnabled] &&
		![aCell isSelectable] &&
		(i != row || j != col)) {
		[aCell setState:0];
		[self _highlightCellAt:i :j lit:NO andDraw:flag];
	    }
	    k++;
	}
    }

    return self;
}


- _makeEditable:theCell :(int)theRow :(int)theCol :(NXEvent *)theEvent
{
    id textEdit;
    NXRect cellBounds;

    if ([window makeFirstResponder:self]) {
	[window endEditingFor:self];
	textEdit = [window getFieldEditor:YES for:self];
	if (_NXGetShlibVersion() <= MINOR_VERS_1_0) [textEdit setCharFilter:NXFieldFilter];
	selectedCell = theCell;
	selectedRow = theRow;
	selectedCol = theCol;
	if (conFlags.calcSize) {
	    [self calcSize];
	}
	[self getCellFrame:&cellBounds at:selectedRow :selectedCol];
	conFlags.editingValid = NO;
	[selectedCell edit:&cellBounds inView:self
	 editor:textEdit delegate:self event:theEvent];
    }

    return self;
}


- _mouseDown_listMode:(NXEvent *)theEvent
{
    id theCell;
    int theRow, theCol;
    BOOL hit, shiftDown, alternateDown;
    NXPoint localPoint;

    hit = [self _mouseHit:&theEvent->location row:&theRow col:&theCol];
    shiftDown = ((NX_SHIFTMASK & theEvent->flags) != 0);
    alternateDown = ((NX_ALTERNATEMASK & theEvent->flags) != 0);
    if (hit) {
	theCell = [cellList objectAt:GETCELLOFFSET(theRow, theCol)];
    } else {
	localPoint = theEvent->location;
	[self convertPoint:&localPoint fromView:nil];
	if (NXMouseInRect(&localPoint, &bounds, [self isFlipped])) {
	    [self lockFocus];
	    [self _turnOffAllExcept:-1:-1 andDraw:YES];
	    [self unlockFocus];
	}
	return self;
    }
    if (shiftDown)
	[self _shiftDown:theEvent :theCell :theRow :theCol];
    else if (alternateDown)
	[self _alternateDown:theEvent :theCell :theRow :theCol];
    else if ([theCell isSelectable])
	[self _makeEditable:theCell :theRow :theCol :theEvent];
    else
	[self _normalDown:theEvent :theCell :theRow :theCol];

    return self;
}


static void doAutoScroll(id self, NXEvent *oldEvent)
{
    [self autoscroll:oldEvent];
    NXPing();
}

#define ACTIVEMATRIXMASK (NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK|NX_TIMERMASK)

- _mouseDown_normalMode:(NXEvent *)theEvent
{
    volatile NXHandler	handler;
    volatile BOOL	upFlag, doneWithOrig;
    volatile id		origCell = nil, mouseWindow = nil;
    volatile int	origRow = -1, origCol = -1;
    volatile int	lastLitCol = -1, lastLitRow = -1;
    BOOL                hit, touchedOne = NO;
    id                  aCell;
    int                 oldMask, clicks = theEvent->data.mouse.click;
    int			hitPos, theRow, theCol;
    NXRect              cBounds, *trackingBounds;
    NXPoint             localPoint;
    NXTrackingTimer     timer;
    NXEvent             lastNonTimerEvent;

    handler.code = 0;

    localPoint = theEvent->location;

    if (doubleAction && clicks == 2) {
	[self convertPoint:&localPoint fromView:nil];
	if (NXMouseInRect(&localPoint, &bounds, [self isFlipped])) {
	    return [self _sendDoubleActionToCellAt:&(theEvent->location)];
	} else {
	    return self;
	}
    } else if (conFlags.ignoreMultiClick && clicks > 1) {
	return self;
    }

    if (theEvent->window != [window windowNum]) {
	mouseWindow = [NXApp findWindow:theEvent->window];
	[mouseWindow convertBaseToScreen:&localPoint];
	[window convertScreenToBase:&localPoint];
    }

    hit = [self _mouseHit:&localPoint row:&theRow col:&theCol];

    if (!hit) {
	[NXApp getNextEvent:NX_MOUSEUPMASK];
	return self;
    }
    hitPos = GETCELLOFFSET(theRow, theCol);

    if (VALID(theRow, theCol)) {
	aCell = [cellList objectAt:hitPos];
	if ([aCell isSelectable] && [aCell isEnabled]) {
	    if (mFlags.radioMode && aCell != selectedCell) {
		[selectedCell setState:0];
		if (VALID(selectedRow, selectedCol)) {
		    [self drawCellAt:selectedRow :selectedCol];
		}
	    }
	    [self _makeEditable:aCell :theRow :theCol :theEvent];
	    return self;
	}
    }
    if ([selectedCell isSelectable] && [self currentEditor]) {
	if ([window makeFirstResponder:self]) {
	    [window endEditingFor:self];
	} else {
	    return self;
	}
    }

    if (mFlags.radioMode && selectedCell != nil) {
	origCell = selectedCell;
	origRow = selectedRow;
	origCol = selectedCol;
	doneWithOrig = NO;
    } else {
	doneWithOrig = YES;
    }

    oldMask = [window addToEventMask:ACTIVEMATRIXMASK];

    [self lockFocus];
    if (mFlags.autoscroll) {
	NXBeginTimer(&timer, MATRIXTIMERDELAY, MATRIXTIMERINTERVAL);
	lastNonTimerEvent = *theEvent;
    }

    for (;;) {
	if (VALID(theRow, theCol)) {
	    aCell = [cellList objectAt:hitPos];
	    if ([aCell isEnabled]) {
		touchedOne = YES;
		if (!doneWithOrig) {
		    if (origCell) {
			_NXSetCellParam(origCell, NX_CELLHIGHLIGHTED, NO);
			[origCell setState:0];
		    }
		    if (VALID(origRow, origCol)) {
			[self drawCellAt:origRow :origCol];
		    }
		    doneWithOrig = YES;
		}
		selectedCell = aCell;
		selectedRow = theRow;
		selectedCol = theCol;
		if (mFlags.autoscroll) {
		    [self scrollCellToVisible:selectedRow :selectedCol];
		}
		if (lastLitRow >= 0 &&
		    (lastLitRow != theRow || lastLitCol != theCol)) {
		    [self highlightCellAt:lastLitRow :lastLitCol lit:NO];
		}
		if ((mFlags.radioMode || mFlags.highlightMode) &&
		    (mFlags.allowEmptySel || mFlags.highlightMode ||
		     lastLitRow != theRow || lastLitCol != theCol)) {
		    [self highlightCellAt:selectedRow :selectedCol lit:YES];
		    [window flushWindow];
		    DPSFlush();
		}
		if (mFlags.radioMode && !mFlags.allowEmptySel) {
		    lastLitRow = theRow;
		    lastLitCol = theCol;
		}
		if ([[selectedCell class] prefersTrackingUntilMouseUp]) {
		    trackingBounds = NULL;
		} else {
		    trackingBounds = &cBounds;				    
		    [self getCellFrame:&cBounds at:theRow :theCol];
		}
		mFlags.reaction = NO;
		if (mFlags.autoscroll) {
		    NXEndTimer(&timer);
		}
		NX_DURING {
		    upFlag = [selectedCell trackMouse:theEvent
			      inRect:trackingBounds ofView:self];
		} NX_HANDLER {
		    handler = NXLocalHandler;
		    upFlag = YES;
		} NX_ENDHANDLER;
		if (mFlags.autoscroll) {
		    NXBeginTimer(&timer, MATRIXTIMERDELAY, MATRIXTIMERINTERVAL);
		}
		if ((upFlag || mFlags.allowEmptySel || mFlags.highlightMode)
		    && (mFlags.radioMode || mFlags.highlightMode) &&
		    !mFlags.reaction) {
		    [self highlightCellAt:selectedRow :selectedCol lit:NO];
		}
		if (upFlag && !mFlags.reaction) {
		    [window flushWindow];
		    DPSFlush();
		}
		if (upFlag) {
		    break;
		}
		NXPing();
	    }
	}
	do {
	    theEvent = [NXApp getNextEvent:ACTIVEMATRIXMASK];
	    if (theEvent->type != NX_TIMER) {
		lastNonTimerEvent = *theEvent;
		if (mouseWindow) {
		    [mouseWindow convertBaseToScreen:
		     &lastNonTimerEvent.location];
		    [window convertScreenToBase:&lastNonTimerEvent.location];
		    lastNonTimerEvent.window = [window windowNum];
		}
		localPoint = lastNonTimerEvent.location;
	    } else {
		doAutoScroll(self, &lastNonTimerEvent);
	    }
	} while (theEvent->type == NX_TIMER);

	if (theEvent->type == NX_MOUSEUP) {
	    if (touchedOne) {
		if (mFlags.allowEmptySel || !mFlags.radioMode) {
		    selectedCell = nil;
		    selectedRow = -1;
		    selectedCol = -1;
		} else {
		    if (lastLitRow < 0) {
			lastLitRow = origRow;
			lastLitCol = origCol;
		    }
		    selectedCell = [cellList objectAt:
		      GETCELLOFFSET(lastLitRow, lastLitCol)];
		    selectedRow = lastLitRow;
		    selectedCol = lastLitCol;
		    [selectedCell incrementState];
		    mFlags.reaction = NO;
		}
		[self sendAction];
		if (!mFlags.allowEmptySel && mFlags.radioMode) {
		    [self _highlightCellAt:selectedRow :selectedCol
		     lit:NO andDraw:!mFlags.reaction];
		}
	    }
	    break;
	}
	if (mFlags.allowEmptySel || !mFlags.radioMode) {
	    if ([self _mouseHit:&localPoint row:&theRow col:&theCol]) {
		hitPos = GETCELLOFFSET(theRow, theCol);
	    } else {
		hitPos = NULLPOS;
	    }
	} else {
	    [self _radioHit:&localPoint row:&theRow col:&theCol];
	    hitPos = GETCELLOFFSET(theRow, theCol);
	}
    }

    if (mFlags.autoscroll)
	NXEndTimer(&timer);

    [window setEventMask:oldMask];
    if (!mFlags.reaction) [window flushWindow];
    if (!mFlags.reaction || [self isFocusView]) [self unlockFocus];

    if (handler.code) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return self;
}

/*
- _selectRange:(BOOL)lit :(int)x :(int)y :(int)ref :(BOOL)includeX
{
    int    i, j, p;
    id     aCell;
    int	    minRow, maxRow, minCol, maxCol;
    int	    xRow, xCol, yRow, yCol, refRow, refCol;
    int	    minRefRow, maxRefRow, minRefCol, maxRefCol;
  
    xRow = ROW(x);
    xCol = COL(x);
    yRow = ROW(y);
    yCol = COL(y);
    minRow = MIN(xRow, yRow);
    maxRow = MAX(xRow, yRow);
    minCol = MIN(xCol, yCol);
    maxCol = MAX(xCol, yCol);
    if (ref >= 0) {
	refRow = ROW(ref);
	refCol = COL(ref);
	minRefRow = MIN(minRow, refRow);
	maxRefRow = MAX(maxRow, refRow);
	minRefCol = MIN(minCol, refCol);
	maxRefCol = MAX(maxCol, refCol);
    } else {
	minRefRow = minRow;
	maxRefRow = maxRow;
	minRefCol = minCol;
	maxRefCol = maxCol;
    }
  
  
    for (i = minRefRow; i <= maxRefRow; i++) {
	for (j = minRefCol; j <= maxRefCol; j++) {
	    p = GETCELLOFFSET(i,j);
	    if (i >= minRow && i <= maxRow &&
		j >= minCol && j <= maxCol &&
	        (includeX || p != x)) {
		aCell = [cellList objectAt:p];
		if (([aCell state] != lit) && [aCell isEnabled] &&
	            ![aCell isSelectable]) {
		    [aCell setState:lit];
		    [self highlightCellAt:i :j lit:lit];
		}
	    }
	}
    }
  
    return self;
}
*/


- _selectRange:(BOOL)lit :(int)x :(int)y :(int)ref :(BOOL)includeX
{
    int i, t;
    id aCell;

    if (x > y) {
	t = x;
	x = y;
	y = t;
	if (!includeX) {
	    y--;
	}
    } else if (x < y) {
	if (!includeX) {
	    x++;
	}
    } else if (!includeX) {
	return self;
    }
    for (i = x; i <= y; i++) {
	aCell = [cellList objectAt:i];
	if (([aCell state] != lit) && [aCell isEnabled] &&
	    ![aCell isSelectable]) {
	    [aCell setState:lit];
	    [self highlightCellAt:ROW(i) :COL(i) lit :lit];
	}
    }

    return self;
}


static int doselection(id self, int anchor, int old, int new, BOOL lit, BOOL same)
{
    BOOL nope = same ? lit : (lit ? NO : YES);

    if (anchor == old)
	[self _selectRange:lit :anchor :new :-1:YES];
    else if (anchor < old) {
	if (new < anchor) {
	    [self _selectRange:lit :new :anchor :-1:YES];
	    [self _selectRange:nope :anchor :old :-1:NO];
	} else {
	    if (old < new)
		[self _selectRange:lit :old :new :anchor :NO];
	    else if (new < old)
		[self _selectRange:nope :new :old :anchor :NO];
	}
    } else {			/* old < anchor */
	if (anchor < new) {
	    [self _selectRange:lit :anchor :new :-1:YES];
	    [self _selectRange:nope :anchor :old :-1:NO];
	} else if (old < new)
	    [self _selectRange:nope :new :old :anchor :NO];
	else if (new < old)
	    [self _selectRange:lit :old :new :anchor :NO];
    }
    old = new;

    return old;
}


- _shiftDown:(NXEvent *)theEvent :theCell :(int)theRow :(int)theCol
{
    BOOL                hit, lit, looping = YES;
    int                 oldMask, clicks = theEvent->data.mouse.click;
    int                 anchor, old, new;
    NXTrackingTimer     timer;
    NXEvent             lastNonTimerEvent;

    if (!selectedCell)
	return self;
    [self lockFocus];
    anchor = new = old = GETCELLOFFSET(theRow, theCol);
    lit = [theCell state] ? NO : YES;
    old = doselection(self, anchor, old, new, lit, YES);
    [window flushWindow];
    oldMask = [window addToEventMask:ACTIVEMATRIXMASK];
    if (mFlags.autoscroll) {
	NXBeginTimer(&timer, MATRIXTIMERDELAY, MATRIXTIMERINTERVAL);
	lastNonTimerEvent = *theEvent;
    }
    while (looping) {
	theEvent = [NXApp getNextEvent:ACTIVEMATRIXMASK];
	if (theEvent->type != NX_TIMER) {
	    lastNonTimerEvent = *theEvent;
	} else {
	    theEvent = &lastNonTimerEvent;
	    doAutoScroll(self, theEvent);
	}
	hit = [self _loopHit:&theEvent->location row:&theRow col:&theCol];
	if (hit) {
	    new = GETCELLOFFSET(theRow, theCol);
	    old = doselection(self, anchor, old, new, lit, YES);
	}
	[window flushWindow];
	if (theEvent->type == NX_LMOUSEUP) {
	    if (mFlags.autoscroll)
		NXEndTimer(&timer);
	    selectedCell = [cellList objectAt:(int)new];
	    selectedRow = ROW(new);
	    selectedCol = COL(new);
	    if (theCell) {
		mFlags.reaction = NO;
		if ((clicks == 2 && doubleAction) ||
		    (!conFlags.ignoreMultiClick && clicks > 1)) {
		    [self sendDoubleAction];
		} else if (clicks == 1) {
		    [self sendAction];
		}
		if (!mFlags.reaction) [window flushWindow];
	    }
	    looping = NO;
	}
    }
    [window setEventMask:oldMask];
    if (!mFlags.reaction || [self isFocusView]) [self unlockFocus];

    return self;
}

- _alternateDown:(NXEvent *)theEvent :theCell :(int)theRow :(int)theCol
{
    BOOL                hit, looping = YES;
    int                 oldMask, clicks = theEvent->data.mouse.click;
    int                 anchor, old, new;
    NXTrackingTimer     timer;
    NXEvent             lastNonTimerEvent;

    if (selectedRow < 0 || selectedCol < 0)
	return self;
    [self lockFocus];
    anchor = old = GETCELLOFFSET(selectedRow, selectedCol);
    new = GETCELLOFFSET(theRow, theCol);
    old = doselection(self, anchor, old, new, YES, NO);
    [window flushWindow];
    oldMask = [window addToEventMask:ACTIVEMATRIXMASK];
    if (mFlags.autoscroll) {
	NXBeginTimer(&timer, MATRIXTIMERDELAY, MATRIXTIMERINTERVAL);
	lastNonTimerEvent = *theEvent;
    }
    while (looping) {
	theEvent = [NXApp getNextEvent:ACTIVEMATRIXMASK];
	if (theEvent->type != NX_TIMER) {
	    lastNonTimerEvent = *theEvent;
	} else {
	    theEvent = &lastNonTimerEvent;
	    doAutoScroll(self, theEvent);
	}
	hit = [self _loopHit:&theEvent->location row:&theRow col:&theCol];
	if (hit) {
	    new = GETCELLOFFSET(theRow, theCol);
	    old = doselection(self, anchor, old, new, YES, NO);
	}
	[window flushWindow];
	if (theEvent->type == NX_LMOUSEUP) {
	    if (mFlags.autoscroll)
		NXEndTimer(&timer);
	    selectedCell = [cellList objectAt:(int)new];
	    selectedRow = ROW(new);
	    selectedCol = COL(new);
	    if (theCell) {
		mFlags.reaction = NO;
		if ((clicks == 2 && doubleAction) ||
		    (!conFlags.ignoreMultiClick && clicks > 1)) {
		    [self sendDoubleAction];
		} else if (clicks == 1) {
		    [self sendAction];
		}
		if (!mFlags.reaction) [window flushWindow];
	    }
	    looping = NO;
	}
    }
    [window setEventMask:oldMask];
    if (!mFlags.reaction || [self isFocusView]) [self unlockFocus];

    return self;
}

- _normalDown:(NXEvent *)theEvent :theCell :(int)theRow :(int)theCol
{
    BOOL                hit, looping = YES;
    int                 oldMask, clicks = theEvent->data.mouse.click;
    int                 anchor, old, new;
    NXTrackingTimer     timer;
    NXEvent             lastNonTimerEvent;

    [self lockFocus];
    anchor = new = old = GETCELLOFFSET(theRow, theCol);
    [self _turnOffAllExcept:theRow :theCol andDraw:YES];
    old = doselection(self, anchor, old, new, YES, NO);
    [window flushWindow];
    oldMask = [window addToEventMask:ACTIVEMATRIXMASK];
    if (mFlags.autoscroll) {
	NXBeginTimer(&timer, MATRIXTIMERDELAY, MATRIXTIMERINTERVAL);
	lastNonTimerEvent = *theEvent;
    }
    while (looping) {
	theEvent = [NXApp getNextEvent:ACTIVEMATRIXMASK];
	if (theEvent->type != NX_TIMER) {
	    lastNonTimerEvent = *theEvent;
	} else {
	    theEvent = &lastNonTimerEvent;
	    doAutoScroll(self, theEvent);
	}
	hit = [self _loopHit:&theEvent->location row:&theRow col:&theCol];
	if (hit) {
	    new = GETCELLOFFSET(theRow, theCol);
	    old = doselection(self, anchor, old, new, YES, NO);
	}
	[window flushWindow];
	if (theEvent->type == NX_LMOUSEUP) {
	    if (mFlags.autoscroll)
		NXEndTimer(&timer);
	    selectedCell = [cellList objectAt:(int)new];
	    selectedRow = ROW(new);
	    selectedCol = COL(new);
	    if (theCell) {
		mFlags.reaction = NO;
		if ((clicks == 2 && doubleAction) ||
		    (!conFlags.ignoreMultiClick && clicks > 1)) {
		    [self sendDoubleAction];
		} else if (clicks == 1) {
		    [self sendAction];
		}
		if (!mFlags.reaction) [window flushWindow];
	    }
	    looping = NO;
	}
    }
    [window setEventMask:oldMask];
    if (!mFlags.reaction || [self isFocusView]) [self unlockFocus];

    return self;
}


- setReaction:(BOOL)flag
{
    mFlags.reaction = (flag ? YES : NO);
    return self;
}


- (int)mouseDownFlags
{
    return mdFlags;
}


- mouseDown:(NXEvent *)theEvent
{
    mdFlags = theEvent->flags;
    if (mFlags.listMode) {
	[self _mouseDown_listMode:theEvent];
    } else {
	[self _mouseDown_normalMode:theEvent];
    }
    mdFlags = 0;
    return self;
}


- (BOOL)performKeyEquivalent:(NXEvent *)theEvent
{
    volatile NXHandler	handler;
    id                  theCell;
    int 	        i, size;
    unsigned short      k;
    BOOL		isMenu = NO;

    handler.code = 0;

    size = (numRows * numCols);
    k = theEvent->data.key.charCode;
    for (i = 0; i < size; i++) {
	theCell = [cellList objectAt:i];
	if ([theCell isEnabled] && k == [theCell keyEquivalent]) {
	    int highlightRow, highlightCol;
	    if ([self _isBlockedByModalResponder:theCell]) {
		return NO;
	    }
	    if ([selectedCell isSelectable]) {
		[self abortEditing];
	    }
	    if (mFlags.radioMode && selectedCell != theCell) {
		[selectedCell setState:0];
		[self drawCellAt:selectedRow :selectedCol];
	    }
	    selectedCell = theCell;
	    selectedRow = i / numCols;
	    selectedCol = i % numCols;
	    isMenu = [selectedCell isKindOf:[MenuCell class]] &&
		[window isKindOf:[Menu class]];
	    if (CANDRAW) {
		[self lockFocus];
		[self highlightCellAt:selectedRow :selectedCol lit:YES];
		[window flushWindow];
	    }
	    if (isMenu && (!CANDRAW || ![window isVisible])) {
	        [window _highlightSupermenu:YES];
	    } else if (isMenu) {
		[window _unattachSubmenu];
	    }
	    if (!mFlags.radioMode) {
		if (!isMenu) {
		    [selectedCell incrementState];
		}
	    } else {
		[selectedCell setState:1];
	    }
	    NXPing();
	    mdFlags = (theEvent ? theEvent->flags : 0);
	    [selectedCell _setMouseDownFlags:mdFlags];
	    highlightRow = selectedRow;
	    highlightCol = selectedCol;
	    NX_DURING {
		handler.code = 0;
		[self sendAction];
	    } NX_HANDLER {
		handler = NXLocalHandler;
	    } NX_ENDHANDLER
	    mdFlags = 0;
	    [[self cellAt:highlightRow :highlightCol] _setMouseDownFlags:0];
	    if (CANDRAW) {
		[self highlightCellAt:highlightRow :highlightCol lit:NO];
		[window flushWindow];
		[self unlockFocus];
	    }
	    if (isMenu && (!CANDRAW || ![window isVisible]))
	        [window _highlightSupermenu:NO];
	    if (handler.code) {
		NX_RAISE(handler.code, handler.data1, handler.data2);
	    }
	    return YES;
	}
    }

    return NO;
}


- sendAction:(SEL)theAction to:theTarget
{
    if (!theAction) {
	theAction = action;
	theTarget = target;
    } else if (!theTarget) {
	theTarget = target;
    }
    return[super sendAction:theAction to:theTarget];
}


- sendAction
{
    if (selectedCell && ![selectedCell isEnabled]) {
	return nil;
    }
    return[self sendAction:[selectedCell action] to:[selectedCell target]];
}


- _sendDoubleActionToCellAt:(NXPoint *)point
{
    int row, col;

    if ([self _mouseHit:point row:&row col:&col] && 
	(selectedCell == [self cellAt:row :col])) {
	[self sendDoubleAction];
    }
    return self;
}

- sendDoubleAction
{
    id theTarget;
    SEL theAction;

    if (![selectedCell isEnabled]) {
	return self;
    }
    if (!(theAction = [selectedCell action])) {
	theAction = doubleAction ? doubleAction : action;
	theTarget = target;
    } else {
	theTarget = [selectedCell target];
    }
    [self sendAction:theAction to:theTarget];

    return self;
}


- textDelegate
{
    return textDelegate;
}


- setTextDelegate:anObject
{
    textDelegate = anObject;
    return self;
}


- (BOOL)textWillEnd:textObject
{
    char *string;
    BOOL retvalue;
    int length,blength;

    retvalue = NO;
    if (selectedCell) {
	length = [textObject textLength];
	blength = [textObject byteLength];
	string = NXZoneMalloc([self zone], blength+1);
	[textObject getSubstring:string start:0 length:length+1];
	string[blength] = '\0';
	retvalue = ![selectedCell isEntryAcceptable:string];
	free(string);
	if (retvalue && errorAction) {
	    [self sendAction:errorAction to:[cell target]];
	}
	if ([textDelegate respondsTo:@selector(textWillEnd:)]) {
	    retvalue = [textDelegate textWillEnd:textObject] || retvalue;
	}
    }
    if (retvalue) NXBeep();

    return retvalue;
}


- (BOOL)textWillChange:textObject
{
    if ([textDelegate respondsTo:@selector(textWillChange:)]) {
	return[textDelegate textWillChange:textObject];
    } else {
	return 0;
    }
}


- textDidEnd:textObject endChar:(unsigned short)whyEnd
{
    [self _validateEditing:textObject];
    if ([textDelegate respondsTo:@selector(textDidEnd:endChar:)]) {
	[textDelegate textDidEnd:textObject endChar:whyEnd];
    }
    [selectedCell endEditing:textObject];
    [self drawCell:selectedCell];
    switch (whyEnd) {
    case NX_RETURN:
	if ([self sendAction])
	    break;
    case NX_TAB:
	[self _selectTextOfNextCell];
	break;
    case NX_BACKTAB:
	[self _selectTextOfPreviousCell];
	break;
    }
    [window flushWindow];	/* why? - pah */
    return self;
}


- textDidChange:textObject
{
    if ([textDelegate respondsTo:@selector(textDidChange:)]) {
	return[textDelegate textDidChange:textObject];
    } else {
	return self;
    }
}


- text:textObject isEmpty:(BOOL)flag
{
    if ([textDelegate respondsTo:@selector(textDidGetKeys:isEmpty:)]) {
	return[textDelegate textDidGetKeys:textObject isEmpty:flag];
    } else if ([textDelegate respondsTo:@selector(text:isEmpty:)]) {
	return[textDelegate text:textObject isEmpty:flag];
    } else {
	return self;
    }
}
- textDidGetKeys:textObject isEmpty:(BOOL)flag
{
    return [self text:textObject isEmpty:flag];
}


- _selectTextOfCell:aCell
{
    int offset;
    id textEdit;
    NXRect cellBounds;

    if (![window makeFirstResponder:self]) return self;
    if (![aCell isEnabled] || ![aCell isSelectable]) return nil;

    textEdit = [window getFieldEditor:YES for:self];
    [window endEditingFor:self];
    if (_NXGetShlibVersion() <= MINOR_VERS_1_0) [textEdit setCharFilter:NXFieldFilter];
    selectedCell = aCell;
    offset = [cellList indexOf:selectedCell];
    selectedCol = (offset % numCols);
    selectedRow = (offset / numCols);
    if (conFlags.calcSize) [self calcSize];
    if (mFlags.autoscroll) [self scrollCellToVisible:selectedRow :selectedCol];
    [self getCellFrame:&cellBounds at:selectedRow :selectedCol];
    conFlags.editingValid = NO;
    [selectedCell select:&cellBounds inView:self editor:textEdit delegate:self start:0 length:32000];

    return aCell;
}


- _selectTextOfNextCell
{
    id aCell;
    int offset, i, size;

    offset = [cellList indexOf:selectedCell];
    size = (numRows * numCols);
    if (offset + 1 != size) {
	for (i = offset + 1; i < size; i++) {
	    aCell = [cellList objectAt:i];
	    if ([aCell isEnabled] && [aCell isSelectable]) {
		return[self _selectTextOfCell:aCell];
	    }
	}
    }
    if (nextText && nextText != self) {
	return[nextText selectText:self];
    }
    for (i = 0; i <= offset; i++) {
	aCell = [cellList objectAt:i];
	if ([aCell isEnabled] && [aCell isSelectable]) {
	    return[self _selectTextOfCell:aCell];
	}
    }

    return self;
}


- _selectTextOfPreviousCell
{
    id aCell;
    int offset, i, size;

    offset = [cellList indexOf:selectedCell];
    size = (numRows * numCols);
    if (offset - 1 >= 0) {
	for (i = offset - 1; i >= 0; i--) {
	    aCell = [cellList objectAt:i];
	    if ([aCell isEnabled] && [aCell isSelectable]) {
		return[self _selectTextOfCell:aCell];
	    }
	}
    }
    if (previousText && previousText != self) {
	return[previousText selectText:self];
    }
    if (offset < size) {
	for (i = size - 1; i > offset; i--) {
	    aCell = [cellList objectAt:i];
	    if ([aCell isEnabled] && [aCell isSelectable]) {
		return[self _selectTextOfCell:aCell];
	    }
	}
    }

    return self;
}


- selectText:sender
{
    id aCell;
    int i, size;

    size = (numRows * numCols);
    if (nextText && sender == nextText && sender != previousText) {
	for (i = size - 1; i >= 0; i--) {
	    aCell = [cellList objectAt:i];
	    if ([aCell isEnabled] && [aCell isSelectable]) {
		return[self _selectTextOfCell:aCell];
	    }
	}
    } else {
	for (i = 0; i < size; i++) {
	    aCell = [cellList objectAt:i];
	    if ([aCell isEnabled] && [aCell isSelectable]) {
		return[self _selectTextOfCell:aCell];
	    }
	}
    }

    return self;
}


- selectTextAt:(int)row :(int)col
{
    if (VALID(row, col)) {
	return[self _selectTextOfCell:[cellList objectAt:GETCELLOFFSET(row, col)]];
    } else {
	return self;
    }
}


- setPreviousText:anObject
{
    previousText = anObject;
    return self;
}


- setNextText:anObject
{
    nextText = anObject;
    if ([anObject respondsTo:@selector(setPreviousText:)] &&
	[anObject respondsTo:@selector(selectText:)]) {
	[anObject setPreviousText:self];
    }
    return self;
}


- (BOOL)acceptsFirstMouse
{
    return mFlags.listMode ? NO : YES;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteObjectReference(stream, nextText);
    NXWriteObjectReference(stream, previousText);
    NXWriteObjectReference(stream, textDelegate);
    NXWriteObjectReference(stream, target);
    NXWriteTypes(stream, "@:@iiii", &cellList, &action, &selectedCell, &selectedRow, &selectedCol, &numRows, &numCols);
    NXWriteSize(stream, &cellSize);
    NXWriteSize(stream, &intercell);
    NXWriteTypes(stream, "ff@@#::s", &backgroundGray, &cellBackgroundGray, &font, &protoCell, &cellClass, &doubleAction, &errorAction, &mFlags);
    NXWriteColor(stream, [self _backColorSpecified] ?
				[self backgroundColor] : _NXNoColor());
    NXWriteColor(stream, [self _cellColorSpecified] ?
				[self cellBackgroundColor] : _NXNoColor());
    return self;
}


- read:(NXTypedStream *) stream
{
    int version;

    [super read:stream];
    version = NXTypedStreamClassVersion(stream, "Matrix");
    nextText = NXReadObject(stream);
    previousText = NXReadObject(stream);
    textDelegate = NXReadObject(stream);
    target = NXReadObject(stream);
    NXReadTypes(stream, "@:@iiii", &cellList, &action, &selectedCell, &selectedRow, &selectedCol, &numRows, &numCols);
    NXReadSize(stream, &cellSize);
    NXReadSize(stream, &intercell);
    if (version < 1) {
	NXReadTypes(stream, "ff@@@::s", &backgroundGray, &cellBackgroundGray, &font, &protoCell, &cellClass, &doubleAction, &errorAction, &mFlags);
    } else if (version >= 1) {
	NXReadTypes(stream, "ff@@#::s", &backgroundGray, &cellBackgroundGray, &font, &protoCell, &cellClass, &doubleAction, &errorAction, &mFlags);
	if (version == 2) {
	    NXColor tmpColor;
	    if (_NXIsValidColor(tmpColor = NXReadColor(stream))) {
		[self setBackgroundColor:tmpColor];
	    }
	    if (_NXIsValidColor(tmpColor = NXReadColor(stream))) {
		[self setCellBackgroundColor:tmpColor];
	    }
	}
    }
    [protoCell _dontFreeText];
    if (numRows == numCols && numRows == 1) {
	cell = [cellList objectAt:0];
    } else {
	cell = nil;
    }
    return self;
}


- resetCursorRects
{
    id *cellPtr;
    NXCoord startX, incrX, incrY;
    NXRect cellBounds, inBounds, clipped;
    int row, col, firstCol, lastCol, firstRow, lastRow;

    if (!numCols || !numRows || ![self getVisibleRect:&inBounds]) return self;

    firstCol =
      MIN(floor((NX_X(&inBounds) - bounds.origin.x)
		/ (cellSize.width + intercell.width)), numCols - 1);
    lastCol =
      MIN(ceil((NX_MAXX(&inBounds) - bounds.origin.x)
	       / (cellSize.width + intercell.width)), numCols);
    firstRow =
      MIN(floor((NX_Y(&inBounds) - bounds.origin.y)
		/ (cellSize.height + intercell.height)), numRows - 1);
    lastRow =
      MIN(ceil((NX_MAXY(&inBounds) - bounds.origin.y)
	       / (cellSize.height + intercell.height)), numRows);
    cellBounds.size = cellSize;
    incrX = intercell.width + cellSize.width;
    incrY = intercell.height + cellSize.height;
    startX = bounds.origin.x + (firstCol * incrX);
    cellBounds.origin.y = bounds.origin.y + (firstRow * incrY);
    cellPtr = (id *)NX_ADDRESS(cellList);
    cellPtr += firstRow * numCols + firstCol;
    for (row = firstRow; row < lastRow; row++) {
	cellBounds.origin.x = startX;
	for (col = firstCol; col < lastCol; col++) {
	    clipped = cellBounds;
	    if (NXIntersectionRect(&inBounds, &clipped)) {
		[*cellPtr resetCursorRect:&clipped inView:self];
	    }
	    cellBounds.origin.x += incrX;
	    cellPtr++;
	}
	cellBounds.origin.y += incrY;
	cellPtr += numCols - (lastCol - firstCol);
    }

    return self;
}


@end

/*
  
Modifications (since 0.8):
  
11/23/88 pah	eliminated insideOnly opacity support
12/4/88  pah	fixed BACKTAB so that it goes from the last cell to
		 the first when received from some other object
12/5/88  pah	added support for factory method setCellClass:
12/11/88 pah	added selectText: as target/action method (retained
		 selectText for backward compatibility)
12/12/88 pah	added selectCell: and selectCellWithTag: methods
		added _highlightCellAt::lit:andDraw: method
		added mFlags._drawingAncestor to avoid infinite looping
		overrode mouseDownFlags from Control (since it is a bit
		 different when you are tracking multiple cells)
12/13/88 pah	rewrote selectCellAt:: to fix some bugs and make cleaner
		fixed all the autodisplay headbutts (changes to setEnabled:,
		 setFont:, setBackgroundGray:, setCellBackgroundGray:,
		 setState:, putCell:at:, 
	 bgy	converted to the List object;
12/14/88 pah	changed matrixdisplay to _NXMatrixDisplay
  
0.82
----
 1/06/89 bgy	use selectAll: instead of selectAll in the text object  
 1/08/89 pah	add NXPing() in normal mode mouseDown:
 1/10/89 pah	fix bug manifested by PULs which come up non-selected
 1/12/89 pah	abortEditing if setEnabled:NO
 1/13/89 pah	RETURN is like TAB if no action gets sent
		blast selectText (without the colon)
 1/14/89 pah	_NXMatrixDisplay superceded by update
 1/25/89 trey	setTitle: takes a const param
 1/27/89 bs 	added read: write:
 2/05/89 bgy	added cursor-tracking methods:
		    - resetCursorRects;
		 This method calls addCursorRect:cursor: for each visible 
		 cell that is enabled and editable or selectable. The rect 
		 for the cell is clipped against the visible rect.
 2/01/89 trey	textDidEnd:endChar: sends endEditing to the cell after
		 the textDelegate gets his textDidEnd:endChar: message
 2/06/89 pah	use tracking timer to do autoscrolling
 2/09/89 wrp	containsRect changed to _NXContainsRect
 2/11/89 pah	turn off timer before allow cell to track or sending an action
 2/12/89 pah	made previousText, setPreviousText: and instance variable
		 previousText public
 2/14/89 pah	always set vFlags.needsDisplay if we want to draw and can't
		rip out shift down, make shift do what command does
 2/16/89 pah	don't send key equivalent through if running modal unless it
 		 is bound for the first responder in the keyWindow
 2/18/89 pah	more char * -> const char *
  
0.83
----
 3/12/89 pah	Put extension of the selection back in under the ALT key
 3/14/89 pah	Enforce radio-ness in setMode:NX_RADIOMODE
 3/15/89 pah	Add selectAll: method
 3/16/89 pah	Fix message send only to FR in keyWindow if running modal
 3/18/89 pah	Check that receiver understands selectText: (as well as
		 setPreviousText:) before sending setPreviousText:

0.91
----
 5/10/89 pah	clearSelectedCell clears all the highlight/state bits as well
 5/12/89 pah	optimize drawing
 5/19/89 trey	minimized static data
		fixed test of insertRowAt: so cell reuse works
 5/21/89 pah	swap order of factories around so that it is least specific
		 to most specific (this was a bug)
 5/23/89 pah	support allowMultiClick
 5/27/89 pah	move resetCursorRect responsibility to Cells

0.92
----
  6/4/89 pah	change allowMultiClick to ignoreMultiClick
  6/5/89 pah	fixed blotting out of forms
  6/6/89 pah	optimized menu drawing with compositing!
 6/12/89 pah	made renewRows:cols: clear the selected cell properly
 6/12/89 trey	cell instance var is always nil'ed to prevent freeing

0.93
----
 6/14/89 pah	make factory ensure that a cell is selected if radio mode
		eliminate _NXSetCellParam() calls
		changed endEditing: to makeFirstResponder: in some cases
 6/15/89 pah	eliminate redundant autodisplay since ActionCell does it now
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps
 6/17/89 wrp	changed ScrollView to ClipView

0.94
----
 7/13/89 pah	bumped the version to 1 so that archiving could be fixed so
		 that cellClass is archived as a class, not as an object
		fixed sizeTo:: to, as best as possible, preserve editing

0.96
----
 7/20/89 pah	change all isEditable tests to isSelectables
		don't allow setState:0 at:: if radio and the cell is selected
		unattach a Menu's attached menu when command keying
		make cell get drawn in textDidEnd:endChar:

79
__
 3/21/90 aozer	Fixed the GETCELLOFFSET macro by putting extra pairs of parens
		around the arguments; the lack of parens caused 
		removeColAt:andFree: bug (4855).

80
--
  4/10/90 pah	Added setAutosizeCells: which allows you to put the Matrix in
		 a mode whereby when it gets resized, the cells automatically
		 resize themselves to fit the frame

83
--
 4/30/90 aozer	Bumped version upto 2 and added read/write of colors; color
		use not implemented yet, however.

84
--
 5/14/90 pah	Fixed bug in setAutosizeCells: (was ignoring intercell).
		Added _getDrawingRow:andCol: to support NXLazyBrowserCell.

85
--
 5/31/90 aozer	selectCellWithTag: returns Matrix if the cell exists; otherwise
		returns nil.  This is different than in the 1.0 documentation.
 6/04/90 pah	Added _setSelectedCell: which sets the cell without drawing

91
--
 8/7/90 aozer	Added code to putCell:at:: to recache the cell variable (bug 
		6800).
 8/10/90 aozer	Implemented all the color stuff
 8/13/90 aozer	Fixed bug 4577 (Matrix with 0 rows or columns doesn't display).
		Fix was to move check for 0 rows or cols from top of drawSelf::
		to further in; seems to work but should watch for problems.

92
--
 8/20/90 gcockrft  Bug fix for tabing when only 1 text item is in a matrix.
 8/18/90 aozer	Fixed bug 7736 by not drawing first rect if drawSelf:: is
		called	with 3 rects.
 8/18/90 aozer	Fixed bug 4022 (double-clicks on disabled cells send double
		action) by adding _sendDoubleActionToCellAt:

93
--
 8/29/90 aozer	getNumRows:numCols: accepts NULL args (8490)
 9/4/90 trey	properly nested focusing and timerEvents in mouseDown loop
		 to prevent "exception handlders not properly removed"

94
--
 9/25/90 gcockrft	use byteLength instead of textLength
 9/30/90 aozer	Make _loopHit: return NO if clicked in intercell area.
		This fixes good old 4419.
 9/30/90 aozer	Fixed bug 7921 by making performKeyEquivalent: unhighlight
		the cell it highlighted rather than the selected one.

95
--
 10/3/90 aozer	In _mouseDown_normalMode check prefersTrackingUntilMouseUp to
		see if the cell should get rect to track in in trackMouse:...

105
---
 11/6/90 glc	use blength not length
 
*/

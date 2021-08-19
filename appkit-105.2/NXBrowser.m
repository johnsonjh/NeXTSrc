/*
	NXBrowser.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "NXBrowser.h"
#import "Matrix_Private.h"
#import "NXLazyBrowserCell.h"
#import "Button.h"
#import "ButtonCell.h"
#import "ClipView.h"
#import "Font.h"
#import "NXImage.h"
#import "SavePanel.h"
#import "ScrollView.h"
#import "Scroller.h"
#import "Text.h"
#import "TextFieldCell.h"
#import "nextstd.h"
#import "timer.h"
#import <defaults.h>
#import "perfTimer.h"
#import <dpsclient/wraps.h>
#import <objc/hashtable.h>
#import <objc/List.h>
#import <math.h>
#import <zone.h>

#define DEFAULT_COLUMN_WIDTH 100
#define BUTTON_WIDTH 19.0
#define BUTTON_SPACER_WIDTH 2.0
#define TITLE_HEIGHT 20.0
#define TITLE_SPACER_HEIGHT 2.0

#define COLUMN_SPACER_WIDTH ([self columnsAreSeparated] ? 4.0 : 1.0)

#define LEFTARROW	172
#define RIGHTARROW	174
#define UPARROW		173
#define DOWNARROW	175

static id buttonPrototype = nil;

@interface SavePanel(Timing)
- _resetTimers;
- _stopTimers:(const char *)func;
@end

@interface NXBrowser(Private)
- _enableButton:(BOOL)flag inMatrix:matrix at:(int)col;
- _findButtonMatrixWithTag:(int)bmTag;
- _updateUpDownButtonsInColumn:(int)column;
- _addButtonMatrixInColumn:(int)column reuse:buttonMatrix;
- _addButtons;
- (BOOL)_drawButton:(int)arrow lockedFocus:(BOOL)lockedFocus;
- _updateButtons;
- _setTitle:(const char *)aString ofColumn:(int)column clearbg:(BOOL)clearbg;
- (BOOL)_selectCell:(const char *)title inColumn:(int)column;
- _displayColumn:(int)column lockFocus:(BOOL)lockFocus;
- _updateAllColumns;
- _createColumn:(const NXRect *)rect;
- _scrollColumnsRightBy:(int)shiftAmount;
- _scrollColumnToVisible:(int)column private:(BOOL)privateCall;
- _kludgeScrollBar;
- _calcNumVisibleColumnsAndColumnSize;
- _bumpSelectedItem:(BOOL)down;
- (NXRect *)_getRightButtonFrame:(NXRect *)theRect;
- (NXRect *)_getDownButtonFrame:(NXRect *)theRect inColumn:(int)column;
- (NXRect *)_getColumnsFrame:(NXRect *)theRect;
- (NXRect *)_calcColumnsFrame:(NXRect *)theRect;
@end

/* Comments:
 *
 * Read this in a wide window.
 * columnSize is the size of a given column without the spacer or anything else.
 * _getColumnsFrame: gets the NXUnionRect of all the column frames.
 * _calcColumnsFrame: does a best-guess at what _getColumnsFrame: should return.
 * LEFTARROW, etc. are the ascii codes in the symbol character set of the arrow keys.
 * LEFTARROW, etc. are also used as an argument to _drawButton: (which has nothing
 * to do with the arrow keys, just the arrow buttons).
 */

@implementation NXBrowser

- (int)numVisibleColumns
{
    return _reserved4[2];
}

- _setNumVisibleColumns:(int)nvc
{
    _reserved4[2] = nvc;
    return self;
}

typedef struct _flags {
    unsigned int        allowMultiSel:1;
    unsigned int        allowBranchSel:1;
    unsigned int        reuseColumns:1;
    unsigned int        isTitled:1;
    unsigned int        titleFromPrevious:1;
    unsigned int        leftButtonIsEnabled:1;
    unsigned int        rightButtonIsEnabled:1;
    unsigned int        leftButtonIsHighlighted:1;
    unsigned int        rightButtonIsHighlighted:1;
    unsigned int        hideLeftRightButtons:1;
    unsigned int        separateColumns:1;
    unsigned int        useScrollButtons:1;
    unsigned int        useScrollBars:1;
    unsigned int        delegateLoadsCells:1;
    unsigned int        delegateSetsTitles:1;
    unsigned int        delegateSelectsCells:1;
    unsigned int        delegateIsVeryLazy:1;
    unsigned int        delegateValidatesColumns:1;
    unsigned int	acceptArrowKeys:1;
    unsigned int	time:1;
    unsigned int	dontDrawTitles:1;
    unsigned int	delegateWillScroll:1;
    unsigned int	delegateDidScroll:1;
    unsigned int	:0;
} BrowserFlags;

typedef struct {
    id columns;
    id unusedColumns;
    short minColumnWidth;
    short firstVisibleColumn;
    short maxVisibleColumns;
    char *firstColumnTitle;
    char *unarchivedPath;
} BrowserPrivate;

/* These are here for NIB compatibility.    */
/* They should be ripped out after nib-122. */

#define bp ((BrowserPrivate *)_private)
#define bpflags ((BrowserFlags *)&_reserved3[2])
#define titles _reserved1[0]
#define columnSize _reserved2
#define numVisibleColumns _reserved4[2]

+ initialize
{
    [self setVersion:2];
    return self;
}

- _doTiming
{
    bpflags->time = YES;
    return self;
}

- _enableButton:(BOOL)flag inMatrix:matrix at:(int)col
{
    id button;
    BOOL isEnabled;

    button = [matrix cellAt:0 :col];
    isEnabled = [button isEnabled];
    if ((flag && !isEnabled) || (!flag && isEnabled)) {
	[button setEnabled:flag];
	if (flag) {
	    [matrix setIcon:(col ? "scrollMenuUp" : "scrollMenuDown") at:0 :col];
	} else {
	    [matrix setIcon:(col ? "scrollMenuUpD" : "scrollMenuDownD") at:0 :col];
	}
    }

    return self;
}

- _findButtonMatrixWithTag:(int)bmTag
{
    int i;
    id subview;

    for (i = [subviews count]-1; i >= 0; i--) {
	subview = [subviews objectAt:i];
	if ([subview tag] == bmTag && [subview class] == [Matrix class] && [subview prototype] == buttonPrototype) return subview;
    }

    return nil;
}

- _updateUpDownButtonsInColumn:(int)column
{
    id clipView, matrix, buttonMatrix;
    NXRect dvframe, cvbounds;
    BOOL enableUp, enableDown;

    matrix = [self matrixInColumn:column];
    if (matrix) {
	clipView = [matrix superview];
	[clipView getBounds:&cvbounds];
	[clipView getDocRect:&dvframe];
	enableUp = (cvbounds.origin.y ? YES : NO);
	enableDown = (cvbounds.origin.y + cvbounds.size.height) < (dvframe.origin.y + dvframe.size.height);
    } else {
	enableUp = enableDown = NO;
    }
    buttonMatrix = [self _findButtonMatrixWithTag:column-bp->firstVisibleColumn];
    if (buttonMatrix) {
	[self _enableButton:enableDown inMatrix:buttonMatrix at:0];
	[self _enableButton:enableUp inMatrix:buttonMatrix at:1];
    }

    return self;
}

- _drawUpDownButtonsInColumn:(int)column
{
    [[self _findButtonMatrixWithTag:column-bp->firstVisibleColumn] display];
    return self;
}

- _addButtonMatrixInColumn:(int)column reuse:buttonMatrix
{
    NXRect mframe, bframe;
    id upButton, downButton;
    NXSize intercell, cellSize;
    NXZone *zone = [self zone];

    if (![self _getDownButtonFrame:&bframe inColumn:column]) return nil;

    mframe.origin.x = bframe.origin.x + 1.0;
    mframe.origin.y = bframe.origin.y + 2.0;
    mframe.size.height = bframe.size.height - 3.0;
    mframe.size.width = bframe.size.width * 2.0 - 3.0;
    intercell.width = 3.0;
    intercell.height = 0.0;
    cellSize.width = bframe.size.width - 3.0;
    cellSize.height = bframe.size.height - 3.0;
    if (!buttonMatrix) {
	if (!buttonPrototype) {
	    buttonPrototype = [[ButtonCell allocFromZone:zone] init];
	    [buttonPrototype setBordered:NO];
	    [buttonPrototype setHighlightsBy:NX_CHANGEBACKGROUND|NX_CONTENTS];
	    [buttonPrototype setShowsStateBy:NX_NONE];
	    [buttonPrototype setAction:@selector(scrollUpOrDown:)];
	    [buttonPrototype setContinuous:YES];
	}
	buttonMatrix = [[Matrix allocFromZone:zone] initFrame:&mframe mode:NX_HIGHLIGHTMODE prototype:buttonPrototype numRows:1 numCols:2];
	[buttonMatrix setTarget:self];
    } else {
	[buttonMatrix setFrame:&mframe];
    }
    [buttonMatrix setIntercell:&intercell];
    [buttonMatrix setCellSize:&cellSize];
    [buttonMatrix setTag:column-bp->firstVisibleColumn];
    downButton = [buttonMatrix cellAt:0 :0];
    upButton = [buttonMatrix cellAt:0 :1];
    [downButton setIcon:"scrollMenuDownD"];
    [downButton setAltIcon:"scrollMenuDownH"];
    [upButton setIcon:"scrollMenuUpD"];
    [upButton setAltIcon:"scrollMenuUpH"];
    [downButton setEnabled:NO];
    [upButton setEnabled:NO];

    [self addSubview:buttonMatrix];

    return self;
}

- _addButtons
{
    int i;
    id buttonMatrix, buttons = nil;

    for (i = [subviews count] - 1; i >= 0; i--) {
	buttonMatrix = [subviews objectAt:i];
	if ([buttonMatrix isKindOf:[Matrix class]] && [buttonMatrix prototype] == buttonPrototype) {
	    if (!buttons) buttons = [[List allocFromZone:[self zone]] init];
	    [buttons addObject:buttonMatrix];
	    [buttonMatrix removeFromSuperview];
	}
    }
    if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
	for (i = 0; i < numVisibleColumns; i++) {
	    [self _addButtonMatrixInColumn:bp->firstVisibleColumn+i reuse:[buttons removeLastObject]];
	}
    }
    [buttons makeObjectsPerform:@selector(setPrototype:) with:nil];
    [buttons freeObjects];
    [buttons free];

    return self;
}

- (BOOL)_drawButton:(int)arrow lockedFocus:(BOOL)lockedFocus
{
    NXPoint p;
    NXRect rect;
    id arrowImage;
    NXSize arrowSize;
    BOOL isLeft, isEnabled, isHighlighted;

    if (bpflags->hideLeftRightButtons) return NO;

    [self _getRightButtonFrame:&rect];
    isLeft = (arrow == LEFTARROW);
    if (isLeft) rect.origin.y += rect.size.height;
    isEnabled = conFlags.enabled ? (isLeft ? bpflags->leftButtonIsEnabled : bpflags->rightButtonIsEnabled) : NO;
    isHighlighted = conFlags.enabled ? (isLeft ? bpflags->leftButtonIsHighlighted : bpflags->rightButtonIsHighlighted) : NO;
    switch ((isLeft ? 4 : 0) + (isEnabled ? 0 : 2) + (isHighlighted ? 1 : 0)) {
	case 0:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuRight"];	break;
	case 1:
	case 3:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuRightH"];	break;
	case 2:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuRightD"];	break;
	case 4:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuLeft"];	break;
	case 5:
	case 7:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuLeftH"];	break;
	case 6:
	    arrowImage = [NXImage findImageNamed:"NXscrollMenuLeftD"];	break;
	default:
	    arrowImage = nil;
    }
    if (arrowImage) {
	[arrowImage getSize:&arrowSize];
	p = rect.origin;
	p.x += floor((rect.size.width - arrowSize.width) / 2.0);
	p.y += floor((rect.size.height +
	    ([self isFlipped] ? arrowSize.height : -arrowSize.height)) / 2.0);
	if ([self canDraw]) {
	    if (!lockedFocus) {
		[self lockFocus];
		lockedFocus = YES;
	    }
	    [arrowImage composite:NX_COPY toPoint:&p];
	}
    }

    return lockedFocus;
}

- _drawButtons
{
    int i;

    [self lockFocus];
    [self _drawButton:LEFTARROW lockedFocus:YES];
    [self _drawButton:RIGHTARROW lockedFocus:YES];
    if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
	for (i = 0; i < numVisibleColumns; i++) {
	    [self _drawUpDownButtonsInColumn:i+bp->firstVisibleColumn];
	}
    }
    [self unlockFocus];

    return self;
}

- _updateButtons
{
    int i;
    BOOL isEnabled, lockedFocus = NO;

    isEnabled = (bp->firstVisibleColumn > 0);
    if (bpflags->leftButtonIsEnabled != isEnabled) {
	bpflags->leftButtonIsEnabled = isEnabled;
	lockedFocus = [self _drawButton:LEFTARROW lockedFocus:lockedFocus];
    }
    isEnabled = (bp->firstVisibleColumn + numVisibleColumns < [bp->columns count]);
    if (bpflags->rightButtonIsEnabled != isEnabled) {
	bpflags->rightButtonIsEnabled = isEnabled;
	lockedFocus = [self _drawButton:RIGHTARROW lockedFocus:lockedFocus];
    }

    if (lockedFocus) {
	[window flushWindow];
	[self unlockFocus];
    }

    if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
	for (i = 0; i < numVisibleColumns; i++) {
	    [self _updateUpDownButtonsInColumn:i+bp->firstVisibleColumn];
	}
    }

    return self;
}

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    [super initFrame:frameRect];

    _private = NXZoneCalloc([self zone], 1, sizeof(BrowserPrivate));

    bp->minColumnWidth = DEFAULT_COLUMN_WIDTH;

    pathSeparator = '/';
    matrixClass = [Matrix class];
    cellPrototype = [[NXBrowserCell allocFromZone:[self zone]] init];

    [self setTitled:YES];
    [self getTitleFromPreviousColumn:YES];
    [self useScrollBars:YES];

    [self setClipping:NO];

    return [self tile];
}

- free
{
    int i;
    id column, matrix;

    [bp->unusedColumns freeObjects];
    [bp->unusedColumns free];
    [cellPrototype free];
    [bp->columns makeObjectsPerform:@selector(removeFromSuperview)];
    for (i = [bp->columns count]-1; i >= 0; i--) {
	column = [bp->columns removeObjectAt:i];
	matrix = [column setDocView:nil];
	[matrix setPrototype:nil];
	[matrix free];
	[column free];
    }
    for (i = [subviews count] - 1; i >= 0; i--) {
	matrix = [subviews objectAt:i];
	if ([matrix isKindOf:[Matrix class]] && [matrix prototype] == buttonPrototype) {
	    [matrix removeFromSuperview];
	    [matrix setPrototype:nil];
	    [matrix free];
	}
    }
    [bp->columns free];
    [titles freeObjects];
    [titles free];
    free(bp->firstColumnTitle);

    return [super free];
}

- (SEL)action
{
    return action;
}

- setAction:(SEL)aSelector
{
    action = aSelector;
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

- (SEL)doubleAction
{
    return doubleAction ? doubleAction : action;
}

- setDoubleAction:(SEL)aSelector
{
    doubleAction = aSelector;
    return self;
}

- setMatrixClass:factoryId
{
    matrixClass = factoryId ? factoryId : matrixClass;
    return self;
}

- cellPrototype
{
    return cellPrototype;
}

- setCellPrototype:aCell
{
    id oldPrototype = cellPrototype;
    if ([aCell isKindOf:[NXBrowserCell class]]) {
	cellPrototype = aCell;
	return oldPrototype;
    }
    return nil;
}

- setCellClass:factoryId
{
    id proto, oldPrototype;

    if (_NXCanAllocInZone(factoryId, @selector(new), @selector(init))) {
	proto = [[factoryId allocFromZone:[self zone]] init];
    } else {
	proto = [factoryId new];
    }

    if (oldPrototype = [self setCellPrototype:proto]) {
	[oldPrototype free];
    } else {
	[proto free];
    }

    return self;
}

- delegate
{
    return delegate;
}

- setDelegate:anObject
{
    if (delegate == anObject) return self;

    bpflags->delegateLoadsCells = [anObject respondsTo:@selector(browser:loadCell:atRow:inColumn:)];
    bpflags->delegateIsVeryLazy = (bpflags->delegateLoadsCells && [anObject respondsTo:@selector(browser:getNumRowsInColumn:)]);
    if (bpflags->delegateIsVeryLazy || [anObject respondsTo:@selector(browser:fillMatrix:inColumn:)]) {
	delegate = anObject;
	bpflags->delegateSetsTitles = [anObject respondsTo:@selector(browser:titleOfColumn:)];
	bpflags->delegateSelectsCells = [anObject respondsTo:@selector(browser:selectCell:inColumn:)];
	bpflags->delegateValidatesColumns = [anObject respondsTo:@selector(browser:columnIsValid:)];
	bpflags->delegateWillScroll = [anObject respondsTo:@selector(browserWillScroll:)];
	bpflags->delegateDidScroll = [anObject respondsTo:@selector(browserDidScroll:)];
    } else {
	delegate = nil;
	bpflags->delegateLoadsCells = NO;
	bpflags->delegateSetsTitles = NO;
	bpflags->delegateSelectsCells = NO;
	bpflags->delegateIsVeryLazy = NO;
    }

    return self;
}

- (BOOL)_allowsMultiSel
{
    return bpflags->allowMultiSel;
}

- allowMultiSel:(BOOL)flag
{
    int i;
    id matrix;

    if ((flag && !bpflags->allowMultiSel) || (bpflags->allowMultiSel && !flag)) {
	bpflags->allowMultiSel = flag ? YES : NO;
	for (i = [self lastColumn]; i >= 0; i--) {
	    matrix = [[bp->columns objectAt:i] docView];
	    [matrix setMode:(flag ? NX_LISTMODE : NX_RADIOMODE)];
	}
    }

    return self;
}

- allowBranchSel:(BOOL)flag
{
    bpflags->allowBranchSel = flag ? YES : NO;
    return self;
}

- reuseColumns:(BOOL)flag
{
    int i;
    id column, matrix;

    if (bpflags->reuseColumns && !flag && bp->unusedColumns) {
	for (i = [bp->unusedColumns count]-1; i >= 0; i--) {
	    column = [bp->unusedColumns removeObjectAt:i];
	    matrix = [column setDocView:nil];
	    [matrix setPrototype:nil];
	    [matrix free];
	    [column free];
	}
	[bp->unusedColumns free];
    }

    bpflags->reuseColumns = flag ? YES : NO;

    return self;
}

- (BOOL)_leftAndRightScrollButtonsAreHidden
{
    return bpflags->hideLeftRightButtons;
}

- hideLeftAndRightScrollButtons:(BOOL)flag
{
    if ((flag && !bpflags->hideLeftRightButtons) || (!flag && bpflags->hideLeftRightButtons)) {
	bpflags->hideLeftRightButtons = flag ? YES : NO;
	[self tile];
	[self update];
    }
    return self;
}

- separateColumns:(BOOL)flag
{
    if ((flag && !bpflags->separateColumns) || (!flag && bpflags->separateColumns)) {
	bpflags->separateColumns = flag ? YES : NO;
	[self tile];
	[self update];
    }
    return self;
}

- (BOOL)columnsAreSeparated
{
    return (bpflags->isTitled || bpflags->separateColumns);
}

- (BOOL)_usesScrollButtons
{
     return bpflags->useScrollButtons;
}

- useScrollButtons:(BOOL)flag
{
    if ((flag && !bpflags->useScrollButtons) || (!flag && bpflags->useScrollButtons)) {
	bpflags->useScrollButtons = flag ? YES : NO;
	[self tile];
	[self update];
    }
    return self;
}

- (BOOL)_usesScrollBars
{
     return bpflags->useScrollBars;
}

- useScrollBars:(BOOL)flag
{
    if ((flag && !bpflags->useScrollBars) || (!flag && bpflags->useScrollBars)) {
	bpflags->useScrollBars = flag ? YES : NO;
	[self tile];
	[self update];
    }
    return self;
}

- acceptArrowKeys:(BOOL)flag
{
    bpflags->acceptArrowKeys = flag ? YES : NO;
    return self;
}

- getTitleFromPreviousColumn:(BOOL)flag
{
    int i;
    id matrix;

    if ((!bpflags->titleFromPrevious && flag) || (bpflags->titleFromPrevious && !flag)) {
	bpflags->titleFromPrevious = flag ? YES : NO;
	if (bpflags->isTitled && bpflags->titleFromPrevious) {
	    for (i = [self lastColumn]; i > 0; i--) {
		matrix = [[bp->columns objectAt:i-1] docView];
		[self setTitle:[[matrix selectedCell] stringValue] ofColumn:i];
	    }
	}
    }

    return self;
}

- setEnabled:(BOOL)flag
{
    int i;
    id view;

    if ((conFlags.enabled && !flag) || (!conFlags.enabled && flag)) {
	conFlags.enabled = flag ? YES : NO;
	[window disableDisplay];
	for (i = [subviews count]-1; i >= 0; i--) {
	    view = [subviews objectAt:i];
	    if ([view isKindOf:[ScrollView class]] || [view isKindOf:[ClipView class]]) {
		view = [view docView];
	    }
	    if ([view respondsTo:@selector(setEnabled:)]) {
		[view setEnabled:flag];
	    }
	}
	if (flag) [self _updateButtons];
	[window reenableDisplay];
	[self update];
    }

    return self;
}


- (BOOL)isTitled
{
    return bpflags->isTitled;
}

- setTitled:(BOOL)flag
{
    int i;
    id matrix;

    if ((flag && !bpflags->isTitled) || (!flag && bpflags->isTitled)) {
	bpflags->isTitled = flag ? YES : NO;
	[self tile];
	[self update];
	if (bpflags->isTitled && bpflags->titleFromPrevious) {
	    for (i = [self lastColumn]; i > 0; i--) {
		matrix = [[bp->columns objectAt:i-1] docView];
		[self setTitle:[[matrix selectedCell] stringValue] ofColumn:i];
	    }
	}
    }

    return self;
}

- (NXRect *)getTitleFrame:(NXRect *)theRect ofColumn:(int)column
{
    if (bpflags->isTitled && [self getFrame:theRect ofColumn:column]) {
	theRect->origin.y += theRect->size.height + TITLE_SPACER_HEIGHT;
	theRect->size.height = [self titleHeight];
	return theRect;
    }
    return NULL;
}

- (const char *)titleOfColumn:(int)column
{
    if (!column && bp->firstColumnTitle) return bp->firstColumnTitle;
    if (column < 0 || column >= [titles count]) return NULL;
    return [[titles objectAt:column] stringValue];
}

- _setTitle:(const char *)aString ofColumn:(int)column clearbg:(BOOL)clearbg
{
    NXRect rect;
    id titleCell;

    if (!column && aString != bp->firstColumnTitle) {
	free(bp->firstColumnTitle);
	bp->firstColumnTitle = NXCopyStringBufferFromZone(aString ? aString : "", [self zone]);
    }

    if (!bpflags->isTitled || column>[self lastColumn] || column<0) return self;

    titleCell = [titles objectAt:column];
    [window disableDisplay];
    [titleCell setStringValue:aString];
    [window reenableDisplay];
    if ([self getTitleFrame:&rect ofColumn:column]) {
	NXInsetRect(&rect, 2.0, 2.0);
	if ([self canDraw]) {
	    [self lockFocus];
	    [self drawTitle:[titleCell stringValue] inRect:&rect ofColumn:column];
	    [window flushWindow];
	    [self unlockFocus];
	} else {
	    vFlags.needsDisplay = YES;
	}
    }

    return self;
}

- setTitle:(const char *)aString ofColumn:(int)column
{
    if (![self isAutodisplay]) [window disableDisplay];
    [self _setTitle:aString ofColumn:column clearbg:YES];
    if (![self isAutodisplay]) [window reenableDisplay];
    return self;
}

static id createTitleCell(NXZone *zone)
{
    id retval = [[TextFieldCell allocFromZone:zone] initTextCell:NULL];
    [retval setTextGray:NX_WHITE];
    [retval setBezeled:NO];
    [retval setFont:[Font newFont:NXBoldSystemFont size:12.0]];
    [retval setAlignment:NX_CENTERED];
    return retval;
}

- loadColumnZero
{
    [self setLastColumn:-1];
    return [self addColumn];
}

- load	/* historical */
{
    return [self loadColumnZero];
}

- _getLoadedCellAtRow:(int)row andColumn:(int)column inMatrix:matrix
{
    id aCell;

    aCell = [matrix cellAt:row :0];
    if (aCell && ![aCell isLoaded]) {
	if ([aCell isKindOf:[NXLazyBrowserCell class]]) {
	    aCell = [cellPrototype copy];
	    if (aCell) {
		[delegate browser:self loadCell:aCell atRow:row inColumn:column];
		[matrix putCell:aCell at:row :0];
	    }
	} else if (aCell) {
	    [delegate browser:self loadCell:aCell atRow:row inColumn:column];
	}
    }

    return aCell;
}

- getLoadedCellAtRow:(int)row inColumn:(int)column
{
    return [self _getLoadedCellAtRow:row andColumn:column inMatrix:[self matrixInColumn:column]];
}

- (BOOL)_selectCell:(const char *)title inColumn:(int)column
{
    int i;
    const char *s;
    id matrix, aCell, cellList;

    if (!title || column < 0 || column > [self lastColumn]) return NO;

    if (bpflags->delegateSelectsCells) return [delegate browser:self selectCell:title inColumn:column];

    matrix = [self matrixInColumn:column];
    cellList = [matrix cellList];
    if (cellList) {
	for (i = [cellList count]-1; i >= 0; i--) {
	    aCell = [self _getLoadedCellAtRow:i andColumn:column inMatrix:matrix];
	    s = [aCell stringValue];
	    if (s && !strcmp(s, title)) {
		[matrix selectCell:aCell];
		[matrix _scrollRowToCenter:i];
		return YES;
	    }
	}
    }

    [matrix selectCellAt:-1 :-1];

    return NO;
}

- setPathSeparator:(unsigned short)charCode
{
    pathSeparator = charCode;
    return self;
}

static BOOL clearSelectedCells(id matrix)
/*
 * Clears all highlighted cells in the matrix and clears the selectedCell.
 */
{
    int rows, cols;
    id cell, window;
    BOOL gotOne = NO;

    window = [matrix window];
    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	cell = [matrix cellAt:rows :0];
	if ([cell isLoaded] && ([cell isHighlighted] || [cell state])) {
	    if (!gotOne) [window disableFlushWindow];
	    gotOne = YES;
	    _NXSetCellParam(cell, NX_CELLSTATE, 0);
	    _NXSetCellParam(cell, NX_CELLHIGHLIGHTED, 0);
	    [matrix drawCellAt:rows :0];
	}
    }
    if (gotOne) {
	[window reenableFlushWindow];
	[window flushWindow];
    }
    [matrix clearSelectedCell];

    return gotOne;
}

- setPath:(const char *)aPath
{
    const char *s;
    int column = 0;
    id matrix, selectedCell;
    char *path, *separator;
    char buffer[MAXPATHLEN+1];

    if (aPath) {
	path = strcpy(buffer, aPath);
	if (*path == pathSeparator) {
	    column = 0;
	    while (*path == pathSeparator) path++;
	} else {
	    column = [self lastColumn];
	}
	separator = path+strlen(path)-1;
	if (*separator == pathSeparator) {
	    while (*separator == pathSeparator) separator--;
	    *(separator+1) = '\0';
	}
    } else {
    	return self;
    }

    if (![bp->columns count]) [self loadColumnZero];

    for (;;) {
	matrix = [self matrixInColumn:column];
	selectedCell = [matrix selectedCell];
	if (!path || !*path) {
	    if (selectedCell) clearSelectedCells(matrix);
	    [self setLastColumn:column];
	    return self;
	}
	separator = strchr(path, pathSeparator);
	if (separator) *separator = '\0';
	s = [selectedCell stringValue];
	if (!s || strcmp(s, path)) {
	    if (![self _selectCell:path inColumn:column]) {
		[self setLastColumn:column];
		return nil;
	    } else {
		selectedCell = [matrix selectedCell];
		if (selectedCell && ![selectedCell isLoaded]) {
		    [delegate browser:self
			     loadCell:[matrix selectedCell]
				atRow:[matrix selectedRow]
			     inColumn:column];
		}
	    }
	    [self setLastColumn:column];
	    if (selectedCell && ![selectedCell isLeaf]) {
		[self addColumn];
	    } else {
		return separator ? nil : self;
	    }
	} else if ([selectedCell isLeaf]) {
	    [self setLastColumn:column];
	    return separator ? nil : self;
	}
	if (separator) *separator = pathSeparator;
	path = separator;
	if (path) while (*path == pathSeparator) path++;
	column++;
    }

    return self;
}

- (char *)getPath:(char *)thePath toColumn:(int)column
{
    int i;
    char *path;
    const char *s;

    if (!thePath || column < 0 || column > [self lastColumn]+1) return NULL;

    path = thePath;
    *path = '\0';
    for (i = 0; i < column; i++) {
	s = [[[self matrixInColumn:i] selectedCell] stringValue];
	if (s) {
	    *path++ = pathSeparator;
	    while (*s) *path++ = *s++;
	    *path = '\0';
	} else {
	    return NULL;
	}
    }

    return thePath;
}

- validateVisibleColumns
{
    int i, lastColumn;

    if (bpflags->delegateValidatesColumns) {
	lastColumn = MIN(bp->firstVisibleColumn+numVisibleColumns-1, [bp->columns count]-1);
	for (i = bp->firstVisibleColumn; i <= lastColumn; i++) {
	    if (![delegate browser:self columnIsValid:i]) {
		[self reloadColumn:i];
	    }
	}
    }

    return self;
}

- _displayColumn:(int)column lockFocus:(BOOL)lockFocus
{
    NXRect rect;

    if (column < 0) return self;

    if (lockFocus) {
	if ([self canDraw]) {
	    [self lockFocus];
	    [window disableFlushWindow];
	} else {
	    vFlags.needsDisplay = YES;
	    return self;
	}
    }

    if ([self getFrame:&rect ofInsideOfColumn:column]) {
	PSsetgray(NX_LTGRAY);
	NXRectFill(&rect);
	if (bpflags->delegateValidatesColumns && ![delegate browser:self columnIsValid:column]) {
	    if (column < [bp->columns count]) [self reloadColumn:column];
	} else {
	    [[bp->columns objectAt:column] display];
	}
    }

    if ([self getTitleFrame:&rect ofColumn:column]) {
	NXInsetRect(&rect, 2.0, 2.0);
	if (column < [bp->columns count]) {
	    [self drawTitle:[[titles objectAt:column] stringValue]
		     inRect:&rect
		   ofColumn:column];
	} else {
	    [self clearTitleInRect:&rect ofColumn:column];
	}
    }

    if (lockFocus) {
	[window reenableFlushWindow];
	[window flushWindow];
	[self unlockFocus];
    }

    return self;
}

- displayColumn:(int)column
{
    return [self _displayColumn:column lockFocus:YES];
}

- reloadColumn:(int)column
{
    char *title;
    int i, rows;
    id matrix, cellList, scrollView;

    matrix = [self matrixInColumn:column];
    title = (char *)[[matrix selectedCell] stringValue];
    if (title) title = NXCopyStringBufferFromZone(title, [self zone]);

    scrollView = [matrix superview];
    [scrollView setDocView:nil];
    [window disableDisplay];

    if (bpflags->delegateIsVeryLazy) {
    	rows = [delegate browser:self getNumRowsInColumn:column];
	[matrix renewRows:rows cols:1];
	[matrix sizeToCells];
	for (i = 0; i < rows; i++) [[[matrix cellAt:i :0] setLoaded:NO] reset];
    } else {
	cellList = [matrix cellList];
	rows = [cellList count];
	for (i = 0; i < rows; i++) [[cellList objectAt:i] reset];
	[matrix renewRows:0 cols:1];
	rows = [delegate browser:self fillMatrix:matrix inColumn:column];
	[matrix renewRows:rows cols:1];
	if (bpflags->delegateLoadsCells) for (i = 0; i < rows; i++) [[cellList objectAt:i] setLoaded:NO];
	[matrix sizeToCells];
    }

    [scrollView setDocView:matrix];
    [window reenableDisplay];
    if (bpflags->useScrollBars) scrollView = [scrollView superview];
    [scrollView display];

    if (!title || ![self _selectCell:title inColumn:column]) {
	[self setLastColumn:column];
    }
    free(title);

    return self;
}

- _updateAllColumns
{
    if (vFlags.disableAutodisplay) {
	vFlags.needsDisplay = YES;
    } else {
	[self displayAllColumns];
    }
    return self;

}

- displayAllColumns
{
    int i;

    if ([self canDraw]) {
	[window disableFlushWindow];
	[self lockFocus];
	for (i = 0; i < numVisibleColumns; i++) {
	    [self _displayColumn:bp->firstVisibleColumn+i lockFocus:NO];
	}
	[self _drawButtons];
	[window reenableFlushWindow];
	[window flushWindow];
	[self unlockFocus];
    } else {
	vFlags.needsDisplay = YES;
    }

    return self;
}

- _unhookSubviews
{
    id retval = subviews;
    subviews = nil;
    bpflags->dontDrawTitles = YES;
    return retval;
}

- _reattachSubviews:svs
{
    subviews = svs;
    bpflags->dontDrawTitles = NO;
    return self;
}

- _createColumn:(const NXRect *)rect
{
    id column;

    if (bpflags->useScrollBars) {
	column = [[ScrollView allocFromZone:[self zone]] initFrame:rect];
	[column setBorderType:NX_NOBORDER];
	[column setHorizScrollerRequired:NO];
	[column setVertScrollerRequired:YES];
	[[column vertScroller] setArrowsPosition:bpflags->useScrollButtons ? NX_SCROLLARROWSMAXEND : NX_SCROLLARROWSNONE];
	[[column vertScroller] checkSpaceForParts];
    } else {
	column = [[ClipView allocFromZone:[self zone]] initFrame:rect];
    }

    return column;
}

- _scrollColumnsRightBy:(int)shiftAmount
/*
 * Allows shifting to [bp->columns count] (non-underbar does not).
 */
{
    int i;
    NXPoint p;
    NXRect rect;
    NXSize cellSize;
    id matrix, scrollView;
    int firstDrawingColumn, commonColumns;

    if (bpflags->delegateWillScroll) [delegate browserWillScroll:self];

    if (bp->firstVisibleColumn - shiftAmount < 0) {
	shiftAmount = bp->firstVisibleColumn;
    } else if (bp->firstVisibleColumn - shiftAmount > (int)[bp->columns count]) {
	shiftAmount = bp->firstVisibleColumn - (int)[bp->columns count];
    }

    bp->firstVisibleColumn -= shiftAmount;

    [window disableDisplay];

    for (i = [self lastColumn]; i >= 0; i--) {
	scrollView = [bp->columns objectAt:i];
	matrix = [scrollView docView];
	if ((!bpflags->useScrollBars && [scrollView isKindOf:[ScrollView class]]) ||
	    (bpflags->useScrollBars && [scrollView isKindOf:[ClipView class]])) {
	    [scrollView setDocView:nil];
	    [scrollView getFrame:&rect];
	    scrollView = [self _createColumn:&rect];
	    [scrollView setDocView:matrix];
	    [[[bp->columns replaceObjectAt:i with:scrollView] removeFromSuperview] free];
	}
	if (bpflags->useScrollBars) {
	    [[scrollView vertScroller] setArrowsPosition:bpflags->useScrollButtons ? NX_SCROLLARROWSMAXEND : NX_SCROLLARROWSNONE];
	    [matrix getCellSize:&cellSize];
	    [scrollView setLineScroll:cellSize.height];
	}
	if ([self getFrame:&rect ofInsideOfColumn:i]) {
	    [scrollView setFrame:&rect];
	    if (![scrollView superview]) [self addSubview:scrollView];
	} else {
	    [scrollView removeFromSuperview];
	}
    }

    [window reenableDisplay];
    [window disableFlushWindow];

    if ([self canDraw]) {
	[self lockFocus];
	firstDrawingColumn = bp->firstVisibleColumn;
	if (NXDrawingStatus == NX_DRAWING && ![self isRotatedOrScaledFromBase]) {
	    if (shiftAmount > 0) {
		commonColumns = numVisibleColumns - shiftAmount;
	    } else {
		commonColumns = numVisibleColumns + shiftAmount;
		if (commonColumns > 0) firstDrawingColumn = bp->firstVisibleColumn + commonColumns;
	    }
	    if (commonColumns > 0) {
		[self _getColumnsFrame:&rect];
		if (bpflags->isTitled) rect.size.height += [self titleHeight] + TITLE_SPACER_HEIGHT;
		if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
		    rect.origin.y -= BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
		    rect.size.height += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
		}
		rect.size.width = - COLUMN_SPACER_WIDTH + (columnSize.width + COLUMN_SPACER_WIDTH) * commonColumns;
		if (shiftAmount < 0) rect.origin.x -= (columnSize.width + COLUMN_SPACER_WIDTH) * shiftAmount;
		p = rect.origin;
		[self convertPoint:&p toView:nil];
		rect.origin.x += (columnSize.width + COLUMN_SPACER_WIDTH) * shiftAmount;
		PScomposite(p.x, p.y, rect.size.width, rect.size.height, [window gState], rect.origin.x, rect.origin.y, NX_COPY);
		shiftAmount = ABS(shiftAmount);
		shiftAmount = MIN(shiftAmount, numVisibleColumns);
	    } else {
		shiftAmount = numVisibleColumns;
	    }
	} else {
	    shiftAmount = numVisibleColumns;
	}
	for (i = 0; i < shiftAmount; i++) [self _displayColumn:firstDrawingColumn+i lockFocus:NO];
	[self unlockFocus];
    } else {
	vFlags.needsDisplay = YES;
    }

    [self _updateButtons];

    if (bpflags->delegateDidScroll) [delegate browserDidScroll:self];

    [window reenableFlushWindow];
    [window flushWindow];

    return self;
}

- scrollColumnsRightBy:(int)shiftAmount
{
    int count;

    count = [self lastColumn];
    if (bp->firstVisibleColumn - shiftAmount > count) {
	if (count) {
	    shiftAmount = bp->firstVisibleColumn - count;
	} else {
	    return self;
	}
    }

    return [self _scrollColumnsRightBy:shiftAmount];
}

- scrollColumnsLeftBy:(int)shiftAmount
{
    return [self scrollColumnsRightBy:- shiftAmount];
}

- _scrollColumnToVisible:(int)column private:(BOOL)privateCall
/*
 * Allows scrolling to [bp->columns count] (non-underbar does not).
 */
{
    int shiftAmount;

    if (column < 0 || column > [bp->columns count] || (column >= bp->firstVisibleColumn && column < (bp->firstVisibleColumn + numVisibleColumns))) return self;

    if (column < bp->firstVisibleColumn) {
	shiftAmount =  bp->firstVisibleColumn - column;
    } else {
	shiftAmount = bp->firstVisibleColumn + numVisibleColumns - 1 - column;
    }

    return privateCall ? [self _scrollColumnsRightBy:shiftAmount] : [self scrollColumnsRightBy:shiftAmount];
}

- scrollColumnToVisible:(int)column
{
    if (column > [self lastColumn]) return self;
    return [self _scrollColumnToVisible:column private:NO];
}

- setLastColumn:(int)column
{
    int i;
    NXRect rect;
    BOOL canDraw;
    id matrix, scrollView;

    if (column < -1 || column > [self lastColumn]) return self;

    MARKTIME2(bpflags->time, "Setting last %s to %d.", (int)"column", column, 2);

    i = [self lastColumn];

    if (i > column) {
	canDraw = [self canDraw];
	if (canDraw) {
	    MARKTIME(bpflags->time, "Locking focus.", 3);
	    [self lockFocus];
	}
	while (i > column) {
	    scrollView = [bp->columns removeObjectAt:i];
	    if (bpflags->reuseColumns) {
		MARKTIME2(bpflags->time, "Removing (for reuse) %s %d.", (int)"column", i, 4);
		if (!bp->unusedColumns) bp->unusedColumns = [[List allocFromZone:[self zone]] init];
		[bp->unusedColumns addObject:scrollView];
		[scrollView removeFromSuperview];
	    } else {
		MARKTIME2(bpflags->time, "Removing (and freeing) %s %d.", (int)"column", i, 4);
		matrix = [scrollView setDocView:nil];
		[matrix setPrototype:nil];
		[matrix free];
		[[scrollView removeFromSuperview] free];
	    }
	    if (canDraw && [self getFrame:&rect ofInsideOfColumn:i]) {
		MARKTIME2(bpflags->time, "Clearing %s %d.", (int)"column", i, 4);
		PSsetgray(NX_LTGRAY);
		NXRectFill(&rect);
		if ([self getTitleFrame:&rect ofColumn:i]) {
		    NXInsetRect(&rect, 2.0, 2.0);
		    [self clearTitleInRect:&rect ofColumn:i];
		}
		[window flushWindow];
	    }
	    i--;
	}
	if (canDraw) {
	    [self unlockFocus];
	} else {
	    vFlags.needsDisplay = YES;
	}
    }

    if (bp->firstVisibleColumn > column) {
	if (column > -1) {
	    MARKTIME(bpflags->time, "Scrolling last column to visible.", 3);
	    [self scrollColumnToVisible:column];
	} else {
	    MARKTIME(bpflags->time, "Last column is -1, clearing all columns.", 3);
	    [self _scrollColumnToVisible:0 private:YES];
	}
    }

    MARKTIME(bpflags->time, "Updating buttons.", 3);
    [self _updateButtons];

    MARKTIME(bpflags->time, "Done with setLastColumn:", 2);

    return self;
}

- _kludgeScrollBar
{
    id scroller;
    NXRect rect, urect;

    if (bpflags->useScrollBars) {
	scroller = [[bp->columns lastObject] vertScroller];
	if ([scroller canDraw]) {
	    [scroller calcRect:&urect forPart:NX_KNOBSLOT];
	    [scroller calcRect:&rect forPart:NX_INCLINE];
	    NXUnionRect(&rect, &urect);
	    [scroller calcRect:&rect forPart:NX_DECLINE];
	    NXUnionRect(&rect, &urect);
	    [scroller lockFocus];
	    _NXSetGrayUsingPattern (0.5);
	    NXRectFill(&urect);
	    [scroller unlockFocus];
	}
    }

    return self;
}

- addColumn
{
    int i, rows;
    NXRect rect;
    NXSize size;
    const char *title;
    NXPoint theOrigin;
    id aCell, cellList, column, matrix, reuse, titleCell;

    MARKTIME2(bpflags->time, "Adding %s %d.", (int)"column", [bp->columns count], 2);

    reuse = bpflags->reuseColumns ? [[bp->unusedColumns objectAt:0] docView] : nil;

    if (matrix = reuse) {
	MARKTIME(bpflags->time, "Renewing rows in reused column.", 3);
	[matrix renewRows:0 cols:1];
	cellList = [matrix cellList];
	rows = [cellList count];
    } else {
	MARKTIME(bpflags->time, "Creating Matrix.", 3);
	if (_NXCanAllocInZone(matrixClass, @selector(newFrame:), @selector(initFrame:))) {
	    matrix = [[matrixClass allocFromZone:[self zone]] initFrame:NULL
				    mode:NX_TRACKMODE
				    prototype:cellPrototype
				    numRows:0
				    numCols:1];
	} else {
	    matrix = [matrixClass newFrame:NULL
				    mode:NX_TRACKMODE
				    prototype:cellPrototype
				    numRows:0
				    numCols:1];
	}
	cellList = [matrix cellList];
	rows = 0;
    }

    if (bpflags->delegateIsVeryLazy) {
	MARKTIME(bpflags->time, "Calling getNumRowsInColumn:", 3);
	aCell = [matrix setPrototype:nil];
	[matrix setCellClass:[NXLazyBrowserCell class]];
	[matrix renewRows:(rows = [delegate browser:self getNumRowsInColumn:[bp->columns count]]) cols:1];
	[matrix setPrototype:aCell];
	if (matrix == reuse) for (i = 0; i < rows; i++) [[[cellList objectAt:i] setLoaded:NO] reset];
    } else {
	for (i = 0; i < rows; i++) [[cellList objectAt:i] reset];
	MARKTIME(bpflags->time, "Calling fillMatrix:", 3);
	rows = [delegate browser:self
			fillMatrix:matrix
			inColumn:[bp->columns count]];
	[matrix renewRows:rows cols:1];
	if (bpflags->delegateLoadsCells) for (i = 0; i < rows; i++) [[cellList objectAt:i] setLoaded:NO];
    }

    [matrix setTarget:self];
    [matrix setAction:@selector(doClick:)];
    [matrix setDoubleAction:@selector(doDoubleClick:)];
    [matrix setAutoscroll:YES];
    [matrix setClipping:NO];

    [window disableFlushWindow];

    MARKTIME(bpflags->time, "Scrolling the column to visible.", 3);
    [self _scrollColumnToVisible:[bp->columns count] private:YES];

    MARKTIME(bpflags->time, "Attaching Matrix to the scroll view.", 3);
    [self getFrame:&rect ofInsideOfColumn:[bp->columns count]];
    if (matrix == reuse) {
	column = [bp->unusedColumns removeObjectAt:0];
	if ((!bpflags->useScrollBars && [column isKindOf:[ScrollView class]]) ||
	    (bpflags->useScrollBars && [column isKindOf:[ClipView class]])) {
		MARKTIME(bpflags->time, "Reconfiguring ScrollView/ClipView state.", 4);
		[column setDocView:nil];
		[column free];
		column = [self _createColumn:&rect];
		[column setDocView:matrix];
	} else {
	    MARKTIME(bpflags->time, "Resetting Matrix origin.", 4);
	    [column setFrame:&rect];
	    theOrigin.x = theOrigin.y = 0.0;
	    [matrix scrollPoint:&theOrigin];
	}
    } else {
	MARKTIME(bpflags->time, "Creating column.", 4);
	column = [self _createColumn:&rect];
	[column setDocView:matrix];
    }

    MARKTIME(bpflags->time, "Determining Matrix size.", 3);
    size.width = size.height = 0.0;
    [matrix setIntercell:&size];
    aCell = cellPrototype;
    if (![aCell stringValue] || !strlen([aCell stringValue])) [aCell setStringValueNoCopy:" "];
    [aCell calcCellSize:&size];
    if (bpflags->useScrollBars) {
	[ScrollView getContentSize:&rect.size
			forFrameSize:&rect.size
			horizScroller:NO
			vertScroller:YES
			borderType:NX_NOBORDER];
    }
    size.width = rect.size.width;
    MARKTIME(bpflags->time, "Setting Matrix size.", 3);
    [matrix setCellSize:&size];
    [matrix sizeToCells];
    [matrix allowEmptySel:YES];
    [matrix setMode:(bpflags->allowMultiSel ? NX_LISTMODE : NX_RADIOMODE)];

    if (!bp->columns) bp->columns = [[List allocFromZone:[self zone]] init];
    [bp->columns addObject:column];

    if (bpflags->isTitled) {
	MARKTIME(bpflags->time, "Setting title.", 3);
	if ([titles count] < [bp->columns count]) {
	    if (!titles) titles = [[List allocFromZone:[self zone]] init];
	    titleCell = createTitleCell([self zone]);
	    [titles addObject:titleCell];
	}
	if (bpflags->titleFromPrevious && [bp->columns count] > 1) {
	    title = [[[[bp->columns objectAt:[self lastColumn]-1] docView] selectedCell] stringValue];
	} else if (bpflags->delegateSetsTitles) {
	    title = [delegate browser:self titleOfColumn:[self lastColumn]];
	} else if ([bp->columns count] == 1 && bp->firstColumnTitle) {
	    title = bp->firstColumnTitle;
	} else {
	    title = NULL;
	}
	[self _setTitle:title ofColumn:[self lastColumn] clearbg:NO];
    }

    MARKTIME(bpflags->time, "Adding column to view hierarchy.", 3);
    [self addSubview:column];
    MARKTIME(bpflags->time, "Kludging the scroll bar.", 3);
    [self _kludgeScrollBar];
    MARKTIME(bpflags->time, "Displaying the column.", 3);
    [column display];
    MARKTIME(bpflags->time, "Updating the buttons.", 3);
    [self _updateButtons];

    MARKTIME(bpflags->time, "Flushing.", 3);
    [window reenableFlushWindow];
    [window flushWindow];

    MARKTIME(bpflags->time, "Done with addColumn.", 2);

    return self;
}

- setMinColumnWidth:(int)columnWidth
{
    if (bp->minColumnWidth != columnWidth) {
	bp->minColumnWidth = columnWidth;
	[self tile];
	[self update];
    }
    return self;
}

- (int)minColumnWidth
{
    return bp->minColumnWidth;
}

- setMaxVisibleColumns:(int)columnCount
{
    if (bp->maxVisibleColumns != columnCount) {
	bp->maxVisibleColumns = columnCount;
	[self tile];
	[self update];
    }
    return self;
}

- (int)maxVisibleColumns
{
    return bp->maxVisibleColumns;
}

- (int)firstVisibleColumn
{
    return bp->firstVisibleColumn;
}

- (int)lastVisibleColumn
{
    int lvc = bp->firstVisibleColumn + numVisibleColumns - 1;
    return MIN(lvc, [self lastColumn]);
}

- (int)lastColumn
{
    return ((int)[bp->columns count]) - 1;
}

- (int)selectedColumn
{
    int column = [self lastColumn];

    while (column > -1) {
	if ([[self matrixInColumn:column] selectedCell]) break;
	column--;
    }

    return column;
}

- (BOOL)isLoaded
{
    return [self lastColumn] >= 0;
}

- (int)columnOf:matrix
{
    int i;

    if (matrix) {
	for (i = [self lastColumn]; i >= 0; i--) {
	    if ([[bp->columns objectAt:i] docView] == matrix) return i;
	}
    }

    return -1;
}

- matrixInColumn:(int)column
{
    if (column >= 0 && column <= [self lastColumn]) {
	return [[bp->columns objectAt:column] docView];
    } else {
	return nil;
    }
}

- (NXRect *)getFrame:(NXRect *)theRect ofColumn:(int)column
{
    if (!theRect || column < 0 || column < bp->firstVisibleColumn || column >= bp->firstVisibleColumn + numVisibleColumns) return NULL;

    [self _getColumnsFrame:theRect];
    theRect->size = columnSize;
    theRect->origin.x += (column - bp->firstVisibleColumn) * (columnSize.width + COLUMN_SPACER_WIDTH);

    return theRect;
}

- (NXRect *)getFrame:(NXRect *)theRect ofInsideOfColumn:(int)column
{
    theRect = [self getFrame:theRect ofColumn:column];
    if (theRect) {
	if ([self columnsAreSeparated]) {
	    NXInsetRect(theRect, 2.0, 1.0);
	    theRect->size.height -= 1.0;
	} else {
	    theRect->size.width -= 1.0;
	}
    }
    return theRect;
}

- drawTitle:(const char *)title inRect:(const NXRect *)aRect ofColumn:(int)column
{
    BOOL lockedFocus = NO;

    if (!aRect) return self;

    if ([NXApp focusView] != self) {
	if ([self canDraw]) {
	    [self lockFocus];
	    lockedFocus = YES;
	} else {
	    vFlags.needsDisplay = YES;
	    return self;
	}
    }
    PSsetgray(NX_DKGRAY);
    NXRectFill(aRect);
    if (title && !bpflags->dontDrawTitles) [[titles objectAt:column] drawSelf:aRect inView:self];
    if (lockedFocus) {
	[window flushWindow];
	[self unlockFocus];
    }

    return self;
}

- clearTitleInRect:(const NXRect *)aRect ofColumn:(int)column
{
    return [self drawTitle:NULL inRect:aRect ofColumn:column];
}

- (NXCoord)titleHeight
{
    return TITLE_HEIGHT;
}

- (NXSize *)_getColumnSize:(NXSize *)cs
{
    if (cs) *cs = columnSize;
    return cs;
}

- _calcNumVisibleColumnsAndColumnSize
{
    NXRect rect;
    NXSize svsize, dvsize;

    [self _calcColumnsFrame:&rect];
    dvsize.height = rect.size.height;
    dvsize.width = bp->minColumnWidth;
    if (bpflags->useScrollBars) {
	[ScrollView getFrameSize:&svsize
		    forContentSize:&dvsize
		    horizScroller:NO
		    vertScroller:YES
			borderType:NX_NOBORDER];
    } else {
	svsize = dvsize;
    }
    numVisibleColumns = (rect.size.width + COLUMN_SPACER_WIDTH) / svsize.width;
    numVisibleColumns = MAX(numVisibleColumns, 1);
    if (bp->maxVisibleColumns > 0) numVisibleColumns = MIN(numVisibleColumns, bp->maxVisibleColumns);
    columnSize.width = floor((rect.size.width + COLUMN_SPACER_WIDTH) / numVisibleColumns) - COLUMN_SPACER_WIDTH;
    if (!bpflags->useScrollBars && bpflags->useScrollButtons) columnSize.width = floor(columnSize.width / 2.0) * 2.0;
    if ([self columnsAreSeparated]) {
	columnSize.height = floor(rect.size.height / 2.0) * 2.0;
    } else if (rect.size.height == floor(rect.size.height / 2.0) * 2.0) {
	columnSize.height = rect.size.height - 1.0;
    } else {
	columnSize.height = rect.size.height;
    }

    return self;
}

- tile
/*
 * Never draws anything.
 */
{
    id matrix;
    NXSize cellSize, dvsize;
    int i, lastVisibleColumn;

    [window disableDisplay];

    [self _calcNumVisibleColumnsAndColumnSize];

    for (i = [self lastColumn]; i >= 0; i--) [[bp->columns objectAt:i] removeFromSuperview];

    [self _addButtons];

    lastVisibleColumn = bp->firstVisibleColumn + numVisibleColumns - 1;
    if ([bp->columns count] && [self lastColumn] < lastVisibleColumn) {
	[self scrollColumnsRightBy:lastVisibleColumn - [self lastColumn]];
    } else {
	[self scrollColumnsRightBy:0];
    }

    if (bpflags->useScrollBars) {
	[ScrollView getContentSize:&dvsize
			forFrameSize:&columnSize
			horizScroller:NO
			vertScroller:YES
			borderType:NX_NOBORDER];
    } else {
	dvsize = columnSize;
    }

    for (i = [self lastColumn]; i >= 0; i--) {
	matrix = [[bp->columns objectAt:i] docView];
	[matrix getCellSize:&cellSize];
	cellSize.width = dvsize.width - ([self columnsAreSeparated] ? 4.0 : 1.0);
	[matrix setCellSize:&cellSize];
	[matrix sizeToCells];
    }

    [window reenableDisplay];

    return self;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    int i, column;
    BOOL canUseCompositing;
    NXPoint compositeOrigin;
    NXRect rect, titleRect, buttonRect;

    PSsetgray(NX_LTGRAY);
    NXRectFill(rects);

    canUseCompositing = (NXDrawingStatus == NX_DRAWING && ![self isRotatedOrScaledFromBase]);

    if (!bpflags->hideLeftRightButtons) {
	[self _getRightButtonFrame:&rect];
	NXDrawButton(&rect, rects);
	rect.origin.y += rect.size.height;
	if (canUseCompositing) {
	    if (NXIntersectsRect(&rect, rects)) {
		compositeOrigin.x = rect.origin.x;
		compositeOrigin.y = rect.origin.y - rect.size.height;
		[self convertPoint:&compositeOrigin toView:nil];
		PScomposite(compositeOrigin.x, compositeOrigin.y,
			    rect.size.width, rect.size.height,
			    [window gState],
			    rect.origin.x, rect.origin.y,
			    NX_COPY);
	    }
	} else {
	    NXDrawButton(&rect, rects);
	}
    }

    column = bp->firstVisibleColumn;

    if ([self columnsAreSeparated]) {
	[self getFrame:&rect ofColumn:column];
	NXDrawGrayBezel(&rect, rects);
    } else {
	[self _getColumnsFrame:&rect];
	NXInsetRect(&rect, -2.0, -1.0);
	rect.size.width -= 1.0;
	rect.size.height += 1.0;
	NXDrawGrayBezel(&rect, rects);
	[self getFrame:&rect ofColumn:column];
    }
    if ([self getTitleFrame:&titleRect ofColumn:column]) NXDrawGrayBezel(&titleRect, rects);
    if ([self _getDownButtonFrame:&buttonRect inColumn:column]) {
	NXDrawButton(&buttonRect, rects);
	buttonRect.origin.x += buttonRect.size.width;
	if (canUseCompositing) {
	    if (NXIntersectsRect(&buttonRect, rects)) {
		compositeOrigin.x = buttonRect.origin.x - buttonRect.size.width;
		compositeOrigin.y = buttonRect.origin.y;
		[self convertPoint:&compositeOrigin toView:nil];
		PScomposite(compositeOrigin.x, compositeOrigin.y,
			    buttonRect.size.width, buttonRect.size.height,
			    [window gState],
			    buttonRect.origin.x, buttonRect.origin.y,
			    NX_COPY);
	    }
	} else {
	    NXDrawButton(&buttonRect, rects);
	}
    }

    if (canUseCompositing) {
	compositeOrigin = [self _getDownButtonFrame:&buttonRect inColumn:column] ? buttonRect.origin : rect.origin;
	[self convertPoint:&compositeOrigin toView:nil];
    }

    while (++column < bp->firstVisibleColumn + numVisibleColumns) {
	if (![self columnsAreSeparated]) {
	    rect.origin.x += rect.size.width;
	    rect.size.width = 1.0;
	    if (NXIntersectsRect(&rect, rects)) {
		PSsetgray(NX_BLACK);
		NXRectFill(&rect);
	    }
	}
	[self getFrame:&rect ofColumn:column];
	if (canUseCompositing) {
	    if ([self getTitleFrame:&titleRect ofColumn:column] && NXIntersectsRect(&titleRect, rects)) NXUnionRect(&titleRect, &rect);
	    if ([self _getDownButtonFrame:&buttonRect inColumn:column]) NXUnionRect(&buttonRect, &rect);
	    if (!rects || NXIntersectsRect(&rect, rects)) {
		PScomposite(compositeOrigin.x, compositeOrigin.y,
			    rect.size.width, rect.size.height,
			    [window gState],
			    rect.origin.x, rect.origin.y,
			    NX_COPY);
	    }
	} else {
	    if ([self columnsAreSeparated]) NXDrawGrayBezel(&rect, rects);
	    if ([self getTitleFrame:&titleRect ofColumn:column]) NXDrawGrayBezel(&titleRect, rects);
	    if ([self _getDownButtonFrame:&buttonRect inColumn:column]) {
		NXDrawButton(&buttonRect, rects);
		buttonRect.origin.x += buttonRect.size.width;
		NXDrawButton(&buttonRect, rects);
	    }
	}
    }

    if (![bp->columns count]) {
	[window disableDisplay];
	[self loadColumnZero];
	[window reenableDisplay];
    }

    if (bpflags->isTitled) {
	for (i = 0; i < numVisibleColumns; i++) {
	    column = bp->firstVisibleColumn+i;
	    if ([self getTitleFrame:&rect ofColumn:column] && (!rects || NXIntersectsRect(rects, &rect))) {
		NXInsetRect(&rect, 2.0, 2.0);
		if (column < [bp->columns count]) {
		    [self drawTitle:[[titles objectAt:column] stringValue]
				inRect:&rect
			    ofColumn:column];
		} else {
		    [self clearTitleInRect:&rect ofColumn:column];
		}
	    }
	}
    }

    [self _drawButton:LEFTARROW lockedFocus:YES];
    [self _drawButton:RIGHTARROW lockedFocus:YES];

    return self;
}
    
static BOOL mouseHit(id view, NXRect *r, NXPoint *p)
{
    NXPoint             localPoint = *p;
    [view convertPoint:&localPoint fromView:nil];
    return (NXMouseInRect(&localPoint, r, [view isFlipped]));
}

#define BROWSERMASK (NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK|NX_TIMERMASK)

- mouseDown:(NXEvent *)theEvent
{
    int                 oldMask;
    NXRect		left, right;
    NXRect		leftInside, rightInside;
    BOOL		oldLeftHit, oldRightHit, leftHit = NO, rightHit = NO;
    NXTrackingTimer	timer;
    const char	       *speed;

    if (bpflags->hideLeftRightButtons || !conFlags.enabled) return self;

    [self _getRightButtonFrame:&right];
    left = right;
    left.origin.y += left.size.height;
    rightInside = right;
    NXInsetRect(&rightInside, 1.0, 2.0);
    rightInside.size.width -= 1.0;
    rightInside.size.height += 1.0;
    leftInside = left;
    NXInsetRect(&leftInside, 1.0, 2.0);
    leftInside.size.width -= 1.0;
    leftInside.size.height += 1.0;

    oldMask = [window addToEventMask:BROWSERMASK];

    if ([self isEnabled]) {
	[self lockFocus];
	speed = NXGetDefaultValue(NXSystemDomainName, "BrowserSpeed");
	NXBeginTimer(&timer, 0.1, atof(speed)/1000.0);
	for (;;) {
	    oldLeftHit = leftHit; oldRightHit = rightHit;
	    if (theEvent->type == NX_MOUSEUP) {
		rightHit = leftHit = NO;
	    } else if (theEvent->type == NX_TIMER) {
		if (!bpflags->rightButtonIsEnabled) rightHit = NO;
		if (!bpflags->leftButtonIsEnabled) leftHit = NO;
	    } else {
		rightHit = bpflags->rightButtonIsEnabled ? mouseHit(self, &right, &theEvent->location) : NO;
		leftHit = bpflags->leftButtonIsEnabled ? mouseHit(self, &left, &theEvent->location) : NO;
	    }
	    if (rightHit != bpflags->rightButtonIsHighlighted) {
		NXHighlightRect(&rightInside);
		bpflags->rightButtonIsHighlighted = rightHit;
		if (!bpflags->rightButtonIsEnabled) [self _drawButton:RIGHTARROW lockedFocus:YES];
		[window flushWindow];
	    }
	    if (leftHit != bpflags->leftButtonIsHighlighted) {
		NXHighlightRect(&leftInside);
		bpflags->leftButtonIsHighlighted = leftHit;
		if (!bpflags->leftButtonIsEnabled) [self _drawButton:LEFTARROW lockedFocus:YES];
		[window flushWindow];
	    }
	    if (theEvent->type == NX_MOUSEUP) break;
	    if (theEvent->type != NX_MOUSEDRAGGED || leftHit != oldLeftHit || rightHit != oldRightHit) {
		if (leftHit) {
		    [self scrollColumnsRightBy:1];
		} else if (rightHit) {
		    [self scrollColumnsLeftBy:1];
		}
	    }
	    theEvent = [NXApp getNextEvent:BROWSERMASK];
	}
	NXEndTimer(&timer);
	[self unlockFocus];
    } else {
	theEvent = [NXApp getNextEvent:NX_MOUSEUPMASK];
    }

    [window setEventMask:oldMask];

    return self;
}

- _bumpSelectedItem:(BOOL)down
{
    int dummy, row, numRows, column;
    id matrix, selectedCell, reselectedCell;

    column = bp->firstVisibleColumn + numVisibleColumns - 1;
    column = MIN([bp->columns count] - 1, column);
    matrix = [self matrixInColumn:column];
    [matrix getNumRows:&numRows numCols:&dummy];
    if (!numRows && column > 0) {
	matrix = [self matrixInColumn:column-1];
	[matrix getNumRows:&numRows numCols:&dummy];
    }
    selectedCell = [matrix selectedCell];
    if (selectedCell) {
	row = [matrix selectedRow];
	if (down) {
	    row++;
	    if (row >= numRows) row = 0;
	} else {
	    row--;
	    if (row < 0) row = numRows - 1;
	}
    } else {
	row = down ? 0 : numRows - 1;
    }
    reselectedCell = [self _getLoadedCellAtRow:row andColumn:column inMatrix:matrix];
    if (reselectedCell && reselectedCell != selectedCell) {
	[matrix selectCellAt:row :0];
	[matrix scrollCellToVisible:row :0];
    }

    return self;
}

- keyDown:(NXEvent *)event
/*
 * Handles one of the arrow keys being pressed.
 * Note that since it might take a while to actually move the selection
 * (if it is large), we check to see if a bunch of arrow key events have
 * stacked up and move them all at once.
 */
{
    id theCell, matrix;

    if (event->data.key.charSet == NX_SYMBOLSET) {
	switch (event->data.key.charCode) {
	    case LEFTARROW:
		if ([self lastColumn] > 0) {
		    [self setLastColumn:[self lastColumn]-1];
		    [self scrollColumnToVisible:[self lastColumn]];
		}
		return self;
	    case RIGHTARROW:
		matrix = [self matrixInColumn:[self lastColumn]];
		theCell = [matrix selectedCell];
		if (theCell && ![theCell isLeaf]) {
		    [matrix selectCell:theCell];
		    [self addColumn];
		}
		return [self scrollColumnToVisible:[self lastColumn]];
	    case UPARROW: return [self _bumpSelectedItem:NO];
	    case DOWNARROW: return [self _bumpSelectedItem:YES];
	}
    }

    return [super keyDown:event];
}

- (BOOL)acceptsFirstResponder
{
    return bpflags->acceptArrowKeys;
}

- sizeTo:(NXCoord)width :(NXCoord)height
{
    [super sizeTo:width :height];
    [self tile];
    [self scrollColumnToVisible:[self lastColumn]];
    return self;
}

- sizeToFit
{
    NXCoord width, height;

    width =  numVisibleColumns * (columnSize.width + COLUMN_SPACER_WIDTH) - COLUMN_SPACER_WIDTH;
    if (!bpflags->hideLeftRightButtons) width += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
    if (![self columnsAreSeparated]) width += 3.0;
    height = columnSize.height;
    if (bpflags->isTitled) height += [self titleHeight] + TITLE_SPACER_HEIGHT;
    if (!bpflags->useScrollBars && bpflags->useScrollButtons) height += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
    if (![self columnsAreSeparated]) height += 3.0;

    return [super sizeTo:width :height];
}

- selectAll:sender
{
    int rows, cols;
    NXRect cellFrame;
    id aCell, matrix;
    BOOL lockedFocus = NO;

    if (bpflags->allowMultiSel) {
	matrix = [self matrixInColumn:[self lastColumn]];
	[matrix selectAll:sender];
	[matrix getNumRows:&rows numCols:&cols];
	if (!bpflags->allowBranchSel) {
	    while (rows--) {
		aCell = [matrix cellAt:rows :0];
		if (![aCell isLeaf]) {
		    if (!lockedFocus) {
			lockedFocus = YES;
			[matrix lockFocus];
		    }
		    [matrix getCellFrame:&cellFrame at:rows :0];
		    [aCell setState:0];
		    [aCell highlight:&cellFrame inView:sender lit:NO];
		}
	    }
	    if (lockedFocus) {
		[window flushWindow];
		[matrix unlockFocus];
	    }
	}
    }

    return self;
}

- doClick:sender
{
    NXRect cellFrame;
    int i, count, column;
    BOOL dontAddColumn = NO;
    id aCell, selectedCell, selectedLeaf = nil;

#ifdef AKDEBUG
    BOOL timeStats = (bpflags->time && [delegate respondsTo:@selector(_resetTimers)]);
    if (timeStats) [delegate _resetTimers];
#endif

    MARKTIME(bpflags->time, "Processing NXBrowser click.", 1);

    [window disableFlushWindow];

    selectedCell = [sender selectedCell];
    if (selectedCell && bpflags->allowMultiSel) {
	for (count = 0, i = [sender cellCount]-1; i >= 0; i--) {
	    aCell = [sender cellAt:i :0];
	    if ([aCell isHighlighted] || [aCell state]) {
		if (bpflags->allowBranchSel || [aCell isLeaf]) {
		    count++;
		    selectedLeaf = aCell;
		} else {
		    [sender getCellFrame:&cellFrame at:i :0];
		    [aCell setState:0];
		    [aCell highlight:&cellFrame inView:sender lit:NO];
		}
	    }
	}
	if (!bpflags->allowBranchSel) {
	    if (!count) {
		[sender selectCell:selectedCell];
	    } else if (![selectedCell isLeaf]) {
		[sender _setSelectedCell:selectedLeaf];
		selectedCell = selectedLeaf;
	    }
	} else {
	    dontAddColumn = (count > 1);
	}
    }

    if ((column = [self columnOf:sender]) >= 0) {
	[self setLastColumn:column];
	if (!dontAddColumn && selectedCell && ![selectedCell isLeaf]) {
	    if (numVisibleColumns == 1) {
		[sender setReaction:YES];
		[sender unlockFocus];
	    }
	    [self addColumn];
	}
    }

    [window reenableFlushWindow];
    [window flushWindow];

    MARKTIME(bpflags->time, "Sending action to target.", 1);

    [self sendAction:[self action] to:[self target]];

    DUMPTIMES(bpflags->time);
    CLEARTIMES(bpflags->time);

#ifdef AKDEBUG
    if (timeStats) [delegate _stopTimers:"doClick"];
#endif

    return self;
}

- doDoubleClick:sender
{
    [self sendAction:[self doubleAction] to:[self target]];
    return self;
}

- scrollUpOrDown:sender
{
    id matrix;
    NXRect mframe;
    NXSize cellSize;
    int visibleColumn;

    visibleColumn = [sender tag];
    matrix = [self matrixInColumn:bp->firstVisibleColumn+visibleColumn];
    if (matrix) {
	[[matrix superview] getBounds:&mframe];
	[matrix getCellSize:&cellSize];
	mframe.origin.y += [sender selectedCol] ? - cellSize.height : cellSize.height;
	[matrix scrollPoint:&mframe.origin];
    }

    return self;
}

- reflectScroll:clipView
{
    int i;

    if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
	for (i = [bp->columns count]-1; i >= 0; i--) if ([bp->columns objectAt:i] == clipView) break;
	if (i >= 0) [self _updateUpDownButtonsInColumn:i];
    }

    return self;
}

/*
 * The following methods get the frame of various components of the
 * browser.  They should always be used (don't compute the position of
 * any element of the browser anywhere except in a method such as these).
 * The method sizeToFit should be kept up to date with the getFrame:
 * methods.
 */

- (NXRect *)_getRightButtonFrame:(NXRect *)theRect
/*
 * The left button is always assumed to be directly on top of the right one.
 */
{
    if (theRect && !bpflags->hideLeftRightButtons) {
	[self _getColumnsFrame:theRect];
	if (![self columnsAreSeparated]) {
	    NXInsetRect(theRect, -2.0, -1.0);
	    theRect->size.width -= 1.0;
	    theRect->size.height += 1.0;
	}
	theRect->origin.x = bounds.origin.x;
	theRect->size.width = BUTTON_WIDTH;
	theRect->size.height = floor(theRect->size.height / 2.0);
	return theRect;
    } else {
	return NULL;
    }
}

- (NXRect *)_getDownButtonFrame:(NXRect *)theRect inColumn:(int)column
{
    if (!bpflags->useScrollBars && bpflags->useScrollButtons && [self getFrame:theRect ofColumn:column]) {
	if (![self columnsAreSeparated]) theRect->origin.y -= 1.0;
	theRect->origin.y -= BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	theRect->size.height = BUTTON_WIDTH;
	theRect->size.width = floor(theRect->size.width / 2.0);
	return theRect;
    }
    return NULL;
}

- (NXRect *)_getColumnsFrame:(NXRect *)theRect
/*
 * This returns the bounds of the rectangle including all the bp->columns.
 */
{
    if (theRect) {
	*theRect = bounds;
	theRect->size.width = (columnSize.width + COLUMN_SPACER_WIDTH) * numVisibleColumns - COLUMN_SPACER_WIDTH;
	theRect->size.height = columnSize.height;
	if (!bpflags->hideLeftRightButtons) theRect->origin.x += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	if (!bpflags->useScrollBars && bpflags->useScrollButtons) theRect->origin.y += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	if (![self columnsAreSeparated]) {
	    theRect->origin.x += 2.0;
	    theRect->origin.y += 1.0;
	}
    }
    return theRect;
}

- (NXRect *)_calcColumnsFrame:(NXRect *)theRect
/*
 * This calculates the rectangle that includes all of the bp->columns.
 */
{
    if (theRect) {
	*theRect = bounds;
	if (!bpflags->hideLeftRightButtons) {
	    theRect->origin.x += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	    theRect->size.width -= BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	}
	if (!bpflags->useScrollBars && bpflags->useScrollButtons) {
	    theRect->origin.y += BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	    theRect->size.height -= BUTTON_WIDTH + BUTTON_SPACER_WIDTH;
	}
	if (bpflags->isTitled) theRect->size.height -= [self titleHeight] + TITLE_SPACER_HEIGHT;
	if (![self columnsAreSeparated]) {
	    NXInsetRect(theRect, 2.0, 1.0);
	    theRect->size.width += 1.0;
	    theRect->size.height -= 1.0;
	}
    }

    return theRect;
}

- write:(NXTypedStream *)s
{
    id saveSubviews;
    char pathbuf[MAXPATHLEN+1];
    char *path;

    saveSubviews = subviews;
    subviews = nil;
    [super write:s];
    path = [self getPath:pathbuf toColumn:[self lastColumn]];
    NXWriteTypes(s, "@@::#@ssssi**", &target, &delegate, &action, &doubleAction,
	&matrixClass, &cellPrototype, &bp->minColumnWidth, &numVisibleColumns,
	&bp->maxVisibleColumns, &pathSeparator, bpflags, &path, &bp->firstColumnTitle);
    subviews = saveSubviews;

    return self;    
}

- read:(NXTypedStream *)s
{
    int version;

    [super read:s];
    _private = NXZoneCalloc([self zone], 1, sizeof(BrowserPrivate));
    version = NXTypedStreamClassVersion(s, "NXBrowser");
    if (version < 1) {
	NXReadTypes(s, "@@::#@sssss*", &target, &delegate, &action, &doubleAction,
	    &matrixClass, &cellPrototype, &bp->minColumnWidth, &numVisibleColumns,
	    &bp->maxVisibleColumns, &pathSeparator, bpflags, &bp->unarchivedPath);
    } else if (version < 2) {
	NXReadTypes(s, "@@::#@ssssi*", &target, &delegate, &action, &doubleAction,
	    &matrixClass, &cellPrototype, &bp->minColumnWidth, &numVisibleColumns,
	    &bp->maxVisibleColumns, &pathSeparator, bpflags, &bp->unarchivedPath);
    } else {
	NXReadTypes(s, "@@::#@ssssi**", &target, &delegate, &action, &doubleAction,
	    &matrixClass, &cellPrototype, &bp->minColumnWidth, &numVisibleColumns,
	    &bp->maxVisibleColumns, &pathSeparator, bpflags, &bp->unarchivedPath, &bp->firstColumnTitle);
    }
    [self tile];

    return self;
}

- awake
{
    [self loadColumnZero];
    [self setPath:bp->unarchivedPath];
    free(bp->unarchivedPath);
    return [super awake];
}

@end

/*

Modifications (starting post-1.0):

77
--
 3/05/90 pah	Control class for 2.0.
		 Provides ability to view an arbitrary hierarchy.
		 Delegate is used to provide the data at the nodes.

79
--
 3/29/90 pah	Added WSM-like look
		Added arrow key support (kind of funky though since it is
		 hard for the NXBrowser itself to become first responder since
		 all the matrices are in the way refusing firstResponder)
		Added setMinColumnWidth:
		Made some instance variables private
		Made it more subclassable
  4/1/90 pah	Added old-style up/down button support

appkit-80
---------
  4/8/90 pah	added super-lazy mode (browser:getNumRowsInColumn:)

84
--
 5/9/90 aozer	Converted from Bitmap to NXImage.

5/14/90 pah	Added more flags (and up'ed class version to 1).
		Added free method.
		Added cellPrototype method.
		Added acceptArrowKeys: functionality.
		Added getLoadedCell:atRow:andColumn: method.
		Fixed setPath: to do the right thing.
		Added reloadColumn: and column validation by delegate.
		Added firstVisibleColumn and lastVisibleColumn methods.
		Fixed doClick: to work right with allow{Empty,Branch}Sel.
85
--
 6/4/90 pah	Fixed bug: setPath: called before load should cause a load
		Fixed bug: reloadColumn: needs to resize the matrix
		Fixed bug: typo - cell was used instead of aCell in doClick:
		Fixed bug: crasher when no cell is selected in doClick:
		Fixed bug: cells stay white even when unhighlighted in doClick:
86
--
 6/12/90 pah	Added validateVisibleColumns method
		Sped up reaction time to user moving from left scroll button
		 to the right scroll button

88
 7/20/90 aozer	Switched from PSsetgray() to _NXSetGrayUsingPattern().
*/

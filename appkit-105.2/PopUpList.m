/*
	PopUpList.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Window_Private.h"
#import "PopUpList.h"
#import "Application.h"
#import "Bitmap.h"
#import "Button.h"
#import "ButtonCell.h"
#import "errors.h"
#import <objc/List.h>
#import "Matrix.h"
#import "MenuCell.h"
#import "Text.h"
#import "privateWraps.h"
#import <math.h>
#import <string.h>

@implementation PopUpList:Menu

void NXAttachPopUpList(id button, PopUpList *popuplist)
{
    const char         *title;
    id                  cell, bfont;

    if (!button || !popuplist)
	return;
    if ([button isKindOf:[Control class]]) {
	cell = [button cell];
    } else {
	cell = button;
    }
    if (![cell isKindOf:[ButtonCell class]]) {
	return;
    }
    bfont = [cell font];
    if (bfont && bfont != [popuplist font])
	[popuplist setFont:bfont];
    [cell setHighlightsBy:NX_NONE];
    [cell sendActionOn:NX_MOUSEDOWNMASK];
    [cell setAlignment:NX_LEFTALIGNED];
    [cell setWrap:NO];
    if ([cell action] != @selector(popUp:) || [cell target] != popuplist) {
	if (![popuplist action]) {
	    [popuplist setAction:[cell action]];
	}
	if (![popuplist target]) {
	    [popuplist setTarget:[cell target]];
	}
	[cell setTarget:popuplist];
	[cell setAction:@selector(popUp:)];
    }
    if (popuplist->menuFlags._changeTitle) {
	[cell setIcon:"popup"];
	[cell setIconPosition:NX_ICONRIGHT];
	[cell setAltIcon:"popupH"];
	title = [popuplist selectedItem];
	if (!title)
	    title = [[popuplist->matrix cellAt:0 :0] title];
	if (title && strlen(title))
	    [cell setTitle:title];
    } else {
	[cell setIcon:"pulldown"];
	[cell setIconPosition:NX_ICONRIGHT];
	[cell setAltIcon:"pulldownH"];
    }
}

id
NXCreatePopUpListButton(PopUpList *popuplist)
{
    const char         *title;
    NXRect              bframe;
    id                  button, cell, font;

    if (!popuplist)
	return nil;

    title = [popuplist selectedItem];
    if (!title)
	title = [[[popuplist itemList] cellAt:0 :0] title];
    [popuplist getButtonFrame:&bframe];
    button = [[Button allocFromZone:[popuplist zone]] initFrame:&bframe
	      title:title
	      tag:0
	      target:popuplist
	      action:@selector(popUp:)
	      key :0
	      enabled:YES];
    cell = [button cell];
    if (popuplist->menuFlags._changeTitle) {
	[cell setIcon:"popup"];
	[cell setAltIcon:"popupH"];
    } else {
	[cell setIcon:"pulldown"];
	[cell setAltIcon:"pulldownH"];
    }
    [cell setIconPosition:NX_ICONRIGHT];
    [cell setHighlightsBy:NX_NONE];
    [cell setAlignment:NX_LEFTALIGNED];
    [cell sendActionOn:NX_MOUSEDOWNMASK];
    [cell setWrap:NO];
    font = [popuplist font];
    if (font && font != [cell font])
	[cell setFont:font];

    return button;
}


+ new
{
    return [self newTitle:NULL];
}

+ newTitle:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTitle:aString];
}

- init
{
    NXSize		intercell;
    NXRect              contentRect;

    contentRect.size.width = contentRect.size.height = 5.0;
    contentRect.origin.x = contentRect.origin.y = 0.0;
    [super initContent:&contentRect style:NX_PLAINSTYLE backing:NX_BUFFERED buttonMask:0 defer:YES];
    [self setBackgroundGray:NX_DKGRAY];
    contentRect.size.width -= 1.0;
    contentRect.size.height -= 1.0;
    contentRect.origin.x += 1.0;
    contentRect.origin.y += 1.0;
    matrix = [[Matrix allocFromZone:[self zone]] initFrame:&contentRect mode:NX_TRACKMODE cellClass:[MenuCell class] numRows:0 numCols:1];
    intercell.width = intercell.height = 0.0;
    [matrix setIntercell:&intercell];
    [matrix moveTo:1.0 :0.0];
    [self setEventMask:NX_MOUSEUPMASK | NX_MOUSEDRAGGEDMASK];
    [contentView addSubview:matrix];
    menuFlags.sizeFitted = NO;
    menuFlags._changeTitle = YES;
    return self;
}

- initTitle:(const char *)aString
{
    return [self init];
}

- _commonAwake
{
    id retval;

    retval = [super _commonAwake];
    if (windowNum > 0) {
	[self _setWindowLevel:NX_MAINMENULEVEL+1];
    }

    return retval;
}


static id
findItem(PopUpList *self, const char *title, int *offset, BOOL addIfNotFound)
{
    id                  cellList;
    int                 i, count;
    const char         *itemTitle;

    if (!title) {
	*offset = 0;
	return NO;
    }
    cellList = [self->matrix cellList];
    count = [cellList count];
    for (i = 0; i < count; i++) {
	itemTitle = [[cellList objectAt:i] title];
	if (itemTitle && !strcmp(itemTitle, title))
	    break;
    }

    if (i == count) {
	if (addIfNotFound)
	    [self insertItem:title at:0];
	*offset = 0;
return addIfNotFound ?[cellList objectAt : 0]:nil;
    } else {
	*offset = i;
	return[cellList objectAt:i];
    }
}

static void
noSubmenus(id matrix)
{
    id                  cell, cellList;
    int                 i, count;
    BOOL                draw;

    cellList = [matrix cellList];
    count = [cellList count];
    for (i = 0; i < count; i++) {
	draw = NO;
	cell = [cellList objectAt:i];
	if ([cell action] == @selector(submenuAction:)) {
	    [cell setAction:(SEL)0];
	    [cell setTarget:nil];
	    [cell setIcon:(char *)0];
	    [cell setAltIcon:(char *)0];
	}
	if ([cell state]) {
	    [cell setState:0];
	    draw = YES;
	}
	if ([cell isHighlighted]) {
	    [cell setParameter:NX_CELLHIGHLIGHTED to:NO];
	    draw = YES;
	}
	if (draw)
	    [matrix drawCellInside:cell];
    }
}

static id getBaseFrame(id view, NXRect *frame)
{
    id			sender = view;
    int			row, col;
    NXPoint             ll, ur;

    if ([view isKindOf:[Matrix class]]) {
	sender = [view selectedCell];
	if ([view getRow:&row andCol:&col ofCell:sender]) {
	    [view getCellFrame:frame at:row :col];
	} else {
	    return nil;
	}
    } else if (view) {
	[view getBounds:frame];
    } else {
	return nil;
    }

    ll = frame->origin;
    ur.x = frame->origin.x + frame->size.width;
    ur.y = frame->origin.y + frame->size.height;
    [view convertPoint:&ll toView:nil];
    [view convertPoint:&ur toView:nil];
    if (ur.x > ll.x) {
	frame->origin.x = ll.x;
	frame->size.width = ur.x - ll.x;
    } else {
	frame->origin.x = ur.x;
	frame->size.width = ll.x - ur.x;
    }
    if (ur.y > ll.y) {
	frame->origin.y = ll.y;
	frame->size.height = ur.y - ll.y;
    } else {
	frame->origin.y = ur.y;
	frame->size.height = ll.y - ur.y;
    }

    return sender;
}


- _fitOnScreen:(float)itemSize :(NXRect *)bFrame
{
    NXRect              screenFrame;

    screenFrame.origin.x = screenFrame.origin.y = 0.0;
    [NXApp getScreenSize:&screenFrame.size];
    NXContainRect(&screenFrame, &frame);
    if (screenFrame.origin.y) {
	screenFrame.origin.y = screenFrame.origin.y > 0.0 ?
	  floor(screenFrame.origin.y / itemSize) + 1 :
	  floor(screenFrame.origin.y / itemSize);
	screenFrame.origin.y *= itemSize;
	[self moveTo:frame.origin.x :frame.origin.y - screenFrame.origin.y];
	screenFrame = frame;
	NXContainRect(&screenFrame, bFrame);
	if (!NXEqualRect(&screenFrame, &frame)) {
	    [self moveTo:screenFrame.origin.x:screenFrame.origin.y];
	}
    }

    return self;
}

- sizeWindow:(NXCoord)width :(NXCoord)height
{
    return [super sizeWindow:width+1.0 :height+1.0];
}

- popUp:sender
{
    NXEvent            *theEvent = [NXApp currentEvent];
    NXRect              buttonFrame, matrixFrame;
    int                 offset;
    NXSize              cellSize;
    id                  cell, sendingView;
    BOOL                autodisplay = YES;
    const char         *title;
    NXHandler		handler;

    if (theEvent->type == NX_MOUSEDOWN) {
	sendingView = sender;
	sender = getBaseFrame(sender, &buttonFrame);	// sender may be cell
	if (!sender)
	    return self;
	if (!NXMouseInRect(&theEvent->location, &buttonFrame, NO))
	    return self;
	[[sendingView window] convertBaseToScreen:&buttonFrame.origin];
	cell = findItem(self,[sender title], &offset, YES);
	if (!offset && ![matrix cellCount])
	    return self;
	if (!cell)
	    cell = [matrix cellAt:offset :0];
	[matrix getCellSize:&cellSize];
	if (cellSize.width != buttonFrame.size.width ||
	    cellSize.height != buttonFrame.size.height ||
	    !menuFlags.sizeFitted) {
	    menuFlags.sizeFitted = YES;
	    cellSize = buttonFrame.size;
	    [self disableDisplay];
	    [matrix setCellSize:&cellSize];
	    [matrix sizeToCells];
	    [self reenableDisplay];
	    [matrix getFrame:&matrixFrame];
	    if (matrixFrame.size.width == frame.size.width-1.0 &&
		matrixFrame.size.height == frame.size.height-1.0) {
		[self display];
	    }
	} else {
	    [matrix getFrame:&matrixFrame];
	}
	if (matrixFrame.size.width != frame.size.width-1.0 ||
	    matrixFrame.size.height != frame.size.height-1.0) {
	    [self sizeWindow:matrixFrame.size.width:matrixFrame.size.height];
	    [self display];
	}
	noSubmenus(matrix);
	[self moveTopLeftTo:buttonFrame.origin.x - 1.0
	 :buttonFrame.origin.y + (offset + 1) * cellSize.height + 1.0];
	[self _fitOnScreen:cellSize.height :&buttonFrame];
	if (menuFlags._changeTitle) {
	    [cell setIcon:"popup"];
	    [cell setAltIcon:"popupH"];
	} else {
	    [cell setIcon:"pulldown"];
	    [cell setAltIcon:"pulldownH"];
	}
	[self orderFront:self];
	NX_DURING {
	    handler.code = 0;
	    autodisplay = [matrix isAutodisplay];
	    [matrix setAutodisplay:NO];
	    [matrix mouseDown:theEvent];
	    if (![matrix selectedCell]) {
	       [matrix selectCell:cell];
	    }
	    if (menuFlags._changeTitle) {
		title = [self selectedItem];
		if (title && strlen(title)) {
		    [sender setTitle:[self selectedItem]];
		    if ([sendingView needsDisplay]) {
			[sendingView display];
		    }
		}
	    }
	} NX_HANDLER {
	    handler = NXLocalHandler;
	} NX_ENDHANDLER;
	[self orderOut:self];
	[matrix setAutodisplay:autodisplay];
	[cell setIcon:(char *)0];
	[cell setAltIcon:(char *)0];
	if (handler.code) {
	    NX_RAISE(handler.code, handler.data1, handler.data2);
	}
    }
    return self;
}

- addItem:(const char *)title
{
    id                  retval;
    int                 lookup;

    [self disableDisplay];
    retval = findItem(self, title, &lookup, NO);
    if (!retval) {
	retval = [super addItem:title action:(SEL)0 keyEquivalent:0];
    }
    [self reenableDisplay];
    return retval;
}

- insertItem:(const char *)title at:(unsigned int)index
{
    id                  cell, cellList;
    int                 i, count, lookup;

    if (findItem(self, title, &lookup, NO))
	[self removeItem:title];

    cell = [super addItem:title action:(SEL)0 keyEquivalent:0];
    cellList = [matrix cellList];
    count = [cellList count];
    for (i = index; i < count; i++) {
	cell = [cellList replaceObjectAt:i with:cell];
    }
    for (i = index; i < count; i++) {
	[matrix drawCellInside:[cellList objectAt:i]];
    }

    return cell;
}

- removeItem:(const char *)title
{
    id                  cell;
    int                 offset;

    if (cell = findItem(self, title, &offset, NO)) {
	[self disableDisplay];
	[matrix removeRowAt:offset andFree:NO];
	[self reenableDisplay];
	menuFlags.sizeFitted = NO;
    }
    return cell;
}

- removeItemAt:(unsigned int)index
{
    id                  cell;

    cell = [matrix cellAt:index :0];
    [self disableDisplay];
    [matrix removeRowAt:index andFree:NO];
    [self reenableDisplay];
    menuFlags.sizeFitted = NO;

    return cell;
}

- (unsigned int)count
{
    int                 rows, cols;

    [matrix getNumRows:&rows numCols:&cols];

    return rows;
}

- changeButtonTitle:(BOOL)flag
{
    menuFlags._changeTitle = flag ? YES : NO;
    return self;
}

- (int)indexOfItem:(const char *)title
{
    int                 offset;

    if (findItem(self, title, &offset, NO)) {
	return offset;
    } else {
	return -1;
    }
}

- font
{
    return[[matrix cellAt:0 :0] font];
}

- setFont:fontId
{
    [self disableDisplay];
    [matrix setFont:fontId];
    [self reenableDisplay];
    menuFlags.sizeFitted = NO;
    return self;
}

- (const char *)selectedItem
{
    return[[matrix selectedCell] title];
}

- getButtonFrame:(NXRect *)bframe
{
    [self disableDisplay];
    [matrix sizeToFit];
    [self reenableDisplay];
    [matrix getCellSize:&bframe->size];
    bframe->origin.x = bframe->origin.y = 0.0;
    return self;
}

- (SEL)action
{
    return[matrix action];
}

- setAction:(SEL)aSelector
{
    [matrix setAction:aSelector];
    return self;
}

- target
{
    return[matrix target];
}

- setTarget:anObject
{
    [matrix setTarget:anObject];
    return self;
}

/* KIT BUG: the one inherited from menu does not work for popuplist */

- setItemList:aMatrix
{
    id old = matrix;
    
    menuFlags.sizeFitted = NO;
    [old removeFromSuperview];
    matrix = aMatrix;
    [contentView addSubview:matrix];
    [matrix moveTo:1.0:0.0];

    return old;
}

@end

/*
	
Modifications (since 0.8):
  
 1/10/89 pah	new for 0.9
 1/25/89 trey	addItem: takes a const param
 1/28/89 pah	take out unnecessary sizeToFits!
		keep getButtonFrame: from displaying
 2/01/89 pah	add changeTitle: method for pulldown menus
 2/15/89 pah	make pulldown menus have different icon (down arrow)
 2/16/89 pah	don't make the menu stay on screen
 3/20/89 pah	make the menu stay on screen

0.91
----
 5/11/89 pah	make PUL deferred
 5/13/89 pah	put dark gray around the PUL (like a menu)
 5/14/89 pah	work around Menu's brain-damage re: setting the selectedCell

0.93
----
 6/14/89 pah	up window level to MML+2
 6/15/89 pah	eliminate redundant autodisplaying since ActionCells do it now
		selectedItem now returns const char *
 6/16/89 pah	support the fact that some kit functions now return const
		pull down list bug fixed: no icon!
 6/21/89 wrp	changed awake to _commonAwake to conform to Window scheme

77
--
  2/6/90 pah	Overrode setItemList: from Menu because the matrix is offset
		 from the origin.

*/


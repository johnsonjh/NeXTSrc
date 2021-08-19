/*
	Menu.m
	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "FontManager_Private.h"
#import "Window_Private.h"
#import "Application_Private.h"
#import "Cell_Private.h"
#import "Menu_Private.h"
#import "MenuCell.h"
#import "Matrix.h"
#import <objc/List.h>
#import "Application.h"
#import "FrameView.h"
#import "privateWraps.h"
#import "publicWraps.h"
#import "packagesWraps.h"
#import <defaults.h>
#import <string.h>
#import <stdio.h>
#import <math.h>
#import <dpsclient/wraps.h>
#import <mach.h>

typedef struct {
    @defs (MenuCell)
}                  *MenuCellId;

static NXZone *menuZone = NULL;

@implementation Menu:Panel

/*
 * Local support
 */

#define MAXDEPTH 25

#define ACTIVEMENUMASK (NX_LMOUSEUPMASK | NX_LMOUSEDRAGGEDMASK | \
			NX_RMOUSEUPMASK | NX_RMOUSEDRAGGEDMASK | \
			NX_LMOUSEDOWNMASK | NX_RMOUSEDOWNMASK | \
			NX_KITDEFINEDMASK)

static BOOL rightMouseDown = NO;

#define isActivationEvent(event) \
    (rightMouseDown && event->type == NX_KITDEFINED && \
		      (event->data.compound.subtype == NX_APPDEACT || \
		       event->data.compound.subtype == NX_APPACT))

#define isKitDefined(type) (rightMouseDown && type == NX_KITDEFINED)

#define isUp(type) ((type == NX_LMOUSEUP && !rightMouseDown) || \
	(type == NX_RMOUSEUP && rightMouseDown))

#define isDragged(type) ((type == NX_LMOUSEDRAGGED && !rightMouseDown) || \
	(type == NX_RMOUSEDRAGGED && rightMouseDown))

#define isDown(type) ((type == NX_LMOUSEDOWN && !rightMouseDown) || \
	(type == NX_RMOUSEDOWN && rightMouseDown))

/* ??? these should go away, and HANDLERS be used instead */
static int          errCode = 0;
static const void  *errData1 = 0, *errData2 = 0;

static int trackMenu(Menu *theMenu, NXEvent *theEvent, BOOL isRoot);


static id _NXDefaultMenuMatrix(BOOL userKeyEquivalents, NXZone *zone)
{
    id                  proto, aMatrix;
    NXRect              localFrame;
    NXSize              intercell;

    localFrame.origin.x = localFrame.origin.y = 0.0;
    localFrame.size.width = 500.0;	/* ??? this allows for the FrameView
					 * bug */
 /* ??? of clipping the title string */
    localFrame.size.height = 0.0;
    proto = [[MenuCell allocFromZone:zone] init];
    if (userKeyEquivalents) [proto _useUserKeyEquivalent];
    aMatrix = [[Matrix allocFromZone:zone] initFrame:&localFrame
	       mode:NX_TRACKMODE
	       prototype:proto
	       numRows:0
	       numCols:1];
    intercell.width = intercell.height = 0.0;
    [aMatrix setIntercell:&intercell];
    return aMatrix;
}

static id
backupForMenu(Menu *aMenu)
{
    NXRect              cRect;
    NXZone *zone = [aMenu zone];
    
    if (aMenu->reserved) {
	[aMenu->reserved placeWindow:&aMenu->frame];
	return aMenu->reserved;
    }
    [Window getContentRect:&cRect forFrameRect:&aMenu->frame
     style:NX_PLAINSTYLE];
    aMenu->reserved = [[Window allocFromZone:zone] initContent:&cRect
		       style:NX_PLAINSTYLE
		       backing:NX_RETAINED
		       buttonMask:0
		       defer:NO];
    [aMenu->reserved setHideOnDeactivate:YES];
    [aMenu->reserved reenableDisplay];
    return aMenu->reserved;
}


static void
hideMenu(Menu *aMenu)
{
    [aMenu orderOut:nil];
    if (aMenu->attachedMenu)
	hideMenu(aMenu->attachedMenu);
}


static void
showMenu(Menu *aMenu)
{
    id                  aSupermenu = aMenu->supermenu;
    NXPoint             aLocation;

    if (aSupermenu && aMenu->menuFlags.attached) {
	[aSupermenu getLocation:&aLocation forSubmenu:aMenu];
	if ((aLocation.x != aMenu->frame.origin.x) ||
	    (aLocation.y != aMenu->frame.origin.y)) {
	    [aMenu moveTo:aLocation.x:aLocation.y];
	}
    }
    [aMenu orderFront:aMenu];
    NXPing();
    if (aMenu->attachedMenu)
	showMenu(aMenu->attachedMenu);
}

static id
findCellForSubmenu(Menu *aMenu, id aSubmenu, int *index)
{
    int                 i, size;
    MenuCellId         *current;
    id                  theCells = [aMenu->matrix cellList];

    if (size = [aMenu->matrix cellCount]) {
	current = (MenuCellId *) NX_ADDRESS(theCells);
	for (i = 0; i < size; i++) {
	    if (((*current)->target == aSubmenu) &&
		((*current)->action == @selector(submenuAction:))) {
		*index = i;
		return (id)(*current);
	    } else
		current++;
	}
    }
    *index = -1;
    return nil;
}


static void
setCellStateForSubmenu(Menu *aMenu, int aState, id aSubmenu)
{
    int                 row;
    id                  theCell = findCellForSubmenu(aMenu, aSubmenu, &row);

    if (theCell) {
	if ([aMenu->matrix canDraw]) {
	    [aMenu->matrix highlightCellAt:row :0 lit:aState];
	} else {
	    _NXSetCellParam(theCell, NX_CELLHIGHLIGHTED, aState);
	}
    }
}


static void
setWindowLevel(Menu *aMenu, int aLevel)
{
    if (aMenu->windowNum > 0) {
	[aMenu _setWindowLevel:aLevel];
	if (aMenu->wFlags.visible)
	    [aMenu orderWindow:NX_ABOVE relativeTo:0];
    }
    if (aMenu->attachedMenu)
	setWindowLevel(aMenu->attachedMenu, aLevel);
}


static int
isAttachedToMenu(Menu *aMenu, Menu *aSubmenu)
{
    if (aMenu == aSubmenu)
	return 1;
    if (!aSubmenu)
	return 0;
    if (!aSubmenu->menuFlags.attached)
	return 0;
    return isAttachedToMenu(aMenu, aSubmenu->supermenu);
}


static int
isAttachedToMainMenu(Menu *aMenu)
{
    if (!aMenu->supermenu)
	return 1;
    if (!aMenu->menuFlags.attached)
	return 0;
    return isAttachedToMainMenu(aMenu->supermenu);
}


static void
setAttachedWindow(Menu *aMenu, id aSubmenu)
{
    Menu               *thisMenu;
    int                 count = 0;
    int                 list[MAXDEPTH];

    if (!aSubmenu) {
	if (aMenu->windowNum > 0)
	    _NXResetOtherWindows(aMenu->windowNum);
	if (aMenu->supermenu && aMenu->menuFlags.attached)
	    setAttachedWindow(aMenu->supermenu, aMenu);
	return;
    }
    if (aMenu->windowNum > 0) {
	for (thisMenu = aMenu; thisMenu; thisMenu = thisMenu->attachedMenu) {
	    list[count++] = thisMenu->windowNum;
	    if (count >= MAXDEPTH)
		break;
	}
	_NXResetOtherWindows(aMenu->windowNum);
	_NXSetOtherWindows(aMenu->windowNum, count, list);
    }
    if (isAttachedToMainMenu(aMenu))
	setWindowLevel(aSubmenu, NX_MAINMENULEVEL);
    else
	setWindowLevel(aSubmenu, NX_SUBMENULEVEL);
    if (aMenu->supermenu) {
	if (aMenu->menuFlags.attached)
	    setAttachedWindow(aMenu->supermenu, aMenu);
	else
	    setWindowLevel(aMenu, NX_SUBMENULEVEL);
    }
}


static void
unattachSubmenus(Menu *aMenu)
{
    BOOL                canDraw;
    Menu               *attachedMenu = aMenu->attachedMenu;

    if (!attachedMenu)
	return;
    unattachSubmenus(attachedMenu);
    canDraw = [aMenu->matrix canDraw];
    if (canDraw) {
	[aMenu->matrix lockFocus];
    }
    setCellStateForSubmenu(aMenu, 0, attachedMenu);
    if (canDraw) {
	[aMenu flushWindow];
	[aMenu->matrix unlockFocus];
    }
    attachedMenu->menuFlags.attached = NO;
    [attachedMenu orderOut:nil];
    aMenu->attachedMenu = nil;
}


static BOOL
setAttachedMenu(Menu *aMenu, Menu *aSubmenu)
{
    Menu		*attachedMenu;

    if (aMenu && (aMenu->windowNum < 0))
	_NXCreateRealWindow(aMenu);

    attachedMenu = aMenu->attachedMenu;
    if (attachedMenu == aSubmenu)
	return YES;
    if (attachedMenu) {
	hideMenu(attachedMenu);
	unattachSubmenus(attachedMenu);
	setCellStateForSubmenu(aMenu, 0, attachedMenu);
	attachedMenu->menuFlags.attached = NO;
	aMenu->attachedMenu = nil;
    }
    if (aSubmenu) {
	setCellStateForSubmenu(aMenu, 1, aSubmenu);
	aSubmenu->menuFlags.attached = YES;
	aSubmenu->menuFlags.tornOff = NO;
	aMenu->attachedMenu = aSubmenu;
	[aSubmenu->_borderView _setCloseEnabled:NO andDisplay:YES];
	showMenu(aSubmenu);
    }
    [aMenu flushWindow];
    return NO;
}


static void
reconnectWindows(Menu *aMenu)
{
    int                 i, max = [aMenu->matrix cellCount];
    id                  theCell, theCellList = [aMenu->matrix cellList];

    if (aMenu->menuFlags.attached && !aMenu->attachedMenu)
	setAttachedWindow(aMenu, nil);
    for (i = 0; i < max; i++) {
	theCell = [theCellList objectAt:i];
	if ([theCell hasSubmenu])
	    reconnectWindows([theCell target]);
    }
}

static void
saveState(Menu *aMenu)
{
    BOOL                canDraw;
    Menu               *supermenu;

    if (aMenu->menuFlags.wasAttached = aMenu->menuFlags.attached) {
	if (supermenu = aMenu->supermenu) {
	    canDraw = [supermenu->matrix canDraw];
	    if (canDraw) {
		[supermenu->matrix lockFocus];
	    }
	    setCellStateForSubmenu(supermenu, 0, aMenu);
	    aMenu->menuFlags.attached = NO;
	    supermenu->attachedMenu = nil;
	    if (canDraw) {
		[supermenu flushWindow];
		[supermenu->matrix unlockFocus];
	    }
	}
    }
}


static void
saveBackups(Menu *aMenu, id root)
{
    id                  matrix = aMenu->matrix;
    int                 i, max = [matrix cellCount];
    id                  theCell, theCellList = [matrix cellList];

    if (aMenu->wFlags.visible && !isAttachedToMenu(root, aMenu)) {
	saveState(aMenu);
	backupForMenu(aMenu);
	[aMenu->reserved moveTo:aMenu->frame.origin.x
	 :aMenu->frame.origin.y];
	[aMenu->reserved _setWindowLevel:NX_SUBMENULEVEL];
	if ([[aMenu->reserved contentView] canDraw]) {
	    [[aMenu->reserved contentView] lockFocus];
	    PScomposite(0.0, 0.0,
			aMenu->frame.size.width, aMenu->frame.size.height,
			[aMenu gState], 0.0, 0.0, NX_COPY);
	    [[aMenu->reserved contentView] unlockFocus];
	}
	[aMenu->reserved orderWindow:NX_BELOW relativeTo:aMenu->windowNum];
	[aMenu orderOut:nil];
	aMenu->menuFlags.wasTornOff = aMenu->menuFlags.tornOff;
    } else
	[aMenu->reserved orderOut:nil];
    if (aMenu->windowNum > 0)
	_NXTopMargin(0.0, aMenu->windowNum);
    for (i = 0; i < max; i++) {
	theCell = [theCellList objectAt:i];
	if ([theCell hasSubmenu])
	    saveBackups([theCell target], root);
    }
}


static void
restoreState(Menu *aMenu)
{
    BOOL                canDraw;
    Menu               *supermenu = aMenu->supermenu;

    if (supermenu && aMenu->menuFlags.wasAttached) {
	canDraw = [supermenu->matrix canDraw];
	if (canDraw) {
	    [supermenu->matrix lockFocus];
	}
	setCellStateForSubmenu(supermenu, 1, aMenu);
	aMenu->menuFlags.attached = YES;
	supermenu->attachedMenu = aMenu;
	if (canDraw) {
	    [supermenu flushWindow];
	    [supermenu->matrix unlockFocus];
	}
    }
}


static void
restoreBackups(Menu *aMenu, Menu *root)
{
    id                  matrix = aMenu->matrix;
    int                 i, max = [matrix cellCount];
    int                 newAttached = (root && root->attachedMenu == aMenu);
    id                  theCell, theCellList = [matrix cellList];
    NXRect		resFrame;

    if (aMenu->reserved && [aMenu->reserved isVisible]) {
	restoreState(aMenu);
	if (!isAttachedToMenu(root, aMenu)) {
	    [aMenu->reserved getFrame:&resFrame];
	    [aMenu moveTo:resFrame.origin.x :resFrame.origin.y];
	    if (aMenu->menuFlags.tornOff = aMenu->menuFlags.wasTornOff) {
		[aMenu->_borderView _setCloseEnabled:YES andDisplay:YES];
	    }
	    [aMenu orderWindow:NX_ABOVE relativeTo:[aMenu->reserved windowNum]];
	} else if (!newAttached)
	    [aMenu orderOut:nil];
	[aMenu->reserved orderOut:nil];
    }
    for (i = 0; i < max; i++) {
	theCell = [theCellList objectAt:i];
	if ([theCell hasSubmenu])
	    restoreBackups([theCell target], root);
    }
    if (aMenu->windowNum > 0) {
	_NXTopMargin(_NXtMargin(NX_MENUSTYLE), aMenu->windowNum);
    }
    if (newAttached && root) {
	unattachSubmenus(aMenu);
	[root->matrix lockFocus];
	setAttachedMenu(root, aMenu);
	[root->matrix unlockFocus];
    }
}


static void
getSubmenuLocation(Menu *aMenu, Menu *aSubmenu, NXPoint *aLocation)
{
    aLocation->x = aMenu->frame.origin.x + aMenu->frame.size.width;
    aLocation->y = aMenu->frame.origin.y + aMenu->frame.size.height -
      aSubmenu->frame.size.height;
}


static void
supermenuMoved(Menu *aMenu)
{
    NXRect              superRect;
    int                 forceIt = 0;

    if (aMenu->windowNum > 0) {
	PScurrentwindowbounds(aMenu->windowNum,
			      &(aMenu->frame.origin.x),
			      &(aMenu->frame.origin.y),
			      &(aMenu->frame.size.width),
			      &(aMenu->frame.size.height));
    } else
	forceIt = 1;
    [aMenu->supermenu getFrame:&superRect];
    if (forceIt || ((superRect.origin.y + superRect.size.height) !=
		    (aMenu->frame.origin.y + aMenu->frame.size.height)))
	showMenu(aMenu);
    if (aMenu->attachedMenu)
	supermenuMoved(aMenu->attachedMenu);
}


static BOOL
isPointInSupermenu(Menu *aMenu, NXPoint *aPoint)
{
    NXRect              theBounds;
    Menu               *supermenu = aMenu->supermenu;

    if (supermenu) {
	[Window getContentRect:&theBounds forFrameRect:&supermenu->frame
	 style:aMenu->wFlags.style];
	/* theBounds.origin.y += 1.0; */ /* this should not be necessary */
	/* theBounds.size.height -= 1.0; */ /* if it was a bug fix, comment? */
	if (NXMouseInRect(aPoint, &theBounds, NO))
	    return YES;
	else
	    return isPointInSupermenu(supermenu, aPoint);
    } else
	return NO;
}


static BOOL
seekingSubmenu(NXPoint *prevPoint, NXPoint *curPoint, id view, id theSubmenu)
{
    NXRect              frame1, frame2;
    float               dx1, dy1, dx2, dy2;
    BOOL		flipped;

    [view getFrame:&frame1];
    frame1.origin.x -= 1.0;
    flipped = [[view superview] isFlipped];
    if (!NXMouseInRect(curPoint, &frame1, flipped))
	return YES;	/* ??? wrp - by looking at caller, this is dead code */
    dx1 = curPoint->x - prevPoint->x;
    if (dx1 <= 0.0)
	return NO;
    dx2 = frame1.size.width - prevPoint->x;
    if (dx2 <= 0.0)
	return NO;
    dy1 = curPoint->y - prevPoint->y;
    if (dy1 < 0.0) {
	dy1 = -dy1;
	dy2 = prevPoint->y;
	if (dy1 / dx1 < dy2 / dx2)
	    return YES;
	else
	    return NO;
    } else {
	[theSubmenu getFrame:&frame2];
	dy2 = frame2.size.height - prevPoint->y;
	if (dy1 / dx1 < dy2 / dx2)
	    return YES;
	else
	    return NO;
    }
}


static BOOL
trackCellWithSubmenu(id theCell, NXEvent *theEvent, int eventWindowNum,
		id eventWindow, NXRect *bounds, id view, int row)
{
    int                 result, alreadyAttached;
    NXPoint             tempPoint, curPoint, lastPoint = theEvent->location;
    id                  window = [view window];
    Menu               *submenu = [theCell target];

    if (theEvent->window != eventWindowNum) {
	eventWindow = [NXApp findWindow:theEvent->window];
	eventWindowNum = theEvent->window;
    }
    if (eventWindow != window) {
	[eventWindow convertBaseToScreen:&lastPoint];
	[window convertScreenToBase:&lastPoint];
    }
    [view convertPoint:&lastPoint fromView:nil];
    alreadyAttached = setAttachedMenu(window, submenu);
    while (1) {
	[NXApp getNextEvent:ACTIVEMENUMASK];
	if (isUp(theEvent->type) || isActivationEvent(theEvent)) {
	    if (alreadyAttached || submenu->menuFlags.wasTornOff) {
		setAttachedMenu(window, nil);
	    } else {
		setAttachedMenu(window, submenu);
	    }
	    result = YES;
	    if (isActivationEvent(theEvent)) {
		DPSPostEvent(theEvent, 1);
	    }
	    break;
	} else if (isDragged(theEvent->type)) {
	    curPoint = theEvent->location;
	    if (theEvent->window != eventWindowNum) {
		eventWindow = [NXApp findWindow:theEvent->window];
		eventWindowNum = theEvent->window;
	    }
	    if (eventWindow != window) {
		[eventWindow convertBaseToScreen:&curPoint];
		[window convertScreenToBase:&curPoint];
	    }
	    tempPoint = curPoint;
	    [view convertPoint:&curPoint fromView:nil];
	    if (NXMouseInRect(&curPoint, bounds, [view isFlipped])) {
		lastPoint = curPoint;
	    } else if (seekingSubmenu(&lastPoint, &curPoint, view, submenu)) {
		[window convertBaseToScreen:&tempPoint];
		if (NXMouseInRect(&tempPoint, &submenu->frame, NO)) {
		    if (trackMenu(submenu, theEvent, NO)) {
			setAttachedMenu(window, nil);
			result = YES;
			break;
		    }
		} else if (isPointInSupermenu(window, &tempPoint)) {
		    result = NO;
		    break;
		} else {
		    lastPoint = curPoint;
		}
	    } else {
		setAttachedMenu(window, nil);
		result = NO;
		break;
	    }
	} else if (isKitDefined(theEvent->type)) {
	    [NXApp sendEvent:theEvent];
	}
    }
    return result;
}


static BOOL
trackCell(id theCell, NXEvent *theEvent, int eventWindowNum,
	  id eventWindow, NXRect *bounds, id view, int row)
{
    int                 result, lastState;
    NXPoint             curPoint, lastPoint;
    id                  window;
    int                 enabled = [theCell isEnabled];

    if (enabled && [theCell hasSubmenu])
	return trackCellWithSubmenu(theCell, theEvent, eventWindowNum,
					eventWindow, bounds, view, row);
    lastPoint = theEvent->location;
    window = [view window];
    if (enabled) {
	setAttachedMenu(window, nil);
	[view highlightCellAt:row :0 lit:YES];
	[window flushWindow];
    }
    if (theEvent->window != eventWindowNum) {
	eventWindow = [NXApp findWindow:theEvent->window];
	eventWindowNum = theEvent->window;
    }
    if (eventWindow != window) {
	[eventWindow convertBaseToScreen:&lastPoint];
	[window convertScreenToBase:&lastPoint];
    }
    [view convertPoint:&lastPoint fromView:nil];
    while (1) {
	[NXApp getNextEvent:ACTIVEMENUMASK];
	if (isUp(theEvent->type) || isActivationEvent(theEvent)) {
	    if (enabled && !isActivationEvent(theEvent)) {
		[[view window] disableDisplay];
		lastState = [theCell state];
		[view selectCellAt:row :0];
		[theCell setState:lastState];
		[[view window] reenableDisplay];
		errCode = 0;
		NX_DURING
		    [view sendAction:[theCell action] to:[theCell target]];
		NX_HANDLER
		    errCode = NXLocalHandler.code;
		    errData1 = NXLocalHandler.data1;
		    errData2 = NXLocalHandler.data2;
		NX_ENDHANDLER
		  if ([theCell state] != lastState)
		    [view drawCellInside:theCell];
	    }
	    result = YES;
	    if (isActivationEvent(theEvent)) {
		DPSPostEvent(theEvent, 1);
	    }
	    break;
	} else if (isDragged(theEvent->type)) {
	    curPoint = theEvent->location;
	    if (theEvent->window != eventWindowNum) {
		eventWindow = [NXApp findWindow:theEvent->window];
		eventWindowNum = theEvent->window;
	    }
	    if (eventWindow != window) {
		[eventWindow convertBaseToScreen:&curPoint];
		[window convertScreenToBase:&curPoint];
	    }
	    [view convertPoint:&curPoint fromView:nil];
	    if (!NXMouseInRect(&curPoint, bounds, [view isFlipped])) {
		result = NO;
		break;
	    }
	} else if (isKitDefined(theEvent->type)) {
	    [NXApp sendEvent:theEvent];
	}
    }
    if (enabled) {
	[view highlightCellAt:row :0 lit:NO];
	[window flushWindow];
    }
    return result;
}


static int
trackMenu(Menu *theMenu, NXEvent *theEvent, BOOL isRoot)
{
    NXRect              cellBounds, viewBounds;
    NXPoint             curLocation, tempPoint;
    int                 row, col;
    int                 oldMask = 0, result;
    id                  theCell, matrix = theMenu->matrix;
    id                  origWindow, eventWindow = nil;
    int			eventWindowNum = -1;
    BOOL		needEvent = YES;

    origWindow = eventWindow = [NXApp findWindow:theEvent->window];
    eventWindowNum = theEvent->window;
    oldMask = [origWindow addToEventMask:ACTIVEMENUMASK];
    [matrix getFrame:&viewBounds];
    [matrix lockFocus];
    while (1) {
	if (isDown(theEvent->type) || isDragged(theEvent->type)) {
	    curLocation = theEvent->location;
	    if (theEvent->window != eventWindowNum) {
		eventWindow = [NXApp findWindow:theEvent->window];
		eventWindowNum = theEvent->window;
	    }
	    if (eventWindow != theMenu) {
		[eventWindow convertBaseToScreen:&curLocation];
		[theMenu convertScreenToBase:&curLocation];
	    }
	    [matrix convertPoint:&curLocation fromView:nil];
	    if ([matrix getRow:&row andCol:&col forPoint:&curLocation]) {
		[matrix getCellFrame:&cellBounds at:row :col];
		theCell = [matrix cellAt:row :col];
		if (trackCell(theCell, theEvent, eventWindowNum, eventWindow,
			      &cellBounds, matrix, row)) {
		    result = YES;
		    break;
		}
		needEvent = NO;
	    }
	    if (!isRoot) {
		tempPoint = theEvent->location;
		if (eventWindow != theMenu)
		    [eventWindow convertBaseToScreen:&tempPoint];
		else
		    [theMenu convertBaseToScreen:&tempPoint];
		if (isPointInSupermenu(theMenu, &tempPoint)) {
		    result = NO;
		    break;
		}
	    }
	} else if (isUp(theEvent->type) || isActivationEvent(theEvent)) {
	    [matrix selectCellAt:-1 :-1];
	    result = YES;
	    if (isActivationEvent(theEvent)) {
		DPSPostEvent(theEvent, 1);
	    }
	    break;
	} else if (isKitDefined(theEvent->type)) {
	    [NXApp sendEvent:theEvent];
	}
	if (needEvent) {
	    theEvent = [NXApp getNextEvent:ACTIVEMENUMASK];
	} else {
	    needEvent = YES;
	}
    }
    [matrix unlockFocus];
    [origWindow setEventMask:oldMask];
    return result;
}


/*
 * External interface
 */

static id           currentBackupMenu = nil;
static id           currentBackupMenuRoot = nil;

+ _repairBackups
{
    if (currentBackupMenu) {
	restoreBackups(currentBackupMenu, currentBackupMenuRoot);
	currentBackupMenu = currentBackupMenuRoot = nil;
    }
    return self;
}

+ setMenuZone:(NXZone *)aZone
{
    menuZone = aZone;
    return self;
}

+ (NXZone *)menuZone
{
    if(!menuZone) {
	menuZone = NXCreateZone(vm_page_size, vm_page_size, YES);
	NXNameZone(menuZone, "Menu");
    } 
    return menuZone;
}

+ new
{
    return [self newTitle:"Menu"];
}

+ newTitle:(const char *)aTitle
{
    return [[self allocFromZone:[self menuZone]] initTitle:aTitle];
}

- init
{
    return [self initTitle:"Menu"];
}

- initTitle:(const char *)aTitle
{
    NXRect              contentRect;
    NXZone 		*zone = [self zone];
    id                  aMatrix = _NXDefaultMenuMatrix([self class] == [Menu class], zone);
    NXPoint             initLoc;

    _NXGetMenuLocation(&initLoc);
    [aMatrix getFrame:&contentRect];
    self = [super initContent:&contentRect style:NX_MENUSTYLE backing:NX_BUFFERED buttonMask:0 defer:YES];
    setWindowLevel(self, NX_SUBMENULEVEL);
    matrix = aMatrix;
    [self setTitle:aTitle];
    [self setEventMask:(NX_MOUSEDOWNMASK | NX_MOUSEUPMASK | NX_MOUSEDRAGGEDMASK | NX_RMOUSEDOWNMASK | NX_RMOUSEUPMASK | NX_RMOUSEDRAGGEDMASK | NX_KITDEFINEDMASK)];
    [[self setContentView:matrix] free];
    [_borderView _setCloseEnabled:NO andDisplay:NO];
    [self moveTopLeftTo:initLoc.x :initLoc.y];
    menuFlags.sizeFitted = NO;
    [self useOptimizedDrawing:YES];

    return self;
}

+ _newTitleFromNibSection:(const char *)aTitle withMatrix:aMatrix
{
    return [[self allocFromZone:[self menuZone]] _initTitleFromNibSection:aTitle withMatrix:aMatrix];
}

- _initTitleFromNibSection:(const char *)aTitle withMatrix:aMatrix
{
    NXPoint		initLoc;	

    _NXGetMenuLocation(&initLoc);
    [super _initContent:NULL style:NX_MENUSTYLE backing:NX_BUFFERED buttonMask:0 defer:YES contentView:aMatrix];
    menuFlags.sizeFitted = [aMatrix cellCount] > 0;
    setWindowLevel(self, NX_SUBMENULEVEL);
    matrix = aMatrix;
    [self setTitle:aTitle];
    [self setEventMask:(NX_MOUSEDOWNMASK | NX_MOUSEUPMASK | NX_MOUSEDRAGGEDMASK | NX_RMOUSEDOWNMASK | NX_RMOUSEUPMASK | NX_RMOUSEDRAGGEDMASK | NX_KITDEFINEDMASK)];
    [_borderView _setCloseEnabled:NO andDisplay:NO];
    [self moveTopLeftTo:initLoc.x :initLoc.y];
    return self;
}


- addItem:(const char *)aString
    action:(SEL)aSelector
    keyEquivalent:(unsigned short)charCode
{
    id                  theCell;
    NXSize              cellSize;
    int                 row = [matrix cellCount];

    [matrix addRow];
    theCell = [matrix cellAt:row :0];
    [theCell setKeyEquivalent:charCode];
    [theCell setTitle:aString];
    [theCell setAction:aSelector];
    [theCell calcCellSize:&cellSize];
    menuFlags.sizeFitted = NO;
    return theCell;
}


- _supermenu
{
    return supermenu;
}

- _removeFromHierarchy
{
    int index;

    findCellForSubmenu(supermenu, self, &index);
    if (index >= 0) {
	[[supermenu itemList] removeRowAt:index andFree:YES];
	[supermenu sizeToFit];
    }

    return self;
}


- submenuAction:sender
{
    id requestingMenu = [sender window];
    if ([requestingMenu isVisible]) setAttachedMenu(requestingMenu, self);
    return self;
}

- setSubmenu:(Menu *)aMenu forItem:aCell
{
    if (aCell) {
	[aCell setTarget:aMenu];
	[aCell setAction:@selector(submenuAction:)];
	[aCell setIcon:"menuArrow"];
	aMenu->supermenu = self;
    }
    return aCell;
}

- itemList
{
    return matrix;
}

- setItemList:aMatrix
{
    menuFlags.sizeFitted = NO;
    return[self setContentView:(matrix = aMatrix)];
}

static id litMatrix = nil;
static int litRow = -1;

- _highlightSubmenuEntry:submenu lit:(BOOL)flag
{
    int rows, cols;

    [matrix getNumRows:&rows numCols:&cols];
    while (rows--) {
	if ([[matrix cellAt:rows :0] target] == submenu) {
	    [matrix lockFocus];
	    [matrix highlightCellAt:rows :0 lit:flag];
	    if (flag) {
		setAttachedMenu(self, nil);
	    }
	    [matrix unlockFocus];
	    if (flag) {
		litMatrix = matrix;
		litRow = rows;
	    } else {
		litMatrix = nil;
		litRow = -1;
	    }
	    return self;
	}
    }

    return nil;
}

- _highlightSupermenu:(BOOL)lit
{
    if ([supermenu isVisible] && [((Menu *)supermenu)->matrix canDraw]) {
	return [supermenu _highlightSubmenuEntry:self lit:lit];
    } else if (supermenu) {
	return [supermenu _highlightSupermenu:lit];
    } else {
	if (!lit && litMatrix && (litRow >= 0)) {
	    [litMatrix lockFocus];
	    [litMatrix highlightCellAt:litRow :0 lit:NO];
	    [litMatrix unlockFocus];
	    return self;
	}
    }
    return self;
}

- display
{
    if (!menuFlags.sizeFitted) {
	[self sizeToFit];
	if ([self isDisplayEnabled]) return self;
    }
    return [super display];
}

- sizeToFit
{
    NXRect              matrixFrame, newFrame, newContent;
    NXSize              theSize;

    [matrix sizeToFit];
    [matrix getFrame:&matrixFrame];
    newFrame = frame;
    newFrame.size.width =
	[Window minFrameWidth:[self title] forStyle:wFlags.style buttonMask:NX_CLOSEBUTTONMASK];
    [FrameView getContentRect:&newContent forFrameRect:&newFrame 
    	style:wFlags.style];
    if (matrixFrame.size.width < newContent.size.width) {
	matrixFrame.size.width = newContent.size.width;
	[matrix getCellSize:&theSize];
	[matrix sizeTo:matrixFrame.size.width:matrixFrame.size.height];
	theSize.width = matrixFrame.size.width;
	[matrix setCellSize:&theSize];
    }
    menuFlags.sizeFitted = YES;		/* do it here to prevent recursion */
    newContent.size = matrixFrame.size;
    [FrameView getFrameRect:&newFrame forContentRect:&newContent 
    	style:wFlags.style];
    if (newFrame.size.width != frame.size.width ||
	newFrame.size.height != frame.size.height) {
	newFrame.origin.x = frame.origin.x;
	newFrame.origin.y = NX_MAXY(&frame) - newFrame.size.height;
	if ([self isVisible] && !menuFlags.attached) {
	    [self constrainFrameRect:&newFrame toScreen:NULL];
	}
	[self _resizeWindow:&newFrame userAction:NO];
    } else
	[super display];
    return self;
}

- moveTopLeftTo:(NXCoord)x :(NXCoord)y
{
    if (!menuFlags.sizeFitted)
	[self sizeToFit];
    return[super moveTopLeftTo:x :y];
}

- windowMoved:(NXEvent *)theEvent
{
    int                 reallyMoved = ((frame.origin.x != theEvent->location.x) ||
				  (frame.origin.y != theEvent->location.y));

    [super windowMoved:theEvent];
    if (menuFlags.attached && reallyMoved) {
	[((Menu *)supermenu)->matrix lockFocus];
	setCellStateForSubmenu(supermenu, 0, self);
	[supermenu flushWindow];
	[((Menu *)supermenu)->matrix unlockFocus];
	menuFlags.attached = NO;
	menuFlags.tornOff = YES;
	setWindowLevel(self, NX_SUBMENULEVEL);
	((Menu *)supermenu)->attachedMenu = nil;
	setAttachedWindow(supermenu, nil);
	[_borderView _setCloseEnabled:YES andDisplay:YES];
    }
    if (attachedMenu && reallyMoved)
	supermenuMoved(attachedMenu);
    return self;
}


- _unattachSubmenu
{
    setAttachedMenu(self, nil);
    return self;
}

- close
{
    if (supermenu && ((Menu *)supermenu)->attachedMenu == self) {
	((Menu *)supermenu)->attachedMenu = nil;
    }
    [super close];
    menuFlags.tornOff = menuFlags.wasTornOff = NO;
    setWindowLevel(self, NX_SUBMENULEVEL);
    if (attachedMenu) {
	if ([matrix canDraw]) {
	    [matrix lockFocus];
	    setAttachedMenu(self, nil);
	    [matrix unlockFocus];
	} else {
	    setAttachedMenu(self, nil);
	}
    }
    return self;
}

- _makeServicesMenu
{
    menuFlags._isServicesMenu = YES;
    return self;
}

- update
{
    id              theCell, target;
    id              appDelegate = [NXApp delegate];
    SEL             updateAction;
    int             row, col;
    BOOL            canDraw = [matrix canDraw];
    BOOL            didDraw = NO, didFlush = NO;
    BOOL	    amServicesMenu = NO;
    Menu	   *supmenu;

    supmenu = self;
    while (supmenu && !(amServicesMenu = supmenu->menuFlags._isServicesMenu)) supmenu = (Menu *)(supmenu->supermenu);
    if (amServicesMenu) {
	if ([self isVisible]) {
	    [NXApp perform:@selector(_doUpdateServicesMenu:) with:self afterDelay:200 cancelPrevious:YES];
	} else if (menuFlags._isServicesMenu) {
	    [NXApp _doUpdateServicesMenu:self];
	}
    }

    if (menuFlags.autoupdate) {
	if (self == [[FontManager new] getFontMenu:NO]) {
	    [self disableFlushWindow];
	    didFlush = YES;
	    [[FontManager new] _updateMenuItems];
	}
	[self disableDisplay];
	[matrix getNumRows:&row numCols:&col];
	for (col--; col >= 0; col--) {
	    for (row--; row >= 0; row--) {
		theCell = [matrix cellAt:row :col];
		if (updateAction = [theCell updateAction]) {
		    if ([delegate respondsTo:updateAction])
			target = delegate;
		    else if ([NXApp respondsTo:updateAction])
			target = NXApp;
		    else if (appDelegate != NXApp && [appDelegate respondsTo:updateAction])
			target = appDelegate;
		    else
			target = nil;
		    if ((BOOL)[target perform:updateAction with:theCell]) {
			if (canDraw) {
			    [self reenableDisplay];
			    if (!didDraw) {
				if (!didFlush) {
				    [self disableFlushWindow];
				    didFlush = YES;
				}
				[matrix lockFocus];
				didDraw = YES;
			    }
			    [matrix drawCellInside:theCell];
			    [self disableDisplay];
			}
		    }
		}
	    }
	}
	[self reenableDisplay];
    }
    [super update];
    if (didFlush) {
	[self reenableFlushWindow];
	[self flushWindow];
    }
    if (didDraw) {
	[matrix unlockFocus];
    }

    return self;
}


- setAutoupdate:(BOOL)flag
{
    menuFlags.autoupdate = flag ? YES : NO;
    return self;
}


- findCellWithTag:(int)aTag
{
    return[matrix findCellWithTag:aTag];
}


- getLocation:(NXPoint *)theLocation forSubmenu:aSubmenu
{
    getSubmenuLocation(self, aSubmenu, theLocation);
    return self;
}

- mouseDown:(NXEvent *)theEvent
{
    id prevCurrentBackupMenu, prevCurrentBackupMenuRoot;

    rightMouseDown = NO;
    saveBackups(self, self);
    prevCurrentBackupMenuRoot = currentBackupMenuRoot;
    prevCurrentBackupMenu = currentBackupMenu;
    currentBackupMenuRoot = self;
    currentBackupMenu = self;
    trackMenu(self, theEvent, YES);
    if (currentBackupMenu) {
	currentBackupMenuRoot = prevCurrentBackupMenuRoot;
	currentBackupMenu = prevCurrentBackupMenu;
	restoreBackups(self, self);
    }
    reconnectWindows(self);
    if (errCode)
	NX_RAISE(errCode, errData1, errData2);
    return self;
}

- rightMouseDown:(NXEvent *)theEvent
{
    NXPoint             oldLocation, newLocation = theEvent->location;
    id                  eventWindow = [NXApp findWindow:theEvent->window];
    int			oldMask, oldKeyMask;
    id			keyWindow;
    id			prevCurrentBackupMenu, prevCurrentBackupMenuRoot;

    if (self != [NXApp mainMenu])
	return [[NXApp mainMenu] rightMouseDown:theEvent];
    rightMouseDown = YES;
    oldLocation = frame.origin;
    keyWindow = [NXApp keyWindow];
    [keyWindow disableCursorRects];
    oldMask = [eventWindow addToEventMask:ACTIVEMENUMASK];
    oldKeyMask = [keyWindow addToEventMask:ACTIVEMENUMASK];
    [eventWindow convertBaseToScreen:&newLocation];
    [eventWindow convertBaseToScreen:&theEvent->location];
    newLocation.x -= frame.size.width / 2.0;
    newLocation.y -= frame.size.height - 10.0;
    saveBackups(self, nil);
    [self moveTo:newLocation.x:newLocation.y];
    [eventWindow convertScreenToBase:&theEvent->location];
    [self orderFront:nil];
    prevCurrentBackupMenuRoot = currentBackupMenuRoot;
    prevCurrentBackupMenu = currentBackupMenu;
    currentBackupMenuRoot = nil;
    currentBackupMenu = self;
    trackMenu(self, theEvent, YES);
    [matrix lockFocus];
    setAttachedMenu(self, nil);
    [matrix unlockFocus];
    if (currentBackupMenu) {
	currentBackupMenuRoot = prevCurrentBackupMenuRoot;
	currentBackupMenu = prevCurrentBackupMenu;
	restoreBackups(self, nil);
    }
    reconnectWindows(self);
    [self moveTo:oldLocation.x:oldLocation.y];
    [eventWindow setEventMask:oldMask];
    [keyWindow setEventMask:oldKeyMask];
    [keyWindow enableCursorRects];
    rightMouseDown = NO;
    if (errCode)
	NX_RAISE(errCode, errData1, errData2);
    return self;
}


- _commonAwake
{
    [super _commonAwake];
    if (windowNum > 0) {
	[self _setWindowLevel:(supermenu ? NX_SUBMENULEVEL : NX_MAINMENULEVEL)];
	if (rightMouseDown)
	    _NXTopMargin(0.0, windowNum);
    }
    return self;
}


- awake
{
    BOOL                shouldEnable;

    [super awake];
    if (!supermenu || menuFlags.attached)
	shouldEnable = NO;
    else
	shouldEnable = YES;
    [_borderView _setCloseEnabled:shouldEnable andDisplay:NO];
    return self;
}


void _NXGetMenuLocation(NXPoint *p)
{
    const char         *def;
    NXSize              screenSize;
    static NXCoord      menuX = NAN;
    static NXCoord      menuY = NAN;
    NXCoord             maxTop;

    if (menuX != menuX) {	/* If menuX is not-a-number */
	def = NXGetDefaultValue([NXApp appName], "NXMenuX");
	if (def) {
	    sscanf(def, "%f", &menuX);
	} else {
	    menuX = -1.0;
	}
	def = NXGetDefaultValue([NXApp appName], "NXMenuY");
	if (def) {
	    sscanf(def, "%f", &menuY);
	} else {
	    menuY = 1000000.0;
	}
	[NXApp getScreenSize:&screenSize];
    /* keeps the top row of grey off the screen */
	maxTop = screenSize.height + 1.0;
	if (menuY > maxTop)
	    menuY = maxTop;
    }
    p->x = menuX;
    p->y = menuY;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWritePoint(stream, &lastLocation);
    NXWriteTypes(stream, "@@@s", &supermenu, &matrix, &attachedMenu, &menuFlags);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadPoint(stream, &lastLocation);
    NXReadTypes(stream, "@@@s", &supermenu, &matrix, &attachedMenu, &menuFlags);
    if (([self class] == [Menu class]) && [matrix isKindOf:[Matrix class]]) {
	int rows, cols;
	[matrix getNumRows:&rows numCols:&cols];
	while (rows--) {
	    id cell = [matrix cellAt:rows :0];
	    if (cell && [cell isKindOf:[MenuCell class]] && ![cell hasSubmenu] ) {
		[cell setKeyEquivalent:[cell userKeyEquivalent]];
		[cell _useUserKeyEquivalent];
	    }
	}
    }
    return self;
}


@end
/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the List object;
  
0.82
----
 1/14/89 pah	made DefaultMatrix non-static (_NXDefaultMenuMatrix)
		add private flag _changeTitle for PULs
		fix convertPoint: bug in trackCell
 1/25/89 trey	addItem:action:keyEquivalent: takes a const param
 1/27/89 bs	added read: and write:
02/05/89 bgy	added support for cursor tracking. In the rightMouseDown:
		 method, the key window's cursor rects are disabled before 
		 tracking, and enabled when the tracking is completed.
 2/10/89 pah	fix spin loop that caused horrendous latency
 2/14/89 pah	rip out extraneous argument to NXPing()
  
0.83
----
 3/15/89 trey	main menu positioned at top-left corner, or according to
		defaults database
 3/03/89 pah	fix lockFocus when we have no real window
		optimize cell drawing as a result of updateAction (call
		 matrix drawInside: instead of drawCellAt:)
		disable flushWindow around the updateAction sending
 3/13/89 pah	fix even more lockFocus headbutts
 3/16/89 pah	updateAction is sent to NXApp and, if NXApp does not respond,
		 then NXApp's delegate

0.91
----
 5/19/89 trey	minimized static data
 5/19/89 wrp	changed sizeToFit to also do a display
		 (_resizeWindow:userAction:)
 		made sizeToFit grow downwards not upwards
 5/22/89 pah	fix event lossage in cell tracking
 
0.93
----
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps
 6/16/89 pah	when a menu is closed, detach it from its superview
 6/17/89 pah	make Menu selectCellAt:-1 :-1 if no item is selected
 6/21/89 wrp	factored out _commonAwake from awake to conform to
		 improved Window scheme

0.95
----
 7/16/89 wrp	fixed bug in sizeToFit where display wasn't being done if size
 		 did not change.

0.96
----
 7/20/89 pah	fixed restoreBackups bug (menu was being returned to the frame
		 it is already at instead of the reserved frame!)
		add hack to unhighlight a higher level menu item even if
		 somehow the menu heirarchy has gotton mucked up (e.g. IB!)
		fix dead right-mouse popups by undoing damage to the
		 event mask done by disableCursorRects

77
--
 2/07/90 trey	moved overriding of _showPanel to Menu instead of testing
		 for Menu class in Panel
 3/05/90 pah	Fixed bug in _NXDefaultMenuMatrix where the message self
		 was being sent to the MenuCell factory instead of class!
		Fixed bug whereby moving a window with the left mouse button
		 while using the right mouse button to manipulate a pop up
		 menu would dump you out of the right mouse loop (this will
		 also be true when SCREEN_CHANGED events start coming thru).
		 The right way to fix this is to have the packages stop sending
		 events through when it sees a right mouse down until it sees
		 the corresponding right mouse up.

79
--
  4/3/90 pah	added _resetModal and _updateModal which update the menu
		 when the user transitions from a modal state to a non-modal
		 state.  this should probably work whether or not the user
		 is modal, but we can't really compatibily take that step
		 (3.0, we hope).  this is compatible with 1.0's update
		 mechanism (whereby the application must implement menu
		 updating).
		added worksWhenModal to return YES since Menus always work
		 when running modal.

80
--
  4/10/90 pah	added request menu support (added a method which sets a bit
		 in a menu which says that it is the request menu so that when
		 update is called, it calls back to NXApp's updateRequestMenu)
		 this is probably not the optimal way to do this, we should
		 expore other options.
		added _removeFromHierarchy which removes the recieving menu
		 from the Menu hierarchy
		fixed bug in modal state whereby PopUpLists were disabled
		 (menus only disable when modal if their class == Menu)

82
--
 4/19/90 aozer	Made sure unattached menus never disappear off the screen when
		their sizes are changed by calling constrainFrameRect:toScreen:
		in sizeToFit.
		If a menu is attached, then we don't muck around with its
		frame; it can go off the screen if it wants to do so.

83
--
  5/3/90 pah	ripped out automatic updating of menus when running modal

84
--
 5/14/90 pah	Menu's no longer worksWhenModal.
 
85
--
 5/22/90 trey	_NXOrderWindowIfActive used instead of _NXToInLevel
 6/04/90 pah	Fixed orderFront: calls so that they only do it when app is
		 active

86
--
 6/12/90 pah	Changed _NXDefaultMenuMatrix() to be a static and made it take
		 an argument which determines whether the MenuCell's in the
		 matrix use user key equivalents (only true if 
		 [self class] == [Menu class])

89
--
 7/29/90 trey	added +_newTitleFromNibSection:withMatrix:

91
--
 8/12/90 trey	nuked _showPanel dead code
 
92
--
 8/20/90 gcockrft  Named the menu malloc zone

98
--
 10/16/90 aozer	Fixed #10107 by preserving the prev values of the two globals
		currentBackupMenu & currentBackupMenuRoot in locals in 
		mouseDown: & rightMouseDown:. A hack. This stuff needs fixing:
		Hit cmd-p in an app and you can move the menu around while
		the modal Print Panel is up; click on "Print" & you can't.

*/

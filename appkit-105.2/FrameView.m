/*
	FrameView.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file defines the FrameView (Private) Class of the AppKit
	
	Modifications:
  
	19Jan87	wrp	file creation from old View.m
	08/16/88 pah	Eliminated WindowButton code
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "View_Private.h"
#import "Window_Private.h"
#import "graphicOps.h"
#import "NXXYShow.h"
#import "color.h"
#import "Application.h"
#import "Button.h"
#import "ButtonCell.h"
#import "Font.h"
#import "FrameView.h"
#import "NXImage.h"
#import <objc/List.h>
#import "Text.h"
#import "TextFieldCell.h"
#import "Window.h"
#import "cursorprivate.h"
#import "perfTimer.h"
#import "packagesWraps.h"
#import "privateWraps.h"
#import "nextstd.h"
#import <dpsclient/dpsNeXT.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <string.h>

typedef struct {
    @defs (Window)
} WindowClass;

#define WINNUMBER(x) (( (WindowClass *) (x))->windowNum)
#define OUTER		(1.0)
#define TBORDER		(1.0)	/* border adjacent to title bar */
#define TITLEHEIGHT	(19.0)	/* height of title bar */
#define ICONWIDTH	(15.0)
#define ICONHEIGHT	(15.0)
#define ICONPAD		(1.0)	/* space between icons */
#define SEPARATOR	(1.0)	/* height of horizontal line below title */
#define IMARGIN		(2.0)	/* horizontal space on outside of icons */
#define ICONVOFFSET	(OUTER + TBORDER + 2.0 + ICONHEIGHT)	/* top-icon.x */
#define MENURIGHTSHADOW (1.0)	/* width of vertical line just inside frame */

#define TITLE_BAR_HEIGHT (OUTER + TBORDER + TITLEHEIGHT + TBORDER + SEPARATOR)

#define TITLEHEIGHT_MINIWINDOW	(11.0)
#define TITLEHEIGHT_TOKEN	(14.0)
#define TOP_BORDER_MINI		(2.0)
#define LEFT_BORDER_MINI	(2.0)
#define BOTTOM_BORDER_MINI	(3.0)
#define RIGHT_BORDER_MINI	(3.0)

#define MENUBORDERCOLOR NX_DKGRAY

#define ICONFONTSIZE	(9.0)
#define FONTSIZE	(12.0)
#define ICONFONTHEIGHT	(11.0)
#define FONTHEIGHT	(15.0)
#define FONTTOWIDTHS(fid) ([fid metrics]->widths)

#define TITLEDRECTCOUNT	(6)	/* count of rects to draw title bar */
#define MENURECTCOUNT	(7)


#define BOTTOM_BORDER_HEIGHT	(6.0)
#define BOTTOM_HILITE_HEIGHT	(1.0)
#define BOTTOM_SEPARATOR_HEIGHT	(1.0)
#define	BORDER_CORNER_WIDTH	(28.0)
#define BORDER_SEPARATOR_WIDTH	(1.0)
#define BORDER_HILITE_WIDTH	(1.0)
#define BORDERED_WINDOW_MIN_WIDTH	(OUTER + BORDER_CORNER_WIDTH + BORDER_SEPARATOR_WIDTH + \
		BORDER_HILITE_WIDTH + BORDER_CORNER_WIDTH + BORDER_SEPARATOR_WIDTH + \
		BORDER_HILITE_WIDTH + BORDER_CORNER_WIDTH + OUTER)

#define SIZE_BAR_HEIGHT		(BOTTOM_BORDER_HEIGHT + BOTTOM_HILITE_HEIGHT + BOTTOM_SEPARATOR_HEIGHT)

#define OFFSET_X_PLAIN		(0.0)
#define OFFSET_X_TITLED		(OUTER)
#define OFFSET_X_SIZEBAR	(OUTER)
#define OFFSET_X_MENU		(OUTER)
#define OFFSET_X_MINIWINDOW	(8.0)
#define OFFSET_X_MINIWORLD	(LEFT_BORDER_MINI)
#define OFFSET_X_TOKEN		(0.0)

#define OFFSET_Y_PLAIN		(0.0)
#define OFFSET_Y_TITLED		(OUTER)
#define OFFSET_Y_SIZEBAR	(OUTER + SIZE_BAR_HEIGHT)
#define OFFSET_Y_MENU		(0.0)
#define OFFSET_Y_MINIWINDOW	(BOTTOM_BORDER_MINI + 1.0)
#define OFFSET_Y_MINIWORLD	(BOTTOM_BORDER_MINI)
#define OFFSET_Y_TOKEN		(0.0)

#define DELTA_WIDTH_PLAIN	(0.0)
#define DELTA_WIDTH_TITLED	(OUTER + OUTER)
#define DELTA_WIDTH_SIZEBAR	(OUTER + OUTER)
#define DELTA_WIDTH_MENU	(OUTER)
#define DELTA_WIDTH_MINIWINDOW	(16.)
#define DELTA_WIDTH_MINIWORLD	(LEFT_BORDER_MINI + RIGHT_BORDER_MINI)
#define DELTA_WIDTH_TOKEN	(0.0)

#define DELTA_HEIGHT_PLAIN	(0.0)
#define DELTA_HEIGHT_TITLED	(TITLE_BAR_HEIGHT + OUTER)
#define DELTA_HEIGHT_SIZEBAR	(TITLE_BAR_HEIGHT + SIZE_BAR_HEIGHT + OUTER)
#define DELTA_HEIGHT_MENU	(TITLE_BAR_HEIGHT)
#define DELTA_HEIGHT_MINIWINDOW	(TOP_BORDER_MINI + TITLEHEIGHT_MINIWINDOW + BOTTOM_BORDER_MINI)
#define DELTA_HEIGHT_MINIWORLD	(TOP_BORDER_MINI + TITLEHEIGHT_TOKEN + BOTTOM_BORDER_MINI)
#define DELTA_HEIGHT_TOKEN	(0.0)


#define LEFT_MOVEABLE		(1)
#define	BOTTOM_MOVEABLE		(2)
#define RIGHT_MOVEABLE		(4)

#define	X	origin.x
#define	Y	origin.y
#define	W	size.width
#define	H	size.height


 /* NXAssert (something to guarantee order of windows) */

static const NXPoint offsets[NX_NUMWINSTYLES] = {
	{OFFSET_X_PLAIN, OFFSET_Y_PLAIN},
	{OFFSET_X_TITLED, OFFSET_Y_TITLED},
	{OFFSET_X_MENU, OFFSET_Y_MENU},
	{OFFSET_X_MINIWINDOW, OFFSET_Y_MINIWINDOW},
	{OFFSET_X_MINIWORLD, OFFSET_Y_MINIWORLD},
	{OFFSET_X_TOKEN, OFFSET_Y_TOKEN},
	{OFFSET_X_SIZEBAR, OFFSET_Y_SIZEBAR}
};
				  
static const NXSize dSizes[NX_NUMWINSTYLES] = {
	{DELTA_WIDTH_PLAIN, DELTA_HEIGHT_PLAIN},
	{DELTA_WIDTH_TITLED, DELTA_HEIGHT_TITLED},
	{DELTA_WIDTH_MENU, DELTA_HEIGHT_MENU},
	{DELTA_WIDTH_MINIWINDOW, DELTA_HEIGHT_MINIWINDOW},
	{DELTA_WIDTH_MINIWORLD, DELTA_HEIGHT_MINIWORLD},
	{DELTA_WIDTH_TOKEN, DELTA_HEIGHT_TOKEN},
	{DELTA_WIDTH_SIZEBAR, DELTA_HEIGHT_SIZEBAR}
};

static const NXCoord topDragHeight[NX_NUMWINSTYLES] = {
	0.,
	TITLE_BAR_HEIGHT,
	TITLE_BAR_HEIGHT,
	NX_TOKENHEIGHT,
	NX_TOKENHEIGHT,
	0.0,		/* Workspace has its own way of dragging tokens */
	TITLE_BAR_HEIGHT
};

static const int styleMasks[NX_NUMWINSTYLES] = {
	0,
	NX_CLOSEBUTTONMASK | NX_MINIATURIZEBUTTONMASK,
	NX_CLOSEBUTTONMASK,
	0,
	0,
	0,
	NX_ALLBUTTONS
};

static const NXCoord titleBarHeights[NX_NUMWINSTYLES] = {
	0.0,
	TITLE_BAR_HEIGHT,
	TITLE_BAR_HEIGHT,
	TOP_BORDER_MINI + TITLEHEIGHT_MINIWINDOW + TOP_BORDER_MINI,
	TOP_BORDER_MINI + TITLEHEIGHT_TOKEN + TOP_BORDER_MINI,
	0.0,
	TITLE_BAR_HEIGHT
};
 
static const NXCoord titleBarInsets[NX_NUMWINSTYLES] = {
	0.0,
	OUTER,
	OUTER + TBORDER,
	TOP_BORDER_MINI,
	TOP_BORDER_MINI,
	0.0,
	OUTER
};

static const BOOL isIconFont[NX_NUMWINSTYLES] = {NO, NO, NO, YES, YES, YES, NO};



/* Values to help draw menu windows */

static const int    menuEdges[] = {1, 2, 3, 0, 1, 2};
static const float  menuSlices[] = {SEPARATOR, MENURIGHTSHADOW, TBORDER, TBORDER, TBORDER, TBORDER};
static const float  menuColors[] = {NX_BLACK, NX_BLACK, NX_WHITE, NX_WHITE, NX_DKGRAY, NX_DKGRAY, NX_BLACK};

/* Values to help draw titled windows */
static const int    titledEdges[] = {1, 3, 0, 1, 2};
static const float  titledSlices[] = {SEPARATOR, TBORDER, TBORDER, TBORDER, TBORDER};

static const float  keyTitledColors[] = {NX_BLACK, NX_LTGRAY, NX_LTGRAY, NX_DKGRAY, NX_DKGRAY, NX_BLACK};
static const float  activeTitledColors[] = {NX_BLACK, NX_LTGRAY, NX_LTGRAY, NX_BLACK, NX_BLACK, NX_DKGRAY};
static const float  otherTitledColors[] = {NX_BLACK, NX_WHITE, NX_WHITE, NX_DKGRAY, NX_DKGRAY, NX_LTGRAY};

static const float *const titledColors[] = {keyTitledColors, activeTitledColors, otherTitledColors};

static const float  textShade[3] = {NX_WHITE, NX_WHITE, NX_BLACK};



@implementation FrameView:View


NXCoord _NXtMargin(int aStyle)
{
    return topDragHeight[aStyle - NX_FIRSTWINSTYLE];
}


static NXCoord rDragMargin(int aMask)
{
    if (aMask & NX_CLOSEBUTTONMASK)
	return OUTER + TBORDER + IMARGIN + ICONWIDTH;
    else
	return 0.0;
}


static NXCoord rTitleMargin(int aMask)
{
    if (aMask & NX_CLOSEBUTTONMASK)
	return OUTER + TBORDER + IMARGIN + ICONWIDTH;
    else
	return OUTER + TBORDER;
}


static NXCoord lDragMargin(int aMask)
{
    if (aMask & NX_MINIATURIZEBUTTONMASK)
	return OUTER + TBORDER + IMARGIN + ICONWIDTH;
    else
	return 0.0;
}


static NXCoord lTitleMargin(int aMask)
{
    if (aMask & NX_MINIATURIZEBUTTONMASK)
	return OUTER + TBORDER + IMARGIN + ICONWIDTH;
    else
	return OUTER + TBORDER;
}


+ newFrame:(const NXRect *)theFrame
    style:(int)aStyle
    buttonMask:(int)theMask
    owner:theWindow
{
    return [[self allocFromZone:NXDefaultMallocZone()]
	initFrame:theFrame style:aStyle buttonMask:theMask owner:theWindow];
}

- initFrame:(const NXRect *)theFrame
    style:(int)aStyle
    buttonMask:(int)theMask
    owner:theWindow
{
    [super initFrame:theFrame];
    [self _setWindow:theWindow];
    [self setNextResponder:theWindow];

    [self setOpaque:YES];
    [self setClipping:NO];
    frFlags.style = aStyle;
    if (aStyle == NX_RESIZEBARSTYLE)
	theMask |= NX_RESIZEBUTTONMASK;
    [self _setMask:theMask];
    [self _setDragMargins:YES :YES :YES];
    [self tile];
    return self;
}


static void initTitleForStyle(id titleCell, int aStyle)
{
    id                  titleFont;

    [titleCell setAlignment:((aStyle == NX_MENUSTYLE) ? NX_LEFTALIGNED : NX_CENTERED)];
    if (isIconFont[aStyle - NX_FIRSTWINSTYLE])
	titleFont = [Font newFont:NXSystemFont size:ICONFONTSIZE];
    else
	titleFont = [Font newFont:NXBoldSystemFont size:FONTSIZE];
    [titleCell setFont:titleFont];
    [titleCell setTextGray:NX_WHITE];
    [titleCell setWrap:NO];
}


+ (NXCoord)minFrameWidth:(const char *)aString
    forStyle:(int)aStyle buttonMask:(int)theMask
{
    static id title = nil;
    NXCoord             width = 0.0;
    NXSize	minSize;

    if (aStyle == NX_MINIWINDOWSTYLE || aStyle == NX_TOKENSTYLE || aStyle == NX_MINIWORLDSTYLE)
	return NX_TOKENWIDTH;
	
    if (aString && aString[0]) {
	if (!title) {
	    NXZone *zone = NXDefaultMallocZone();
	    title = [[TextFieldCell allocFromZone:zone] initTextCell:aString];
	} else
	    [title setStringValue:aString];
	initTitleForStyle(title, aStyle);
	[title calcCellSize:&minSize];
	width = minSize.width;
    }
    width += rTitleMargin(theMask) + lTitleMargin(theMask);
    if (aStyle == NX_MENUSTYLE)
	width += 2.0;
    else if (aStyle == NX_RESIZEBARSTYLE && width < BORDERED_WINDOW_MIN_WIDTH)
	width = BORDERED_WINDOW_MIN_WIDTH;

    return width;
}


+ getFrameRect:(NXRect *)fRect
    forContentRect:(const NXRect *)cRect
    style:(int)aStyle
{
    register const NXSize *sp = &dSizes[aStyle - NX_FIRSTWINSTYLE];
    register const NXPoint *pp = &offsets[aStyle - NX_FIRSTWINSTYLE];

    NX_X(fRect) = NX_X(cRect) - pp->x;
    NX_Y(fRect) = NX_Y(cRect) - pp->y;
    NX_WIDTH(fRect) = NX_WIDTH(cRect) + sp->width;
    NX_HEIGHT(fRect) = NX_HEIGHT(cRect) + sp->height;

    return self;
}


+ getContentRect:(NXRect *)cRect
    forFrameRect:(const NXRect *)fRect
    style:(int)aStyle
{
    register const NXSize *sp = &dSizes[aStyle - NX_FIRSTWINSTYLE];
    register const NXPoint *pp = &offsets[aStyle - NX_FIRSTWINSTYLE];

    NX_X(cRect) = NX_X(fRect) + pp->x;
    NX_Y(cRect) = NX_Y(fRect) + pp->y;
    NX_WIDTH(cRect) = NX_WIDTH(fRect) - sp->width;
    NX_HEIGHT(cRect) = NX_HEIGHT(fRect) - sp->height;

    return self;
 /* ??? there needs to be an error return if the crect has negative width */
}


static int legalMask(int aStyle)
{
    return styleMasks[aStyle - NX_FIRSTWINSTYLE];
}


static id newButton(id self, const char *const icons[], SEL action)
{
    id		 button;
    id		 cell;
    NXZone 	*zone = [self zone];

    button = [[Button allocFromZone:zone] initFrame:(NXRect *)0
	      icon:icons[0] tag:-1 target:self
	      action:action
	      key:0 enabled:YES];
    [button setAltIcon:icons[1]];
    cell = [button cell];
    [cell setHighlightsBy:NX_CONTENTS];
    [cell setShowsStateBy:NX_NONE];
    [button setBordered:NO];
    [button setOpaque:YES];	/* because unbordered buttons are not opaque
				 * by default */
    [button sizeTo:ICONWIDTH :ICONHEIGHT];
    [self addSubview:button];
    return button;
}


- (BOOL)_setMask:(int)theMask
{
    static const char  *const closeIcons[] = {"NXclose", "NXcloseH"};
    static const char  *const iconifyIcons[] = {"NXiconify", "NXiconifyH"};

    int                 hasButton, wantsButton;

    theMask = theMask & legalMask(frFlags.style);

    if (theMask == frFlags.buttonMask)
	return NO;

    hasButton = frFlags.buttonMask & NX_MINIATURIZEBUTTONMASK;
    wantsButton = theMask & NX_MINIATURIZEBUTTONMASK;
    if (hasButton != wantsButton) {
	if (hasButton) {
	    [iconifyButton free];
	    iconifyButton = nil;
	} else
	    iconifyButton = newButton(self, iconifyIcons, @selector(doIconify:));
    }
    hasButton = frFlags.buttonMask & NX_CLOSEBUTTONMASK;
    wantsButton = theMask & NX_CLOSEBUTTONMASK;
    if (hasButton != wantsButton) {
	if (hasButton) {
	    [closeButton free];
	    closeButton = nil;
	} else
	    closeButton = newButton(self, closeIcons, @selector(doClose:));
    }
    frFlags.buttonMask = theMask;
    return YES;
}



- _commonFrameViewInit
{
    return self;
}


- getCloseButton
{
    return closeButton;
}


- getIconifyButton
{
    return iconifyButton;
}


- doIconify:theSender
{
    [window miniaturize:self];
    return self;
}


- free
{
    if (titleCell)
	titleCell = [titleCell free];
    return[super free];
}


- setTitle:(const char *)aString
{
    if (!titleCell) {
	NXZone *zone;
	if (!aString || !aString[0])
	    return self;
	zone = [self zone];
	titleCell = [[TextFieldCell allocFromZone:zone] initTextCell:aString];
	initTitleForStyle(titleCell, frFlags.style);
    } else
	[titleCell setStringValue:aString];
    return self;
}

- doClose:button
{
    register id         delegate;

    delegate = [window delegate];
    if ([delegate respondsTo:@selector(windowWillClose:)]) {
	if ([delegate windowWillClose:window])
	    [window close];
    } else if ([window respondsTo:@selector(windowWillClose:)]) {
	if ([window windowWillClose:window])
	    [window close];
    } else
	[window close];
    return self;
}

#define MINWIDTH (BORDERED_WINDOW_MIN_WIDTH)
#define MINHEIGHT (OUTER + TBORDER + TITLEHEIGHT + TBORDER + SEPARATOR + BOTTOM_SEPARATOR_HEIGHT + BOTTOM_HILITE_HEIGHT + BOTTOM_BORDER_HEIGHT + OUTER)

static void constrainRect(NXRect *aRect, NXCoord x, NXCoord y, int nubbieNumber)
{
    if (nubbieNumber & LEFT_MOVEABLE)
	if (x >= (NX_MAXX(aRect) - MINWIDTH)) {
	    aRect->X += aRect->W - MINWIDTH;
	    aRect->W = MINWIDTH;
	} else {
	    aRect->W += aRect->X - x;
	    aRect->X = x;
	}
    else if (nubbieNumber & RIGHT_MOVEABLE)
	if (x <= aRect->X + MINWIDTH) {
	    aRect->W = MINWIDTH;
	} else {
	    aRect->W = x - aRect->X;
	}

    if (nubbieNumber & BOTTOM_MOVEABLE)
	if (y >= (NX_MAXY(aRect) - MINHEIGHT)) {
	    aRect->Y += aRect->H - MINHEIGHT;
	    aRect->H = MINHEIGHT;
	} else {
	    aRect->H += aRect->Y - y;
	    aRect->Y = y;
	}
}


static void placeEdgesOnRect(register NXRect *tr)
{
    NXCoord e[16];
    
    e[0] = e[8] = e[12] = tr->X;
    e[1] = e[5] = e[9] = tr->Y;
    e[2] = e[6] = e[11] = e[15] = 1.0;
    e[3] = e[7] = tr->H;
    e[10] = e[14] = tr->W;
    e[4] = tr->X + tr->W - 1.0;
    e[13] = tr->Y + tr->H - 1.0;
    
    _NXPlaceEdgeWindows(e);
}



#define	RESIZEEVENTMASK (NX_LMOUSEUPMASK|NX_MOUSEDRAGGEDMASK)

- _resize:(NXEvent *)theEvent
{
    register int        i;
    NXRect              windowFrame;
    NXRect              newFrame, oldFrame;
    NXCoord             hx, hy, oldx, oldy, deltaX, deltaY;
    id                  winDelegate;
    int                 oldMask;
    BOOL                waitBigMove = YES;
    BOOL                mouseUpNotReceived = YES;
    NXHandler		exception;

    [window disableCursorRects];
    oldMask = [window addToEventMask:RESIZEEVENTMASK];

    [window getFrame:&windowFrame];
    _NXEnsureEdgeWindows();
    exception.code = 0;
    NX_DURING {
	placeEdgesOnRect(&windowFrame);
    
	i = [self _inResize:&theEvent->location];
	_NXShowEdgeWindows();
    
	[window convertBaseToScreen:&theEvent->location];
	if (i & LEFT_MOVEABLE)
	    deltaX = NX_X(&windowFrame) - theEvent->location.x;
	else if (i & RIGHT_MOVEABLE)
	    deltaX = NX_MAXX(&windowFrame) - theEvent->location.x;
	else
	    deltaX = 0.0;
	if (i & BOTTOM_MOVEABLE)
	    deltaY = NX_Y(&windowFrame) - theEvent->location.y;
	else
	    deltaY = 0.0;
    
	oldx = theEvent->location.x;
	oldy = theEvent->location.y;
	oldFrame = newFrame = windowFrame;
    
	winDelegate = [window delegate];
	if (![winDelegate respondsTo:@selector(windowWillResize:toSize:)]) {
	    if ([window respondsTo:@selector(windowWillResize:toSize:)]) {
		winDelegate = window;
	    } else {
		winDelegate = 0;
	    }
	}

 /* while mouse is down, track appropriate edges */
	while (mouseUpNotReceived) {
	    theEvent = [NXApp getNextEvent:(RESIZEEVENTMASK)];
	    if (theEvent->type == NX_LMOUSEUP)
		mouseUpNotReceived = NO;
	    [window convertBaseToScreen:&theEvent->location];
	    if (waitBigMove) {
		hx = (i & (LEFT_MOVEABLE | RIGHT_MOVEABLE)) ? theEvent->location.x - oldx : 0.0;
		hy = theEvent->location.y - oldy;
		if (hx * hx + hy * hy >= 16.0) {
		    if (hx / hy > 2.0 || hx / hy < -2.0)
			i &= ~BOTTOM_MOVEABLE;
		    waitBigMove = NO;
		}
	    }
	    if (!waitBigMove) {
		if (theEvent->location.x != oldx || theEvent->location.y != oldy) {
		    newFrame = windowFrame;
		    constrainRect(&newFrame, theEvent->location.x + deltaX, theEvent->location.y + deltaY, i);
    
		/* allow delegate to constrain sizing */
		    if (winDelegate) {
			NXSize              newSize = newFrame.size;
    
			[winDelegate windowWillResize:window toSize:&newSize];
			if (i & BOTTOM_MOVEABLE) {
			    newFrame.Y += newFrame.H - newSize.height;
			    newFrame.H = newSize.height;
			}
			if (i & LEFT_MOVEABLE)
			    newFrame.X += newFrame.W - newSize.width;
			if (i & (LEFT_MOVEABLE | RIGHT_MOVEABLE))
			    newFrame.W = newSize.width;
		    }
		    if (!NXEqualRect(&newFrame, &oldFrame)) {
			placeEdgesOnRect(&newFrame);
			NXPing();
			oldFrame = newFrame;
		    }
		}
		oldx = theEvent->location.x;
		oldy = theEvent->location.y;
	    }
	}
	if (!NXEqualRect(&newFrame, &windowFrame))
	    [window _resizeWindow:&newFrame userAction:YES];
    } NX_HANDLER {
	exception = NXLocalHandler;
    } NX_ENDHANDLER
    _NXHideEdgeWindows();

    [window setEventMask:oldMask];
    [window enableCursorRects];
    if (exception.code)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return self;
}


- (const char *)title
{
    static const char   empty[1] = {'\0'};

    return titleCell ? [titleCell stringValue] : empty;
}


- setCloseTarget:aTarget
{
    [closeButton setTarget:aTarget];
    return self;
}


- setCloseAction:(SEL)anAction
{
    [closeButton setAction:anAction];
    return self;
}


- becomeKeyWindow
{
    return self;
}


- resignKeyWindow
{
    return self;
}


- _focusDown:(BOOL)clipNeeded
{
    return self;
}

static NXRect *darkGrayRects = NULL;
static int dgrCount = 0;
static NXRect *lightGrayRects = NULL;
static int lgrCount = 0;
static NXRect *whiteRects = NULL;
static int wrCount = 0;
static NXRect *blackRects = NULL;
static int brCount = 0;
static const int RFCHUNKSIZE = 20;

- (BOOL)_optimizedRectFill:(const NXRect *)rect gray:(float)gray
{
    NXRect *r;
    NXZone *zone = [self zone];

    if (!rect || (gray != (float)NX_DKGRAY && gray != (float)NX_WHITE && gray != (float)NX_BLACK && gray != (float)NX_LTGRAY) ||
	![self _canOptimizeDrawing]) {
	return NO;
    } else {
	if (gray == (float)NX_DKGRAY) {
	    if (!darkGrayRects) {
		dgrCount = 0;
		NX_ZONEMALLOC(zone, darkGrayRects, NXRect, RFCHUNKSIZE);
	    } else if (!(dgrCount % RFCHUNKSIZE)) {
		NX_ZONEREALLOC(zone, darkGrayRects, NXRect, 
		    (dgrCount/RFCHUNKSIZE+1)*RFCHUNKSIZE);
	    }
	    r = darkGrayRects+dgrCount;
	    dgrCount++;
	} else if (gray == (float)NX_WHITE) {
	    if (!whiteRects) {
		wrCount = 0;
		NX_ZONEMALLOC(zone, whiteRects, NXRect, RFCHUNKSIZE);
	    } else if (!(wrCount % RFCHUNKSIZE)) {
		NX_ZONEREALLOC(zone, whiteRects, NXRect, 
		    (wrCount/RFCHUNKSIZE+1)*RFCHUNKSIZE);
	    }
	    r = whiteRects+wrCount;
	    wrCount++;
	} else if (gray == (float)NX_BLACK) {
	    if (!blackRects) {
		brCount = 0;
		NX_ZONEMALLOC(zone, blackRects, NXRect, RFCHUNKSIZE);
	    } else if (!(brCount % RFCHUNKSIZE)) {
		NX_ZONEREALLOC(zone, blackRects, NXRect, 
		    (brCount/RFCHUNKSIZE+1)*RFCHUNKSIZE);
	    }
	    r = blackRects+brCount;
	    brCount++;
	} else if (gray == (float)NX_LTGRAY) {
	    if (!lightGrayRects) {
		lgrCount = 0;
		NX_ZONEMALLOC(zone, lightGrayRects, NXRect, RFCHUNKSIZE);
	    } else if (!(lgrCount % RFCHUNKSIZE)) {
		NX_ZONEREALLOC(zone, lightGrayRects, NXRect, 
		    (lgrCount/RFCHUNKSIZE+1)*RFCHUNKSIZE);
	    }
	    r = lightGrayRects+lgrCount;
	    lgrCount++;
	} else {
	    return NO;
	}
	*r = *rect;
    }

    return YES;
}

- (BOOL)_drawOptimizedRectFills
{
    BOOL drew = NO;

    if (dgrCount && darkGrayRects) {
	_NXJustFillRects(NX_DKGRAY, (float *)darkGrayRects, dgrCount*4);
	NX_FREE(darkGrayRects); dgrCount = 0;
	darkGrayRects = NULL;
	drew = YES;
    }
    if (wrCount && whiteRects) {
	_NXJustFillRects(NX_WHITE, (float *)whiteRects, wrCount*4);
	NX_FREE(whiteRects); wrCount = 0;
	whiteRects = NULL;
	drew = YES;
    }
    if (brCount && blackRects) {
	_NXJustFillRects(NX_BLACK, (float *)blackRects, brCount*4);
	NX_FREE(blackRects); brCount = 0;
	blackRects = NULL;
	drew = YES;
    }
    if (lgrCount && lightGrayRects) {
	_NXJustFillRects(NX_LTGRAY, (float *)lightGrayRects, lgrCount*4);
	NX_FREE(lightGrayRects); lgrCount = 0;
	lightGrayRects = NULL;
	drew = YES;
    }

    return drew;
}

static id xyshows = nil;
static id currentxyshow = nil;

- (BOOL)_optimizedXYShow:(const char *)text numChars:(int)numChars at:(NXCoord)x :(NXCoord)y
{
    if (!text || ![self _canOptimizeDrawing]) {
	return NO;
    } else if (currentxyshow) {
	y = bounds.size.height - y;
	[currentxyshow moveTo:x :y];
	[currentxyshow show:text count:numChars];
    } else {
	NX_ASSERT(YES, "No current NXXYShow object!");
    }

    return YES;
}

- (BOOL)_setOptimizedXYShowFont:font gray:(float)gray
{
    NXXYShow *xyshow;
    int i, count;

    if (!font || ![self _canOptimizeDrawing]) {
	return NO;
    } else {
	count = [xyshows count];
	for (i = 0; i < count; i++) {
	    xyshow = (NXXYShow *)[xyshows objectAt:i];
	    if ((xyshow->font == font || xyshow->screenFont == font) && xyshow->gray == gray) {
		currentxyshow = xyshow;
		break;
	    }
	}
	if (i == count) {
	    currentxyshow = [[NXXYShow allocFromZone:[self zone]] initFont:font gray:gray];
	    if (!xyshows) xyshows = [[List allocFromZone:[self zone]] init];
	    [xyshows addObject:currentxyshow];
	}
    }

    return YES;
}

- (BOOL)_drawOptimizedXYShow
{
    int count;

    count = [xyshows count];
    if (count) {
	PStranslate(0.0, bounds.size.height);
	PSscale(1.0, -1.0);
	for (count = [xyshows count]; count; count--) {
	    [[[xyshows removeLastObject] finalize] free];
	}
	PSscale(1.0, -1.0);
	PStranslate(0.0, - bounds.size.height);
    }
    [xyshows free];
    xyshows = nil;

    return count ? YES : NO;
}


- displayBorder
{
    [self _drawFrame:&bounds :1];
    [window disableFlushWindow];
    [iconifyButton display];
    [closeButton display];
    [window reenableFlushWindow];
    return self;
}


- _getTitleRect:(NXRect *)tRect
{
    int index = frFlags.style - NX_FIRSTWINSTYLE;
    NXRect aRect;
    
    aRect = bounds;
    NXDivideRect(&aRect, tRect, titleBarHeights[index], 3);
    NXInsetRect(tRect, titleBarInsets[index], titleBarInsets[index]);
    return self;
}


- _displayTitle
{
    NXRect tRect;
    
    [self _getTitleRect:&tRect];
    [self _drawFrame:&tRect :1];
    [window disableFlushWindow];
    [iconifyButton display];
    [closeButton display];
    [window reenableFlushWindow];
    return self;
}


- drawSelf:(const NXRect *)clips :(int)clipCount
{
    id                  cView;
    NXRect              cRect, tempRect;
    BOOL		needsFrame = YES;
    BOOL		needsFill = NO;

    if (frFlags.style == NX_TOKENSTYLE)
	return self;
    if (cView = [window contentView]) {
	[cView getFrame:&cRect];
	needsFrame = !(NXContainsRect(&cRect, clips));
	needsFill = ![cView isOpaque];
    }
    if (needsFrame)
	[self _drawFrame:clips :clipCount];
    if (needsFill) {
	if ([window _colorSpecified] && [self shouldDrawColor]) {
	    NXSetColor([window backgroundColor]);
	} else {
	    PSsetgray([window backgroundGray]);
	}
	if (clipCount == 1) {
	    NXIntersectionRect(clips, &cRect);
	    NXRectFill(&cRect);
	} else
	    while (--clipCount) {
		tempRect = cRect;
		NXIntersectionRect(++clips, &tempRect);
		NXRectFill(&tempRect);
	    }
    }
    return self;
}

#define TIME_DISPLAY 0

- _display:(const NXRect *)rects :(int)rectCount
{
    id retval;
    BOOL optimized;
#ifdef POSSIBLE_FIX_TO_PREVENT_TRASHING_OF_FRAME
    NXRect clipRect;
    id cview;
#endif

    CLEARTIMES(TIME_DISPLAY);
    MARKTIME(TIME_DISPLAY, "FrameView _display::", 1);
    retval = [super _display:rects :rectCount];
#ifdef POSSIBLE_FIX_TO_PREVENT_TRASHING_OF_FRAME
    cview = [window contentView];
    if (cview) {
	[cview getFrame:&clipRect];
	NXRectClip(&clipRect);
    }
#endif
    optimized = [self _drawOptimizedRectFills];
    optimized = [self _drawOptimizedXYShow] || optimized;
    if (optimized) {
	MARKTIME(TIME_DISPLAY, "FrameView optimized _display:: finished.", 1);
    } else {
	MARKTIME(TIME_DISPLAY, "FrameView _display:: finished.", 1);
    }
    DUMPTIMES(TIME_DISPLAY);

    return retval;
}

- _drawFrame:(const NXRect *) clips :(int)clipCount
{
    switch (frFlags.style) {
    case NX_PLAINSTYLE:
	break;
    case NX_MINIWORLDSTYLE:
    case NX_MINIWINDOWSTYLE:
	[self _drawMiniWorld:clips :clipCount];
	break;
    case NX_TITLEDSTYLE:
    case NX_RESIZEBARSTYLE:
	[self _drawTitledFrame:clips :clipCount];
	break;
    case NX_MENUSTYLE:
	[self _drawMenuFrame:clips :clipCount];
	break;
    }
    return self;
}

- _drawMiniWorld:(const NXRect *) clips :(int)clipCount
{
    register NXCoord    titleOffset;

    if (frFlags.style == NX_MINIWORLDSTYLE) {
	/* if ([window _wsmOwnsWindow]) */
	/*    return self; */
	titleOffset = 14.0;
	[[NXImage findImageNamed:"NXminiWorld"]
		composite:NX_COPY fromRect:clips toPoint:&clips->origin];
    } else {		/* frFlags.style == NX_MINIWINDOWSTYLE */
	titleOffset = 12.0;
	[[NXImage findImageNamed:"NXminiWindow"]
		composite:NX_COPY fromRect:clips toPoint:&clips->origin];
    }
    if (titleCell) {
	NXRect              tempRect;

	tempRect.origin.y = NX_TOKENHEIGHT - titleOffset;
	tempRect.size.height = ICONFONTHEIGHT;
	tempRect.origin.x = bounds.origin.x;
	tempRect.size.width = bounds.size.width;
	if (NXIntersectsRect(&tempRect, clips))
	    [titleCell drawSelf:&tempRect inView:self];
    }
    return self;
}


static void ClipRectFill(NXRect *aRect, const NXRect *clip)
{
    if (NXIntersectionRect(clip, aRect))
	NXRectFill(aRect);
}


static void PaintTiledRects(NXRect *aRect, const NXCoord *slices, const int *edges, const float *colors, int count, const NXRect *clip)
{
    NXRect              tempRect;
    register NXRect    *bRect = &tempRect;
    register float      thiscolor, lastcolor = -1.0;

    while (--count) {
	NXDivideRect(aRect, bRect, *slices++, *edges++);
	thiscolor = *colors++;
	if (lastcolor != thiscolor) {
	    PSsetgray(thiscolor);
	    lastcolor = thiscolor;
	}
	ClipRectFill(bRect, clip);
    }
    if (lastcolor != *colors)
	PSsetgray(*colors);
    *bRect = *aRect;
    ClipRectFill(bRect, clip);
}


- _drawMenuFrame:(const NXRect *) clips :(int)clipCount
{
    NXRect              aRect, bRect;
    NXRect             *arp = &aRect;
    NXRect             *brp = &bRect;

    *arp = bounds;

 /* paint outer border */
    PSsetgray(MENUBORDERCOLOR);
    NXDivideRect(arp, brp, OUTER, 3);
    ClipRectFill(brp, clips);
    NXDivideRect(arp, brp, OUTER, 0);
    ClipRectFill(brp, clips);

 /* get the title and the other rects around it */
    NXDivideRect(arp, brp, TBORDER + TITLEHEIGHT + TBORDER + SEPARATOR, 3);

    PaintTiledRects(brp, menuSlices, menuEdges, menuColors, MENURECTCOUNT, clips);

    if (titleCell) {
	NX_X(brp) = bounds.origin.x + lTitleMargin(frFlags.buttonMask) + 2.0;
	NX_WIDTH(brp) = bounds.size.width - lTitleMargin(frFlags.buttonMask) - rTitleMargin(frFlags.buttonMask) - 2.0;
	NX_Y(brp) += floor((NX_HEIGHT(brp) - FONTHEIGHT) / 2.0);
	NX_HEIGHT(brp) = FONTHEIGHT;
	if (NXIntersectsRect(brp, clips))
	    [titleCell drawSelf:brp inView:self];
    }
    return self;
}


static void drawResizeBar(const NXRect *barRect)
{
    NXRect              aRect, bRect, tRect;
    NXRect             *arp = &aRect;
    NXRect             *brp = &bRect;
    NXCoord             x1, x2;

    *arp = *barRect;

    NXDivideRect(arp, brp, BOTTOM_BORDER_HEIGHT, 1);

    PSsetgray(NX_LTGRAY);
    NXRectFill(brp);
    
    x1 = NX_X(brp) + BORDER_CORNER_WIDTH;
    x2 = NX_X(brp) + NX_WIDTH(brp) - BORDER_CORNER_WIDTH -
      BORDER_HILITE_WIDTH - BORDER_SEPARATOR_WIDTH;

    tRect = *brp;
    tRect.size.width = BORDER_SEPARATOR_WIDTH;

    PSsetgray(NX_DKGRAY);

    tRect.origin.x = x1;
    NXRectFill(&tRect);

    tRect.origin.x = x2;
    NXRectFill(&tRect);

    NXDivideRect(arp, brp, BOTTOM_SEPARATOR_HEIGHT, 3);
    NXRectFill(brp);

    PSsetgray(NX_WHITE);

    tRect.origin.x = x1 + BORDER_SEPARATOR_WIDTH;
    NXRectFill(&tRect);

    tRect.origin.x = x2 + BORDER_SEPARATOR_WIDTH;
    NXRectFill(&tRect);

    NXRectFill(arp);
}


static void ClipFrameRect(const NXRect *aRect, const NXRect *clip)
{
    NXRect	tRect = *aRect;
    NXRect	bRect;
    int i;
	
    if(NXContainsRect(clip, &tRect))
	NXFrameRect(&tRect);
    else
	for (i = 0; i < 4; i++) {
	    NXDivideRect(&tRect, &bRect, 1.0, i);
	    ClipRectFill(&bRect, clip);
	}
}


- _drawTitledFrame:(const NXRect *) clips :(int)clipCount
{
    int                 titleType;
    NXRect              aRect, bRect;
    NXRect             *arp = &aRect;
    NXRect             *brp = &bRect;

    titleType = [window isKeyWindow] ? 0 : ([window isMainWindow] ? 1 : 2);
    *arp = bounds;

 /* paint outer border */
    PSsetgray(NX_BLACK);
    ClipFrameRect(arp, clips);

 /* paint inner gray border */
    NXInsetRect(arp, OUTER, OUTER);
    NXDivideRect(arp, brp, TBORDER + TITLEHEIGHT + TBORDER + SEPARATOR, 3);

    PaintTiledRects(brp, titledSlices, titledEdges, titledColors[titleType], TITLEDRECTCOUNT, clips);

    if (titleCell) {
	NX_X(brp) = bounds.origin.x + lTitleMargin(frFlags.buttonMask);
	NX_WIDTH(brp) = bounds.size.width - lTitleMargin(frFlags.buttonMask) - rTitleMargin(frFlags.buttonMask);
	NX_Y(brp) += floor((NX_HEIGHT(brp) - FONTHEIGHT) / 2.0);
	NX_HEIGHT(brp) = FONTHEIGHT;
	if (NXIntersectsRect(brp, clips)) {
	    [titleCell setTextGray:textShade[titleType]];
	    [titleCell drawSelf:brp inView:self];
	}
    }
    if (frFlags.style == NX_RESIZEBARSTYLE) {
	NXDivideRect(arp, brp, BOTTOM_BORDER_HEIGHT + BOTTOM_HILITE_HEIGHT + BOTTOM_SEPARATOR_HEIGHT, 1);
	if (NXIntersectsRect(brp, clips))
	    drawResizeBar(brp);
    }
    return self;
}


- (int)_inResize:(const NXPoint *)thePoint
{
    NXRect              bar;
    NXRect              boundsRect;
    BOOL		flipped;

    boundsRect = bounds;

    NXInsetRect(&boundsRect, OUTER, OUTER);
    NXDivideRect(&boundsRect, &bar, BOTTOM_SEPARATOR_HEIGHT + BOTTOM_HILITE_HEIGHT + BOTTOM_BORDER_HEIGHT, 1);

    flipped = [self isFlipped];
    if (!NXMouseInRect(thePoint, &bar, flipped))
	return 0;

    bar.size.width -= BORDER_CORNER_WIDTH;
    if (!NXMouseInRect(thePoint, &bar, flipped))
	return RIGHT_MOVEABLE | BOTTOM_MOVEABLE;
    bar.size.width = BORDER_CORNER_WIDTH;
    if (!NXMouseInRect(thePoint, &bar, flipped))
	return BOTTOM_MOVEABLE;
    else
	return LEFT_MOVEABLE | BOTTOM_MOVEABLE;
}

/* over-ride View method to prevent contentViews from interferring */
/* with dragging for miniWindows */

- hitTest:(NXPoint *)aPoint
{
    if (frFlags.style == NX_MINIWINDOWSTYLE || frFlags.style == NX_MINIWORLDSTYLE)
	return self;
    else
	return [super hitTest:aPoint];
}

/* various mouse methods prevent event from going up the reponder chain if
   we are the lastLeftHit (i.e., if we were the view found by hit testing).  I
   imagine this is to keep apps from getting their hands on events in the
   window frame, since otherwise they would bubble up to the window.
 */

- mouseDown:(NXEvent *)theEvent
{
    register id         delegate;

    if (frFlags.style == NX_MINIWINDOWSTYLE) {
	if (theEvent->data.mouse.click == 2) {
	    [window deminiaturize:self];
	    [NXApp activateSelf:YES];
	    delegate = [[window _getCounterpart] delegate];
	    if ([delegate respondsTo:@selector(windowDidDeminiaturize:)])
		if (_NXGetShlibVersion() <= MINOR_VERS_1_0)
		    [delegate windowDidDeminiaturize:self];
		else
		    [delegate windowDidDeminiaturize:window];
	}
    } else if (frFlags.style == NX_MINIWORLDSTYLE) {
	if (theEvent->data.mouse.click == 2)
	    [NXApp unhide:self];
    } else if (self == [window _lastLeftHit]) {
        if ((frFlags.style == NX_RESIZEBARSTYLE) &&
		[self _inResize:&theEvent->location])
	    [self _resize:theEvent];
    } else
	return [super mouseDown:theEvent];

    return self;
}


- mouseUp:(NXEvent *)theEvent
{
    if (self == [window _lastLeftHit])
	return self;
    else
	return [super mouseUp:theEvent];
}


- rightMouseDown:(NXEvent *)theEvent
{
    return [NXApp rightMouseDown:theEvent];
}


- rightMouseUp:(NXEvent *)theEvent
{
    if (self == [window _lastRightHit])
	return self;
    else
	return [super rightMouseUp:theEvent];
}


- sizeTo:(NXCoord)width :(NXCoord)height
{
    BOOL                rightChanged;
    BOOL                topChanged;

    rightChanged = (width != bounds.size.width);
    topChanged = (height != bounds.size.height);

    if (rightChanged || topChanged) {
	[super sizeTo:width :height];
	[self _setDragMargins:NO :rightChanged :topChanged];
	[self tile];
    }
    return self;
}

- awake
{
    [self _setDragMargins:YES :YES :YES];
    return self;
}

- _setDragMargins:(BOOL)left :(BOOL)right :(BOOL)top
{
    int                 wnum;

    wnum = WINNUMBER(window);
    if (wnum <= 0)
	return self;

    if (frFlags.style != NX_PLAINSTYLE) {
	if (left)
	    _NXLeftMargin(lDragMargin(frFlags.buttonMask), wnum);
	if (right)
	    _NXRightMargin(rDragMargin(frFlags.buttonMask), wnum);
	if (top)
	    _NXTopMargin(_NXtMargin(frFlags.style), wnum);
    } else if (_NXShowAllWindows) {
	if (left)
	    _NXLeftMargin(0.0, wnum);
	if (right)
	    _NXRightMargin(0.0, wnum);
	if (top)
	    _NXTopMargin(frame.size.height, wnum);
    }

    return self;
}

- tile
{
    NXCoord             iconX, closeX, height;

    height = bounds.size.height - ICONVOFFSET;

    if (closeButton) {
	closeX = NX_MAXX(&bounds) - OUTER - TBORDER - IMARGIN - ICONWIDTH;
	[closeButton moveTo:closeX :height];
    }
    if (iconifyButton) {
	iconX = NX_X(&bounds) + OUTER + TBORDER + IMARGIN;
	[iconifyButton moveTo:iconX :height];
    }
    return self;
}

- moveTo:(NXCoord)x :(NXCoord)y
{
    return self;
}


- (BOOL)acceptsFirstMouse
{
    return YES;
}

- read:(NXTypedStream *) s
{
    return self;
}

- write:(NXTypedStream *) s
{
    return self;
}


- _setCloseEnabled:(BOOL)aFlag andDisplay:(BOOL)display
{
    int                 tempMask;

    tempMask = (frFlags.buttonMask & ~NX_CLOSEBUTTONMASK) | (aFlag ? NX_CLOSEBUTTONMASK : 0);

    if ([self _setMask:tempMask]) {
	[self _setDragMargins:NO :YES :NO];
	[self tile];
	if (display)
	    [window _displayTitle];
    }
    return self;
}

- (BOOL)worksWhenModal
{
    return [window worksWhenModal];
}

@end


/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
  
0.82
----
 1/25/89 trey	setTitle: takes a const param
02/05/89 bgy	added support for cursor tracking. the doResize: method now
		 disables cursor rect tracking before it installs it's own
		 tracking rects. After the resize operation is finished,
		 the cursor rects are enabled.
		 
 2/09/89 wrp	PaintTiledRects, displayFont, initClass, lMargin, lTitleMargin, 
 		 legalMask, rMargin, rTitleMargin, tMargin declared static
 2/09/89 wrp	containsRect changed to _NXContainsRect
 2/17/89 wrp	tMargin changed to _NXtMargin and made private extern
 3/09/89 wrp	all new resizing
 3/16/89 wrp	removed writeSelf:, made readSelf: a noop, put in noop
		 over-rides for read: write:.  The reason for these changes:
		 FrameView should never be archived. Window creates one when
		 it is read in.
 3/16/89 wrp	removed resizeButton IV
 3/17/89 wrp	fixed bug in title where it only returned the visible portion.
 3/17/89 wrp	used TextFieldCell to draw title.
 3/18/89 wrp	major cleanup
 3/22/89 wrp	added const where needed
 3/25/89 wrp	fixed title button display bug by making them opaque
 3/27/89 wrp	removed NX_BORDEREDSTYLE, added NX_SIZEBARSTYLE, removed abort of resize, added hysteresis
 3/27/89 wrp	factored out  _placeWindow code into new method.
 4/02/89 wrp	removed defaultIcon (NeXT logo) from miniwindowstyle drawing.

0.91
----
05/17/89 wrp	put resizing windows in the packages 
05/18/89 wrp	moved _placeWindow to Window Class as _resizeWindow:userAction:
 5/19/89 trey	minimized static data
05/22/89 wrp	changed mouse handling methods to forward event along responder
		 chain if not intended for FrameView to make FrameView more
		 transparent to developer.
05/22/89 wrp	removed private method _setNIBborder
05/22/89 wrp	sent windowDidDeminiaturize: to window's counterpart rather
		 than window in mouseDown:
06/09/89 wrp	removed _wsmOwnsWindow test in mouseDown: because if workspace
		 owns this window, it will get the events.
06/09/89 wrp	added clipping to minimize postscript to draw frames
06/11/89 wrp	cleanedup #defines
		put all window geometry data at top of file
		rewrote rMargin, rTitledMargin to rDragMargin, rTitledMargin,
		 used correctly throughout
		rewrote minFrameWidth:forStyle:buttonMask: to use a 
		 TextFieldCell to measure the text length
		factored out initTitleForStyle() from setTitle:
		took delegate calls to windowDidMiniaturize from doIconify and 
		 put them in Window's miniaturize method.
		added _getTitleRect: _displayTitle methods
		prototyped PaintTiledRects and added clip parameter
		added ClipRectFill(), ClipFrameRect()
		added hitTest: to over-ride View's method, so that contentView 
		 of miniWorld or miniWindow will not get mouseDown:
		rewrote tile method to make geometry correct

0.93
----
 6/15/89 pah	make title return const char *
 6/28/89 wrp	removed _NXContainsRect to rect.m and named NXContainsRect
 7/11/89 wrp	fixed bug in _resize: where mouseUp was only used to exit the 
 		 loop, not do any resizing

0.96
----
 7/20/89 pah	fix non-retained window title drawing by having ClipRectFill
		 not modify the rect that is passed to it

 7/24/89 wrp	made horizontal constraining not lock in as hard by changing 
 		 condition to be slope < .5 rather than slope <= .5

77
--
 2/26/89 trey	nuked call needless call to DoDragWindow in mouseDown.  The
		 packages always drag icons.

79
--
  4/3/90 pah	added worksWhenModal since the close/iconify buttons should
		 work in any window which worksWhenModal.

83
--
 4/26/90 aozer	Draw in color if window has background color specified.
 4/27/90 aozer	Bitmap->NXImage (use NXminiWindow instead of miniWindow)

85
--
 5/28/90 trey	rearranged tables of various per-window-type constants to
		 get rid of indices array
 6/04/90 pah	Overrode _optimizedRectFill:gray:, _optimizedXYShow:at::
		 and _setOptimizedXYShowFont:gray: to gather up the optimized
		 rects and text for _drawOptimized*
		Added _drawOptimizedRectFills, _drawOptimizedXYShow, and
 		 overrode _display:: to call them

 7/20/90 aozer	Changed isOnColorScreen -> canStoreColor

91
--
 8/8/90 trey	windowDidDeminiaturize passes the Window instead of the
		 FrameView
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor

93
--
 9/4/90 trey	resize protected by DURING/HANDLER's

*/

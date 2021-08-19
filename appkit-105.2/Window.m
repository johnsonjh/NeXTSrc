/*
	Window.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Window_Private.h"
#import "Application_Private.h"
#import "NXJournaler_Private.h"
#import "View_Private.h"
#import "packagesWraps.h"
#import <objc/List.h>
#import "FrameView.h"
#import "Font.h"
#import "Text.h"
#import "Button.h"
#import "Panel.h"
#import "PrintInfo.h"
#import "Cell.h"
#import "NXCursor.h"
#import "cursorRect.h"
#import <defaults.h>
#import "screens.h"
#import "nextstd.h"
#import "privateWraps.h"
#import "publicWraps.h"
#import <objc/Storage.h>
#import <dpsclient/wraps.h>
#import <stdio.h>
#import <limits.h>
#import <sys/param.h>
#import <sys/file.h>
#import <zone.h>

typedef struct {
    @defs (View)
} ViewId;

typedef struct {
    @defs (FrameView)
} FViewId;

#define DEFFONTSIZE	12.0

static const char       windowPackageName[] = "windowPackage";

#define	DEFAULTBACKING	NX_BUFFERED	/* Buffered window */

#define DEFAULTSTYLE	NX_PLAINSTYLE

#define	PLAINEVENTMASK	(NX_MOUSEDOWNMASK |	\
			 NX_MOUSEUPMASK |	\
			 NX_RMOUSEDOWNMASK |	\
			 NX_RMOUSEUPMASK |	\
			 NX_KEYDOWNMASK |	\
			 NX_KEYUPMASK |		\
			 NX_APPDEFINEDMASK |	\
			 NX_KITDEFINEDMASK |	\
			 NX_SYSDEFINEDMASK |	\
			 NX_MOUSEENTEREDMASK |	\
			 NX_MOUSEEXITEDMASK )

#define TITLEDEVENTMASK	(PLAINEVENTMASK)
#define MENUEVENTMASK	(PLAINEVENTMASK)
#define ICONEVENTMASK	(PLAINEVENTMASK & ~(NX_KEYDOWNMASK | NX_KEYUPMASK))

static const int    eventMasks[] = {PLAINEVENTMASK,
				    TITLEDEVENTMASK,
				    MENUEVENTMASK,
				    ICONEVENTMASK,
				    ICONEVENTMASK,
				    ICONEVENTMASK,
				    TITLEDEVENTMASK};

#define CURSORAT(BASE, OFFSET) ((NXCursorRect *) ((char *) (BASE) + (OFFSET)))
#define CURSOR_RECT_MASK \
    (NX_MOUSEENTEREDMASK|NX_MOUSEEXITEDMASK|NX_CURSORUPDATEMASK)
    
    /* Test to see if this is a good window for the windows menu */
#define WINDOWSMENUWINDOW(w)   (!private->wFlags3.excludedFromWindowsMenu && \
								[w canBecomeMainWindow] && [w _isDocWindow])

static id defaultFont = nil;
static int windowOwnedByWSM = -5;
static NXWindowDepthType defaultDepthLimit = 0;

static void changeKeyAndMain(Window *self, BOOL limitedBecomeKeyOk);
static void changeCursorRects(Window *self, BOOL add);
static NXCursorRect *findCursorRect(Window *self);
static void discardCursorRect(id self, int index);
static void setCursorRect(id self, const NXRect *r, int index);
static void moveRectToScreen (NXRect *rect, const NXScreen *screen);
static const NXScreen *screenForRect (NXRect *rect);
static const NXScreen *closestScreenToRect (NXRect *rect);
static BOOL makeFrameVisible(NXRect *frameRect, int aStyle, const NXScreen *inScreen);

typedef struct {
    int num;
    id owner;
} TrackInfo;

static unsigned trackInfoHash(const void *info, const void *data);
static int trackInfoEqual(const void *info, const void *data1, const void *data2);
static BOOL validateMouseEvent(Window *win, NXEvent *ev);

static const NXHashTablePrototype trackInfoProto = {
    trackInfoHash, trackInfoEqual, NULL, 0
};

/*
 * The structure that we hang off the end of every instance of Window.
 * Best way to refer to this is through the private macro below.
 */
typedef struct _WindowPrivate {
    NXColor *backgroundColor;
    struct _wFlags3 {
	unsigned int wantsToBeOnMainScreen:1;
	unsigned int optimizedDrawingOk:1;
	unsigned int optimizeDrawing:1;
	unsigned int titleIsFilename:1;
	unsigned int excludedFromWindowsMenu:1;
	unsigned int depthLimit:4;
	unsigned int delegateReturnsValidRequestor:1;
	unsigned int lmouseupPending:1;
	unsigned int rmouseupPending:1;
	unsigned int wantsToDestroyRealWindow:1;
	unsigned int :3;
    } wFlags3;
    int lastResizeTime;		/* VertRetrace of the last placewindow */
} WindowPrivate;

#define private ((WindowPrivate *)_private)

@implementation Window:Responder

+ initialize
{
    [self setVersion:2];
    return self;
}


/** Instantiation **/

/*
 * Little piece of factored code that appears at the end of all window creation methods
 * with explicit screen arguments. This argument indicates that the specified
 * contentRect is relative to the screen.  If screen is NULL, then the contentRect is 
 * absolute.
 *
 * This code is necessary because of the way newContent:...screen: calls newContent:.
 * The newContent:... moves the window to the main screen, and this function undoes
 * that damage.
 */ 
static void fromScreenCommonCode(Window *self, const NXScreen *screen, const NXRect *cRect)
{
    NXRect desiredFrameRect = self->frame;

    ((WindowPrivate *)(self->_private))->wFlags3.wantsToBeOnMainScreen = NO;
    if (!screen) {
	[[self class]
	    getFrameRect:&desiredFrameRect forContentRect:cRect style:[self style]];
    }
    moveRectToScreen (&desiredFrameRect, screen);
    [self moveTo:NX_X(&desiredFrameRect) :NX_Y(&desiredFrameRect) screen:NULL];
}

- _initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    screen:(const NXScreen *)screen
    contentView:aView
{
    [self _initContent:contentRect style:aStyle backing:bufferingType buttonMask:mask defer:flag contentView:aView];
    fromScreenCommonCode(self, screen, contentRect);
    return self;
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    screen:(const NXScreen *)screen
{
  /* must call this instead of sharing code with above method in case someone overrides newContent:style:backing:buttonMask:defer: in a subclass. */
    self = [self newContent:contentRect style:aStyle backing:bufferingType
		 buttonMask:mask defer:flag];
    fromScreenCommonCode(self, screen, contentRect);
    return self;	
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    return [[self allocFromZone:NXDefaultMallocZone()] initContent:contentRect style:aStyle backing:bufferingType buttonMask:mask defer:flag];
}

/* Creates a window with the given content view.  If a contentRect is passed in then that is used to size the window.  If its NULL, then it uses the frame of the contentView passed in. 
This method will bring the window up on the main screen on a multiple headed system.
*/
- _initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    contentView:aView
{
    static const NXRect defaultRect = {{100., 100.}, {100., 100.}};
    NXRect              inner, outer;
    register NXRect    *irp = &inner;
    register NXRect    *orp = &outer;
    NXZone    *zone = [self zone];

  /* HACK! We used to have a resize button, now we have a bar... */
    if (aStyle == NX_TITLEDSTYLE && (mask & NX_RESIZEBUTTONMASK))
	aStyle = NX_RESIZEBARSTYLE;
    else if (aStyle == NX_RESIZEBARSTYLE)
	mask |= NX_RESIZEBUTTONMASK;

    [super init];
    private = NXZoneCalloc(zone, 1, sizeof(WindowPrivate));
    private->wFlags3.titleIsFilename = NO;
    private->wFlags3.excludedFromWindowsMenu = NO;

    if (contentRect)
	*irp = *contentRect;
    else {
	if (aView)
	    [aView getFrame:irp];
	else
	    *irp = defaultRect;
    }

    if ((aStyle == NX_TITLEDSTYLE) || (aStyle == NX_RESIZEBARSTYLE)) {
	/*
	 * If deferred, we don't actually move the window to the main screen
	 * until we create it.
	 */
	if (!flag) {
	    moveRectToScreen (irp, [NXApp mainScreen]);
	} else {
	    private->wFlags3.wantsToBeOnMainScreen = YES;
	}
    }
	    
    NXIntegralRect(irp);
    [FrameView getFrameRect:orp forContentRect:irp style:aStyle];

    if (flag) {
	windowNum = -1;
    } else {
	_NXWindow((char *)windowPackageName, bufferingType,
		  NX_X(orp), NX_Y(orp),
		  NX_WIDTH(orp), NX_HEIGHT(orp),
		  &windowNum,
		  aStyle == NX_PLAINSTYLE);
    }
    [NXApp setWindowNum:windowNum forWindow:self];
    [NXApp _addWindow:self];

    wFlags.style = aStyle;
    wFlags.buttonMask = mask;
    wFlags.backing = bufferingType;
    frame = *orp;
    backgroundGray = NX_LTGRAY;

    winEventMask = eventMasks[aStyle];
    firstResponder = self;
    wFlags.isPanel = [self isKindOf:[Panel class]];

    NX_X(irp) -= NX_X(orp);
    NX_Y(irp) -= NX_Y(orp);
    NX_X(orp) = NX_Y(orp) = 0.0;

    _borderView = [[FrameView allocFromZone:zone] initFrame:orp style:aStyle buttonMask:mask owner:self];

    [self setContentView:(aView ? aView : [[View allocFromZone:zone] initFrame:irp])];

    if (flag)
	wFlags2.deferred = YES;
    else
	[self _commonWindowInit:YES];
    return self;
}

- initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    screen:(const NXScreen *)screen
{
  /* must call this instead of sharing code with above method in case someone overrides newContent:style:backing:buttonMask:defer: in a subclass. */
    [self initContent:contentRect style:aStyle backing:bufferingType buttonMask:mask defer:flag];
    fromScreenCommonCode(self, screen, contentRect);
    return self;	
}

- initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    return [self _initContent:contentRect style:aStyle backing:bufferingType buttonMask:mask defer:flag contentView:nil];
}

- init
{
    return [self initContent:NULL style:DEFAULTSTYLE backing:DEFAULTBACKING buttonMask:0 defer:NO];
}

+ new
{
    return [self newContent:NULL style:DEFAULTSTYLE backing:DEFAULTBACKING buttonMask:0 defer:NO];
}


+ getFrameRect:(NXRect *)fRect forContentRect:(const NXRect *)cRect style:(int)aStyle
{
    [FrameView getFrameRect:fRect forContentRect:cRect style:aStyle];
    return self;
}


+ getContentRect:(NXRect *)cRect forFrameRect:(const NXRect *)fRect style:(int)aStyle
{
    [FrameView getContentRect:cRect forFrameRect:fRect style:aStyle];
    return self;
}

+ (NXCoord)minFrameWidth:(const char *)aTitle
    forStyle:(int)aStyle buttonMask:(int)aMask
{
    return[FrameView minFrameWidth:aTitle forStyle:aStyle buttonMask:aMask];
}

- _commonWindowInit:(BOOL)displayBorder
{
    int                 gs;
#define BOOLBUTNOTYESORNO 2
    static BOOL forceOneShot = BOOLBUTNOTYESORNO;

    _NXInitGState(windowNum, &gs);
    [_borderView _setBorderViewGState:gs];
    if (!defaultFont)
	defaultFont = [Font newFont:NXSystemFont size:DEFFONTSIZE];
    PSgsave();
    NXSetGState(gs);
    [defaultFont set];		/* in case anyone wants to 'show' without
				 * setting font */
    NXCopyCurrentGState(gs);
    PSgrestore();

    _NXSetEventMask(winEventMask, windowNum);
    if (displayBorder)
	[self displayBorder];	/* let the border draw, but don't let */
    _displayDisabled = 1;	/* anything else draw until [window display] */
    wFlags2._validCursorRects = NO;
    if (forceOneShot == BOOLBUTNOTYESORNO)
	if (NXGetDefaultValue([NXApp appName], "NXAllWindowsOneShot"))
	    forceOneShot = YES;
	else
	    forceOneShot = NO;
    if (forceOneShot)
	wFlags.oneShot = YES;
    return self;
}


- free
{
    if (_trectTable) {
	NXFreeHashTable(_trectTable);
	_trectTable = 0;
    }
    wFlags2._windowDying = YES;
    [NXApp _removeWindow:self];

    if ([self _wsmOwnsWindow])
	windowNum = -1;
    fieldEditor = [fieldEditor freeAndUnlink];
    contentView = [contentView freeAndUnlink];
    _borderView = [_borderView freeAndUnlink];
    if (_cursorRects) {
	free(_cursorRects);
	_cursorRects = 0;
    }
    if (windowNum > 0) {
	_NXTermWindow(windowNum);
	DPSUndefineUserObject(windowNum);
    }
    
    if (counterpart) {
	Window *oldCounterpart;

	oldCounterpart = counterpart;
	((Window *)counterpart)->counterpart = nil;
	counterpart = nil;
	[oldCounterpart free];
    }

    if (private->backgroundColor) {
	NX_FREE(private->backgroundColor);
    }
    NX_FREE(private);
    
    return[super free];
}


- _viewDetaching:aView
{
    id newResp;

    if (firstResponder == aView) {
	newResp = self;
	firstResponder = newResp;	/* set even if old resp refuses */
    }
    if (lastLeftHit == aView)
	lastLeftHit = nil;
    if (lastRightHit == aView)
	lastRightHit = nil;

    return self;
}

- _viewFreeing:aView
{
    if (delegate == aView)
	delegate = nil;
    return [self _viewDetaching:aView];
}

/** Automatic I/O **/

- _commonAwake
{
    BOOL deferredCreate;

    if (windowNum == 0 && wFlags2.deferred) {
	[NXApp _addWindow:self];
	windowNum = -1;
    } else if (windowNum <= 0) {
	deferredCreate = (windowNum < 0);
	if (private->wFlags3.wantsToBeOnMainScreen) {
	    moveRectToScreen(&frame, [NXApp mainScreen]);
	    /*
	     * Once a window does appear on screen, we don't want it to ever
	     * get moved (for instance, whenever a one-shot window gets
	     * recreated.)
	     */
	    private->wFlags3.wantsToBeOnMainScreen = NO;
	}
	_NXWindow((char *)windowPackageName, wFlags.backing,
		  frame.origin.x, frame.origin.y,
		  frame.size.width, frame.size.height,
		  &windowNum,
		  wFlags.style == NX_PLAINSTYLE);
	_NXSetHideOnDeactivate(windowNum, wFlags.hideOnDeactivate, wFlags.visible);
	if ([self hasDynamicDepthLimit]) {
	    [self _adjustDynamicDepthLimit];
	} else if ([self depthLimit] != NX_DefaultDepth) {
	    PSsetwindowdepthlimit ([self depthLimit], windowNum);
	}
	[NXApp _addWindow:self];
	[self _commonWindowInit:NO];
	if (deferredCreate)
	    [self->_borderView _setDragMargins:YES :YES :YES];
    }
    [NXApp setWindowNum:windowNum forWindow:self];

    return self;
}


- awake
{
    [super awake];
    [self _commonAwake];
    [_borderView awake];
    return self;
}

- setExcludedFromWindowsMenu:(BOOL)flag
{
    private->wFlags3.excludedFromWindowsMenu = flag;
    return self;
}

- (BOOL)isExcludedFromWindowsMenu
{
    return(private->wFlags3.excludedFromWindowsMenu);
}

- setTitle:(const char *)aString 

{
    private->wFlags3.titleIsFilename = NO;    
    [self _dosetTitle:aString];
    
    return self;
}

- _dosetTitle:(const char *)aString
{
    const char *title = [_borderView title];

    if (title && aString && !strcmp(title, aString)) return self;

    [_borderView setTitle:aString];
    [self _displayTitle];
    if (wFlags.style != NX_MINIWINDOWSTYLE)
	[counterpart setTitle:aString];
    if (WINDOWSMENUWINDOW(self)) 
	[NXApp changeWindowsItem:self title:aString 
		    filename:private->wFlags3.titleIsFilename];

    return self;
}

/*
 * Convert a string to the form of   lastpartofpath  -- path
 * and set this as the title of a window.
 * If the format changes also make sure appWindows.m deconvertname is updated
 */
- setTitleAsFilename:(const char *)aString
{
    char title[MAXPATHLEN+5];
    char *s;

    s = strrchr(aString, '/');
    if (!s) {
	strcpy(title, aString);
    } else {
	strcpy(title, s+1);
	strcat(title, "  \320  ");	/* \320 is a "long dash" */
	if (s == aString)
	    strcat(title, "/");		/* for files in / */

	else
	    strncat(title, aString, s - aString);
    }    
    private->wFlags3.titleIsFilename = YES;    
    [self _dosetTitle:title];
    return self;
}


/*??? check to make sure that aView does not have any superview */
- setContentView:aView
{
    id                  oldContents;
    NXRect              oRect, iRect;

    oldContents = contentView;
    [oldContents removeFromSuperview];
    contentView = aView;
    [contentView setClipping:YES];
    [_borderView getFrame:&oRect];
    [FrameView getContentRect:&iRect forFrameRect:&oRect style:wFlags.style];
    [contentView setFrame:&iRect];
    [_borderView addSubview:contentView];
    [contentView setNextResponder:self];
    return oldContents;
}


- contentView
{
    return contentView;
}


- setDelegate:anObject
{
    if (wFlags.style != NX_MINIWINDOWSTYLE && wFlags.style != NX_MINIWORLDSTYLE && wFlags.style != NX_TOKENSTYLE) {
	delegate = anObject;
	private->wFlags3.delegateReturnsValidRequestor = [delegate respondsTo:@selector(validRequestorForSendType:andReturnType:)];
    }
    [NXApp setDelegateUpdates:[delegate respondsTo:@selector(windowDidUpdate:)] forWindow:self];
    return self;
}


- delegate
{
    return delegate;
}


- (const char *)title
{
    return[_borderView title];
}


- (int)buttonMask

{
    return (wFlags.buttonMask);
}


- (int)windowNum
{
    return windowNum;
}


- getFieldEditor:(BOOL)createFlag for:anObject
{
    id                  fe;
    
    if (wFlags2._windowDying)
	return nil;
    
    if (delegate != anObject &&
	[delegate respondsTo:@selector(windowWillReturnFieldEditor:toObject:)] &&
	(fe = [delegate windowWillReturnFieldEditor:self toObject:anObject]))
	return fe;
    
    if (createFlag && !fieldEditor) {
	fieldEditor = [[Text allocFromZone:[self zone]] init];
	[fieldEditor setCharFilter:NXFieldFilter];
	[fieldEditor setSelectable:YES];
	[fieldEditor setFontPanelEnabled:NO];
    }
    return fieldEditor;
}


- endEditingFor:anObject
{
    if (fieldEditor) {
	if (firstResponder == fieldEditor)
	/* end text edition */
	/* ??? This is not clean. Let's think of a clever solution wrp 041388 */
	    if (![self makeFirstResponder:self])	/* cmf 032488 */
		firstResponder = self;
	[fieldEditor removeFromSuperview];
	[fieldEditor setDelegate:nil];
	if ([fieldEditor textLength])
	    [fieldEditor setText:""];
    }
    return self;
}


/** Window Geometry **/

- placeWindowAndDisplay:(const NXRect *)frameRect
{
    return [self _resizeWindow:frameRect userAction:NO];
}

- placeWindow:(const NXRect *)frameRect
{
    return [self placeWindow:frameRect screen:NULL];
}

- placeWindow:(const NXRect *)frameRect screen:(const NXScreen *)screen
{
    NXRect              inner, outer;
    register NXRect    *irp = &inner;
    register NXRect    *orp = &outer;

    *orp = *frameRect;

    if (screen) {
	moveRectToScreen (orp, screen);
    }
    if ([self isVisible] && [self _isDocWindow]) {
	[self constrainFrameRect:orp toScreen:NULL];
    }
    NXIntegralRect(orp);
    
    if (NXEqualRect(&frame, orp))
	return self;
	
    frame = *orp;
    if (windowNum > 0) {
	_NXPlaceWindow(NX_X(orp), NX_Y(orp), NX_WIDTH(orp), NX_HEIGHT(orp), windowNum, [self gState], &private->lastResizeTime);
	[_borderView _invalidateFocus];
	[_borderView sizeTo:NX_WIDTH(orp) :NX_HEIGHT(orp)];
	[self displayBorder];
    } else
        [_borderView sizeTo:NX_WIDTH(orp) :NX_HEIGHT(orp)];

    [FrameView getContentRect:irp forFrameRect:orp style:wFlags.style];
    [contentView sizeTo:NX_WIDTH(irp) :NX_HEIGHT(irp)];

    return self;
}


- _resizeWindow:(const NXRect *)frameRect userAction:(BOOL)userFlag
{
    id window;
    BOOL usedCoverWindow;

    if (wFlags.visible && !_NXShowAllWindows && !_NXAllWindowsRetained) {
	usedCoverWindow = YES;
	_NXCoverWindow(windowNum);
    } else
	usedCoverWindow = NO;
    NX_DURING {
	[self disableDisplay];
	[self placeWindow:frameRect];
	if (userFlag) {
	    if ([delegate respondsTo:@selector(windowDidResize:)]) {
		[delegate windowDidResize:self];
	    } else if ([self respondsTo:@selector(windowDidResize:)]) {
		window = self;
		[window windowDidResize:self];
	    }
	}
	[self reenableDisplay];
	[_borderView display];
	if (usedCoverWindow) {
	    usedCoverWindow = NO;
	    _NXUncoverWindow(windowNum);
	}
	[self invalidateCursorRectsForView:contentView];
    } NX_HANDLER {
	if (usedCoverWindow)
	    _NXUncoverWindow(windowNum);
	NX_RERAISE();
    } NX_ENDHANDLER

    return self;
}


- sizeWindow:(NXCoord)width :(NXCoord)height
{
    NXRect              iRect, oRect;
    NXRect             *irp = &iRect;
    NXRect             *orp = &oRect;

    NX_WIDTH(irp) = width;
    NX_HEIGHT(irp) = height;
    [FrameView getFrameRect:orp forContentRect:irp style:wFlags.style];
    orp->origin = frame.origin;

    return ([self placeWindow:orp]);
}

/*
 * If screen is NULL, move the window to the absolute location indicated by
 * x, y. If screen is not NULL, move the window to location x, y on the
 * specified screen. Tries to make sure window appears on the specified
 * screen even if the screen is larger than x, y.
 */
- moveTo:(NXCoord)x :(NXCoord)y screen:(const NXScreen *)screen
{
    NXRect newFrame = {{x, y}, frame.size};

    if (screen) {
	moveRectToScreen (&newFrame, screen);
    }
    if ([self isVisible] && [self _isDocWindow]) {
	[self constrainFrameRect:&newFrame toScreen:NULL];
    }

    if (windowNum > 0)
	_NXMoveWindow(NX_X(&newFrame), NX_Y(&newFrame), windowNum);
    frame.origin = newFrame.origin; /* wot: assuming no windowmoved
				     * event is generated */
    return self;
}

- moveTo:(NXCoord)x :(NXCoord)y
{
    return [self moveTo:x :y screen:NULL];
}

- moveTopLeftTo:(NXCoord)x :(NXCoord)y
{
    return [self moveTo:x :(y - frame.size.height)];
}

- moveTopLeftTo:(NXCoord)x :(NXCoord)y screen:(const NXScreen *)screen
{
    return [self moveTo:x :(y - frame.size.height) screen:screen];
}



- getFrame:(NXRect *)theRect
{
    *theRect = frame;
    return self;
}

- getFrame:(NXRect *)rect andScreen:(const NXScreen **)screen
{
    const NXScreen *tmpScreen = [self screen];
    *rect = frame;
    if (tmpScreen) {
	NX_X(rect) -= NX_X(&(tmpScreen->screenBounds));
	NX_Y(rect) -= NX_Y(&(tmpScreen->screenBounds));
    }
    if (screen) {
	*screen = tmpScreen;
    }
    return self;
}

- getMouseLocation:(NXPoint *)thePoint
{
    if (windowNum > 0)
	PScurrentmouse(windowNum, &thePoint->x, &thePoint->y);
    return self;
}

- (BOOL)_isDocWindow
{
    return ((wFlags.style == NX_TITLEDSTYLE) ||
	    (wFlags.style == NX_RESIZEBARSTYLE));
}

- (int)style
{
    return wFlags.style;
}


/* called when we get an error at the top level */
- _resetDisableCounts
{
    _flushDisabled = 0;
    _displayDisabled = 0;
    return nil;		/* so we keep going and do all the windows */
}


- disableFlushWindow
{
    _flushDisabled++;
    return self;
}


- reenableFlushWindow
{
    if (--_flushDisabled < 0)
	_flushDisabled = 0;
    return self;
}



- flushWindow
{
    if (windowNum < 0 || NXDrawingStatus != NX_DRAWING || wFlags.backing != NX_BUFFERED)
	return self;
	
    wFlags2._needsFlush = _flushDisabled ? YES : NO;
    
    if (!_flushDisabled) {
	if ([[NXApp focusView] window] == self)
	    PSflushgraphics();
	else
	    _NXFlushGraphics([self gState]);
    }
    return self;
}


- flushWindowIfNeeded
{
    if (wFlags2._needsFlush)
	[self flushWindow];
    return self;
}


- disableDisplay
{
    _displayDisabled++;
    return self;
}


- reenableDisplay
{
    if (--_displayDisabled < 0)
	_displayDisabled = 0;
    return self;
}


- (BOOL)isDisplayEnabled
{
    return (_displayDisabled ? NO : YES);
}


- displayIfNeeded
{
    [contentView displayIfNeeded];
    return self;
}


- (BOOL)_canOptimizeDrawing
{
    return private->wFlags3.optimizeDrawing && 
    	   private->wFlags3.optimizedDrawingOk;
}


- useOptimizedDrawing:(BOOL)flag
{
    private->wFlags3.optimizeDrawing = flag ? YES : NO;
    return self;
}


- display
{
    _displayDisabled = 0;
    private->wFlags3.optimizedDrawingOk = YES;
    [_borderView display];
    private->wFlags3.optimizedDrawingOk = NO;
    return (self);
}


- update
{
    if ([delegate respondsTo:@selector(windowDidUpdate:)]) {
	[delegate windowDidUpdate:self];
    }
    return self;
}


- _borderView
{
    return _borderView;
}



/** Event Processing **/

- (int)setEventMask:(int)newMask
{
    int                 oldMask;

    oldMask = winEventMask;
    if (newMask != winEventMask) {
	if (newMask & NX_KEYDOWNMASK)
	    newMask |= NX_KITDEFINEDMASK;
	NX_ASSERT(!((newMask ^ winEventMask) & NX_KEYDOWNMASK) ||
		  !wFlags.visible,
	    "setting NX_KEYDOWN mask while window is visible is illegal");
	[[NXApp slaveJournaler] _adjustNewEventMask:&newMask for:self];
	winEventMask = newMask;
	if (wFlags.isKeyWindow && !wFlags2._cursorRectsDisabled)
	    newMask |= CURSOR_RECT_MASK;
	if (windowNum > 0)
	    _NXSetEventMask(newMask, windowNum);
    }
    return oldMask;
}


- (int)addToEventMask:(int)newEvents
{
    return ([self setEventMask:(winEventMask | newEvents)]);
}


- (int)removeFromEventMask:(int)oldEvents
{
    return ([self setEventMask:(winEventMask & ~oldEvents)]);
}


- (int)eventMask
{
    return (winEventMask);
}

/* ??? It would be better if this caused an error when the same tracking
   number were reused, but that would be incompatible.  In fact, the tracking
   number shouldnt even be exported at all, but just used by the kit.
 */
- setTrackingRect:(const NXRect *)aRect inside:(BOOL)insideFlag owner:anObject
    tag:(int)trackNum left:(BOOL)leftDown right:(BOOL)rightDown
{
    TrackInfo temp, *old, *new;
    NXZone *zone = [self zone];

    if (!_trectTable)
	_trectTable = NXCreateHashTableFromZone(trackInfoProto, 0, 0, zone);
    temp.num = trackNum;
    old = NXHashGet(_trectTable, &temp);
    if (!NXHashGet(_trectTable, &temp)) {
	new = NXZoneMalloc(zone, sizeof(TrackInfo));
	new->num = trackNum;
	new->owner = anObject;
	NXHashInsert(_trectTable, new);
    } else
    	old->owner = anObject;		/* replace existing one */

    NX_ASSERT([anObject respondsTo:@selector(mouseEntered:)] &&
		[anObject respondsTo:@selector(mouseExited:)],
		"Window: target of tracking rect doesnt understand -mouseEntered: or -mouseExited messages");

    PSsettrackingrect(aRect->origin.x, aRect->origin.y,
			aRect->size.width, aRect->size.height,
			leftDown, rightDown, insideFlag, trackNum,
			trackNum,[self gState]);
    return self;
}


- discardTrackingRect:(int)trackNum
{
    TrackInfo temp, *old;
    static const char errMsg[] = "Window: discardTrackingRect called before adding rect";

    if (!_trectTable) {
	NX_ASSERT(_trectTable, errMsg);
    } else {
	temp.num = trackNum;
	old = NXHashRemove(_trectTable, &temp);
	if (old) {
	    NX_ASSERT(old, errMsg);
	    free(old);
	}
    }
    PScleartrackingrect(trackNum, [self gState]);
    return self;
}


- makeFirstResponder:aResponder
{
    if (firstResponder != aResponder) {
	if (![firstResponder resignFirstResponder])
	    return (NULL);
	firstResponder = aResponder;
	if (![firstResponder becomeFirstResponder])
	    firstResponder = self;
    }
    return self;
}


- firstResponder
{
    return firstResponder;
}


- sendEvent:(NXEvent *)theEvent
{
    extern void _DPSPrintEvent(FILE *fp, NXEvent *evTo); 
  /* the time of the last event we did PostEvent on.  Used to prevent infinite loops of posting events, ignoring first mouse, and posting some more. */
    static long lastRepostTime = 0; 

    if (_NXTraceEvents) {
	fprintf(stderr, "In Window: ");
	_DPSPrintEvent(stderr, theEvent);
    }
    switch (theEvent->type) {
    case NX_LMOUSEDOWN:
      /* Do not process an event that snuck though before we hid window */
	if (wFlags2._tempHidden || !wFlags.visible || (private->lastResizeTime && theEvent->time < private->lastResizeTime)) {
	    private->wFlags3.lmouseupPending = NO;
	    break;
	}

      /* cons up a fake mouse click if we previously ignored one (because it
         initiated an activation), but then later we got a double click.
       */
	if (theEvent->time > lastRepostTime && wFlags2._ignoredFirstMouse && theEvent->data.mouse.click == 2) {
	    lastRepostTime = theEvent->time;
	    DPSPostEvent(theEvent, YES);
	    theEvent->data.mouse.click = 1;
	    theEvent->type = NX_MOUSEUP;
	    DPSPostEvent(theEvent, YES);
	    theEvent->type = NX_MOUSEDOWN;
	    DPSPostEvent(theEvent, YES);
	    wFlags2._ignoredFirstMouse = NO;
	    return self;
	}
	private->wFlags3.lmouseupPending = YES;
	wFlags2._ignoredFirstMouse = NO;
	if (!lastLeftHit || theEvent->data.mouse.click < 2) {
	    lastLeftHit = [_borderView hitTest:&theEvent->location];
	}
      /* if this was the first, activating click in this window or its a click 
	 in a non key window (thats not in a window button)
       */
	if ([NXApp _isInvalid] ||
		(!wFlags.isKeyWindow &&
		 !(lastLeftHit &&
		   (lastLeftHit == [_borderView getCloseButton] ||
		    lastLeftHit == [_borderView getIconifyButton])))) {
	    BOOL                wasKeyWindow = wFlags.isKeyWindow;

	  /* if this is part of an activation sequence */
	    if ([NXApp _isInvalid]) {
		if (self != [NXApp _keyWindow]) {
		    [self disableDisplay];
		    changeKeyAndMain(self, [lastLeftHit acceptsFirstResponder]);
		    [self reenableDisplay];
		}
		_NXShowKeyAndMain();
		[NXApp _setInvalid:NO];
	    } else
		changeKeyAndMain(self, [lastLeftHit acceptsFirstResponder]);
	  /* if we're making a new key window and the hit view doesnt
	     take the first mouse, pitch the event.  We dont pitch the event
	     is we didnt make it to key window status (otherwised we could
	     get into an infinite loop when the pitched event is reposted
	     at the top of this method.
	   */
	    if ([self canBecomeKeyWindow] && !wasKeyWindow && ![lastLeftHit acceptsFirstMouse] && wFlags.isKeyWindow) {
		lastLeftHit = nil;
		wFlags2._ignoredFirstMouse = YES;
	    }
	}
	if (lastLeftHit != firstResponder)
	    if ([lastLeftHit acceptsFirstResponder])
		if (![self makeFirstResponder:lastLeftHit])
		    lastLeftHit = firstResponder;
	if (validateMouseEvent(self, theEvent))
	    [lastLeftHit mouseDown:theEvent];
	break;

    case NX_LMOUSEUP:
      /* Do not process an event that snuck though before we hid window */
	if ((!wFlags2._tempHidden && wFlags.visible) || private->wFlags3.lmouseupPending)
	    if (validateMouseEvent(self, theEvent))
		[lastLeftHit mouseUp:theEvent];
	private->wFlags3.lmouseupPending = NO;
	break;

    case NX_RMOUSEDOWN:
      /* Do not process an event that snuck though before we hid window */
	if (wFlags2._tempHidden || !wFlags.visible) {
	    private->wFlags3.rmouseupPending = NO;
	    break;
	}
	private->wFlags3.rmouseupPending = YES;

	if ([self isKeyWindow]) {
	    lastRightHit = [_borderView hitTest:&theEvent->location];
	    if (lastRightHit) {
		[lastRightHit rightMouseDown:theEvent];
	    } else {
		[NXApp rightMouseDown:theEvent];
	    }
	} else {
	    [NXApp rightMouseDown:theEvent];
	}
	break;

    case NX_RMOUSEUP:
      /* Do not process an event that snuck though before we hid window */
	if ((!wFlags2._tempHidden && wFlags.visible) || private->wFlags3.rmouseupPending)
	    [lastRightHit rightMouseUp:theEvent];
	private->wFlags3.rmouseupPending = NO;
	break;

    case NX_MOUSEMOVED:
	[firstResponder mouseMoved:theEvent];
	break;

    case NX_LMOUSEDRAGGED:
      /* Do not process an event that snuck though before we hid window */
	if ((!wFlags2._tempHidden && wFlags.visible) || private->wFlags3.lmouseupPending)
	    if (validateMouseEvent(self, theEvent))
		[lastLeftHit mouseDragged:theEvent];
	break;

    case NX_RMOUSEDRAGGED:
      /* Do not process an event that snuck though before we hid window */
	if ((!wFlags2._tempHidden && wFlags.visible) || private->wFlags3.rmouseupPending)
	    [lastRightHit rightMouseDragged:theEvent];
	break;

    case NX_MOUSEENTERED:
    case NX_MOUSEEXITED:
	if (_trectTable) {
	    TrackInfo temp, *tinfo;

	    temp.num = theEvent->data.tracking.trackingNum;
	    tinfo = NXHashGet(_trectTable, &temp);
	    if (tinfo)
		if (theEvent->type == NX_MOUSEENTERED)
		    [tinfo->owner mouseEntered:theEvent];
		else
		    [tinfo->owner mouseExited:theEvent];
	}
	break;


    case NX_KEYDOWN:
	[firstResponder keyDown:theEvent];
	break;

    case NX_KEYUP:
	[firstResponder keyUp:theEvent];
	break;

    case NX_FLAGSCHANGED:
	[firstResponder flagsChanged:theEvent];
	break;

    default:
	NXLogError("Unrecognized event in [Window -sendEvent:, type = %d",
			theEvent->type);
	break;
    }
    return self;
}


- _lastLeftHit
{
    return lastLeftHit;
}


- _lastRightHit
{
    return lastRightHit;
}


- windowExposed:(NXEvent *)theEvent
{
    NXRect              aRect;

    aRect.origin = theEvent->location;
    aRect.size.width = (float)theEvent->data.compound.misc.L[0];
    aRect.size.height = (float)theEvent->data.compound.misc.L[1];
    [_borderView display:&aRect:1];
    if ([delegate respondsTo:@selector(windowDidExpose:)])
	[delegate windowDidExpose:self];
    return self;
}

/*
 * This method should be called after the windowMoved: message comes through.
 * (The packages will try to make sure this is the case.)
 * Otherwise the window's real location and frame aren't the same.
 */ 
- screenChanged:(NXEvent *)theEvent
{
    if ([delegate respondsTo:@selector(windowDidChangeScreen:)]) {
	[delegate windowDidChangeScreen:self];
    }
    if ([self hasDynamicDepthLimit]) {
	[self _adjustDynamicDepthLimit];
    }
    return self;
}

/*
 * Because a window may move and the event posted when the app is real busy
 * doing other things (including moving the window!) this method has to check
 * the current bounds of the window and assign that to frame rather than 
 * trust the value in the event...
 */ 
- windowMoved:(NXEvent *)theEvent
{
    NXRect actualFrame;
    if (windowNum > 0) {
	PScurrentwindowbounds (windowNum,
			&NX_X(&actualFrame), &NX_Y(&actualFrame), 
			&NX_WIDTH(&actualFrame), &NX_HEIGHT(&actualFrame));
    } else {
	NX_X(&actualFrame) = theEvent->location.x;
	NX_Y(&actualFrame) = theEvent->location.y;
    }

    if (NX_X(&frame) != NX_X(&actualFrame) || 
	NX_Y(&frame) != NX_Y(&actualFrame)) {
	frame.origin = actualFrame.origin;
	if ([delegate respondsTo:@selector(windowDidMove:)])
	    [delegate windowDidMove:self];
    }
    return self;
}


/* ??? need to get rid of this method, as it will never be used with
present resizing behavior */
- windowResized:(NXEvent *)theEvent
{
    id			window;
    NXRect              oRect;
    NXRect              iRect;

    frame.origin = theEvent->location;
    frame.size.width = (float)theEvent->data.compound.misc.L[0];
    frame.size.height = (float)theEvent->data.compound.misc.L[1];
    oRect.origin.x = oRect.origin.y = 0.0;
    oRect.size = frame.size;
    [FrameView getContentRect:&iRect forFrameRect:&oRect style:wFlags.style];
    [_borderView sizeTo:oRect.size.width:oRect.size.height];
    [_borderView _invalidateFocus];	/* ??? is this redundant? */
    [contentView sizeTo:iRect.size.width:iRect.size.height];
    if ([delegate respondsTo:@selector(windowDidResize:)]) {
	[delegate windowDidResize:self];
    } else if ([self respondsTo:@selector(windowDidResize:)]) {
	window = self;
	[window windowDidResize:self];
    }
    [_borderView display];
    [self invalidateCursorRectsForView:contentView];
    return self;
}


- makeKeyWindow
{
    if (![NXApp _isInvalid])
	changeKeyAndMain(self, YES);
    return self;
}


- becomeKeyWindow
{
    wFlags.isKeyWindow = YES;
    if (firstResponder &&
	firstResponder != self &&
	[firstResponder respondsTo:@selector(becomeKeyWindow)])
	[firstResponder becomeKeyWindow];
    if ([delegate respondsTo:@selector(windowDidBecomeKey:)])
	[delegate windowDidBecomeKey:self];
    if (windowNum > 0)
	_NXSetEventMask(winEventMask | CURSOR_RECT_MASK, windowNum);
    if (!wFlags2._cursorRectsDisabled) {
	_NXResetCursorState(windowNum, otherEvents, YES);
	if (wFlags2._validCursorRects)
	    changeCursorRects(self, YES);
    }
    return self;
}


- resignKeyWindow
{
    wFlags.isKeyWindow = NO;
    if (firstResponder &&
	firstResponder != self &&
	[firstResponder respondsTo:@selector(resignKeyWindow)])
	[firstResponder resignKeyWindow];
    if ([delegate respondsTo:@selector(windowDidResignKey:)])
	[delegate windowDidResignKey:self];
    if (windowNum > 0)
	_NXSetEventMask(winEventMask, windowNum);
    changeCursorRects(self, NO);
    return self;
}


- becomeMainWindow
{
    wFlags.isMainWindow = YES;
    if ([delegate respondsTo:@selector(windowDidBecomeMain:)])
	[delegate windowDidBecomeMain:self];
    return self;
}


- resignMainWindow
{
    wFlags.isMainWindow = NO;
    if ([delegate respondsTo:@selector(windowDidResignMain:)])
	[delegate windowDidResignMain:self];
    return self;
}



- _displayTitle
{
    if ([_borderView canDraw] && ![self _wsmOwnsWindow]) {
	[_borderView lockFocus];
	[_borderView _displayTitle];
	[self flushWindow];
	[_borderView unlockFocus];
    }
    return self;
}


- displayBorder
{
    if ([_borderView canDraw] && ![self _wsmOwnsWindow]) {
	[_borderView lockFocus];
	[_borderView displayBorder];
	[self flushWindow];
	[_borderView unlockFocus];
    }
    return self;
}


- rightMouseDown:(NXEvent *)theEvent
{
    return [NXApp rightMouseDown:theEvent];
}


- (BOOL)commandKey:(NXEvent *)theEvent
{
    return NO;
}


- close
{
    wFlags2._windowDying = (!wFlags.dontFreeWhenClosed);
    [self orderOut:self];
    if (wFlags2._windowDying) {
	[NXApp _removeWindow:self];
	[NXApp delayedFree:self];
    }
    return nil;
}


- setFreeWhenClosed:(BOOL)flag
{
    wFlags.dontFreeWhenClosed = flag ? 0 : 1;
    return self;
}

- _makeMiniView
{
    id miniView = nil; 
    id miniCell;
    
    if (_miniIcon) {
	if (_NXFindIconNamed(_miniIcon)) {
	    miniView = [[Control allocFromZone:[self zone]] init];
	    miniCell = [[Cell allocFromZone:[self zone]] initIconCell:_miniIcon];
	    [miniView setCell:miniCell];
	} else 
	    _miniIcon = NULL;
    }
    [[counterpart setContentView:miniView] free];
    return self;
}

- miniaturize:sender
{
    NXRect              iconFrame, cRect;

    if (windowNum < 0)
	return self;
    if (!counterpart) {
	_NXGetIconFrame(&iconFrame);
	[Window getContentRect:&cRect forFrameRect:&iconFrame style:NX_MINIWINDOWSTYLE];
	counterpart = [[Window allocFromZone:[self zone]] initContent:&cRect style:NX_MINIWINDOWSTYLE backing:NX_RETAINED buttonMask:0 defer:NO];
	[counterpart _setCounterpart:self];
	[counterpart setTitle:[self title]];
	[[counterpart setContentView:nil] free];
	_NXSetIcon([counterpart windowNum]);
    }
    if (wFlags2._newMiniIcon) {
	wFlags2._newMiniIcon = NO;
	[self _makeMiniView];
    }
    if ((delegate) &&
	[delegate respondsTo:@selector(windowWillMiniaturize:toMiniwindow:)])
	[delegate windowWillMiniaturize:self toMiniwindow:counterpart];
    [counterpart display];
    [counterpart orderFront:sender];
    if ((delegate) &&
	[delegate respondsTo:@selector(windowDidMiniaturize:)])
	[delegate windowDidMiniaturize:self];
    return self;
}


- deminiaturize:sender
{
    [counterpart makeKeyAndOrderFront:sender];
    return self;
}


- (BOOL)tryToPerform:(SEL)anAction with:anObject
{
    if ([super tryToPerform:anAction with:anObject])
	return YES;
    if ([delegate respondsTo:anAction])
	if ([delegate perform:anAction with:anObject])
	    return YES;
    return NO;
}


- validRequestorForSendType:(NXAtom)sendType andReturnType:(NXAtom)returnType
{
    id retval;

    if (delegate != self && ((![delegate isKindOf:[Responder class]] || ![delegate nextResponder]) && private->wFlags3.delegateReturnsValidRequestor)) {
	retval = [delegate validRequestorForSendType:sendType andReturnType:returnType];
	if (retval) return retval;
    }

    return [NXApp validRequestorForSendType:sendType andReturnType:returnType];
}


- _setCounterpart:theCounterpart
{
    counterpart = theCounterpart;
    return self;
}


- _getCounterpart
{
    return counterpart;
}


- setBackgroundGray:(float)value
{
    backgroundGray = MAX(MIN(value,1.0),0.0);
    return self;
}


- (float)backgroundGray
{
    return backgroundGray;
}

- setBackgroundColor:(NXColor)color
{
    NXZone *zone = [self zone];
    
    if (!(private->backgroundColor)) {
	private->backgroundColor = NXZoneMalloc(zone, sizeof(NXColor));
    }
    *private->backgroundColor = color;
    return self;
}

- (NXColor)backgroundColor
{
    return (private->backgroundColor ?
		*private->backgroundColor : 
		NXConvertGrayToColor(backgroundGray));
}

- (BOOL)_colorSpecified
{
    return (private->backgroundColor != NULL) ? YES : NO;
}

- _sendColor
{
    if (private->backgroundColor && [self canStoreColor]) {
	NXSetColor(*private->backgroundColor);
    } else {
	PSsetgray(backgroundGray);
    }	
    return self;
}

- dragFrom:(float)x :(float)y eventNum:(int)num
{
    float               newX, newY;

    if (windowNum > 0)
	_NXDoDragWindow(x, y, num, windowNum, &newX, &newY);
    return self;
}


- setHideOnDeactivate:(BOOL)flag
{
    wFlags.hideOnDeactivate = flag;
    if (windowNum > 0)
	_NXSetHideOnDeactivate(windowNum, flag, wFlags.visible);
    return self;
}


- (BOOL)doesHideOnDeactivate
{
    return wFlags.hideOnDeactivate;
}

- setDynamicDepthLimit:(BOOL)flag
{
    wFlags2.dynamicDepthLimit = flag;
    if (flag) {
	[self _adjustDynamicDepthLimit];
    } else {
	[self setDepthLimit:NX_DefaultDepth];
    }
    return self;
}

- (BOOL)hasDynamicDepthLimit
{
    return wFlags2.dynamicDepthLimit ? YES : NO;
}

/*
 * Use the following array to map from depthLimit in wFlags3 to
 * actual depth limit values.  The depthLimit and setDepthLimit: methods
 * should be used to get/set depth limits.
 */
static const NXWindowDepthType depthLimitValues[] = {
    NX_DefaultDepth, NX_TwoBitGrayDepth, NX_EightBitGrayDepth,
    NX_TwelveBitRGBDepth, NX_TwentyFourBitRGBDepth, -1
};

+ (NXWindowDepthType)defaultDepthLimit
{
    int limit;
    if (defaultDepthLimit == 0) {
	PScurrentdefaultdepthlimit (&limit);
	defaultDepthLimit = limit;
    }
    return defaultDepthLimit;
}

- setDepthLimit:(NXWindowDepthType)limit
{
    int cnt = 0;
    while (depthLimitValues[cnt] != -1) {
	if (depthLimitValues[cnt] == limit) {
	    private->wFlags3.depthLimit = cnt;
	    if (windowNum > 0) {
		PSsetwindowdepthlimit (limit, windowNum);
		[_borderView display];
	    }
	    return self;
	} else {
	    cnt++;
	}
    }
    return self;
}

- (NXWindowDepthType)depthLimit
{
    return depthLimitValues[private->wFlags3.depthLimit];
}

/*
 * Call this method when the window is created or changes screens,
 * only if dynamic depth limit is YES.
 * This method will fix up the depth limit of the window so that it 
 * follows the depth limit of the screen it is on.
 */
- _adjustDynamicDepthLimit
{
    const NXScreen *screen;
    NXWindowDepth newDepth;
    if ((screen = [self bestScreen]) || (screen = [NXApp colorScreen])) {
	newDepth = MIN(screen->depth, [Window defaultDepthLimit]);
	if (newDepth != [self depthLimit]) {
	    [self setDepthLimit:newDepth];
	}
    }
    return self;
}

- center
{
    const NXRect *screenRect = &([NXApp mainScreen]->screenBounds);
    NXCoord screenThird;
    NXPoint		newLoc;
#define GAP	10.0

    screenThird = NX_HEIGHT(screenRect) / 3.0;
    newLoc.x = floor((NX_WIDTH(screenRect) - frame.size.width) / 2.0);
    newLoc.y = floor((screenThird * 2.0 - frame.size.height) / 2.0 +
		screenThird);
  /* force title plus gap to be on screen */
    if (newLoc.y > NX_HEIGHT(screenRect) - NX_HEIGHT(&frame) - GAP)
	newLoc.y = NX_HEIGHT(screenRect) - NX_HEIGHT(&frame) - GAP;
  /* sacrifice gap if bottom is off screen */
    if (newLoc.y < 0.0)
	newLoc.y += MIN(GAP, -newLoc.y);
    [self moveTo:newLoc.x + NX_X(screenRect):newLoc.y + NX_Y(screenRect)];
    return self;
}

- makeKeyAndOrderFront:sender
{
    [self orderWindow:NX_ABOVE relativeTo:0];
    [self makeKeyWindow];
    return self;
}


- orderFront:sender
{
    [self orderWindow:NX_ABOVE relativeTo:0];
    return self;
}

- orderBack:sender

{
    [self orderWindow:NX_BELOW relativeTo:0];
    return self;
}


- orderOut:sender
{
    [self orderWindow:NX_OUT relativeTo:0];
    return self;
}

- _justOrderOut
{
  /* same as order out, but doesnt recalc key window or affect windows menu */
    return [self _doOrderWindow:NX_OUT relativeTo:0 findKey:NO level:MININT forcounter:NO];
}


- orderWindow:(int)place relativeTo:(int)otherWin
{
    return [self _doOrderWindow:place relativeTo:otherWin findKey:YES level:MININT forcounter:NO];
}

/*
 * Same as below, but will not be called recursively in the case changing the state
 * of the counterpart window.
 */
- _doOrderWindow:(int)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc level:(int)level
{
    return([self _doOrderWindow:place relativeTo:otherWin findKey:doKeyCalc level:level forcounter:NO]);
}

/* 
 * The workhorse routine that everything else eventually calls to change
 * a window orders.
 */
- _doOrderWindow:(int)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc level:(int)level forcounter:(BOOL)aCounter
{
    BOOL deferredCreation = (windowNum <= 0 && place != NX_OUT);
  /*
   * This method performs ordering.  In the orderOut case, it will find a new
   * key window conditionally, based on the doKey flag.  If level != MININT,
   * it will set the window level to level before ordering it.
   */	
    if ([self _wsmOwnsWindow])
	return self;
    if (place != NX_OUT && !wFlags.visible)
	[self update];
    if (deferredCreation)
	[self _commonAwake];

    if ((place != NX_OUT) && [self _isDocWindow]) {
	NXRect newFrame = frame;
	if ([self constrainFrameRect:&newFrame toScreen:NULL]) {
	    [self moveTo:NX_X(&newFrame) :NX_Y(&newFrame)];
	}
    }
    if (level != MININT)
	[self _setWindowLevel:level];
    if (place != NX_OUT) 
	[counterpart _doOrderWindow:NX_OUT relativeTo:0 findKey:YES level:MININT forcounter:YES];
    if (deferredCreation)
	[self display];
    if (windowNum > 0) {
	if (wFlags.hideOnDeactivate)
	    _NXOrderPanelIfActive(place, otherWin, windowNum);
	else {
	  /* Avoid the situation of having a half hidden app. */
	    if ((place == NX_ABOVE || place == NX_BELOW) && [NXApp isHidden] && wFlags.style != NX_MINIWORLDSTYLE && wFlags.style != NX_TOKENSTYLE)
		[NXApp unhideWithoutActivation:self];
	    if (place == NX_ABOVE && otherWin == 0 && !wFlags.isPanel) {
		if ([NXApp _isDoingOpenFile])
		    _NXSafeOrderFrontDuringOpenFile(windowNum);
		else
		    _NXSafeOrderFront(windowNum);
	    } else
		PSorderwindow(place, otherWin, windowNum);
	}
    }

    if (place == NX_OUT && doKeyCalc) {
	id                  newKeyWindow;
	id                  newMainWindow;
	id		    currKeyWindow = [NXApp _keyWindow];
	BOOL                wasKeyWindow = (currKeyWindow == self);
	BOOL                wasMainWindow = ([NXApp _mainWindow] == self);
	BOOL		    isActive = [NXApp isActive];
	BOOL		    usingOldMainWindow = NO;

	if (!aCounter) [NXApp removeWindowsItem:self]; /* Remove From Windows menu */

	if (wFlags.oneShot)
 	    [self _destroyRealWindow:YES];
	if (wasMainWindow) {
	    [NXApp _setMainWindow:nil];
	    _NXSendWindowNotification(self, @selector(resignMainWindow));
	}
	if (wasKeyWindow) {
	    [NXApp _setKeyWindow:nil];
	    _NXSendWindowNotification(self, @selector(resignKeyWindow));
	}
	if (isActive && (wasMainWindow || wasKeyWindow) && !wFlags2._windowDying)
	    [self _displayTitle];

      /* If the old window was the key window, choose a new one:
	1) If we're running modal, we take the top one on the modal list.
	2) If we just closed a window that could become main, we look for a non-panel to become key.
	3) If we just closed a panel (or if #1 failed) we look for any window that can become key.
	 If the old window was the main window, then we find a new one of those out of the windows available to become main.
       */
	if (wasKeyWindow && ![NXApp _isInvalid]) {
	    newKeyWindow = nil;
	    if ([NXApp _isRunningModal] && _NXLastModalSession) {
		if (_NXLastModalSession->window != self)
		    newKeyWindow = _NXLastModalSession->window;
		else if (_NXLastModalSession->prevSession)
		    newKeyWindow = _NXLastModalSession->prevSession->window;
	    }
	    if (!newKeyWindow && wasMainWindow)
		newKeyWindow = [NXApp makeWindowsPerform: @selector(canBecomeMainWindow) inOrder:YES];

	    if (!newKeyWindow)
		newKeyWindow = [NXApp makeWindowsPerform: @selector(_visibleAndCanBecomeKey) inOrder:YES];
	    usingOldMainWindow = (newKeyWindow == [NXApp _mainWindow]);

	    if (newKeyWindow) {
		changeKeyAndMain(newKeyWindow, NO);
		if (isActive && !usingOldMainWindow) {
		    _NXSafeOrderFront([newKeyWindow windowNum]);
		    NXPing();
		}
	    } else if (isActive)
		[NXArrow set];
	} else if (wasMainWindow && ![NXApp _isInvalid]) {
	    newMainWindow = [NXApp makeWindowsPerform:
				@selector(canBecomeMainWindow) inOrder:YES];
	    if (newMainWindow) {
		[NXApp _setMainWindow:newMainWindow];
		if (isActive) {
		    _NXSendWindowNotification(newMainWindow, @selector(becomeMainWindow));
		    if (![NXApp _isDeactPending])
			[newMainWindow _displayTitle];
		    if (currKeyWindow)
			[newMainWindow orderWindow:NX_BELOW
					relativeTo:[currKeyWindow windowNum]];
		    else
			_NXSafeOrderFront([newMainWindow windowNum]);
		    NXPing();
		}
	    }
	}
	[NXApp _removeHiddenWindow:windowNum];
    }
    wFlags.visible = (place != NX_OUT);
    [NXApp setVisible:wFlags.visible forWindow:self];
    if(place != NX_OUT && WINDOWSMENUWINDOW(self)) /* add to Windows menu */
	    [NXApp addWindowsItem:self title:[self title]
		    filename:private->wFlags3.titleIsFilename];
    return self;
}

- (BOOL)_wantsToDestroyRealWindow
{
    return private->wFlags3.wantsToDestroyRealWindow = NO;
}

- (BOOL)_isInFocusStack
{
    int i;
    focusStackElem *fsp;
    id focusStack = [NXApp _focusStack];

    for (i = [focusStack count]-1; i >= 0; i--) {
	fsp = (focusStackElem *)[focusStack elementAt:i];
	if ([fsp->view window] == self) return YES;
    }

    return NO;
}

- _destroyRealWindow:(BOOL)orderingOut
{
    if (windowNum > 0 && (orderingOut || ![self isVisible]) && ![self _isInFocusStack]) {
	_NXTermWindow(windowNum);
	windowNum = -1;
	[NXApp setWindowNum:windowNum forWindow:self];
	[_borderView _invalidateGStates];
	[_borderView freeGState];
	private->wFlags3.wantsToDestroyRealWindow = NO;
    } else {
	private->wFlags3.wantsToDestroyRealWindow = YES;
    }
    return self;
}

void _NXCreateRealWindow(Window *self)
{
    [self _commonAwake];
    [self display];
}


 /*
  * 8/22/89 wrp:  This method (_setWindowLevel:) must remain for backward
  * compatibility with FrameMaker. They discovered this feature and based
  * their user-interface on it. They are using this for their 1.0 release. We
  * should consider making this a public method in a future release. 
  */

- _setWindowLevel:(int)newLevel
{
    if (windowNum > 0)
	PSsetwindowlevel(newLevel, windowNum);
    return self;
}


- _tempHide:(BOOL)hideIt relWin:(int)otherWin
{
    if (hideIt) {
	if (wFlags.visible) {
	    [self _justOrderOut];
	    wFlags2._tempHidden = 1;
	}
    } else {
	if (wFlags2._tempHidden) {
	    if (otherWin)
		[self orderWindow:NX_BELOW relativeTo:otherWin];
	    else
		[self orderWindow:NX_ABOVE relativeTo:0];
	    wFlags2._tempHidden = 0;
	}
    }
    return self;
}


- setMiniwindowIcon:(const char *)aString
{
    const char *oldType;
    
    oldType = _miniIcon;
    if (aString  && *aString)
	_miniIcon = NXUniqueString(aString);
    else 
	_miniIcon = NULL;
    if (!wFlags2._newMiniIcon) 
	wFlags2._newMiniIcon = (oldType != _miniIcon);
    return self;
}


- (const char *) miniwindowIcon
{
    return _miniIcon;
}


- setDocEdited:(BOOL)flag
{
    if (wFlags2.docEdited != flag) {
	wFlags2.docEdited = flag ? 1 : 0;
	[[_borderView getCloseButton] setIcon:(flag ? "editing" : "close")];
	[self _displayTitle];
	[NXApp updateWindowsItem:self];
    }
    return self;
}


- (BOOL)isDocEdited
{
    return (wFlags2.docEdited);
}


- (BOOL)isVisible
{
    return wFlags.visible;
}


- (BOOL)isKeyWindow
{
    return (wFlags.isKeyWindow && [NXApp isActive]);
}


- (BOOL)isMainWindow
{
    return (wFlags.isMainWindow && [NXApp isActive]);
}


- (BOOL)canBecomeKeyWindow
{
    return (winEventMask & NX_KEYDOWNMASK) ? YES : NO;
}

- (BOOL)canBecomeMainWindow
{
    return (wFlags.visible && !wFlags.isPanel && (winEventMask & NX_KEYDOWNMASK));
}

- (BOOL)worksWhenModal
{
    return NO;
}


- (BOOL)_visibleAndCanBecomeKey
 /*
  * Private method that return YES if the window is visible and 
  * canBecomeKeyWindow returns YES. 
  */
{
    return wFlags.visible && !wFlags2._limitedBecomeKey && [self canBecomeKeyWindow];
}


- convertBaseToScreen:(NXPoint *)aPoint
{
    aPoint->x += frame.origin.x;
    aPoint->y += frame.origin.y;
    return self;
}

- convertScreenToBase:(NXPoint *)aPoint
{
    aPoint->x -= frame.origin.x;
    aPoint->y -= frame.origin.y;
    return self;
}


- performClose:sender
{
    id button;

    button = [_borderView getCloseButton];
    if (button)
	[button performClick:self];
    else
	NXBeep();
    return self;
}


- performMiniaturize:sender
{
    id button;

    button = [_borderView getIconifyButton];
    if (button)
	[button performClick:self];
    else
	NXBeep();
    return self;
}


- (int)gState
{
    return ([_borderView gState]);
}


- setOneShot:(BOOL)flag
{
    wFlags.oneShot = flag;
    return self;
}

- (BOOL)isOneShot
{
    return wFlags.oneShot;
}

static void changeKeyAndMain(Window *self, BOOL limitedBecomeKeyOk)
 /* This sets the key window to be self, also making it the main window if
  * possible.   Resign and become methods are sent to the windows.  Titlebars
  * are redrawn as needed.
  */
{
    id                  mainWindow = [NXApp mainWindow];
    id                  keyWindow = [NXApp keyWindow];
    BOOL                mainChanged = NO;	/* does self become the main */
					/* window as well as the key window? */
    BOOL                isActive = [NXApp isActive];

    if (self == keyWindow)
	return;
    if (![self _visibleAndCanBecomeKey] &&
	(!self->wFlags2._limitedBecomeKey || !limitedBecomeKeyOk))
	return;
    mainChanged = ([self canBecomeMainWindow] && self != mainWindow);
    if (isActive) {
	[NXApp _setKeyWindow:self];
	_NXSendWindowNotification(keyWindow, @selector(resignKeyWindow));
	_NXSendWindowNotification(self, @selector(becomeKeyWindow));
	if (mainChanged) {
	    [NXApp _setMainWindow:self];
	    _NXSendWindowNotification(mainWindow, @selector(resignMainWindow));
	    _NXSendWindowNotification(self, @selector(becomeMainWindow));
	}
	if (![NXApp _isDeactPending]) {
	    [keyWindow _displayTitle];
	    if (mainChanged && keyWindow != mainWindow)
		[mainWindow _displayTitle];
	    [self _displayTitle];
	}
    } else {
	[NXApp _setKeyWindow:self];
	if (mainChanged) 
	    [NXApp _setMainWindow:self];
    }
}


void _NXOrderKeyAndMain(void)
 /* reorders and shows key and main window in right order, using safeUpFront to 
  * avoid sync problems with another activation going on.  Doesnt change the 
  * key or main window.
  */
{
    id mainWindow = [NXApp _mainWindow];
    id keyWindow = [NXApp _keyWindow];
    int mainWinNum = [mainWindow windowNum];
    int keyWinNum = [keyWindow windowNum];

    if ([NXApp _isRunningModal]) {
	_NXSafeOrderFront(mainWinNum);
	if (mainWindow != keyWindow)
	    _NXSafeOrderFront(keyWinNum);
    } else {
	_NXSafeOrderFront(keyWinNum);
	if (mainWindow != keyWindow && mainWinNum > 0)
	    [mainWindow orderWindow:NX_BELOW relativeTo:keyWinNum];
    }
}


void _NXShowKeyAndMain(void)
 /* renotifies and redraws main and key window.  Doesnt change either. */
{
    Window	*mainWindow = [NXApp _mainWindow];
    Window	*keyWindow = [NXApp _keyWindow];
    BOOL	oldMainFlag = NO;		/* for clean -Wall */
    BOOL	oldKeyFlag = NO;		/* for clean -Wall */

  /* setting these bits is a hack so we can draw the title bars before we send the becomeMainWindow/becomeKeyWindow messages to the first resp.  We must restore these so that the notify messages go through.  */
    if (![NXApp _isDeactPending]) {
	if (mainWindow) {
	    oldMainFlag = mainWindow->wFlags.isMainWindow;
	    mainWindow->wFlags.isMainWindow = YES;
	}
	if (keyWindow) {
	    oldKeyFlag = keyWindow->wFlags.isKeyWindow;
	    keyWindow->wFlags.isKeyWindow = YES;
	}
    
	[mainWindow _displayTitle];
	if (mainWindow != keyWindow)
	    [keyWindow _displayTitle];
    
	if (mainWindow)
	    mainWindow->wFlags.isMainWindow = oldMainFlag;
	if (keyWindow)
	    keyWindow->wFlags.isKeyWindow = oldKeyFlag;
    }
    _NXSendWindowNotification(mainWindow, @selector(becomeMainWindow));
    _NXSendWindowNotification(keyWindow, @selector(becomeKeyWindow));
}


- _wsmIconInitFor:(int)localWinNum
{
    int                 gs;
    NXRect appIconClipRect = {{2.0, 2.0},
			      {NX_TOKENWIDTH - 4.0, NX_TOKENHEIGHT - 4.0}};

    _NXInitGStateWithClip(localWinNum, &gs, &appIconClipRect);
    windowNum = localWinNum;
    [NXApp setWindowNum:windowNum forWindow:self];
    windowOwnedByWSM = localWinNum;
    [_borderView addSubview:contentView];
    [_borderView _setBorderViewGState:gs];
    [_borderView awake];
    return self;
}

- (BOOL)_wsmOwnsWindow
{
    return (windowOwnedByWSM == windowNum);
}


/* Printing and Copying PostScript methods, making Window like View */

- _remotePrintWindow:(NXEvent *)theEvent
/*
 * Sets up the output file for printing based on the event data
 * and tells the window to print itself.
 */
{
    id                  pInfo = [NXApp printInfo];
    char                oldPath[MAXPATHLEN];
    char                newPath[40];
    const char         *oldPathPtr;
    id                  retVal;

    oldPathPtr = [pInfo outputFile];
    if (oldPathPtr) {
	strcpy(oldPath, oldPathPtr);
	oldPathPtr = oldPath;
    }
    sprintf(newPath, "/tmp/windowdump%d.eps",
	    theEvent->data.compound.misc.L[1]);
    [pInfo setOutputFile:newPath];
    retVal = [self _realSmartPrintPSCode:self doPanel:NO forFax:NO];
    [pInfo setOutputFile:oldPathPtr];
    return retVal;
}


- _remoteCopyWindow:(NXEvent *)theEvent
/*
 * Sets up the output file for printing based on the event data
 * and tells the window to copy itself.
 */
{
    char                filename[40];
    id                  retVal;
    int                 fd;
    NXStream           *stream;

    sprintf(filename, "/tmp/windowdump%d.eps",
	    theEvent->data.compound.misc.L[1]);
 /* errors??? */
    if ((fd = open(filename, O_CREAT | O_EXCL | O_WRONLY, 0666)) < 0) {
	fprintf(stderr, "Could not open spool file in %s\n", filename);
	return NULL;
    }
    stream = NXOpenFile(fd, NX_WRITEONLY);
    retVal = [self copyPSCodeInside:NULL to:stream];
    NXClose(stream);
    close(fd);
    return retVal;
}


- printPSCode:sender
{
    return[_borderView _realPrintPSCode:sender 
    	   helpedBy:self doPanel:YES forFax:NO];
}


- faxPSCode:sender
{
    return[_borderView _realPrintPSCode:sender 
           helpedBy:self doPanel:YES forFax:YES];
}


- smartFaxPSCode:sender
{
    return[self _realSmartPrintPSCode:sender doPanel:YES forFax:YES];
}


- copyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream
{
    return[_borderView _realCopyPSCodeInside:rect to:stream helpedBy:self];
}


- smartPrintPSCode:sender
{
    return[self _realSmartPrintPSCode:sender doPanel:YES forFax:NO];
}


- _realSmartPrintPSCode:sender doPanel:(BOOL)panelFlag forFax:(BOOL)faxFlag
 /*
  * Implements smart window printing.  Extra arg is whether to put up
  * the print panel.
  */
{
    id                  pInfo = [NXApp printInfo];
    BOOL                oldHCentered, oldVCentered;	/* saved pInfo values */
    char                oldOrientation;
    int                 oldHPagMode, oldVPagMode;
    id			retVal;

    oldHCentered = [pInfo isHorizCentered];
    oldVCentered = [pInfo isVertCentered];
    oldOrientation = [pInfo orientation];
    oldHPagMode = [pInfo horizPagination];
    oldVPagMode = [pInfo vertPagination];

    [pInfo setHorizCentered:YES];
    [pInfo setVertCentered:YES];
    [pInfo setOrientation:(frame.size.width <= frame.size.height ?
			   NX_PORTRAIT : NX_LANDSCAPE) andAdjust:YES];
    [pInfo setHorizPagination:NX_FITPAGINATION];
    [pInfo setVertPagination:NX_FITPAGINATION];

    retVal = [_borderView _realPrintPSCode:sender helpedBy:self
		doPanel:panelFlag forFax:faxFlag];

    [pInfo setHorizCentered:oldHCentered];
    [pInfo setVertCentered:oldVCentered];
    [pInfo setOrientation:oldOrientation andAdjust:YES];
    [pInfo setHorizPagination:oldHPagMode];
    [pInfo setVertPagination:oldVPagMode];

    return retVal;
/*
    ??? SHOULD BE: return retVal ? self : nil;
    Fix this for 3.0 - why take little risks liek this for 2.0?
*/
}


- (BOOL)knowsPagesFirst:(int *)firstPageNum last:(int *)lastPageNum
{
    return[_borderView knowsPagesFirst:firstPageNum last:lastPageNum];
}


- openSpoolFile:(char *)filename
{
    return[_borderView openSpoolFile:filename];
}


- beginPSOutput
{
    return[_borderView beginPSOutput];
}


- beginPrologueBBox:(const NXRect *)boundingBox creationDate:(const char *)dateCreated
    createdBy:(const char *)anApplication fonts:(const char *)fontNames
    forWhom:(const char *)user pages:(int)numPages title:(const char *)aTitle
{
    return[_borderView beginPrologueBBox:boundingBox creationDate:dateCreated
	   createdBy:anApplication fonts:fontNames
	   forWhom:user pages:numPages title:aTitle];
}


- endHeaderComments
{
    return[_borderView endHeaderComments];
}


- endPrologue
{
    return[_borderView endPrologue];
}


- beginSetup
{
    return[_borderView beginSetup];
}


- endSetup
{
    return[_borderView endSetup];
}


- beginPage:(int)ordinalNum label:(const char *)aString bBox:(const NXRect *)pageRect
    fonts:(const char *)fontNames
{
    return[_borderView beginPage:ordinalNum label:aString bBox:pageRect
	   fonts:fontNames];
}


- beginPageSetupRect:(const NXRect *)aRect placement:(const NXPoint *)location
{
    return[_borderView beginPageSetupRect:aRect placement:location];
}


- endPageSetup
{
    return[_borderView endPageSetup];
}


- endPage
{
    return[_borderView endPage];
}


- beginTrailer
{
    return[_borderView beginTrailer];
}


- endTrailer
{
    return[_borderView endTrailer];
}


- endPSOutput
{
    return[_borderView endPSOutput];
}


- spoolFile:(const char *)filename
{
    return[_borderView spoolFile:filename];
}


- (float)heightAdjustLimit
{
    return[_borderView heightAdjustLimit];
}


- (float)widthAdjustLimit
{
    return[_borderView widthAdjustLimit];
}


- (BOOL)getRect:(NXRect *)theRect forPage:(int)page
{
    return[_borderView getRect:theRect forPage:page];
}


- placePrintRect:(const NXRect *)aRect offset:(NXPoint *)location
{
    return[_borderView placePrintRect:aRect offset:location];
}


- addCursorRect:(const NXRect *)aRect cursor:anObj forView:aView
{
    NXCursorArray      *array;
    NXCursorRect       *cRect, *startRect;
    NXRect              contentRect, tempRect;

    cRect = findCursorRect(self);
    array = _cursorRects;
    [contentView getFrame:&contentRect];
    tempRect = *aRect;
    if (!NXIntersectionRect(&contentRect, &tempRect))
	return nil;
    cRect->cursorRect = tempRect;
    cRect->cursor = anObj;
    cRect->view = aView;
    cRect->isRectUp = NO;
    startRect = array->cursorRects;
    if (cRect == CURSORAT(startRect, array->chunk.used))
	array->chunk.used += sizeof(NXCursorRect);
    if (!wFlags2._cursorRectsDisabled && wFlags.isKeyWindow) {
	int index = cRect - startRect;
	setCursorRect(self, aRect, (-1) - index);
	cRect->isRectUp = YES;
    }
    return self;
}

- removeCursorRect:(const NXRect *)aRect cursor:anObj forView:aView
{
    NXRect              contentRect, tempRect;
    NXCursorArray      *array = _cursorRects;
    NXCursorRect       *cRect, *lastRect, *startRect;
    int index = -1;

    if (!array)
	return self;
    [contentView getFrame:&contentRect];
    tempRect = *aRect;
    if (!NXIntersectionRect(&contentRect, &tempRect))
	return nil;
    startRect = array->cursorRects;
    lastRect = CURSORAT(startRect, array->chunk.used);
    cRect = startRect;
    for (;cRect < lastRect; cRect++, index--) {
	if (aView == cRect->view && anObj == cRect->cursor &&
	    NXEqualRect(&tempRect, &cRect->cursorRect)) {
	    cRect->cursor = 0;
	    if (!wFlags2._cursorRectsDisabled && wFlags.isKeyWindow) {
		discardCursorRect(self, index);
	    }
	    wFlags2._haveFreeCursorRects = YES;
	    return self;
	}
    }
    return nil;
}

- (BOOL)_hasCursorRects
{
    return(!wFlags2._cursorRectsDisabled);
}

- disableCursorRects
{
    if (wFlags2._cursorRectsDisabled)
	return self;
    wFlags2._cursorRectsDisabled = YES;
    changeCursorRects(self, NO);
    if (wFlags.isKeyWindow && (windowNum > 0)) {
	_NXSetEventMask(winEventMask, windowNum);
    }
    return self;
}

- enableCursorRects
{
    if (!wFlags2._cursorRectsDisabled)
	return self;
    wFlags2._cursorRectsDisabled = NO;
    if (wFlags.isKeyWindow)
	_NXSetEventMask(winEventMask | CURSOR_RECT_MASK, windowNum);
    changeCursorRects(self, YES);
    return self;
}

- discardCursorRects
{
    NXCursorArray      *array = _cursorRects;

    if (!array)
	return self;
    changeCursorRects(self, NO);
    array->chunk.used = 0;
    wFlags2._haveFreeCursorRects = NO;
    return self;
}

- invalidateCursorRectsForView:aView
{
    NXCursorArray *array = _cursorRects;
    if (_invalidCursorView)
	return self;
    _invalidCursorView = contentView;
    wFlags2._validCursorRects = NO;
    if (wFlags2._cursorRectsDisabled) {
    /*
     * here the tracking rects are down, your cursor rects are invalid, so we
     * gun the existing cursor rects 
     */
	if (array)
	    array->chunk.used = 0;
	wFlags2._haveFreeCursorRects = NO;
    }
    return self;
}

- resetCursorRects
{
    BOOL wasDisabled = wFlags2._cursorRectsDisabled;
    [self discardCursorRects];
    _invalidCursorView = contentView;
    wFlags2._cursorRectsDisabled = YES;
    [_invalidCursorView _resetCursorRects:NO];
    wFlags2._cursorRectsDisabled = wasDisabled;
    _invalidCursorView = nil;
    wFlags2._validCursorRects = YES;
    changeCursorRects(self, YES);
    return self;

}

- _discardCursorRectsForView:aView
{
    NXCursorArray      *array = _cursorRects;
    NXCursorRect       *cRect, *lastRect, *startRect;
    int                 index = -1;
    BOOL discard;

    if (!array)
	return self;
    discard = (!wFlags2._cursorRectsDisabled && wFlags.isKeyWindow);
    startRect = array->cursorRects;
    lastRect = CURSORAT(startRect, array->chunk.used);
    cRect = startRect;
    for (;cRect < lastRect; cRect++, index--) {
	if (aView == cRect->view) {
	    cRect->cursor = 0;
	    if (discard) 
		discardCursorRect(self, index);
	    wFlags2._haveFreeCursorRects = YES;
	}
    }
    return self;
}


- (BOOL)canStoreColor
{
    NXWindowDepthType depth;

    if ((NXDrawingStatus == NX_PRINTING) && !NXScreenDump) {
	/*
	 * ??? Look in the PDF files some day... Can't yet.
	 */
	return YES;
    }

    if ((depth = [self depthLimit]) == NX_DefaultDepth) {
	depth = [Window defaultDepthLimit];
    }

    return NXNumberOfColorComponents(NXColorSpaceFromDepth(depth)) > 1;
}


/*
 * Return pointer to deepest screen.  If window lies on no
 * screens, return NULL.
 */
- (const NXScreen *)bestScreen
{
    int numScreens, deepestScreen = -1;
    NXScreen *screens;

    [NXApp getScreens:&screens count:&numScreens];

    if (numScreens == 1) {
	if (NXIntersectsRect (&frame, &(screens[0].screenBounds))) {
	    deepestScreen = 0;
	} 
    } else {
	NXWindowDepthType depth, maxDepth = 0;
	int cnt;
	for (cnt = 0; cnt < numScreens; cnt++) {
	    if (NXIntersectsRect (&frame, &(screens[cnt].screenBounds))) {
		depth = screens[cnt].depth;
		if (depth > maxDepth) {
		    maxDepth = depth;
		    deepestScreen = cnt;
		}
	    }
	}
    }

    return deepestScreen >= 0 ? &screens[deepestScreen] : NULL;
}


/*
 * Method to return the screen that displays more of the window than other
 * screens.  If the window does not lie on any screen, this method returns
 * NULL.
 */
- (const NXScreen *)screen;
{
    return screenForRect(&frame);
}

/*
 * This method will make sure the window whose frame is described by the
 * newFrame argument is on screen. If not, it will bring
 * the drag area of the frame to the closest edge of the closest screen.
 */
- (BOOL)constrainFrameRect:(NXRect *)newFrame toScreen:(const NXScreen *)screen
{
    return (makeFrameVisible (newFrame, [self style], screen));
}

/*
 * This function finds the screen that holds most of the specified
 * rectangle.  If the rectangle does not lie on any screen, then this
 * function will return NULL.
 */
static const NXScreen *screenForRect (NXRect *rect)
{
    int numScreens, cnt;
    NXScreen *allScreens, *screen = NULL;
    float maxArea = 0.0, curArea;
    
    [NXApp getScreens:&allScreens count:&numScreens];

    for (cnt = 0; cnt < numScreens; cnt++) {
	NXRect tmpRect = allScreens[cnt].screenBounds;
	if (NXIntersectionRect (rect, &tmpRect) &&
	    (curArea = NX_WIDTH(&tmpRect) * NX_HEIGHT(&tmpRect)) > maxArea) {
	    maxArea = curArea;
	    screen = &allScreens[cnt];
	}
    }

    return screen;
}

/*
 * Returns the draggable area for a window, in screen coordinates.
 * For windows without title bars, returns the whole window.
 */
static void getDragRect(NXRect *dragRect, NXRect *frameRect, int aStyle)
{
    *dragRect = *frameRect;
    if (_NXtMargin(aStyle) > 0.0) {
	NX_Y(dragRect) += (NX_HEIGHT(dragRect) - _NXtMargin(aStyle));
	NX_HEIGHT(dragRect) = _NXtMargin(aStyle);
    }
}

#define MINYHANDLE 10.0 /* Amount we make visible if we bring window on.*/
#define MINXHANDLE 80.0

/*
 * This function tries to make sure the drag area of frameRect is on screen.
 * (The drag area is determined by the style.) If the window needs to move
 * winFrame is modified and the function returns YES; otherwise the function
 * returns NO.
 */ 
static BOOL makeFrameVisible(NXRect *frameRect, int aStyle, const NXScreen *screen) 
{
    NXRect dragRect; /* Rect we are trying to bring on screen */
    NXCoord xOffset = 0.0, yOffset = 0.0;

    getDragRect(&dragRect, frameRect, aStyle);
    NXInsetRect(&dragRect, 1.0, 1.0);

    /*
     * For success, either screen is NULL and the dragRect is on some screen, or
     * screen is specified and dragRect falls on that screen. If one of these conditions
     * is true, then we don't have to do any work...
     */ 
    if ((!screen && screenForRect(&dragRect)) || 
	(screen && NXIntersectsRect (&dragRect, &(screen->screenBounds)))) {
	return NO;
    }

    if (screen || (screen = closestScreenToRect (&dragRect))) {
	const NXRect *scrRect = &(screen->screenBounds);
	if (NX_MAXY(&dragRect) <= NX_Y(scrRect)) {
	    yOffset = NX_Y(scrRect) - NX_MAXY(&dragRect) + MINYHANDLE;
	} else if (NX_Y(&dragRect) >= NX_MAXY(scrRect)){
	    yOffset = NX_MAXY(scrRect) - NX_Y(&dragRect) - MINYHANDLE;
	}
	if (NX_MAXX(&dragRect) <= NX_X(scrRect)) {
	    xOffset = NX_X(scrRect) - NX_MAXX(&dragRect) + MINXHANDLE;
	} else if (NX_X(&dragRect) >= NX_MAXX(scrRect)) {
	    xOffset = NX_MAXX(scrRect) - NX_X(&dragRect) - MINXHANDLE;
	}
	NX_X(frameRect) += xOffset;
	NX_Y(frameRect) += yOffset;
    }
    return (xOffset != 0.0 || yOffset != 0.0);    
}

/*
 * This function finds the screen that is closest to the given rectangle.
 * (Currently the search is very rough.) It is guaranteed to return a screen.
 */
static const NXScreen *closestScreenToRect (NXRect *rect)
{
    int numScreens, cnt;
    NXScreen *allScreens, *screen = NULL;
    int min = INT_MAX, cur;
    NXPoint mid = {NX_MIDX(rect), NX_MIDY(rect)};
  
    [NXApp getScreens:&allScreens count:&numScreens];

    for (cnt = 0; cnt < numScreens; cnt++) {
	const NXRect *tmpRect = &(allScreens[cnt].screenBounds);
	cur = abs((int)(NX_MIDX(tmpRect) - mid.x)) +
	      abs((int)(NX_MIDY(tmpRect) - mid.y));
	if (cur < min) {
	    min = cur;
	    screen = &allScreens[cnt];
	}
    }

    return screen;
}

/*
 * Assuming rect is the frame of some window, this method modifies
 * rect such that it is in the same place on the specified screen as it is
 * on the original screen.  If it can't determine where that place is, this
 * function will center the rect on the given screen.
 * This function should be used when the window is first being created: It will
 * move the frame around radically to make sure it appears reasonably well.
 * After it's created, use moveFrameToScreen() (or constrainFrameRect:) to
 * make sure the window is visible on a given screen.
 *
 * ??? Don't know how well this will do (in practice) on a machine with
 * variable sized screens.
 */
static void moveRectToScreen (NXRect *rect, const NXScreen *screen)
{
    /*
     * If no screen is specified, make sure the rect falls on some screen.
     * If yes, then the rect is OK. Otherwise bring it to the closest screen.
     */
    if (!screen) {
	if (screenForRect(rect)) {
	    return;
	} else {
	    screen = closestScreenToRect (rect);
        }
    }
	    
    if (!NXIntersectsRect (rect, &(screen->screenBounds))) {
	NXRect zeroScreenFrame = [NXApp _zeroScreen]->screenBounds;
	NXRect relativeRect = *rect;

	if (!NXIntersectsRect (rect, &zeroScreenFrame)) {
	    NX_X(&relativeRect) =
		(int)NX_X(&relativeRect) % (int)NX_WIDTH(&zeroScreenFrame);
	    NX_Y(&relativeRect) =
		(int)NX_Y(&relativeRect) % (int)NX_HEIGHT(&zeroScreenFrame);
	}

	NX_X(rect) = NX_X(&relativeRect) + NX_X(&(screen->screenBounds));
	NX_Y(rect) = NX_Y(&relativeRect) + NX_Y(&(screen->screenBounds));

	/* No need to enable this code until we fill in the if below */
#if 0
	if (!NXIntersectsRect (rect, &(screen->screenBounds))) {
	    /*
	     * We get here if all our attempts have failed; probably a machine with
	     * multiple, variable-sized screens. Let the automatic-window-off-screen
	     * protection take care of it.
	     */
	}
#endif
    }
}

- write:(NXTypedStream *) s
{
    const char         *title = [_borderView title];

    [super write:s];
    NXWriteRect(s, &frame);
    NXWriteTypes(s, "@@ifss**s", &contentView, &counterpart,
		 &winEventMask, &backgroundGray, &wFlags, &wFlags2, &title, 
		 &_miniIcon, &private->wFlags3);
    NXWriteColor(s, [self _colorSpecified] && 
		    !_NXIsPureGray(*private->backgroundColor, backgroundGray) ? 
		    *private->backgroundColor : _NXNoColor());
    NXWriteObjectReference(s, delegate);
    NXWriteObjectReference(s, firstResponder);
    return self;
}

- read:(NXTypedStream *) s
{
    NXRect              frameRect;
    char               *title;
    int			version;
    NXZone    *zone = [self zone];

    [super read:s];
    private = NXZoneCalloc(zone, 1, sizeof(WindowPrivate));
    version = NXTypedStreamClassVersion(s, "Window");
    NXReadRect(s, &frame);
    if (version == 0) {
	NXReadTypes(s, "@@ifss*", &contentView, &counterpart,
		    &winEventMask, &backgroundGray, &wFlags, &wFlags2, &title);
    } else if (version == 1) {
	NXReadTypes(s, "@@ifss**", &contentView, &counterpart,
		    &winEventMask, &backgroundGray, &wFlags, &wFlags2, &title,
		    &_miniIcon);
    } else if (version >= 2) {
	NXColor tmpColor;
	NXReadTypes(s, "@@ifss**s", &contentView, &counterpart,
		    &winEventMask, &backgroundGray, &wFlags, &wFlags2, &title,
		    &_miniIcon, &private->wFlags3);
	if (_NXIsValidColor((tmpColor = NXReadColor(s)))) {
	    [self setBackgroundColor:tmpColor];
	}
    }
	    
    delegate = NXReadObject(s);
    private->wFlags3.delegateReturnsValidRequestor = [delegate respondsTo:@selector(validRequestorForSendType:andReturnType:)];
    firstResponder = NXReadObject(s);
    frameRect = frame;
    frameRect.origin.x = frameRect.origin.y = 0.0;
    _borderView = [[FrameView allocFromZone:zone] initFrame:&frameRect
		   style:wFlags.style
		   buttonMask:wFlags.buttonMask
		   owner:self];
    [_borderView setTitle:title];
    [_borderView addSubview:contentView];
    return self;
}


@end

static unsigned trackInfoHash(const void *info, const void *data)
{
    const TrackInfo *tinfo = data;
    return tinfo->num;
}

static int trackInfoEqual(const void *info, const void *data1, const void *data2)
{
    const TrackInfo *tinfo1 = data1;
    const TrackInfo *tinfo2 = data2;

    return (tinfo1->num == tinfo2->num);
}


static void initShared(Window *self)
{
    int                 gs;

    _NXInitGState(self->windowNum, &gs);
    [self->_borderView _setBorderViewGState:gs];
    [self->_borderView addSubview:self->contentView];
}


extern void _NXBuildSharedWindows(void)
{
    NXRect              theFrame;
    extern Window *NXSharedCursors, *NXSharedIcons;

    NXSetRect(&theFrame, 0., 0., 64., 64.);
    NXSharedCursors = [[Window allocFromZone:[NXApp zone]] initContent:&theFrame style:NX_PLAINSTYLE backing:NX_RETAINED buttonMask:0 defer:YES];
    NXSetRect(&theFrame, 0., 0., 176., 149.);
    NXSharedIcons = [[Window allocFromZone:[NXApp zone]] initContent:&theFrame style:NX_PLAINSTYLE backing:NX_RETAINED buttonMask:0 defer:YES];
    _NXGetSharedWindows(&NXSharedCursors->windowNum,
			&NXSharedIcons->windowNum);
    initShared(NXSharedCursors);
    initShared(NXSharedIcons);
}

#ifdef DEBUG
static BOOL doWinTrace = NO;
#define WINTRACE(obj, sel)	if (doWinTrace) fprintf(stderr, "0x%x %s\n", obj, sel)
#else
#define WINTRACE(obj, sel)
#endif

/* sends a become/resign main/key notification to a window.  Ensures we dont send these redundantly. */
void _NXSendWindowNotification(Window *win, SEL sel)
{
    if (win) {
	if (sel == @selector(becomeKeyWindow)) {
	    if (!win->wFlags.isKeyWindow || (_NXGetShlibVersion() <= MINOR_VERS_1_0 && [NXApp appName] && !strcmp([NXApp appName], "Wingz"))) {
		(void)[win becomeKeyWindow];
		WINTRACE(win, "becomeKeyWindow");
	    }
	} else if (sel == @selector(resignKeyWindow)) {
	    if (win->wFlags.isKeyWindow) {
		(void)[win resignKeyWindow];
		WINTRACE(win, "resignKeyWindow");
	    }
	} else if (sel == @selector(becomeMainWindow)) {
	    if (!win->wFlags.isMainWindow) {
		(void)[win becomeMainWindow];
		WINTRACE(win, "becomeMainWindow");
	    }
	} else if (sel == @selector(resignMainWindow)) {
	    if (win->wFlags.isMainWindow) {
		(void)[win resignMainWindow];
		WINTRACE(win, "resignMainWindow");
	    }
	}
    }
}


static void discardCursorRect(id self, int index)
{
    _NXDiscardCursorRect(index,[self gState]);
}


static void setCursorRect(id self, const NXRect *r, int index)
{
    _NXSetCursorRect(
	  NX_X(r), NX_Y(r), NX_WIDTH(r), NX_HEIGHT(r), index,[self gState]);
}


static void changeCursorRects(Window *self, BOOL add)
{
    NXCursorArray      *array = self->_cursorRects;
    NXCursorRect       *cRect, *lastRect;
    int                 index = -1;
    BOOL isActive = [NXApp isActive];

    if (!array)
	return;
    if (add) {
	if (!self->wFlags.isKeyWindow || self->wFlags2._cursorRectsDisabled)
	    return;
	_NXResetCursorState(self->windowNum, allEvents, YES);
    }
    cRect = array->cursorRects;
    lastRect = CURSORAT(cRect, array->chunk.used);
    for (; cRect < lastRect; cRect++, index--) {
	if (cRect->cursor) {
	    if (add && isActive) {
		if (!cRect->isRectUp)
		    setCursorRect(self, &cRect->cursorRect, index);
		cRect->isRectUp = YES;
	    } else {
		if (cRect->isRectUp)
		    discardCursorRect(self, index);
		cRect->isRectUp = NO;
	    }
	}
    }
    if (isActive && !add)
	_NXResetCursorState(self->windowNum, eventsFor, NO);
}

static NXCursorRect *findCursorRect(Window *self)
{
    NXCursorRect	*cRect = 0, *lastRect;
    NXCursorArray	*array = self->_cursorRects;
    NXZone 		*zone = [self zone];

    if (!array) {
	array = (NXCursorArray *)NXChunkZoneMalloc(0, sizeof(NXCursorRect), zone);
	cRect = array->cursorRects;
    } else {
	if (self->wFlags2._haveFreeCursorRects) {
	    cRect = array->cursorRects;
	    lastRect = CURSORAT(cRect, array->chunk.used);
	    for (;cRect < lastRect; cRect++) {
		if (!cRect->cursor)
		    break;
	    }
	    if (cRect == lastRect)
		self->wFlags2._haveFreeCursorRects = NO;
	}
	if (!self->wFlags2._haveFreeCursorRects) {
	    if (array->chunk.allocated == array->chunk.used)
		array = (NXCursorArray *)NXChunkZoneRealloc(&array->chunk, zone);
	    cRect = CURSORAT(array->cursorRects, array->chunk.used);
	}
    }
    self->_cursorRects = array;
    return cRect;
}

#ifdef MINIMAL_CURSOR_UPDATE
#define ISMARKED(v) ( ((ViewId *) v)->_vFlags.mark)
static void removeInvalidCursors(id self)
{
    NXCursorRect       *cRect, *lastRect, *goodRect, *firstRect;
    int                 index = -1;
    NXCursorArray      *array = self->_cursorRects;

    if (!array)
	return;
    [self->_invalidCursorView _mark:YES];
    firstRect = CURSORAT(array->cursorRects, 0);
    lastRect = CURSORAT(firstRect, array->chunk.used);
    cRect = firstRect;
    for (;cRect < lastRect; cRect++, index--) {
	if (cRect->cursor && ISMARKED(cRect->view)) {
	    discardCursorRect(self, index);
	    cRect->cursor = nil;
	}
    }
    cRect = firstRect;
    goodRect = firstRect;
    for (;cRect < lastRect; cRect++) {
	if (goodRect != cRect)
	    *goodRect = *cRect;
	if (cRect->cursor)
	    goodRect++;
    }
    array->chunk.used = (goodRect - firstRect) * sizeof(NXCursorRect);
    self->wFlags2._haveFreeCursorRects = NO;
    [self->_invalidCursorView _mark:NO];
}

#endif

extern NXCursorRect *_NXIndexToCursorRect(Window *self, int index)
{
    int                 offset;
    NXCursorArray      *array = self->_cursorRects;

    if (!array)
	return 0;
    offset = index * sizeof(NXCursorRect);
    if (offset > array->chunk.used)
	return 0;
    return CURSORAT(array->cursorRects, offset);
}


/* looks for a Wingz panel about to receive a double click */
static BOOL validateMouseEvent(Window *win, NXEvent *ev)
{
    BOOL invalid =
      /* make sure its the 1.0 Wingz */
	_NXGetShlibVersion() <= MINOR_VERS_1_0 &&
    	[NXApp appName] && !strcmp([NXApp appName], "Wingz") &&

      /* make sure its a double click mouse event */
	NX_EVENTCODEMASK(ev->type) & (NX_LMOUSEDOWNMASK | NX_LMOUSEUPMASK | NX_LMOUSEDRAGGEDMASK) &&
	ev->data.mouse.click >= 2 &&

      /* Wingz panels have no titles or borderView buttons */
	!((FViewId *)(win->_borderView))->closeButton && 
	!((FViewId *)(win->_borderView))->iconifyButton && 
	!((FViewId *)(win->_borderView))->titleCell &&

      /* Look for the Font, Custom Format, Help or Paste Formula panels */
	((win->frame.size.width == 238 && win->frame.size.height == 364) ||
	 (win->frame.size.width == 303 && win->frame.size.height == 371) ||
	 (win->frame.size.width == 313 && win->frame.size.height == 331) ||
	 (win->frame.size.width == 408 && win->frame.size.height == 386));
    return !invalid;
}


/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
0.82
----
01/05/89 trey	added covers for View printing methods that message to
		  the border view
01/17/89 trey	added smartPrintPSCode:
01/26/89 bgy	made orderWindow:NX_BELOW relativeTo:0 order above the desk
		 window.
 1/25/89 trey	setTitle: takes a const param
 1/27/89 bs	added read: write:
02/05/89 bgy	added support for cursor tracking. the following are new 
 		 methods:
		- addCursorRect:(NXRect *)aRect cursor:anObj forView:aView;
		- removeCursorRect:(NXRect *)aRect cursor:anObj forView:aView;
		- disableCursorRects;
		- enableCursorRects;
		- discardCursorRects;
		- invalidateCursorRectsForView:aView;
		- resetCursorRects;
		- _discardCursorRectsForView:aView;
 2/09/89 trey	moved _orderWindow:relativeTo: into orderWindow:relativeTo:
 		added makeKeyAndOrderFront:
 2/10/89 pah	add displayIfNeeded (API only)
 2/13/89 trey	changed error: message to NX_ASSERT when sendEvent has an
 		 unrecognized event
 2/15/89 trey	added center method to support runModalFor:
  
0.83
----
 3/13/89 wrp	removed performResize: method from API
 3/12/89 pah	implement displayIfNeeded:
 3/15/89 wrp	added _setNIBBorder:(BOOL)aflag for private NIB use.
 4/02/89 wrp	fixed eventmask for miniwindowstyle. It was getting keys, and
		 consequently was becoming keywindow when you would hide its
		 counterpart.  Also, this caused extra drawing of the window
		 frame which would over-draw any special app drawing.

0.91
----
 4/23/89 trey	made performClose: and performMiniaturize: always return
		 self, regardless of whether the window implements has a
		 close of miniaturize button.  Thus only the keywindow
		 can be closed by the menu command or key equiv.
 5/19/89 trey	when a view that is the firstResp is freed, the window
		 delegate is no longer made first responder.
		minimized static data
		-center forces title bar to be on screen
		pid is set in window's PS dictionary for unarchived
		 windows (fixes cmd-.)
05/19/89 wrp	added _resizeWindow:userAction: to provide a way to resize
		 windows on the screen with minimum flash.  This is called in
		 by FrameView when the user resizes as well as Menu from 
		 'sizeToFit'
05/22/89 wrp	added private methods: _lastLeftHit _lastRightHit

0.92
----
 5/24/89 pah	make double-clicks which activate a window be preceded by
		 the corresponding single-click even if !acceptsFirstMouse
  6/8/89 pah	fix above fix regarding double-clicks that activate a window
06/09/89 wrp	added private method: _displayTitle and replaced most calls to 
		  displayBorder
		added _newMiniIcon to wFlags2 IV
		added _miniIcon to IVs
		added _makeMiniView method
		implemented setMiniwindowIcon:
		added miniwindowIcon
		removed setMiniWindowView: from the API (not implemented)
		added delegate method windowWillMiniaturize:toMiniwindow:
		added initialize method to set Class version for archiving
		modified 'miniaturize' method to make miniMiniView if 
		  neccessary and to send miniaturization delegate methods 
		restructured orderWindow to use _NXToInLevel, removed 
		  doOrdering() calls
 6/10/89 pah	give the window an opportunity to didResize: and willResize:
		 iff delegate does not want to
06/13/89 bgy	made orderWindow:relativeTo: do the correct thing when 
		 ordering NX_OUT and the the application wasn't currently
		 active.

0.93
----
 6/15/89 pah	make title method return const char *
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps
 6/21/89 wrp	added new private method _setWindowLevel: and converted all
  		 callers of NXSetWindowLevel
 6/24/89 wrp	made 'free' free the window's counterpart.
 6/25/89 wrp	made NXDebugging disable cover window on resize. 
 6/25/89 wrp	added flushWindowIfNeeded method and wFlags2._needsFlush IV. 
 6/27/89 wrp	in free, set counterpart->counterpart = nil to prevent free 
 		 again 
 6/29/89 wrp	in setMiniwindowIcon: if _wFlags._newIcon already == YES, don't 
 		 change its value. This fixes Edit bug where it was setting 
		 twice before miniaturizing, so icon never was created.

0.94
----
 7/13/89 pah	fix invalidate so that it sets the bit even if array is NULL

0.95
----
 7/19/89 wrp	fixed windowMoved to early exit if not really moved. Bug #1571
 8/02/89 wrp	fixed bug where one-shot windows were not having their gstates
 		 invalidated when destroyed.

77
--
 2/1/90 trey	removed unused event arg and dead code from changeKeyAndMain
		nuked _borderInvalid flag and methods
 2/06/90 trey	redid window activation
		 added _NXShowKeyAndMain and _NXOrderKeyAndMain
		 nuked -_fixBorders
		 bugs fixed with losing a panel as key window after unhide
		 bugs fixed with unbalanced become/resign notifications
		 -_justOrderOut added
		 -_doOrderWindow added, with args for level and finding new key
		 -_tempHide uses _justOrderOut to avoid change key window
		 -_tempHide orders relative to another window
		   (for showingfront to back)
 2/22/90 trey	added support for hiding/showing panels in packages
 2/25/90 trey	implemented _NXAllWindowsRetained
 2/28/90 trey	added incremental flush bit and method
 3/10/90 trey	nuked safeUpFront
		in -orderWindow, when ordering out, made sure we pick the main
		  window for the new key window if it exists.  Also changed
		  so if we close main window, tries to find a new main window.
 3/12/90 bgy/trey  added hashtable for tracking rect numbers and targets

79
--
 2/1/90 trey	undefs winNum userobject with DPSUndefineUserObject
 4/3/90 pah	added support for limitedBecomeKey
		 allows the ability for a window to become key window only
		  if the user has moused down on a view which can become
		  the first responder.
		 involves adding an argument to changeKeyAndMain which is
		  whether a limitedBecomeKey window can indeed become key
		  during this invocation of changeKeyAndMain
		modified code to support change from _NXLastModalPanel to
		 _NXLastModalSession->window
		added canBecomeKeyWindow method
		 allows programmer to control whether window can become key
		renamed _visibleAndTakesKeys to _visibleAndCanBecomeKey
		 to support canBecomeKeyWindow method

80
--
 4/8/90 trey	added -setTitleAsFilename:
 4/8/90 aozer	Added screen-change functionality: windowChangedScreen:,
		setDisplayOnScreenChange: and displayOnScreenChange:.
 4/8/90 aozer	Fixed center to look at the bounds for the main screen
		rather than looking at screenSize.
 4/9/90 aozer	Added screen method to return the screen the Window is in
 4/9/90 aozer	Added newContent:style:backing:buttonMask:defer:onScreen:.
		newContent:style:backing:buttonMask:defer: invokes this method
		with a reasonable screen (depending on the window style).

82
--
 4/18/90 aozer	Added various functions for window frame management (making
		sure windows appear on visible areas of screens, etc).
		Added constrainFrameRect:toScreen:, which is invoked by methods
		such as placeWindow: and orderWindow:relative:.

83
--
 4/23/90 aozer	Fixed bug in _realSmartPrintPSCode:doPanel: where chunk of code
		to restore original pinfo settings was never being executed.
 4/26/90 aozer	instead of new -> newContent:... -> newContent:...screen:, the
		calling order is new -> newContent:...screen: -> newContent:.
		Thus the method to orderride remains newContent:... (without
		the screen:).
 4/26/90 aozer	Made setMiniwindowIcon: look for NXImages as well as Bitmaps.
 4/26/90 aozer	Added support for color & private struct; incremented version.
 4/26/90 aozer	wantsToBeOnMainScreen flag determines if window was created
		without an explicit screen argument; that way we remember that
		the window wants to come up on the main screen (when it is
		finally created, for instance...)
 4/26/90 aozer	Added isOnColorScreen; this method currently returns YES if
		the machine has a color frame buffer and the only visible parts
		of the window don't lie on a monochrome frame buffer.  
 4/26/90 aozer	windowChangedScreen: renamed to screenChanged:. This method
		causes redraw of either the rectangle that 
		changed screens or the whole rectangle depending on the
		situation; for instance, if a window moves such that 
		isOnColorScreen used to be true but isn't anymore, the whole
		window will get a drawSelf::.

84
--
 5/13/90 trey	nuked incremental flush code

85
--
 5/22/90 trey	converted cruft using old kit window tiers to setwindowlevel
		nuked unused _selfOrOtherCouldBeActive
		implemented NXTraceEvents
 5/30/90 aozer	Fixed #5980; deferred windows which were moveTo:: or 
		sizeWindow::'ed off the screen wouldn't come back on screen.
 6/01/90 aozer	Renamed displayOnScreenChange to dontDisplayOnScreenChange;
		we want default to redraw.
 6/04/90 pah	Added _canOptimizeDrawing and userOptimizedDrawing: which
		 says whether we can optimize drawing and sets whether we
		 can optimize drawing (respectively).
		Added code to display so that we can optimize drawing during
		 the initial draw of a window.
86
--
 6/7/90 aozer	Added bestScreen method to return from among the 
		screens the window lies on the screen with the most depth.
 6/11/90 greg	Trapped setting window labels and added private methods on
  		mapping and unmapping windows to support the Windows Menu.
		Also added - setExcludedFromWindowsMenu: and
		- (BOOL)isExcludedFromWindowsMenumethods.
		
87
--
 7/12/90 glc	Rewrote most of the windows menu stuff.
 		
89
--
 7/19/90 chris	Added faxPSCode: and smartFaxPSCode:
 7/20/90 aozer	Nuked isOnColorScreen, added canStoreColor. This method looks
		at the depth limit of the window instead of its location.
		Rewrote screenChanged: to look at dynamicDepthLimit. The code
		is now considerably simpler.
 7/22/90 aozer	Implemented depthLimit, setDepthLimit:, defaultDepthLimit.
 7/27/90 trey	added +_newContent*.

91
--
 8/12/90 trey	nuked _showPanel/_hidePanel dead code

92
--
 8/19/90 bryan	changed setDelegate to use the to record whether or not the
 		delegate responds to the windowDidUpdate: method:. Also, the
		orderwindow method now records in the App cache whether or not
		a window is visible. Also, all references to the app's
		windowList have been removed, and now Window sends the 
		_addWindow message to the application object. windowNums are
		also maintained in the app cache.
 8/18/90 aozer	Fixed bug 7442 in findScreenCommonCode() by removing the
		"if (screen != mainScreen) then movewindow" optimization.
 8/18/90 aozer	Fixed problem with moving window while resizing (bug 7886) by
		making windowMoved: look at the actual location of the window.

93
--
 8/26/90 aozer	Added code to _wsmIconInitFor: to establish a clip area
		for the app icon to protect against evil apps.
 8/29/90 aozer	Added _adjustDynamicDepthLimit to get dynamicDepthLimits
		working.  If dynamicDepthLimit is YES, then the window's depth
		limit will be set to minimum of the depth of the best screen
		it's on an the default depth limit. Otherwise window's depth
		limit is set through the setDepthLimit: method is not
		constrained by the default depth limit.
 9/4/90 trey	re-fixed half-hidden apps bug.  _doOrderWindow no longer tries
		 to prevent recursive unhides.
		resize protected by DURING/HANDLER's
		makeKeyAndOrderFront changed to call orderWindow:NX_ABOVE
		 instead of orderFront: for 1.0 Display Talk compatibility
		fixed infinite loop when clicking on a view that doesn't
		 accept first mouse in a limitedBecomeKey window

94
--
 9/25/90 trey	prevented calls to changeKeyAndMain when [NXApp _isInvalid]
		ensured that when we close a modal panel, the next modal
		 panel is made key, regardless of window ordering
		brute force solution so we never get in an infinite loop of
		 reposting events at the top of -sendEvent:

95
--
 9/30/90 trey	added -placeWindowAndDisplay:

105
--
 11/6/90 glc	Added _hasCursorRects method.
 
*/

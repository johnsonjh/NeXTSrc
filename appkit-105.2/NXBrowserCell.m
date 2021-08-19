/*
	NXBrowserCell.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXBrowserCell.h"
#import "NXBrowser.h"
#import "Matrix.h"
#import "NXImage.h"
#import "Window.h"
#import <dpsclient/dpsclient.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <zone.h>

#define BORDER 2.0

typedef struct {
    @defs (Window)
} *WindowId;

@implementation NXBrowserCell

+ branchIcon
{
    return [NXImage findImageNamed:"NXmenuArrow"];
}

+ branchIconH
{
    return [NXImage findImageNamed:"NXmenuArrowH"];
}

+ new
{
    return [self newTextCell];
}


+ newTextCell
{
    return [[self newTextCell:NULL] setStringValueNoCopy:"BrowserItem" shouldFree:NO];
}


+ newTextCell:(const char *)aString
{
    return [[self allocFromZone:NXDefaultMallocZone()] initTextCell:aString];
}

- initTextCell:(const char *)aString
{
    [super initTextCell:aString];
    cFlags2.noWrap = YES;
    return self;
}

- init
{
    [self initTextCell:NULL];
    [self setStringValueNoCopy:"BrowserItem"];
    return self;
}

- (BOOL)isOpaque
{
    return YES;
}

- (BOOL)isLeaf
{
    return cFlags2._isLeaf;
}

- setLeaf:(BOOL)flag
{
    cFlags2._isLeaf = flag;
    return self;
}

- (BOOL)isLoaded
{
    return !cFlags2._isLoaded;
}

- setLoaded:(BOOL)flag
{
    cFlags2._isLoaded = flag ? NO : YES;
    return self;
}

- reset
{
    cFlags1.state = 0;
    cFlags1.highlighted = NO;
    return self;
}

- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXSize arrowSize;

    [[[self class] branchIcon] getSize:&arrowSize];
    [super calcCellSize:theSize inRect:aRect];
    if (theSize->height < arrowSize.height + BORDER * 2) {
	theSize->height = arrowSize.height + BORDER * 2;
    }
    if (theSize->width < arrowSize.width + BORDER * 2) {
	theSize->width = arrowSize.width + BORDER * 2;
    }

    return self;
}

- (BOOL)_checkLoaded:controlView rect:(const NXRect *)cellFrame flushWindow:(BOOL *)flushWindow
{
    id browser;
    int row, col;

    *flushWindow = NO;

    if (![self isLoaded] && [controlView isKindOf:[Matrix class]]) {
	browser = [controlView superview];
	while (browser && [browser class] != [NXBrowser class]) {
	    browser = [browser superview];
	}
	if (browser) {
	    [controlView getRow:&row andCol:&col ofCell:self];
	    [[browser delegate] browser:browser
			       loadCell:self
				  atRow:row
			       inColumn:[browser columnOf:controlView]];
	    *flushWindow = NO;
	}
    }

    [self setLoaded:YES];

    return YES;
}

- drawInside:(const NXRect *)cellFrame inView:controlView
{
    id window;
    NXRect rect;
    NXSize arrowSize;
    BOOL highlighted, state, flushWindow;

    if (![self _checkLoaded:controlView rect:cellFrame flushWindow:&flushWindow])
	return self;

    if (cFlags1.state || cFlags1.highlighted || _cFlags3.isWhite) {
	if (cFlags1.state || cFlags1.highlighted) {
	    PSsetgray(NX_WHITE);
	} else {
	    PSsetgray(NX_LTGRAY);
	}
	NXRectFill(cellFrame);
    }

    [[[self class] branchIcon] getSize:&arrowSize];

    rect = *cellFrame;
    rect.size.width -= arrowSize.width + BORDER;

    highlighted = cFlags1.highlighted ? YES : NO;
    state = cFlags1.state ? YES : NO;

    cFlags1.state = 0;
    cFlags1.highlighted = 0;

    [super drawInside:&rect inView:controlView];

    cFlags1.highlighted = highlighted;
    cFlags1.state = state;

    _cFlags3.isWhite = (cFlags1.state || cFlags1.highlighted);

    if (![self isLeaf]) {
	rect.origin.x += rect.size.width;
	rect.origin.y += floor((rect.size.height + ([controlView isFlipped] ? 
			 	arrowSize.height : -arrowSize.height)) / 2.0);
	[(_cFlags3.isWhite ? [[self class] branchIconH] : [[self class] branchIcon]) composite:NX_COPY toPoint:&rect.origin];
    }

    if (flushWindow) {
	window = [controlView window];
	if (((WindowId)window)->_flushDisabled == 1) {
	    [window reenableFlushWindow];
	    [window flushWindow];
	    DPSFlush();
	    [window disableFlushWindow];
	} else if (!((WindowId)window)->_flushDisabled) {
	    [window flushWindow];
	    DPSFlush();
	}
    }

    return self;
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    return [self drawInside:cellFrame inView:controlView];
}

- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)lit
{
    BOOL flushWindow;

    if ((cFlags1.highlighted && !lit) || (!cFlags1.highlighted && lit)) {
	if (((cFlags1.state || !cFlags1.highlighted) && !_cFlags3.isWhite) ||
	    (!cFlags1.state && cFlags1.highlighted && _cFlags3.isWhite)) {
	    if (NXDrawingStatus == NX_DRAWING && [self isLoaded]) {
		cFlags1.highlighted = lit ? YES : NO;
		NXHighlightRect(cellFrame);
		_cFlags3.isWhite = !_cFlags3.isWhite;
	    } else {
		if (![self isLoaded]) [self _checkLoaded:controlView rect:cellFrame flushWindow:&flushWindow];
		if ([self isLoaded]) {
		    cFlags1.highlighted = lit ? YES : NO;
		    [self drawSelf:cellFrame inView:controlView];
		}
	    }
	}
    }

    return self;
}

@end

/*

Modifications (starting post-1.0):

 3/05/90 pah	New cell subclass for 2.0.
		 Basic cell appropriate for use in NXBrowser.
		 All cells used in the NXBrowser must inherit from this.
		 Handles lazy callback to delegate to load per-cell info.
		 Can't have a bezel! (bit is overloaded)

79
--
  4/3/90 pah	Changed sense of setLoaded: flag so that by default is is true

80
--
  4/8/90 pah	removed setBezeled: hack for optimization (too gross)
		added _checkLoaded:... to support NXLazyBrowserCell

84
--
  5/9/90 aozer	Replaced use of Bitmap with NXImage
 5/14/90 pah	Overrode isOpaque and awake methods.
		Made printing of NXBrowserCell's work.
*/


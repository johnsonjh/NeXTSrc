/*
	ScrollView.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ClipView_Private.h"
#import "ScrollView.h"
#import "Cursor.h"
#import "Text.h"
#import "Scroller.h"
#import "Window.h"
#import "NXRulerView.h"
#import <dpsclient/wraps.h>
#import <math.h>

#define	X	origin.x
#define	Y	origin.y
#define	W	size.width
#define	H	size.height

/* ScrollStatus */
#define	NOTNEEDED	(0)
#define	NEEDED		(1)

/* Default line and page scrolling */
#define	DEFPAGECONTEXT	(10.0)
#define	DEFLINEAMOUNT	(10.0)
#define RULERHEIGHT (20.0)

#ifndef SHLIB
    /* this references category .o files, forcing them to be linked into an app which is linked against a archive version of the kit library. */
    asm(".reference .NXTextCategorySelectUtil\n");
#endif

@interface ScrollView(Private)

- _newScroll:(BOOL)vFlag;
- _commonNewScroll:anObject;
- _doScroller:scroller;
- _update;

@end

@implementation ScrollView:View


/* Backwards Compatibility Methods - leave out of spec sheets - rip out soon */
- scrollRectToVisible:(const NXRect *)aRect fromView:aView
{
    return ([contentView _scrollRectToVisible:aRect fromView:aView]);
}

/** Factory Methods **/

+ initialize
{
    [self setVersion:1];
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    NXZone *zone = [self zone];
    [super initFrame:frameRect];
    [self setFlipped:YES];
    [self setOpaque:YES];
    [self setClipping:NO];
    [self setAutoresizeSubviews:YES];

    contentView = [[ClipView allocFromZone:zone] initFrame:&bounds];
    [self addSubview:contentView];

    pageContext = DEFPAGECONTEXT;
    lineAmount = DEFLINEAMOUNT;
    return self;
}


+ new
{
    return [self newFrame:NULL];
}


+ getFrameSize:(NXSize *)fSize forContentSize:(const NXSize *)cSize horizScroller:(BOOL)hFlag vertScroller:(BOOL)vFlag borderType:(int)aType
{
    NXCoord             offset;

    offset = aType == NX_BEZEL ? 4.0 : (aType == NX_RIDGE ? 4.0 : (aType == NX_LINE ? 2.0 : 0.0));
    fSize->width = cSize->width + offset + (vFlag ? NX_SCROLLERWIDTH + 1.0 : 0.0);
    fSize->height = cSize->height + offset + (hFlag ? NX_SCROLLERWIDTH + 1.0 : 0.0);
    return self;
}


+ getContentSize:(NXSize *)cSize forFrameSize:(const NXSize *)fSize horizScroller:(BOOL)hFlag vertScroller:(BOOL)vFlag borderType:(int)aType
{
    NXCoord             offset;

    offset = aType == NX_BEZEL ? 4.0 : (aType == NX_RIDGE ? 4.0 : (aType == NX_LINE ? 2.0 : 0.0));
    cSize->width = fSize->width - offset - (vFlag ? NX_SCROLLERWIDTH + 1.0 : 0.0);
    cSize->height = fSize->height - offset - (hFlag ? NX_SCROLLERWIDTH + 1.0 : 0.0);
    return self;
}

/** Instance Methods **/

- setDocView:aView
{
    return ([contentView setDocView:aView]);
}

- docView
{
    return ([contentView docView]);
}

- getDocVisibleRect:(NXRect *)aRect
{
    return ([contentView getDocVisibleRect:aRect]);
}


- setDocCursor:anObj
{
    return ([contentView setDocCursor:anObj]);
}


/** Geometry **/

- getContentSize:(NXSize *)contentViewSize
{
    NXRect theFrame;
    [contentView getFrame:&theFrame];
    *contentViewSize = theFrame.size;
    return self;
}


- resizeSubviews:(const NXSize *)oldSize
{
    [self _update];
    return self;
}



/** Drawing Support **/

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXRect              aRect, bRect;
    NXCoord             inset;
    int			clipped;

    aRect = bounds;
    clipped = 0;
    
    if((rectCount != 1 || !NXEqualRect(rects, &bounds))
    		 && _sFlags.borderType == NX_BEZEL) {
	clipped = 1;
	PSgsave();
	if(rectCount > 1) {
	    rects++;
	    rectCount--;
	}
	NXRectClipList(rects, rectCount);
    }
    
    if (_sFlags.borderType != NX_NOBORDER) {
	if (_sFlags.borderType == NX_LINE) {
	    PSsetgray(NX_BLACK);
	    NXFrameRect(&aRect);
	} else if (_sFlags.borderType == NX_BEZEL) {
	    _NXDrawGrayBezel(&aRect, NULL, self);
	} else if (_sFlags.borderType == NX_RIDGE) {
	    _NXDrawGroove(&aRect, NULL, self);
	}
	inset = (_sFlags.borderType == NX_LINE ? 1.0 : 2.0);
	NXInsetRect(&aRect, inset, inset);
    }
    if (_sFlags.vScrollerRequired) {
	NXDivideRect(&aRect, &bRect, NX_SCROLLERWIDTH, 0);
	NXDivideRect(&aRect, &bRect, 1.0, 0);
	PSsetgray(NX_BLACK);
	NXRectFill(&bRect);
    }
    if (_sFlags.hScrollerRequired) {
	NXDivideRect(&aRect, &bRect, NX_SCROLLERWIDTH, 3);
	NXDivideRect(&aRect, &bRect, 1.0, 3);
	PSsetgray(NX_BLACK);
	NXRectFill(&bRect);
    }
    if(clipped)
	PSgrestore();
    return (self);
}



/** ScrollView Configuration **/

- (int)borderType
{
    return _sFlags.borderType;
}


- setBorderType:(int)aType
{
    if (_sFlags.borderType != aType) {
	_sFlags.borderType = aType;
	[self _update];
    }
    return self;
}


- setBackgroundGray:(float)value
{
    return ([contentView setBackgroundGray:value]);
}


- (float)backgroundGray
{
    return ([contentView backgroundGray]);
}


- setBackgroundColor:(NXColor)color
{
    return ([contentView setBackgroundColor:color]);
}


- (NXColor)backgroundColor
{
    return ([contentView backgroundColor]);
}


- setVertScrollerRequired:(BOOL)flag
{
    if (flag == _sFlags.vScrollerRequired)
	return self;

    if (_sFlags.vScrollerRequired = flag) {
	if (!vScroller)
	    [self _newScroll:YES];
    } else
	[vScroller moveTo:-100.0:-100.0];
    [self _update];

    return self;
}


- setHorizScrollerRequired:(BOOL)flag
{
    if (flag == _sFlags.hScrollerRequired)
	return self;

    if (_sFlags.hScrollerRequired = flag) {
	if (!hScroller)
	    [self _newScroll:NO];
    } else
	[hScroller moveTo:-100.0:-100.0];
    [self _update];

    return self;
}


- vertScroller
{
    return (vScroller);
}


- horizScroller
{
    return (hScroller);
}


- setVertScroller:anObject
{
    id oldScroller;
    
    [vScroller removeFromSuperview];
    oldScroller = vScroller;
    [self _commonNewScroll:anObject];
    vScroller = anObject;
    
    return(oldScroller);
}


- setHorizScroller:anObject
{
    id oldScroller;
    
    [hScroller removeFromSuperview];
    oldScroller = hScroller;
    [self _commonNewScroll:anObject];
    hScroller = anObject;
    
    return(oldScroller);
}


- _newScroll:(BOOL)vFlag
{
    static const NXRect nilRect = {{0.0, 0.0}, {0.0, 0.0}};
    NXRect              tRect;
    id                  tScroll;

    tRect = nilRect;
    if (vFlag)
	tRect.H = 100.0;
    else
	tRect.W = 100.0;
    tScroll = [Scroller allocFromZone:[self zone]];
    [tScroll initFrame:&tRect];
    [self _commonNewScroll:tScroll];
    [tScroll setArrowsPosition:(vFlag ? 0 : 1)];
    if (vFlag)
	vScroller = tScroll;
    else
	hScroller = tScroll;
    return self;
}

/*
 * An item is being moved in the text ruler.
 * dehighlight the old position if it is positive and highlight
 * the new position.
 */
- _rulerline:(NXCoord)old :(NXCoord)new last:(BOOL)last
{
NXRect	aRect;
NXPoint aPt;
NXCoord	h;
    [contentView getFrame:&aRect];
    h = aRect.size.height;
    
    [contentView lockFocus];
    if(old >= 0) {
	aPt.x = old;
	aPt.y = 0;    
	[self convertPoint:&aPt fromView:_ruler];
	NXSetRect(&aRect,aPt.x,aPt.y,1.0,h);
	[self convertRect:&aRect toView:contentView];
	NXHighlightRect(&aRect);
    }
    if(!last) {
	aPt.x = new;
	aPt.y = 0;    
	[self convertPoint:&aPt fromView:_ruler];
	NXSetRect(&aRect,aPt.x,aPt.y,1.0,h);
	[self convertRect:&aRect toView:contentView];
	NXHighlightRect(&aRect);
    }
    [contentView unlockFocus];        
    return self;
}


- _commonNewScroll:anObject
{
    [self addSubview:anObject];
#ifdef ALLOCGSTATESINSCROLLVIEWS 
    [anObject allocateGState];
#endif
    [anObject setTarget:self];
    [anObject setAction:@selector(_doScroller:)];
    return self;
}


- setLineScroll:(float)value
{
    lineAmount = value;
    return self;
}


- setPageScroll:(float)value
{
    pageContext = (value > 0.0) ? value : 0.0;
    return self;
}


/** Scrolling Behavior **/

- setCopyOnScroll:(BOOL)flag
{
    return[contentView setCopyOnScroll:flag];
}


- setDisplayOnScroll:(BOOL)flag
{
    return[contentView setDisplayOnScroll:flag];
}


- setDynamicScrolling:(BOOL)flag
{
    _sFlags.noDynamicScrolling = !flag;
    return self;
}


/** Scrolling **/

- _doScroller:scroller
{
    float               delta;
    NXRect              cBounds, dcRect;
    int                 hitPart;
    float               val;
    int                 isHoriz;

    hitPart = [scroller hitPart];

    if (hitPart == NX_KNOB && _sFlags.noDynamicScrolling)
	return self;

    isHoriz = (scroller == hScroller);

    [contentView getBounds:&cBounds];
    [contentView getDocRect:&dcRect];

    if (hitPart == NX_KNOB || hitPart == NX_JUMP) {
	val = [scroller floatValue];
	if (isHoriz)
	    cBounds.X = dcRect.X + val * (dcRect.W - cBounds.W);
	else {
	    val = [contentView isFlipped] ? val : 1.0 - val;
	    cBounds.Y = dcRect.Y + val * (dcRect.H - cBounds.H);
	}
    } else {
	if (hitPart == NX_DECLINE || hitPart == NX_INCLINE)
	    delta = lineAmount;
	else {
	    delta = (isHoriz ? cBounds.W : cBounds.H);
	    delta = (delta < pageContext) ? delta / 2.0 : delta - pageContext;
	}
	if (hitPart == NX_DECLINE || hitPart == NX_DECPAGE)
	    delta = -delta;

	if (isHoriz)
	    cBounds.X += delta;
	else
	    cBounds.Y += [contentView isFlipped] ? delta : -delta;
    }
    [contentView _scrollTo:&cBounds.origin];
    return self;
}


/** ScrollView Retiling **/

- _update
{
    [window disableDisplay];
    [self tile];
    [window reenableDisplay];
    [self display];
    return self;
}


- tile
{
    NXRect              aRect, bRect;
    NXCoord             inset;

    aRect = bounds;

    if (_sFlags.borderType != NX_NOBORDER) {
	inset = (_sFlags.borderType == NX_LINE ? 1.0 : 2.0);
	NXInsetRect(&aRect, inset, inset);
    }
    if (_sFlags.rulerInstalled) {
	NXDivideRect(&aRect, &bRect, [_ruler height], 1);
	[_ruler setFrame:&bRect];
	NXDivideRect(&aRect, &bRect, 0, 1.0);
    }
    if (_sFlags.vScrollerRequired) {
	NXDivideRect(&aRect, &bRect, NX_SCROLLERWIDTH, 0);
	[vScroller setFrame:&bRect];
	NXDivideRect(&aRect, &bRect, 1.0, 0);
    }
    if (_sFlags.hScrollerRequired) {
	NXDivideRect(&aRect, &bRect, NX_SCROLLERWIDTH, 3);
	[hScroller setFrame:&bRect];
	NXDivideRect(&aRect, &bRect, 1.0, 3);
    }
    [contentView setFrame:&aRect];
    return self;
}

- reflectScroll:cView
{
    NXRect              cRect, dcRect;
    BOOL                vNewStatus, hNewStatus;
    float               val, size;

    [cView getBounds:&cRect];
    [cView getDocRect:&dcRect];
    
    vNewStatus = (cRect.H < dcRect.H);
    hNewStatus = (cRect.W < dcRect.W);

    if (vScroller && vNewStatus != _sFlags.vScrollerStatus) {
	[vScroller setEnabled:(_sFlags.vScrollerStatus = vNewStatus)];
	[vScroller display];
    }
    if (hScroller && hNewStatus != _sFlags.hScrollerStatus) {
	[hScroller setEnabled:(_sFlags.hScrollerStatus = hNewStatus)];
	[hScroller display];
    }

    if (_sFlags.vScrollerRequired && _sFlags.vScrollerStatus) {
	val = (dcRect.H == cRect.H) ? 0.0 :
	  ((cRect.Y - dcRect.Y) / (dcRect.H - cRect.H));
	size = cRect.H / dcRect.H;
	if (![cView isFlipped])
	    val = 1.0 - val;
	[vScroller setFloatValue:val :size];
    }
    if (_sFlags.hScrollerRequired && _sFlags.hScrollerStatus) {
	val = (dcRect.W == cRect.W) ? 0.0 :
	  ((cRect.X - dcRect.X) / (dcRect.W - cRect.W));
	size = cRect.W / dcRect.W;
	[hScroller setFloatValue:val :size];
    }
    return self;
}


- read:(NXTypedStream *) stream
{
    int			version;

    [super read:stream];

    version = NXTypedStreamClassVersion(stream, "ScrollView");
    if (version == 0) {
	id dcView;
	NXRect dcRect;
	
	NXReadTypes(stream, "@@@@ffs", &vScroller, &hScroller, &contentView, &dcView, &pageContext, &lineAmount, &_sFlags);
	NXReadRect(stream, &dcRect);
	[contentView _setDocViewFromRead:dcView];
	[contentView _pinDocRect];
    } else if (version >= 1)
	NXReadTypes(stream, "@@@ffs", &vScroller, &hScroller, &contentView, &pageContext, &lineAmount, &_sFlags);

    return self;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "@@@ffs", &vScroller, &hScroller, &contentView, &pageContext, &lineAmount, &_sFlags);
    return self;
}


@end

/*
  
Modifications (starting at 0.8):
  
0.82
----
1/27/88	bs	added read: write:;
02/05/89 bgy	added support for cursor tracking. In mouseDown:, the window's
		 cursor rects are disabled before tracking, and enabled when
		 the tracking is complete. In _scrollTo:, the window is 
		 notified that the cursor rects are now invalid with the
		 invalidateCursorRectsForView: method.
 2/14/89 pah	add a way for marking a ScrollView as being used by Cell class
 3/21/89 wrp	integrated Boynton's fix to clear out scrollView when docView sizes smaller.
 3/21/89 wrp	added const to declarations
 4/02/89 wrp	added code to _update to ensure that contentView still covered docView.
 4/02/89 wrp	added code to _scroll to force scroll on device pixels

0.93
----
 6/17/89 wrp	put most all of the scrolling behavior into ClipView
 6/17/89 wrp	removed COMPILER_BUG ifdef
 6/17/89 pah	add clip rect to NXDrawGrayBezel() call
 6/26/89 wrp	implemented setHorizScroller: and setVertScroller:

0.94
----
 7/08/89 wrp	fixed drawSelf:: to do NXRectFill instead NXFrameRect for 
 		 Scroller dividers
 7/13/89 pah	ripped out resetCursorRects (it just called ClipView's version
		 which is redundant because resetCursorRect recurses down the
		 View heirarchy anyway)
0.95
----
 7/08/89 wrp	getContentSize to get frame size rather than bounds size.  This
 		 was a bug that was being masked because most ClipViews are not 
		 scaled.

77
--
 3/05/90 pah	Added support for an NX_RIDGE'd border.
 3/20/90 bgy	Support for text object's ruler. See textSelectUtil.m
 		 for a ScrollView category for more ruler support.

80
--
 4/9/90 aozer	Added backgroundColor and setBackgroundColor:.

87
--
 7/11/90 aozer	Removed gstate allocation for the scrollers. (Actually
		conditional on ALLOCGSTATESINSCROLLVIEWS; define in
		ClipView_Private.h)
		
91
--
 8/11/90 glc	Do clipped drawing so nested scrollviews will work.
 		added _rulerline for giving feedback when dragging items
		 on ruler.
		Made ruler instance private.

*/


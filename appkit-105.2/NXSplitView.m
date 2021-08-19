/*
	NXSplitView.m
  	Copyright 1989, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "Application.h"
#import "NXImage.h"
#import "Window.h"
#import "nextstd.h"
#import <objc/List.h>
#import <dpsclient/wraps.h>
#import <zone.h>

#import "NXSplitView.h"

typedef struct {
	@defs (View)
} *viewId;

static NXRect   dimpleRect = {{4.0, 5.0}, {8.0, 7.0}};
static	id	dimple = nil;

#define	DIVIDERHEIGHT 	(dimpleRect.size.height+1.0)

static int mouseHit(NXSplitView *self, NXPoint *localPoint);
static void getYBounds(NXSplitView *self, int offset, NXCoord *minY, NXCoord *maxY);
static void resizeViews(NXSplitView *self, int offset, NXCoord y, NXRect *invalRect);
static BOOL checkSubviews(NXSplitView * self);

@interface NXSplitView(BackwardCompat)
- view:sender resizeSubviews:(const NXSize *)oldSize;
@end

@implementation  NXSplitView : View

- initFrame:(const NXRect *)r
{    
    [super initFrame:r];
    [self setFlipped:YES];
    _vFlags.autoresizeSubviews = YES;
    return self;
}

- (NXCoord) dividerHeight { return DIVIDERHEIGHT; }

- write:(NXTypedStream *) stream {
    [super write:stream];
    NXWriteTypes(stream, "@", &delegate);
    return self;
}


- read:(NXTypedStream *) stream {
    [super read:stream];
    NXReadTypes(stream, "@", &delegate);
    return self;
}

- delegate { return delegate; }

- setDelegate:anObject { delegate = anObject; return self; }

- setAutoresizeSubviews:(BOOL)flag {
    // dont allow to change this flag
    return self;
}

- resizeSubviews:(const NXSize *)oldSize {
    if ([delegate respondsTo:@selector(splitView:resizeSubviews:)]) {
        [delegate splitView:self resizeSubviews:oldSize];
    } else if ([delegate respondsTo:@selector(view:resizeSubviews:)]) {
        [delegate view:self resizeSubviews:oldSize];
    } else {
	[self adjustSubviews];
    }
    if ([delegate respondsTo:@selector(splitViewDidResizeSubviews:)]) {
    	[delegate splitViewDidResizeSubviews:self];
    }
    return self;
}

- adjustSubviews {
    NXCoord	pseudoHeight = 0.0, totalHeight;
    float	percent;
    NXRect	subviewFrame;
    int		i, count;
    id		*subviewPtr;
    
    if (!(count = [subviews count])) return self;
    if (!dimple) dimple = [NXImage findImageNamed:"NXscrollKnob"];
    subviewPtr = NX_ADDRESS(subviews);
    for (i = 0; i < count; i++) {
	pseudoHeight += ((viewId)*subviewPtr)->frame.size.height;
        subviewPtr++;
    }
    [window disableDisplay];
    if (!pseudoHeight) {
        pseudoHeight = 1.0;
        NXSetRect(&subviewFrame, 0.0, 0.0, frame.size.width, 1.0);
	[[subviews objectAt:0] setFrame:&subviewFrame];
    }
    totalHeight = frame.size.height - ((count - 1) * [self dividerHeight]);
    if (totalHeight <= 0.0) totalHeight = 0.0;
    percent = totalHeight/pseudoHeight;
    subviewPtr = NX_ADDRESS(subviews);
    subviewFrame = bounds;
    for (i = 0; i < count-1; i++) {
	subviewFrame.size.height 
		= floor(percent * ((viewId)*subviewPtr)->frame.size.height);
        [*subviewPtr setFrame:&subviewFrame];
	subviewPtr++;
	subviewFrame.origin.y += subviewFrame.size.height + [self dividerHeight];
    }
    subviewFrame.size.height = NX_MAXY(&bounds) - NX_Y(&subviewFrame);
    [*subviewPtr setFrame:&subviewFrame];
    [window reenableDisplay];
    return self;
}

- drawSelf:(const NXRect *) rects :(int)rectCount {
    NXRect	dividerRect, prevFrame, nextFrame;
    int		i, count;
    id		*subviewPtr;
    
    if (!checkSubviews(self)) [self resizeSubviews:&bounds.size];
    PSsetgray(NX_LTGRAY);
    NXRectFill(rects);
    if (!(count = [subviews count])) return self;
    subviewPtr = NX_ADDRESS(subviews);
    dividerRect.origin.x = bounds.origin.x;
    dividerRect.size.width = bounds.size.width;
    [*subviewPtr getFrame:&prevFrame];
    for (i = 1; i < count; i++) {
        subviewPtr++;
	[*subviewPtr getFrame:&nextFrame];
    	dividerRect.origin.y = NX_MAXY(&prevFrame);
	dividerRect.size.height = [self dividerHeight];
	if (NXIntersectsRect(rects, &dividerRect))
	    [self drawDivider:&dividerRect];
	prevFrame = nextFrame;	  
    }
    return self;
}

- drawDivider:(const NXRect *)aRect {
    NXPoint	dimpleOrigin;
    float	dimpleHeight = NX_HEIGHT(&dimpleRect); 
   
    if (!dimple) dimple = [NXImage findImageNamed:"NXscrollKnob"];
    dimpleOrigin.x = 
    	NX_X(aRect) + floor((NX_WIDTH(aRect) - NX_WIDTH(&dimpleRect))/2);
    dimpleOrigin.y = 
    	1.0 + NX_Y(aRect) + floor((NX_HEIGHT(aRect) +
			([self isFlipped] ? dimpleHeight : -dimpleHeight))/2);
    [dimple composite:NX_COPY fromRect:&dimpleRect toPoint:&dimpleOrigin];
    return self;
}

- (BOOL) acceptsFirstMouse {
   return YES;
}

#define DRAGMASK	(NX_LMOUSEUPMASK|NX_MOUSEDRAGGEDMASK)

- mouseDown:(NXEvent *)theEvent {
    NXPoint 	localPoint;
    NXCoord	minY, maxY, y , origY, delta, lastY;
    NXRect	aRect;
    int		offset, oldMask;
    BOOL 	looping = YES;
    id 		firstResponder;
    id		*subviewPtr;
    
    localPoint = theEvent->location;
    [self convertPoint:&localPoint fromView:nil];
    if ((offset = mouseHit(self, &localPoint)) == -1) {
	[NXApp getNextEvent:NX_LMOUSEUPMASK waitFor:NX_FOREVER
					threshold:NX_MODALRESPTHRESHOLD];
        return self;
    }
    firstResponder = [window firstResponder];
    if (firstResponder) [window makeFirstResponder:self];
    [self lockFocus];
    getYBounds(self, offset, &minY, &maxY);
    PSsetinstance(YES);
    oldMask = [window eventMask];
    [window setEventMask:(oldMask | DRAGMASK)];
    subviewPtr = NX_ADDRESS(subviews);
    subviewPtr += offset;
    [*subviewPtr getFrame:&aRect];
    origY = y = NX_MAXY(&aRect);
    if (y < minY) y = minY;
    if (y > maxY) y = maxY;
    delta = y - localPoint.y;
    /* start */
    PSgsave();
    PSsetgray(NX_BLACK);
    PSsetalpha(0.3333);
    PScompositerect(0.0, y, frame.size.width, [self dividerHeight], NX_SOVER);
    lastY = y;
    while (looping) {
	theEvent = [NXApp getNextEvent:DRAGMASK waitFor:NX_FOREVER
					threshold:NX_MODALRESPTHRESHOLD];
	localPoint = theEvent->location;
	[self convertPoint:&localPoint fromView:nil];
	y = localPoint.y + delta;
	if (y < minY) y = minY;
	else if (y > maxY) y = maxY;
	if (y != lastY) {
	    PSnewinstance();
	    PScompositerect(0.0, y, frame.size.width, [self dividerHeight], NX_SOVER);
	    lastY = y;
	}
	if (theEvent->type == NX_LMOUSEUP) {
	    looping = NO;
	}
    }
    [window setEventMask:oldMask];
    PSgrestore();
    PSsetinstance(NO);
    [self unlockFocus];
    if (y != origY) {
        resizeViews(self, offset , y, &aRect);
    } else PSnewinstance();
    if (y != origY) {
        [self display:&aRect :1];
	[window invalidateCursorRectsForView:self];
    }
    if (firstResponder) [window makeFirstResponder:firstResponder];
    return self;
}

static int mouseHit(NXSplitView *self, NXPoint *localPoint) {
    id		*subviewPtr;
    NXRect	dividerRect, subviewFrame;
    int		i, count;	

    if (!(count = [self->subviews count])) return -1;
    subviewPtr = NX_ADDRESS(self->subviews);
    dividerRect.origin.x = self->bounds.origin.x;
    dividerRect.size.width = self->bounds.size.width;
    dividerRect.size.height = [self dividerHeight];
    for (i = 0; i < count; i++) {
	[*subviewPtr getFrame:&subviewFrame];
    	dividerRect.origin.y = NX_MAXY(&subviewFrame);
	if (NXPointInRect(localPoint, &dividerRect)) return i;	  
        subviewPtr++;
    }
    return -1;
}

static void getYBounds(NXSplitView *self, int offset, NXCoord *minY, NXCoord *maxY) {
    NXRect	prevFrame, nextFrame;
    NXCoord	theMin, theMax;
    id		*subviewPtr = NX_ADDRESS(self->subviews);
    
    subviewPtr += offset;
    [*subviewPtr getFrame:&prevFrame];
    subviewPtr++;
    [*subviewPtr getFrame:&nextFrame];
    *minY = theMin = NX_Y(&prevFrame);
    *maxY = theMax = NX_MAXY(&nextFrame) - [self dividerHeight];
    if (self->delegate 
        && [self->delegate respondsTo:@selector(splitView:getMinY:maxY:ofSubviewAt:)]) {
    	[self->delegate splitView:self getMinY:minY maxY:maxY ofSubviewAt:offset];
    }
    if (*minY < theMin) *minY = theMin;
    if (*maxY > theMax) *maxY = theMax;
    if (*minY > *maxY) {*minY = theMin; *maxY = theMax; }
}

static void resizeViews(NXSplitView *self, int offset, NXCoord y, NXRect *invalRect) {
    NXRect	prevFrame, nextFrame;
    NXCoord	delta;
    id		*subviewPtr = NX_ADDRESS(self->subviews);
    
    [self->window disableDisplay];
    subviewPtr += offset;
    [*subviewPtr getFrame:&prevFrame];
    [*(subviewPtr+1) getFrame:&nextFrame];
    prevFrame.size.height += (delta = y - NX_MAXY(&prevFrame));
    nextFrame.origin.y = y + [self dividerHeight];
    nextFrame.size.height -= delta;
    [*subviewPtr setFrame:&prevFrame];
    [*(subviewPtr+1) setFrame:&nextFrame];
    prevFrame.size.height += [self dividerHeight]+nextFrame.size.height;
    [self->window reenableDisplay];
    *invalRect = prevFrame;
    if ([self->delegate respondsTo:@selector(splitViewDidResizeSubviews:)]) {
    	[self->delegate splitViewDidResizeSubviews:self];
    }
}

static BOOL checkSubviews(NXSplitView * self) {
    int		i, count;
    id		*subviewPtr;
    NXCoord	x, y, w;
    NXRect	subviewFrame;
    
    if (!(count = [self->subviews count])) return YES;
    subviewPtr = NX_ADDRESS(self->subviews);
    x = self->bounds.origin.x;
    y = self->bounds.origin.y;
    w = self->bounds.size.width;
    for (i = 0; i < count-1; i++) {
        subviewFrame = ((viewId)*subviewPtr)->frame;
	if ((subviewFrame.origin.x != x)
	    || (subviewFrame.origin.y != y)
	    || (subviewFrame.size.width != w)) return NO;
	y += subviewFrame.size.height + [self dividerHeight];
	subviewPtr++;
    }
    subviewFrame = ((viewId)*subviewPtr)->frame;
    if ((subviewFrame.origin.x != x)
	|| (subviewFrame.origin.y != y)
	|| (subviewFrame.size.width != w)) return NO;
    if (subviewFrame.size.height != NX_MAXY(&self->bounds) - NX_Y(&subviewFrame))
        return NO;
    return YES;
}


@end

/*
  
Modifications (starting post-1.0):
  
77
--
 3/05/90 pah	New class for 2.0.
		 Separates all its subviews by a moveable horizontal bar.
		 Its delegate is used to control the relative sizing.
		 The divider's height is adjustable and can be drawn in any
		  way via subclassing.

84
--
 5/8/90 aozer	Converted Bitmap to NXImage.

*/



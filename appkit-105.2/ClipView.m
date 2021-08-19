/*
	ClipView.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ClipView_Private.h"
#import "View_Private.h"
#import "Window_Private.h"
#import "ScrollView.h"
#import "Application.h"
#import "Text.h"
#import "Cursor.h"
#import "color.h"
#import "nextstd.h"
#import <privateWraps.h>
#import <publicWraps.h>
#import <dpsclient/wraps.h>
#import <objc/typedstream.h>
#import <math.h>

#define	X	origin.x
#define	Y	origin.y
#define	W	size.width
#define	H	size.height

/* autoscroll amount = AUTOFACTOR * (amount mouse is outside ScrollView) */
#define	AUTOFACTOR	(4.0)

/*
 * The structure that we hang off the end of every instance of ClipView.
 * Best way to refer to this is through the private macro below.
 *
 * The structure itself is lazy; however, if we add more fields to the
 * structure then we probably won't make it lazy anymore.
 */
typedef struct _ClipViewPrivate {
    NXColor backgroundColor;
} ClipViewPrivate;

#define private ((ClipViewPrivate *)_private)

@implementation ClipView:View

+ initialize
{
    [self setVersion:3];
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    [super initFrame:frameRect];
    [self setClipping:YES];
    [self setOpaque:YES];
#ifdef ALLOCGSTATESINSCROLLVIEWS
    [self allocateGState];
#endif
    private = NXZoneCalloc([self zone], 1, sizeof(ClipViewPrivate));
    return self;
}


- _setSuperview:sView
 /* over-ride View method to cache info about superview */
{
    _clFlags._reflectScroll = [sView respondsTo:@selector(reflectScroll:)];
    _clFlags._scrollClipTo = [sView respondsTo:@selector(scrollClip:to:)];
    return ([super _setSuperview:sView]);
}


- awake
{
    _clFlags._reflectScroll = [superview respondsTo:@selector(reflectScroll:)];
    _clFlags._scrollClipTo = [superview respondsTo:@selector(scrollClip:to:)];
    return [super awake];
}

- setDocView:aView
{
    id                  oView;

    oView = docView;
    [docView notifyAncestorWhenFrameChanged:NO];
    [docView notifyWhenFlipped:NO];
 /*
  * if the docView is later freed, we won't be able to clean up because the
  * window will be nil, so we must do this here. 
  */
    [docView removeFromSuperview];

    if (docView = [self addSubview:aView]) {
	[docView notifyAncestorWhenFrameChanged:YES];
	[docView notifyWhenFlipped:YES];

	[self _alignCoords];
	[self _pinDocRect];
	[self setDrawOrigin:_docRect.X:_docRect.Y];
	if (_clFlags._reflectScroll)
	    [superview reflectScroll:self];
    }
    return (oView);
}


- docView
{
    return docView;
}


/* kludge for backwards compatibility of archive files */
- _setDocViewFromRead:aView
{
    docView = aView;
    return self;
}


- getDocRect:(NXRect *)aRect
{
    *aRect = _docRect;
    return self;
}


- getDocVisibleRect:(NXRect *)aRect
{
    [self getBounds:aRect];
    [docView convertRectFromSuperview:aRect];

    return self;
}


- _markUsedByCell
{
    _clFlags._usedByCell = YES;
    return self;
}


- (BOOL)_isUsedByCell
{
    return _clFlags._usedByCell;
}


- setBackgroundGray:(float)value
{
    backgroundGray = value;
    _clFlags.isGraySet = YES;
    return self;
}


- (float)backgroundGray
{
    return (_clFlags.isGraySet ? backgroundGray :[window backgroundGray]);
}

- (BOOL)_colorSpecified
{
    return (private && _NXIsValidColor(private->backgroundColor));
}

- setBackgroundColor:(NXColor)color
{
    if (!private) NX_ZONEMALLOC([self zone], private, ClipViewPrivate, 1);
    private->backgroundColor = color;
    if (!_clFlags.isGraySet) [self setBackgroundGray:NXGrayComponent(color)];
    return self;
}

- (NXColor)backgroundColor
{
    if (_clFlags.isGraySet) {
	return ([self _colorSpecified] ? 
			private->backgroundColor : 
			NXConvertGrayToColor([self backgroundGray]));
    } else {
	return [window backgroundColor];
    }
}

static NXCoord
NXEdgeSeparation(const NXRect *outer, const NXRect *inner, int edge)
{
    switch (edge) {
    case 0:
	return (NX_X(inner) - NX_X(outer));
    case 1:
	return (NX_Y(inner) - NX_Y(outer));
    case 2:
	return (NX_MAXX(outer) - NX_MAXX(inner));
    case 3:
	return (NX_MAXY(outer) - NX_MAXY(inner));
    }
    return (0.);	/* keep compiler happy */
}


static int
NXCalcMarginRects(const NXRect *outer, const NXRect *inner, NXRect *margin)
{
    static const int    sliceOrder[] = {1, 3, 0, 2};
    NXRect              tempRect;
    register NXRect    *tRect = &tempRect;
    register NXCoord    slice;
    register int        i;
    register int        n = 0;

    *tRect = *outer;
    for (i = 0; i < 4; i++)
	if ((slice = NXEdgeSeparation(tRect, inner, sliceOrder[i])) > 0.0)
	    NXDivideRect(tRect, &margin[n++], slice, sliceOrder[i]);
    return n;
}


- drawSelf:(const NXRect *)rects :(int)rectCount
{
    id                  dView;
    int                 graySent = NO;
    NXRect              mRects[4];
    NXRect              sRects[4];
    NXRect              dRect;
    register const NXRect *rRect;
    register NXRect    *bRect;
    register int        n, m, i;

    if (rectCount > 1) {
	rects++;
	rectCount--;
    }
    dView = docView;

    if ([dView isOpaque] || _clFlags._onlyUncovered ) {
	[dView getFrame:&dRect];
	NXIntersectionRect(&bounds, &dRect);	/* this better intersect */
	if (NXEqualRect(&bounds, &dRect))
	    return self;

	if (m = NXCalcMarginRects(&bounds, &dRect, &mRects[0]))
	    for (rRect = rects; rRect < rects + rectCount; rRect++) {
		for (n = 0, bRect = mRects; bRect < mRects + m; bRect++) {
		    sRects[n] = *bRect;
		    if (NXIntersectionRect(rRect, &sRects[n]))
			n++;
		}
		if (n) {
		    if (!graySent) {
			if (_clFlags.isGraySet) {
			    if (private && [self shouldDrawColor]) {
				NXSetColor ([self backgroundColor]);
			    } else {
				PSsetgray ([self backgroundGray]);
			    }
			} else {
			    [window _sendColor];
			}
			graySent = YES;
		    }
		    for (i = 0; i < n; i++) 
			[self centerScanRect:&sRects[i]];
		    NXRectFillList(sRects, n);
		}
	    }
    } else {
	if (_clFlags.isGraySet) {
	    if (private && [self shouldDrawColor]) {
		NXSetColor ([self backgroundColor]);
	    } else {
		PSsetgray ([self backgroundGray]);
	    }
	} else {
	    [window _sendColor];
	}
	graySent = YES;
	n = rectCount;
	for (i = 0; i < n; i++) {
	    sRects[i] = rects[i];
	    [self centerScanRect:&sRects[i]];
	}
	NXRectFillList(sRects, n);
    }
    return self;
}


- _pinDocRect
{
    NXRect              cRect;

    if (!docView)
	return self;

    [docView getFrame:&_docRect];
    cRect = bounds;
    if (cRect.W > _docRect.W)
	_docRect.W = cRect.W;
    if (cRect.H > _docRect.H)
	_docRect.H = cRect.H;

    return self;
}


- moveTo:(NXCoord)x :(NXCoord)y
{
    NXPoint aPoint;
    
    aPoint.x = x;
    aPoint.y = y;
    [superview _crackPoint:&aPoint];
    
    return ([super moveTo:aPoint.x :aPoint.y]);
}


- sizeTo:(NXCoord)width :(NXCoord)height
{
    NXRect aRect;
    
    aRect.origin = frame.origin;
    aRect.size.width = width;
    aRect.size.height = height;
    [superview _crackRect:&aRect];
    [super sizeTo:aRect.size.width :aRect.size.height];
    [self _selfBoundsChanged];
    return self;
}


- rotateTo:(NXCoord)angle
{
    return self;
}


- setDrawOrigin:(NXCoord)x :(NXCoord)y
{
    [super setDrawOrigin:x :y];
    [self _selfBoundsChanged];
    return self;
}


- setDrawSize:(NXCoord)width :(NXCoord)height
{
    [super setDrawSize:width :height];
    [self _selfBoundsChanged];
    return self;
}


- setDrawRotation:(NXCoord)angle
{
    return self;
}


- translate:(NXCoord)x :(NXCoord)y
{
    [super translate:x :y];
    if (!_drawMatrix)
	[self _crackRect:&bounds];	/* if there is no _drawMatrix, */
					/*_computeBounds will not get called */
    [self _selfBoundsChanged];
    return self;
}


- scale:(NXCoord)x :(NXCoord)y
{
    [super scale:x :y];
    [self _selfBoundsChanged];
    return self;
}


- rotate:(NXCoord)angle
{
    return self;
}


- _computeBounds
{
    [super _computeBounds];
    [self _crackRect:&bounds];
    
    return self;
}


- _selfBoundsChanged
/* clipView scale changed or clipView frame size changed (tile case) */	
{
    NXRect              cRect;
    
    if (!docView)
	return self;
	
    [self _pinDocRect];  /* this could be changed to getFrame: */
    cRect = bounds;
    if (_NXCoverRect(&cRect, &_docRect)) /* because this does a pinRect */
	[super translate:(bounds.X - cRect.X) :(bounds.Y - cRect.Y)];
    [self display];
    if (_clFlags._reflectScroll)
	[superview reflectScroll:self];
    return self;
}


- descendantFrameChanged:sender
{
    NXRect              cRect, dRect;
    NXPoint             oldOrg;
    NXPoint             newOrg;

    if (sender != docView)
	return self;

    oldOrg = _docRect.origin;
    [self _pinDocRect];
    newOrg = _docRect.origin;
    cRect = bounds;
    _NXCoverRect(&cRect, &_docRect);
    [super translate:(oldOrg.x - newOrg.x) :(oldOrg.y - newOrg.y)];
    if (![self _scrollTo:&(cRect.origin)]) {
	if (_clFlags._reflectScroll)
	    [superview reflectScroll:self];
    }
 /* make sure that uncovered portions of clipView are cleared. */
    [docView getFrame:&dRect];
    if (!NXEqualRect(&dRect, &_docRect)) {
     	if([self canDraw]) {
	    [self lockFocus];
	    _clFlags._onlyUncovered = YES;
	    [self drawSelf:&bounds:1];
	    _clFlags._onlyUncovered = NO;
	    [self unlockFocus];
	    }
	else {
	    vFlags.needsDisplay = YES;
	}
    }
    return self;
}


- descendantFlipped:sender
{
    if (sender != docView)
	return self;
    return ([self _alignCoords]);
}


- _alignCoords
{
    if ([docView isFlipped] != [self isFlipped])
	[self setFlipped:[docView isFlipped]];
    return self;
}


- autoscroll:(NXEvent *)theEvent
{
    NXPoint             thePoint;
    NXRect              cBounds;
    register NXPoint   *tp = &thePoint;
    register NXRect    *cb = &cBounds;

    *cb = bounds;
    *tp = theEvent->location;
    [self convertPoint:tp fromView:nil];

    if (tp->x < NX_X(cb))
	NX_X(cb) -= AUTOFACTOR * (NX_X(cb) - tp->x);
    else if (tp->x > NX_MAXX(cb))
	NX_X(cb) -= AUTOFACTOR * (NX_MAXX(cb) - tp->x);

    if (tp->y < NX_Y(cb))
	NX_Y(cb) -= AUTOFACTOR * (NX_Y(cb) - tp->y);
    else if (tp->y > NX_MAXY(cb))
	NX_Y(cb) -= AUTOFACTOR * (NX_MAXY(cb) - tp->y);

    return ([self _scrollTo:&(cb->origin)]);
}


- _scrollPoint:(const NXPoint *)aPoint fromView:aView
{
    NXPoint             tPoint;

    tPoint = *aPoint;
    [self convertPoint:&tPoint fromView:aView];
    return ([self _scrollTo:&tPoint]);
}


- _scrollRectToVisible:(const NXRect *)aRect fromView:aView
{
    NXRect              cBounds;
    NXRect              sRect;

    register NXRect    *cb = &cBounds;
    register NXRect    *sr = &sRect;

    *sr = *aRect;
    [self convertRect:sr fromView:aView];
    *cb = bounds;
    if (NXContainRect(cb, sr))
	return ([self _scrollTo:&cb->origin]);
    return nil;
}

- _scrollTo:(const NXPoint *)newOrigin
{
    NXRect              cBounds;
    register NXRect    *cb = &cBounds;
    NXPoint             oldOrigin;
    NXPoint             nOrigin;

    [self _pinDocRect];		/* put in for browserScroll */

    nOrigin = *newOrigin;
    [self constrainScroll:&nOrigin];
    *cb = bounds;
    oldOrigin = cb->origin;
    cb->origin = nOrigin;
    [docView convertRectFromSuperview:cb];
    [docView adjustScroll:cb];
    [docView convertRectToSuperview:cb];
    [self _crackPoint:&cb->origin];
    if (NX_X(cb) == oldOrigin.x && NX_Y(cb) == oldOrigin.y)
	return nil;

    if (_clFlags._scrollClipTo)
	[superview scrollClip:self to:&cb->origin];
    else
	[self rawScroll:&cb->origin];
    [window invalidateCursorRectsForView:docView];
    
    if (_clFlags._reflectScroll)
	[superview reflectScroll:self];
	
    return self;
}


- constrainScroll:(NXPoint *)newOrigin
{
    NXRect              cBounds;

    cBounds = bounds;

    if (newOrigin->x + cBounds.W > _docRect.X + _docRect.W)
	newOrigin->x = _docRect.X + _docRect.W - cBounds.W;
    if (newOrigin->x < _docRect.X)
	newOrigin->x = _docRect.X;

    if (newOrigin->y + cBounds.H > _docRect.Y + _docRect.H)
	newOrigin->y = _docRect.Y + _docRect.H - cBounds.H;
    if (newOrigin->y < _docRect.Y)
	newOrigin->y = _docRect.Y;

    return self;
}


- setCopyOnScroll:(BOOL)f
{
    _vFlags.noCopyOnScroll = !f;
    return self;
}


- setDisplayOnScroll:(BOOL)f
{
    _vFlags.noDisplayOnScroll = !f;
    return self;
}


#define LEFT	(-1)
#define NONE	(0)
#define RIGHT	(1)
#define DOWN	(-1)
#define UP	(1)

- rawScroll:(const NXPoint *)newOrigin
 /*
  * ??? consider using the above method calcUpdateRects rather than the code
  * below 
  */
{
 /*
  * calling routine should have error code to prevent scrolling a rotated
  * View 
  */
    register NXCoord    deltaX;	/* the amount to scroll in the x direction */
    register NXCoord    deltaY;	/* the amount to scroll in the y direction */

    register short      scrollHoriz;	/* scroll LEFT, NONE, or RIGHT */
    register short      scrollVert;	/* scroll DOWN, NONE, or UP */

    NXPoint             destPoint;	/* coord of dest origin */
    NXRect              sourceRect;
    NXRect              updateRects[3];	/* upUnion, upHoriz, upVert, source */
    register NXRect    *source;
    register NXRect    *upUnion;
    register NXRect    *upHoriz;
    register NXRect    *upVert;
    register NXPoint   *dest;
    int                 updateCount;
    NXRect		visRect;
    NXCoord		transDeltaX, transDeltaY;

 /* compute scroll amount, early exit if no scroll needed */
    deltaX = bounds.origin.x - newOrigin->x;
    deltaY = bounds.origin.y - newOrigin->y;

    scrollHoriz = (deltaX > 0.0) ? RIGHT : ((deltaX == 0.0) ? NONE : LEFT);
    scrollVert = (deltaY > 0.0) ? UP : ((deltaY == 0.0) ? NONE : DOWN);

    if (!(scrollHoriz || scrollVert))
	return self;

    if (![self canDraw] || ![self getVisibleRect:&visRect]) {
	[super translate:deltaX :deltaY];
	return self;
    }
    
    visRect.origin.x -= deltaX;		/* adjust for future translate */
    visRect.origin.y -= deltaY;
    
    transDeltaX = deltaX;
    transDeltaY = deltaY;
    
 /* use absolute values for deltas */
    if (scrollHoriz == LEFT)
	deltaX = -deltaX;
    if (scrollVert == DOWN)
	deltaY = -deltaY;

 /* optimize for entire area needs update */
    if (_vFlags.noCopyOnScroll || deltaX >= visRect.size.width || deltaY >= visRect.size.height) {
	[self lockFocus];
	[super translate:transDeltaX :transDeltaY];
	if (_vFlags.noDisplayOnScroll) {
	    [self drawSelf:&visRect :1];
	    [self invalidate:&visRect :1];
	} else
	    [self _display:&visRect :1];
	[self unlockFocus];
	[window flushWindow];
	return self;
    }
    source = &sourceRect;
    dest = &destPoint;
    upUnion = &updateRects[0];
    upHoriz = upUnion + 1;
    upVert = upHoriz + 1;

 /* compute update rectangles, source rectangle and destination point */
    *dest = visRect.origin;
    *source = *upUnion = *upVert = *upHoriz = visRect;
    if (scrollHoriz) {
	source->size.width -= deltaX;
	upVert->size.width = deltaX;
	if (scrollHoriz == LEFT) {
	    source->origin.x += deltaX;
	    upVert->origin.x += source->size.width;
	} else
	    dest->x += deltaX;
    }
    if (scrollVert) {
	source->size.height -= deltaY;
	upHoriz->size.height = deltaY;
	if (scrollHoriz)
	    upVert->size.height = source->size.height;
	if (scrollVert == DOWN) {
	    source->origin.y += deltaY;
	    upHoriz->origin.y += source->size.height;
	} else {
	    dest->y += deltaY;
	    if (scrollHoriz)
		upVert->origin.y += deltaY;
	}
    }
 /* scroll the bits, clear the update rectangles and force redraw */
    [self lockFocus];
    {
      /* optimization to keep gstate from being copied just yet */
	 int temp = _gState; 
	 _gState = 0;
	[super translate:transDeltaX :transDeltaY];
	 _gState = temp;
    }

 /* centerScan source and dest to compensate for postscript over-scanning */
    if (vFlags.rotatedOrScaledFromBase) {
	[self centerScanRect:source];
	[self _centerScanPoint:dest];
    }
    
    {
	NXPoint deviceVector;		/* to keep halftone aligned */
	
	deviceVector.x = dest->x - source->origin.x;
	deviceVector.y = dest->y - source->origin.y;
	if (vFlags.rotatedOrScaledFromBase) {
	    NXPoint originPt;
	    originPt.x = originPt.y = 0.0;
	    [self convertPoint:&deviceVector toView:nil];
	    [self convertPoint:&originPt toView:nil];
	    deviceVector.x -= originPt.x;
	    deviceVector.y -= originPt.y;
	    deviceVector.y = -deviceVector.y;
	} else if (!vFlags.needsFlipped)
	    deviceVector.y = -deviceVector.y;
	    
	_NXScroll(NX_X(source), NX_Y(source), 
		  NX_WIDTH(source), NX_HEIGHT(source),
		  dest->x, dest->y,
		  deviceVector.x, deviceVector.y); 
	if (_gState)			/* Now we can copy the gstate */
	    NXCopyCurrentGState(_gState);
    }
    updateCount = 3;

    if (!scrollVert) {
	upUnion = upVert;
	updateCount = 1;
    }
    if (!scrollHoriz){
	upUnion = upHoriz;
	updateCount = 1;
    }
    if (_vFlags.noDisplayOnScroll) {
	[self drawSelf:upUnion :updateCount];
	[self invalidate:upUnion :updateCount];
    } else
	[self _display:upUnion :updateCount];
	
    [window flushWindow];
    [self unlockFocus];

    return self;
}


- setDocCursor:anObj
{
    id                  oldCursor;

    oldCursor = cursor;
    cursor = anObj;
    [window invalidateCursorRectsForView:self];
    return oldCursor;
}


- resetCursorRects
{
    NXRect              cRect;

    if (cursor) {
	[self getVisibleRect:&cRect];
	[self addCursorRect:&cRect cursor:cursor];
    }
    return self;
}

/* ??? should read and write cursor */

- read:(NXTypedStream *) s
{
    int			version;
    BOOL		isGraySet;

    [super read:s];
    private = NXZoneCalloc([self zone], 1, sizeof(ClipViewPrivate));
    if (NXSystemVersion(s) < 900)
	return self;
	
    version = NXTypedStreamClassVersion(s, "ClipView");
    if (version == 0) {
	NXReadTypes(s, "fc", &backgroundGray, &isGraySet);
	_clFlags.isGraySet = isGraySet;
    } else if (version >= 1) {
	NXReadTypes(s, "@fs", &docView, &backgroundGray, &_clFlags);
	[self _pinDocRect];
	if (version >= 2) {
	    NXColor tmpColor = NXReadColor(s);
	    if (_NXIsValidColor(tmpColor)) {
		[self setBackgroundColor:tmpColor];
	    }
	}
	if (version >= 3) {
	    NXReadTypes(s, "@", &cursor);
	}
    }
    if ([docView isKindOf:[Text class]]) {
	[self setDocCursor:NXIBeam];
    }

    return self;
}


- write:(NXTypedStream *) s
{
    [super write:s];
    NXWriteTypes(s, "@fs", &docView, &backgroundGray, &_clFlags);
    /*
     * Write the color out only if it is defined and is different
     * than the gray. 
     */
    if ([self _colorSpecified] && 
	!_NXIsPureGray(private->backgroundColor, backgroundGray)) {
	NXWriteColor(s, private->backgroundColor);
    } else {
	NXWriteColor(s, _NXNoColor());
    }
    NXWriteTypes(s, "@", &cursor);
    return self;
}

- free
{
    NX_FREE(private);
    return [super free];
}

@end

/*
  
Modifications (starting at 0.8):
  
 3/21/89 wrp	added const to declarations
 
0.93
----
 6/17/89 wrp	made into public class after substantial restructuring of 
 		 ScrollView
 6/24/89 wrp	fixed halftone stitching bug by calling NXCopyCurrentGState
 		 after the scroll operation.  Also optimized out the call to
		 NXCopyCurrentGState that translate:: would do.
 6/26/89 wrp	made rawScroll: public and added owner method scrollClip:to:
 		 to allow implementation of coordinated scrolling Views ala
		 Workspace tableView.

0.94
----
 7/13/89 pah	fixed setDocCursor: so that it simply invalidates the rects
		 and lets Application reset the cursor next time through the
		 run loop

 7/14/89 wrp	added check to descendantFrameChanged: which ensures that there 
 		 will be something drawn before doing a lockFocus

0.96
----
 7/24/89 pah	ripped out _viewFreeing call in setDocView: (I believe that
		 it is redundant)

83
--
 4/30/90 aozer	Added backgroundColor support. Bumped version to 2.

87
--
 7/11/90 aozer	Removed gstate allocation for the clipview. (Actually
		conditional on ALLOCGSTATESINSCROLLVIEWS; define in
		ClipView_Private.h)

91
--
 8/12/90 aozer	Changed isOnColorScreen -> shouldDrawColor

94
--
 9/25/90 gcockrft  	Write cursor when archiving.	
 			Set needsDisplay in descendantFrameChanged when
			we can't draw. 
*/


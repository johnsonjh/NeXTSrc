/*
	View.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB


#import "appkitPrivate.h"
#import "View_Private.h"
#import "Application_Private.h"
#import "PrintInfo_Private.h"
#import "PrintPanel_Private.h"
#import "Window_Private.h"
#import "Font_Private.h"
#import <objc/List.h>
#import "Window.h"
#import "PSMatrix.h"
#import "ScrollView.h"
#import "NXFaxPanel.h"
#import "PageLayout.h"
#import "Listener.h"
#import "Speaker.h"
#import "FocusState.h"
#import "Alert.h"
#import "nextstd.h"
#import "errors.h"
#import <defaults.h>
#import "printSupport.h"
#ifndef OLD_SPOOLER
#import "npd.h"
#endif
#import "privateWraps.h"
#import "publicWraps.h"
#import "packagesWraps.h"
#import <objc/Storage.h>
#import <dpsclient/wraps.h>
#import <objc/error.h>
#import <stdio.h>
#import <sys/param.h>
#import <sys/file.h>
#import <sys/message.h>
#import <servers/netname.h>
#import <string.h>
#import <sys/ioctl.h>
#import <nextdev/npio.h>
#import <NXCType.h>
#import <zone.h>
#import <time.h>

#ifndef SHLIB
    /* this references category .o files, forcing them to be linked into an app which is linked against a archive version of the kit library. */
    asm(".reference .NXViewCategoryIconDragging\n");
#endif

typedef struct {
    @defs (Window)
} WindowId;

#define WINNUMBER(x) (( (WindowId *) (x))->windowNum)

#define PRINT_PACKAGE		"/usr/lib/NextStep/printPackage.ps"
#define NEXT_PRINT_PACKAGE	"/usr/lib/NextStep/nlpPrintPackage.ps"

static id focusState = nil;	/* The object which reduces focusing PS */

static void autoSizeView(int coeff, NXCoord oldL, NXCoord newL, NXCoord *x, NXCoord *w);

@implementation View:Responder

 /*
  * translatedDraw is only valid if there is no _drawMatrix. drawInSuperview
  * is only valid if there is no _drawMatrix and no _frameMatrix.
  * needsFlipped applies independent of transformation matrices, so is always
  * valid. drawInSuperview and needsFlipped are independent. 
  */

/** Instantiation **/

+ initialize
{
    NXZone *zone = [NXApp zone];
    if (!zone) zone = NXDefaultMallocZone();
    if (!focusState) focusState = [[FocusState allocFromZone:zone] init];
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[super allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}

- initFrame:(const NXRect *)frameRect
{
    [super init];
    if (frameRect) {
	frame = *frameRect;
	bounds.size = frameRect->size;
    }
    vFlags.needsDisplay = YES;
    [self _commonAwake];
    return self;
}


+ new
{
    return [self newFrame:NULL];
}

- init
{
    return [self initFrame:NULL];
}


- awake
{
    [super awake];
    [self _commonAwake];
    return self;
}


- _commonAwake
{
    _vFlags.specialClip = [View instanceMethodFor:@selector(clipToFrame:)] !=
    			  [self methodFor:@selector(clipToFrame:)];
    _vFlags.wantsGState = NO;
    return self;
}


/* Automatic I/O */

- free
{
    id children;

    children = subviews;		/* snip whole list from view tree, */
    subviews = nil;			/* preventing recursive list removal */
    [children freeObjects];
    [children free];
    [window _viewFreeing:self];
    [window invalidateCursorRectsForView:self];
    [[superview subviews] removeObject:self];	/* this only needed for root */
    [self _removeFreedViewFromFocusStack];
    [_frameMatrix free];
    [_drawMatrix free];
    [self freeGState];
    return [super free];
}


- freeAndUnlink
{
    return ([self free]);
}


- _setWindow:theWindow
{
    [self windowChanged:theWindow];
    if (!theWindow && window) 
	[window _viewDetaching:self];
    window = theWindow;
    if (subviews)
	[subviews makeObjectsPerform:@selector(_setWindow:) with :theWindow];
    return self;
}


- window
{
    return window;
}


/** Ownership Hierarchy **/

- superview
{
    return superview;
}


- subviews
{
    return subviews;
}


- (BOOL)isDescendantOf:aView
{
    View	      *tView;

    for (tView = self; tView; tView = tView->superview)
	if (tView == aView)
	    return YES;
    return NO;
}


- findAncestorSharedWith:aView
{
    View	       *v;
    id                  retVal = nil;

    if (aView == nil)
	return nil;

 /* ??? Consider putting an ASSERT here */
    if (window != [aView window])
	return nil;

    for (v = aView; v; v = v->superview)
	v->_vFlags.mark = YES;

    for (v = self; v; v = v->superview)
	if (v->_vFlags.mark) {
	    retVal = v;
	    break;
	}
    for (v = aView; v; v = v->superview)
	v->_vFlags.mark = NO;

    return (retVal);
}


- opaqueAncestor
{
    View	      *ancestor;

    for (ancestor = self; ancestor->superview; ancestor = ancestor->superview)
	if (ancestor->vFlags.opaque)
	    break;

    return ancestor;
}


/*??? check to make sure that aView is not the contentView of its window */
- addSubview:aView
{
    if (![aView isKindOf:[View class]])
	return nil;

    [aView _setSuperview:self];
    [aView _setWindow:window];
    [aView setNextResponder:self];
    [aView _alreadyFlipped:vFlags.needsFlipped];
    [aView _setRotatedFromBase:vFlags.rotatedFromBase];
    [aView _setRotatedOrScaledFromBase:vFlags.rotatedOrScaledFromBase];
    [window invalidateCursorRectsForView:self];
    return aView;
}


- addSubview:aView :(int)place relativeTo:otherView
{
    unsigned            index;
    id                  retval;

    if (otherView == nil)
	index = (unsigned)(-1);
    else
	index = [subviews indexOf:otherView];
    if (index == (unsigned)(-1)) {
	if (place == NX_BELOW)
	    index = 0;
    } else if (place == NX_ABOVE)
	index++;

    if (retval = [self addSubview:aView])
	if (index != (unsigned)(-1)) {
	    [subviews removeObject:aView];
	    [subviews insertObject:aView at:index];
	}
    return retval;
}

- windowChanged:newWindow
{
    return self;
}

- removeFromSuperview
{
    [window invalidateCursorRectsForView:self];
    [self _setSuperview:nil];
    [self _setWindow:nil];
    [self setNextResponder:nil];
    return self;
}


- replaceSubview:oldView with:newView
{
    unsigned            index;

    if (!subviews)
	return nil;
    index = [subviews indexOf:oldView];
    if (index == (unsigned)(-1))
	return nil;

    if (oldView == newView)
	return oldView;
    else if ([self addSubview:newView]) {
	[subviews removeObject:newView];
	[subviews insertObject:newView at:index];
	[oldView removeFromSuperview];
	return oldView;
    } else
	return nil;
}


- _setSuperview:sView
{
    [superview _removeSubview:self];
    superview = sView;
    [superview _addSubview:self];
    return self;
}


- _addSubview:aView
{
    if (subviews == nil) {
	NXZone *zone = [self zone];
	subviews = [[List allocFromZone:zone] initCount:0];
    }
    [subviews addObject:aView];
    return self;
}


- _removeSubview:aView
{
    [aView _invalidateGStates];
    [subviews removeObject:aView];
    return self;
}


/* Notification bits */
/*
	Notify Off -		_vFlags.ancestorNotifyWasEnabled = 0
				_vFlags.needsAncestorNotify = 0
	Notify On -		_vFlags.ancestorNotifyWasEnabled = 0
				_vFlags.needsAncestorNotify = 1
	Notify Suspended -	_vFlags.ancestorNotifyWasEnabled = 1
				_vFlags.needsAncestorNotify = 0
	Notify Suspended/Dirty -_vFlags.ancestorNotifyWasEnabled = 1
				_vFlags.needsAncestorNotify = 1
*/

- notifyAncestorWhenFrameChanged:(BOOL)flag
{
    _vFlags.ancestorNotifyWasEnabled = NO;
    _vFlags.needsAncestorNotify = flag;
    return self;
}


- suspendNotifyAncestorWhenFrameChanged:(BOOL)flag
{
    BOOL                notifyEnabled;

    if (_vFlags.needsAncestorNotify || _vFlags.ancestorNotifyWasEnabled) {
	notifyEnabled = _vFlags.needsAncestorNotify && !_vFlags.ancestorNotifyWasEnabled;
	if (notifyEnabled == flag) {
	    if (_vFlags.needsAncestorNotify && _vFlags.ancestorNotifyWasEnabled)
		[superview descendantFrameChanged:self];
	    _vFlags.needsAncestorNotify = !notifyEnabled;
	    _vFlags.ancestorNotifyWasEnabled = notifyEnabled;
	}
    }
    return self;
}


- notifyWhenFlipped:(BOOL)flag
{
    _vFlags.notifyWhenFlipped = flag;
    return self;
}


- descendantFrameChanged:sender

 /*
  * Note: cannot pass oldFrame here, because this notification can be
  * suspended and when it is unsuspended, a dirty bit may trigger this
  * message. Previous oldFrames are already gone. This should not really be a
  * problem. 
  */
{
    return ([superview descendantFrameChanged:sender]);
}


- descendantFlipped:sender
{
    return ([superview descendantFlipped:sender]);
}

- resizeSubviews:(const NXSize *)oldSize
{
    [subviews makeObjectsPerform:@selector(superviewSizeChanged:) with :(id)oldSize];
    return self;
}

#define XSIZING_BITS(mask)	((mask) & 7)
#define YSIZING_BITS(mask)	(((mask) >> 3) & 7)
	/* these depend on resizing constants in View.h */

- _setNoVerticalAutosizing:(BOOL)flag
{
    vFlags._noVerticalAutosizing = flag ? YES : NO;
    return self;
}

- superviewSizeChanged:(const NXSize *)oldSize
{
 /*
  * Assumes that there is a minimum size for the view and that the subviews
  * are inside the bounds of the superview. Otherwise these computations are
  * wrong. Should be used when resizing a window with a minimum size. See jmh
  * for more details. 
  */
    NXSize              newOffset, newSize;
    BOOL                flipped;
    int			sizingBits;

    newOffset.width = frame.origin.x - ((View *)superview)->bounds.origin.x;
    newOffset.height = frame.origin.y - ((View *)superview)->bounds.origin.y;

    flipped = [superview isFlipped];
    newSize = frame.size;
    sizingBits = _vFlags.autosizing;
    if (((View *)superview)->vFlags._noVerticalAutosizing) sizingBits &= XSIZING_BITS(077);

    autoSizeView(XSIZING_BITS(sizingBits),
		 oldSize->width, ((View *)superview)->bounds.size.width,
		 &newOffset.width, &newSize.width);

    autoSizeView(YSIZING_BITS(sizingBits),
		 oldSize->height, ((View *)superview)->bounds.size.height,
		 &newOffset.height, &newSize.height);

    newOffset.width += ((View *)superview)->bounds.origin.x;
    newOffset.height += ((View *)superview)->bounds.origin.y;

    if ((newOffset.width != frame.origin.x) ||
	(newOffset.height != frame.origin.y))
	[self moveTo:newOffset.width:newOffset.height];

    if ((newSize.width != frame.size.width) ||
	(newSize.height != frame.size.height))
	[self sizeTo:newSize.width:newSize.height];
    return self;
}


static void autoSizeView(int coeff, NXCoord oldL, NXCoord newL, NXCoord *x, NXCoord *w)
{
    float               prop;
    NXCoord             tmp;

    switch (coeff) {
    case NX_NOTSIZABLE:
    case NX_MAXXMARGINSIZABLE:
	break;
    case NX_WIDTHSIZABLE:
	*w = newL - (oldL - *w);
	if (*w < 0.0)
	    *w = 0.0;
	break;
    case NX_WIDTHSIZABLE | NX_MAXXMARGINSIZABLE:
	if (*w)
	    prop = *w / (oldL - *x);
	else
	    prop = 1.0;
	*w = floor((newL - *x) * prop);
	if (*w < 0.0)
	    *w = 0.0;
	break;
    case NX_MINXMARGINSIZABLE:
	*x = newL - (oldL - *x);
	if (*x < 0.0)
	    *x = 0.0;
	break;
    case NX_MINXMARGINSIZABLE | NX_MAXXMARGINSIZABLE:
	if (*x)
	    prop = *x / (oldL - *w);
	else
	    prop = 1.0;
	*x = floor((newL - *w) * prop);
	if (*x < 0.0)
	    *x = 0.0;
	break;
    case NX_MINXMARGINSIZABLE | NX_WIDTHSIZABLE:
	tmp = *x + *w;
	if (tmp)
	    prop = *x / tmp;
	else
	    prop = 0.5;
	*x = floor((newL - (oldL - tmp)) * prop);
	*w = newL - (*x + (oldL - tmp));
	if (*x < 0.0)
	    *x = 0;
	if (*w < 0.0)
	    *w = 0;
	break;
    case NX_MINXMARGINSIZABLE | NX_WIDTHSIZABLE | NX_MAXXMARGINSIZABLE:
	if (oldL)
	    prop = *x / oldL;
	else
	    prop = 0.333;
	*x = floor(newL * prop);
	if (oldL)
	    prop = *w / oldL;
	else
	    prop = 0.333;
	*w = floor(newL * prop);
	break;
    }
}


- setAutoresizeSubviews:(BOOL)flag
{
    _vFlags.autoresizeSubviews = flag;
    return self;
}


- setAutosizing:(unsigned int)mask
{
    _vFlags.autosizing = mask;
    return self;
}


/** View Geometry **/

- moveTo:(NXCoord)x :(NXCoord)y
{
    register NXPoint   *fp = &frame.origin;
    if (fp->x == x && fp->y == y) {
    	/*
	 * On a resize of nested views the position may not change relative to the 
	 * superview, but the absolute position may have changed. Flush gstates.
	 */
	vFlags.validGState = NO;
	return self;
	}

    fp->x = x;
    fp->y = y;
    if (vFlags.drawInSuperview)
	bounds.origin = *fp;

    [self _invalidateFocus];

    if (_vFlags.ancestorNotifyWasEnabled)
	_vFlags.needsAncestorNotify = YES;
    else if (_vFlags.needsAncestorNotify)
	[superview descendantFrameChanged:self];

    return self;
}



- sizeTo:(NXCoord)width :(NXCoord)height
{
    register NXSize    *sp = &frame.size;
    NXSize              oldSize;

    if (sp->width == width && sp->height == height)
	return self;
    oldSize = bounds.size;

    sp->width = width;
    sp->height = height;
    if (!_drawMatrix)
	bounds.size = *sp;
    else			/* wrp 6/24/88 */
	[self _computeBounds];	/* wrp 6/24/88 */

    if (!vFlags.noClip || _drawMatrix)
	[self _invalidateFocus];

    if (_vFlags.autoresizeSubviews && !_drawMatrix && subviews)	/* bounds changed */
	[self resizeSubviews:&oldSize];

    if (_vFlags.ancestorNotifyWasEnabled)
	_vFlags.needsAncestorNotify = YES;
    else if (_vFlags.needsAncestorNotify)
	[superview descendantFrameChanged:self];

    return self;
}


- setFrame:(const NXRect *)frameRect
{
    [self moveTo:frameRect->origin.x:frameRect->origin.y];
    [self sizeTo:frameRect->size.width:frameRect->size.height];
    return self;
}


- rotateTo:(NXCoord)angle
{
    if (!_frameMatrix) {
	NXZone *zone = [self zone];
	_frameMatrix = [[PSMatrix allocFromZone:zone] init];
	[_frameMatrix _doRotationOnly];
	vFlags.drawInSuperview = NO;
	[self _setRotatedFromBase:YES];
    }
    [_frameMatrix rotateTo:angle];

    [self _invalidateFocus];

    return self;
}


- moveBy:(NXCoord)deltaX :(NXCoord)deltaY
{
    return ([self moveTo:frame.origin.x + deltaX:frame.origin.y + deltaY]);
}


- sizeBy:(NXCoord)deltaWidth :(NXCoord)deltaHeight
{
    return ([self sizeTo:frame.size.width + deltaWidth:frame.size.height + deltaHeight]);
}


- rotateBy:(NXCoord)deltaAngle
{
    return ([self rotateTo:[self frameAngle] + deltaAngle]);
}


- getFrame:(NXRect *)theRect
{
    *theRect = frame;
    return self;
}


- (float)frameAngle
{
    return (_frameMatrix ?[_frameMatrix getRotationAngle] : 0.0);
}


- setDrawOrigin:(NXCoord)x :(NXCoord)y
{
    if (_drawMatrix) {
	[_drawMatrix translateTo:-x:-y];
	[self _computeBounds];
    } else {
	bounds.origin.x = x;
	bounds.origin.y = y;
	vFlags.translatedDraw = (bounds.origin.x != 0.0 ||
				 bounds.origin.y != 0.0);
    }

    [self _invalidateFocus];
    return self;
}


- setDrawSize:(NXCoord)width :(NXCoord)height
{
    [[self _drawMatrix] scaleTo:(frame.size.width / width)
      :(frame.size.height / height)];
    [self _computeBounds];

    [self _invalidateFocus];
    return self;
}


- setDrawRotation:(NXCoord)angle
{
    [[self _drawMatrix] rotateTo:angle];
    [self _setRotatedFromBase:YES];
    [self _computeBounds];

    [self _invalidateFocus];
    return self;
}


- translate:(NXCoord)x :(NXCoord)y
{
    if (x == 0.0 && y == 0.0)
	return self;

    if (_drawMatrix) {
	[_drawMatrix translate:x :y];
	[self _computeBounds];
    } else {
	bounds.origin.x -= x;
	bounds.origin.y -= y;
	vFlags.translatedDraw = (bounds.origin.x != 0.0 ||
				 bounds.origin.y != 0.0);
    }
    if ([self isFocusView]) {
	PStranslate(x, y);
	if (_gState)
	    NXCopyCurrentGState(_gState);
	[self _invalidateSubviewsFocus];
    } else
	[self _invalidateFocus];

    return self;
}


- scale:(NXCoord)x :(NXCoord)y
{
    if (x == 1.0 && y == 1.0)
	return self;

    [[self _drawMatrix] scale:x :y];
    [self _computeBounds];

    if ([self isFocusView]) {
	PSscale(x, y);
	if (_gState)
	    NXCopyCurrentGState(_gState);
	[self _invalidateSubviewsFocus];
    } else
	[self _invalidateFocus];

    return self;
}


- rotate:(NXCoord)angle
{
    if (angle == 0.0)
	return self;

    [[self _drawMatrix] rotate:angle];
    [self _setRotatedFromBase:YES];
    [self _computeBounds];

    if ([self isFocusView]) {
	PSrotate(angle);
	if (_gState)
	    NXCopyCurrentGState(_gState);
	[self _invalidateSubviewsFocus];
    } else
	[self _invalidateFocus];

    return self;
}



- getBounds:(NXRect *)theRect
{
    *theRect = bounds;
    return self;
}


- (float)boundsAngle
{
    if (_drawMatrix)
	return ([_drawMatrix getRotationAngle]);
    else
	return (0.0);
}

static void
subviewsPerform(aView, aSelector, aFlag)
    View	       *aView;
    SEL                 aSelector;
    BOOL                aFlag;
{
    NXListId           *subviews;
    register id        *this, *last;

    subviews = (NXListId *) aView->subviews;
    this = subviews->dataPtr;
    last = this + subviews->numElements;
    for (; this < last; this++)
	if (*this)
	    objc_msgSend(*this, aSelector, aFlag);
}



- setFlipped:(BOOL)flag
{
    if (vFlags.needsFlipped == flag)
	return self;

    vFlags.needsFlipped = flag;

    if (subviews)
	subviewsPerform(self, @selector(_alreadyFlipped:), flag);

    [self _invalidateFocus];

    if (_vFlags.notifyWhenFlipped)
	[superview descendantFlipped:self];

    return self;
}

- setFlip:(BOOL)flag
{
    return [self setFlipped:flag];
}

- (BOOL)isFlipped
{
    return (vFlags.needsFlipped);
}


- _setRotatedFromBase:(BOOL)f
{
    if (vFlags.rotatedFromBase == f)
	return self;

    if (vFlags.rotatedFromBase = f || [_drawMatrix rotated] || [_frameMatrix rotated])
	vFlags.rotatedOrScaledFromBase = YES;

    if (subviews)
	subviewsPerform(self, @selector(_setRotatedFromBase:), vFlags.rotatedFromBase);
    return self;
}


- _setRotatedOrScaledFromBase:(BOOL)f
{
    if (vFlags.rotatedOrScaledFromBase == f)
	return self;

    vFlags.rotatedOrScaledFromBase = f || _drawMatrix || _frameMatrix;

    if (subviews)
	subviewsPerform(self, @selector(_setRotatedOrScaledFromBase:), vFlags.rotatedOrScaledFromBase);
    return self;
}


- (BOOL)isRotatedFromBase
{
    return (vFlags.rotatedFromBase);
}


- (BOOL)isRotatedOrScaledFromBase
{
    return (vFlags.rotatedOrScaledFromBase);
}


- _alreadyFlipped:(BOOL)state
{
    vFlags.alreadyFlipped = state;
    return self;
}


- setOpaque:(BOOL)flag
{
    vFlags.opaque = flag;
    return self;
}


- (BOOL)isOpaque
{
    return (BOOL)vFlags.opaque;
}


- _drawMatrix
{
    if (!_drawMatrix) {
	NXZone *zone = [self zone];
	_drawMatrix = [[PSMatrix allocFromZone:zone] init];
	if (vFlags.translatedDraw)
	    [_drawMatrix translate:-bounds.origin.x:-bounds.origin.y];
	vFlags.drawInSuperview = NO;
	[self _setRotatedOrScaledFromBase:YES];

    }
    return _drawMatrix;
}


- _computeBounds
{
    bounds.size = frame.size;
    bounds.origin.x = bounds.origin.y = 0.0;
    [_drawMatrix invTransformRect:&bounds];
    return self;
}


/** View Computation **/

- convertPointFromSuperview:(NXPoint *)aPoint
{
    [self _convertPointFromSuperview:(NXPoint *)aPoint test:NO];
    return self;
}

- (BOOL)_convertPointFromSuperview:(NXPoint *)aPoint test:(BOOL)testFlag
 /*
  * Converts a point to the coordinate system of the receiving View from the
  * coordinate system of the receiving View's superview. 
  *
  * If testFlag is YES, this method first does hit testing to see whether the
  * point lies within the receiving View's frame rectangle.  If it does,
  * the point is converted and YES is returned.  If it doesn't, the point
  * is left unconverted and NO is returned. 
  *
  * If testFlag is NO, the point is converted and YES is returned regardless
  * of where the point is located. 
  */
{
    if (vFlags.drawInSuperview) {
	if (testFlag)
	    if (!NXMouseInRect(aPoint, &frame, vFlags.alreadyFlipped))
		return NO;
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aPoint->y = (frame.origin.y + frame.size.height) -
	      (aPoint->y - frame.origin.y);
    } else {
	aPoint->x -= frame.origin.x;
	aPoint->y -= frame.origin.y;

	if (_frameMatrix)
	    [_frameMatrix invTransform:aPoint];

	if (testFlag) {
	    NXRect              fRect;

	    fRect.origin.x = fRect.origin.y = 0.0;
	    fRect.size = frame.size;

	    if (!NXMouseInRect(aPoint, &fRect, vFlags.alreadyFlipped))
		return NO;
	}
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aPoint->y = frame.size.height - aPoint->y;

	if (_drawMatrix)
	    [_drawMatrix invTransform:aPoint];
	else if (vFlags.translatedDraw) {
	    aPoint->x += bounds.origin.x;
	    aPoint->y += bounds.origin.y;
	}
    }
    return YES;
}


- convertPointToSuperview:(NXPoint *)aPoint
{
    if (vFlags.drawInSuperview) {
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aPoint->y = (frame.origin.y + frame.size.height) -
	      (aPoint->y - frame.origin.y);
    } else {
	if (_drawMatrix)
	    [_drawMatrix transform:aPoint];
	else if (vFlags.translatedDraw) {
	    aPoint->x -= bounds.origin.x;
	    aPoint->y -= bounds.origin.y;
	}
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aPoint->y = frame.size.height - aPoint->y;

	if (_frameMatrix)
	    [_frameMatrix transform:aPoint];

	aPoint->x += frame.origin.x;
	aPoint->y += frame.origin.y;
    }
    return (self);
}



- convertRectFromSuperview:(NXRect *)aRect
{
    [self _convertRectFromSuperview:aRect test:NO];
    return self;
}


- (BOOL)_convertRectFromSuperview:(NXRect *)aRect test:(BOOL)testFlag
 /*
  * Converts a rectangle to the coordinate system of the receiving View from
  * the coordinate system of the receiving View's superview. 
  *
  * If testFlag is YES, this method first checks to see whether the rectangle
  * intersects with (has any area in common with) the receiving View's
  * frame rectangle.  If it does, the rectangle is converted and YES is
  * returned.  If it doesn't, the rectangle is left unconverted and NO is
  * returned. 
  *
  * If testFlag is NO, the rectangle is converted and YES is returned
  * regardless of whether the rectangles intersect. 
  */
{
    if (vFlags.drawInSuperview) {
	if (testFlag)
	    if (!NXIntersectionRect(&frame, aRect))
		return NO;
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aRect->origin.y = (frame.origin.y + frame.size.height) -
	      ((aRect->origin.y + aRect->size.height) - frame.origin.y);
    } else {
	aRect->origin.x -= frame.origin.x;
	aRect->origin.y -= frame.origin.y;

	if (_frameMatrix)
	    [_frameMatrix invTransformRect:aRect];

	if (testFlag) {
	    NXRect              fRect;

	    fRect.origin.x = fRect.origin.y = 0.0;
	    fRect.size = frame.size;

	    if (!NXIntersectionRect(&fRect, aRect))
		return NO;
	}
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aRect->origin.y = frame.size.height -
	      (aRect->origin.y + aRect->size.height);

	if (_drawMatrix)
	    [_drawMatrix invTransformRect:aRect];
	else if (vFlags.translatedDraw) {
	    aRect->origin.x += bounds.origin.x;
	    aRect->origin.y += bounds.origin.y;
	}
    }
    return YES;
}


- convertRectToSuperview:(NXRect *)aRect
{
    if (vFlags.drawInSuperview) {
	if (vFlags.alreadyFlipped != vFlags.needsFlipped)
	    aRect->origin.y = (frame.origin.y + frame.size.height) -
	      ((aRect->origin.y + aRect->size.height) - frame.origin.y);
    } else {
	if (_drawMatrix)
	    [_drawMatrix transformRect:aRect];
	else if (vFlags.translatedDraw) {
	    aRect->origin.x -= bounds.origin.x;
	    aRect->origin.y -= bounds.origin.y;
	}
	if (vFlags.alreadyFlipped != vFlags.needsFlipped) {
	    aRect->origin.y = frame.size.height -
	      (aRect->origin.y + aRect->size.height);
	}
	if (_frameMatrix)
	    [_frameMatrix transformRect:aRect];

	aRect->origin.x += frame.origin.x;
	aRect->origin.y += frame.origin.y;
    }
    return (self);
}


- convertPoint:(NXPoint *)aPoint fromView:aView
{
    id                  ancestor;

    ancestor = [self findAncestorSharedWith:aView];
    [aView _convertPoint:aPoint toView:ancestor];
    [self _convertPoint:aPoint fromView:ancestor];

    return self;
}


- convertPoint:(NXPoint *)aPoint toView:aView
{
    id                  ancestor;

    ancestor = [self findAncestorSharedWith:aView];
    [self _convertPoint:aPoint toView:ancestor];
    [aView _convertPoint:aPoint fromView:ancestor];

    return self;
}


- _convertPoint:(NXPoint *)aPoint fromView:aView
{
    if (self != aView) {
	[superview _convertPoint:aPoint fromView:aView];
	[self convertPointFromSuperview:aPoint];
    }
    return self;
}


- _convertPoint:(NXPoint *)aPoint toView:aView
{
    if (self != aView) {
	[self convertPointToSuperview:aPoint];
	[superview _convertPoint:aPoint toView:aView];
    }
    return self;
}


/* forces result to be positive */
static void convertSize(src, dest, aSize)
    View	       *src, *dest;
    NXSize             *aSize;
{
    _NXPureConvertSize(src, dest, aSize);
    if (aSize->width < 0)
	aSize->width = -aSize->width;
    if (aSize->height < 0)
	aSize->height = -aSize->height;
}


/* doesnt force result to be positive */
void _NXPureConvertSize(src, dest, aSize)
    View	       *src, *dest;
    NXSize             *aSize;
{
    NXPoint             vectorBase, vectorTail;

    if (src == dest)
	return;

    vectorBase.x = vectorBase.y = 0.0;
    vectorTail.x = aSize->width;
    vectorTail.y = aSize->height;

    if (src && src->superview == dest) {
	[src convertPointToSuperview:&vectorBase];
	[src convertPointToSuperview:&vectorTail];
    } else if (dest && dest->superview == src) {
	[dest convertPointFromSuperview:&vectorBase];
	[dest convertPointFromSuperview:&vectorTail];
    } else if (src == nil) {
	[dest convertPoint:&vectorBase fromView:nil];
	[dest convertPoint:&vectorTail fromView:nil];
    } else {
	[src convertPoint:&vectorBase toView:dest];
	[src convertPoint:&vectorTail toView:dest];
    }
    aSize->width = vectorTail.x - vectorBase.x;
    aSize->height = vectorTail.y - vectorBase.y;
}


- convertSize:(NXSize *)aSize fromView:aView
{
    convertSize(aView, self, aSize);
    return self;
}


- convertSize:(NXSize *)aSize toView:aView
{
    convertSize(self, aView, aSize);
    return self;
}


static void
transformRect(src, dest, aRect, xform)
    id                  src, dest;
    NXRect             *aRect;
    SEL                 xform;
{
    NXPoint             corners[4];
    register NXCoord    minX;
    register NXCoord    maxX;
    register NXCoord    minY;
    register NXCoord    maxY;
    register int        i;
    register NXPoint   *pp;
    register NXRect    *rp;

    rp = aRect;
    corners[0].x = corners[1].x = NX_X(rp);
    corners[2].x = corners[3].x = corners[0].x + NX_WIDTH(rp);
    corners[0].y = corners[3].y = NX_Y(rp);
    corners[1].y = corners[2].y = corners[0].y + NX_HEIGHT(rp);

    pp = &corners[0];
    objc_msgSend(src, xform, pp, dest);
    minX = maxX = pp->x;
    minY = maxY = pp->y;

    for (i = 3; i--;) {
	pp++;
	objc_msgSend(src, xform, pp, dest);
	if (pp->x < minX)
	    minX = pp->x;
	else if (pp->x > maxX)
	    maxX = pp->x;
	if (pp->y < minY)
	    minY = pp->y;
	else if (pp->y > maxY)
	    maxY = pp->y;
    }

    NX_X(rp) = minX;
    NX_WIDTH(rp) = maxX - minX;
    NX_Y(rp) = minY;
    NX_HEIGHT(rp) = maxY - minY;
}


- convertRect:(NXRect *)aRect fromView:aView
{
    transformRect(self, aView, aRect, @selector(convertPoint:fromView:));
    return self;
}


- convertRect:(NXRect *)aRect toView:aView
{
    transformRect(self, aView, aRect, @selector(convertPoint:toView:));
    return self;
}


static float uniRound(float x)
{
    return (x > 0.0 ? (floor(x + 0.5)) : (-ceil(-(x + 0.5))));
}


- centerScanRect:(NXRect *)aRect
{
    NXCoord             min, max;

    if (vFlags.rotatedOrScaledFromBase)
	[self convertRect:aRect toView:nil];
    if (NX_WIDTH(aRect) > 0.0 && NX_HEIGHT(aRect) > 0.0) {
	min = uniRound(NX_X(aRect)) + 0.5;
	max = uniRound(NX_MAXX(aRect)) - 0.5;
	if ((max - min) <= 0.0) {
	    NX_X(aRect) = uniRound(NX_X(aRect) + (NX_WIDTH(aRect)/2.0) - 0.5) + 0.4;
	    NX_WIDTH(aRect) = 0.2;
	} else {
	    NX_X(aRect) = min;
	    NX_WIDTH(aRect) = max - min;
	}
	min = uniRound(NX_Y(aRect)) + 0.5;
	max = uniRound(NX_MAXY(aRect)) - 0.5;
	if ((max - min) <= 0.0) {
	    NX_Y(aRect) = uniRound(NX_Y(aRect) + (NX_HEIGHT(aRect)/2.0) - 0.5) + 0.4;
	    NX_HEIGHT(aRect) = 0.2;
	} else {
	    NX_Y(aRect) = min;
	    NX_HEIGHT(aRect) = max - min;
	}
    }
    if (vFlags.rotatedOrScaledFromBase)
	[self convertRect:aRect fromView:nil];
    return self;
}


- _centerScanPoint:(NXPoint *)aPoint
{
    if (vFlags.rotatedOrScaledFromBase)
	[self convertPoint:aPoint toView:nil];
    aPoint->x = uniRound(aPoint->x) + 0.5;
    aPoint->y = uniRound(aPoint->y) + ((vFlags.rotatedOrScaledFromBase && vFlags.needsFlipped) ? -0.5 : 0.5);
    if (vFlags.rotatedOrScaledFromBase)
	[self convertPoint:aPoint fromView:nil];
    return self;
}


- _crackRect:(NXRect *)aRect
{
    NXCoord             minx, miny, maxx, maxy;

    if (vFlags.rotatedOrScaledFromBase)
	[self convertRect:aRect toView:nil];
    minx = uniRound(NX_X(aRect));
    miny = uniRound(NX_Y(aRect));
    maxx = uniRound(NX_MAXX(aRect));
    maxy = uniRound(NX_MAXY(aRect));
    NX_X(aRect) = minx;
    NX_Y(aRect) = miny;
    NX_WIDTH(aRect) = maxx - minx;
    NX_HEIGHT(aRect) = maxy - miny;
    if (vFlags.rotatedOrScaledFromBase)
	[self convertRect:aRect fromView:nil];
    return self;
}


- _crackPoint:(NXPoint *)aPoint
{
    if (vFlags.rotatedOrScaledFromBase)
	[self convertPoint:aPoint toView:nil];
    aPoint->x = uniRound(aPoint->x);
    aPoint->y = uniRound(aPoint->y);
    if (vFlags.rotatedOrScaledFromBase)
	[self convertPoint:aPoint fromView:nil];
    return self;
}



/** Drawing Support **/

- (BOOL)canDraw
{
    return ((window != nil) && ((WindowId *) window)->windowNum > 0 &&
	    !(((WindowId *) window)->_displayDisabled));
}

- setAutodisplay:(BOOL)flag
{
    if (vFlags.disableAutodisplay == flag) {
	vFlags.disableAutodisplay = !flag;
	if (flag && vFlags.needsDisplay)
	    [self display];
    }
    return self;
}


- (BOOL)isAutodisplay
{
    return (!vFlags.disableAutodisplay);
}


- setNeedsDisplay:(BOOL)flag
{
    if (vFlags.disableAutodisplay)
	vFlags.needsDisplay = flag;
    return self;
}


- (BOOL)needsDisplay
{
    return (vFlags.needsDisplay);
}


- update
{
    if (vFlags.disableAutodisplay)
	vFlags.needsDisplay = YES;
    else
	[self display];
    return self;
}


- drawInSuperview
{
    if (!(_frameMatrix || _drawMatrix)) {
	bounds.origin = frame.origin;
	vFlags.drawInSuperview = YES;
	vFlags.translatedDraw = (bounds.origin.x != 0.0 ||
				 bounds.origin.y != 0.0);
    }
    return self;
}


- (int)gState
{
    return _gState;
}


- _setBorderViewGState:(int)gs
 /* sets the gState for the _borderView */
{
    _gState = gs;
    vFlags.validGState = YES;	/* _borderView is not ever supposed to be
				 * invalid */
    return self;
}


- allocateGState
{
 /* Allocate later */
    _vFlags.wantsGState = YES;
    return self;
}


- freeGState
{
    _vFlags.wantsGState = NO;
    vFlags.newGState = YES;
    if (_gState) {
	_NXNullifyGState(_gState);
	DPSUndefineUserObject(_gState);
	_gState = 0;
    }
    return self;
}


- notifyToInitGState:(BOOL)flag
{
    _vFlags.notifyToInitGState = flag;
    return self;
}


- initGState
{
    return self;
}


- renewGState
{
    vFlags.newGState = YES;
    return self;
}


- _clipToFrame:(const NXRect *)frameRect
{
    if (_vFlags.specialClip)
	[focusState flush];
    [self clipToFrame:frameRect];
    return self;
}


- clipToFrame:(const NXRect *)frameRect
{
    [focusState clip:frameRect];
    return self;
}



- _focusDown:(BOOL)clipNeeded
 /*
  * You never call this method.  This method is part of View and Window's
  * focusView machinery.  This method is called by Window's pushFocus
  * which is in turn called by View's focusView method. 
  *
  * However, for the terminally curious, what this method does is to move the
  * graphics state from this view's superview to this view. 
  */
{
    register NXRect    *f = &frame;
    register NXRect    *b = &bounds;
    NXRect              normalizedFrame;

    normalizedFrame.origin.x = normalizedFrame.origin.y = 0.0;
    normalizedFrame.size = f->size;

 /*
  * if we are the top level View of a printing display, the focusing
  * identifies the frame coords of the view with that of the page. 
  */
    if (NXDrawingStatus != NX_DRAWING &&
	[[NXApp printInfo] _privateData]->specialPrintFocus) {
	if (!vFlags.noClip && clipNeeded)
	    [self _clipToFrame:&normalizedFrame];
	if (_drawMatrix) {
	    if (vFlags.needsFlipped) {
		[focusState translate:0.0 :NX_HEIGHT(f)];
		[focusState scale:1.0 :-1.0];
	    }
	    [focusState concat:_drawMatrix];
	} else {
	    if (vFlags.needsFlipped) {
		[focusState translate:-NX_X(b) :NX_HEIGHT(f) + NX_Y(b)];
		[focusState scale:1.0 :-1.0];
	    } else if (NX_X(b) != 0.0 || NX_Y(b) != 0.0)
		[focusState translate:-NX_X(b) :-NX_Y(b)];
	}
    } else if (vFlags.drawInSuperview) {
	if (!vFlags.noClip && clipNeeded)
	    [self _clipToFrame:f];
	if (vFlags.alreadyFlipped != vFlags.needsFlipped) {
	    [focusState translate:0.0 :NX_Y(f) + NX_HEIGHT(f) + NX_Y(f)];
	    [focusState scale:1.0 :-1.0];
	}
    } else if (_frameMatrix || _drawMatrix) {
	if (NX_X(f) != 0.0 || NX_Y(f) != 0.0)
	    [focusState translate:NX_X(f) :NX_Y(f)];

	if (_frameMatrix)
	    [focusState concat:_frameMatrix];

	if (!vFlags.noClip && clipNeeded)
	    [self _clipToFrame:&normalizedFrame];

	if (vFlags.alreadyFlipped != vFlags.needsFlipped) {
	    [focusState translate:0.0 :NX_HEIGHT(f)];
	    [focusState scale:1.0 :-1.0];
	}
	if (_drawMatrix)
	    [focusState concat:_drawMatrix];
	else if (vFlags.translatedDraw)
	    [focusState translate:-NX_X(b) :-NX_Y(b)];
    } else {
	if (!vFlags.noClip && clipNeeded)
	    [self _clipToFrame:f];

	if (vFlags.alreadyFlipped != vFlags.needsFlipped) {
	    [focusState translate:NX_X(f) - NX_X(b) :NX_Y(f) + NX_HEIGHT(f) + NX_Y(b)];
	    [focusState scale:1.0 :-1.0];
	} else if (NX_X(f) != NX_X(b) || NX_Y(f) != NX_Y(b))
	    [focusState translate:NX_X(f) - NX_X(b) :NX_Y(f) - NX_Y(b)];
    }
    return self;
}


static void
_focusFromAncestor(oldView, newView)
    View	      *oldView, *newView;

 /* oldView must be an ancestor of newView */
{
    if (newView != oldView) {
	_focusFromAncestor(oldView, newView->superview);
	[newView _focusDown:YES];

	if (NXDrawingStatus == NX_DRAWING) {
	    if (newView->_vFlags.wantsGState && !newView->_gState) {
		[focusState flush];
		PSgstate();
		newView->_gState = DPSDefineUserObject(0);
		newView->vFlags.newGState = YES;
		newView->vFlags.validGState = YES;

	    } else if (newView->_gState && !newView->vFlags.validGState) {
		[focusState flush];
		NXCopyCurrentGState(newView->_gState);
		newView->vFlags.newGState = YES;
		newView->vFlags.validGState = YES;
	    }
	} else
	    newView->vFlags.newGState = YES;

	if (newView->vFlags.newGState && newView->_vFlags.notifyToInitGState) {
	    [newView initGState];
	    if (NXDrawingStatus == NX_DRAWING)
		NXCopyCurrentGState(newView->_gState);
	    newView->vFlags.newGState = NO;
	}
    }
}

static void
_focusFromTo(oldView, newView)
    register id         oldView;
    register id         newView;
{
    View	      *tView;

    if (oldView == newView)
	return;

    if (NXDrawingStatus == NX_DRAWING) {
    /* find closest ancestor with valid gState or the current focus */
	for (tView = newView; tView; tView = tView->superview)
	    if (tView == oldView ||
		(tView->_gState && tView->vFlags.validGState))
		break;
	if (tView && tView != oldView)
	    NXSetGState(tView->_gState);
	else if (!tView)	/* must have dropped off because no window */
	    _NXSetNullGState();
    } else {
    /* see if current focus is an ancestor */
	for (tView = newView; tView; tView = tView->superview)
	    if (tView == oldView)
		break;
    }
    [focusState reset];
    _focusFromAncestor(tView, newView);
    [focusState flush];
}

static void
_focusFromAncestorAux(View *newView)
{
    if (newView) {
	_focusFromAncestorAux(newView->superview);
	[newView _focusDown:NO];
    }
}

- _genBaseMatrix
{
    NXRect *f = &frame;

    [focusState reset];

    /*
     * Focus from the top view down to the superview of this view.
     */
    _focusFromAncestorAux(self->superview);

    /*
     * The following chunk of code (up to the PSgsave()) is from
     * _focusDown: and allows us to focus on the frame of this view.
     * This is necessary because when printing a view we start the focusing
     * stuff on the draw coords of this view, bypassing the frame coords.
     */
    if (!vFlags.drawInSuperview) {
	if (NX_X(f) != 0.0 || NX_Y(f) != 0.0) {
	    [focusState translate:NX_X(f) :NX_Y(f)];
	}
	if (_frameMatrix) {
	    [focusState concat:_frameMatrix];
	}
    }
    if (vFlags.alreadyFlipped) {
	[focusState translate:0.0 :NX_HEIGHT(f)];
	[focusState scale:1.0 :-1.0];
    }

    PSgsave();
    [focusState sendInv];
    DPSPrintf(DPSGetCurrentContext(), " /__NXbasematrix matrix currentmatrix def\n");
    PSgrestore();
    return self;
}

extern NXHandler *_NXAddAltHandler(void (*proc)(void *data, int code,
					  void *data1, void *data2),
				   void *context);
extern void _NXRemoveAltHandler(NXHandler *errorData);


static void
unFocus(void *data, int code, void *data1, void *data2)
{
    id                  focusStack;

    focusStack = [NXApp _focusStack];
    [focusStack removeLastElement];
    if ([focusStack count]) {
	PSgrestore();
	[((id)(data)) _fixInvalidatedFocus];
    }
}


- (BOOL)lockFocus
{
    id                  focusStack, oldFocus;
    focusStackElem	fse;
    PrivatePrintInfo   *privPInfo = NULL;

    NX_ASSERT(window, "No window during lockFocus on a view");
    NX_ASSERT([window windowNum] > 0, "No REAL window during lockFocus on a view");
    NX_PSDEBUG();
 /*
  * If this is the top-level focus of a printing job, we only focus from the
  * superview to the top-level print view. 
  */
    focusStack = [NXApp _focusStack];
    if (NXDrawingStatus != NX_DRAWING &&
	(privPInfo = [[NXApp printInfo] _privateData])->specialPrintFocus)
	oldFocus = superview;
    else
	oldFocus = [NXApp focusView];
    if ([focusStack count])	/* gsave only necessary for nested calls */
	PSgsave();
    _focusFromTo(oldFocus, self);
    
    fse.valid = YES;
    fse.view = self;
    fse.errorData = _NXAddAltHandler(&unFocus, self);
    [focusStack addElement:&fse];
    
    if (NXDrawingStatus != NX_DRAWING)
	privPInfo->specialPrintFocus = NO;
    return (oldFocus == self);
}


- unlockFocus
{
    int i;
    focusStackElem *fsp;
    id focusStack, unlockedView;

    unlockedView = [NXApp focusView];
    NX_ASSERT(unlockedView == self, "Unlocking Focus on wrong View");
    
    focusStack = [NXApp _focusStack];
    fsp = (focusStackElem *)[focusStack elementAt:([focusStack count] - 1)];
    _NXRemoveAltHandler(fsp->errorData);

    NX_PSDEBUG();
    unFocus(self, 0, NULL, NULL);
    
    if ([window _wantsToDestroyRealWindow]) {
	for (i = [focusStack count]-1; i >= 0; i--) {
	    fsp = (focusStackElem *)[focusStack elementAt:i];
	    if ([fsp->view window] == window) break;
	}
	if (i < 0) [window _destroyRealWindow:NO];
    }

    return self;
}


- (BOOL)isFocusView
{
    return ([NXApp focusView] == self);
}


- _mark:(BOOL)flag
{
 /* n.b. the _borderView should always have a valid gstate */
    _vFlags.mark = flag;
    if (flag && _gState)
	vFlags.validGState = NO;
    if (subviews)
	subviewsPerform(self, @selector(_mark:), flag);
    return self;
}


static void
nukeFocusStack()
{
    register id         focusStack;
    register int        i, size;
    focusStackElem	*fsp;

    focusStack = [NXApp _focusStack];
    size = [focusStack count];

    for (i = 0; i < size; i++) {
	fsp = (focusStackElem *)[focusStack elementAt:i];
	if (((View *)(fsp->view))->_vFlags.mark)
	    fsp->valid = NO;
    }
}


- _invalidateGStates
{
    id         focusStack;
    int        i, size;
    focusStackElem	*fsp;

    [self _mark:YES];
    
    focusStack = [NXApp _focusStack];
    size = [focusStack count];

    for (i = 0; i < size; i++) {
	fsp = (focusStackElem *)[focusStack elementAt:i];
	NX_ASSERT(!(((View *)(fsp->view))->_vFlags.mark), "You removed a View from the View hierarchy that had been lockFocus'ed");
    }
    [self _mark:NO];

    return self;
}


- _invalidateFocus
{
    if (!superview)
	return ([self _invalidateSubviewsFocus]);

    [self _mark:YES];
    nukeFocusStack();
    [self _mark:NO];
    [self _fixInvalidatedFocus];

    return self;
}


- _invalidateSubviewsFocus
{
    if (subviews)
	subviewsPerform(self, @selector(_mark:), YES);
    nukeFocusStack();
    if (subviews)
	subviewsPerform(self, @selector(_mark:), NO);

    return self;
}


- _fixInvalidatedFocus
{
    id                  focusStack, newFocus;
    focusStackElem	*fsp;

    if (!(newFocus = [NXApp focusView]))
	return self;

    focusStack = [NXApp _focusStack];
    fsp = (focusStackElem *)[focusStack elementAt:([focusStack count] - 1)];
    if (!fsp->valid) {
	_focusFromTo(nil, newFocus);
	fsp->valid = YES;
    }
    return self;
}


- _removeFreedViewFromFocusStack
{
    id                  focusStack;
    int	                i;
    focusStackElem	*fsp;
    
    focusStack = [NXApp _focusStack];
    
    for (i = [focusStack count] - 1; i >= 0; i--) {
	fsp = (focusStackElem *)[focusStack elementAt:i];
	if (fsp->view == self)
	    [focusStack removeAt:i];
    }
    return self;
}


- setClipping:(BOOL)flag
{
    vFlags.noClip = !flag;
    return self;
}


- (BOOL)doesClip
{
    return (vFlags.noClip ? 0 : 1);
}


- (int)_clip:(const NXRect *)superClips :(int)superCount :(NXRect *)selfClips
 /*
  * You never call this method.  This method is called from _display.  It
  * converts superCount rects from superClips and puts them into the clips
  * array and returns count.  In some cases, count will be less than
  * superCount. (for example, when self.bounds does not intersect one of
  * the rects in superClips) 
  */
{
    int                 count = 0;
    int                 sCount = superCount;
    register NXRect    *theRect = selfClips;
    register int        i;

    do {
	*theRect = *superClips++;

	if ([self _convertRectFromSuperview:theRect test:YES]) {
	    count++;
	    theRect++;
	}
    } while (count && --sCount);	/* drop out if first rect no sect */

    if (count == 1) {
	if (superCount != 1)
	    count = 0;		/* only sect 1st of 3 rects */
    } else {
	selfClips[0] = selfClips[1];
	for (i = 2; i < count; i++) {
	    NXUnionRect(&selfClips[i], &selfClips[0]);
	}
	if (count == 2)
	    count = 1;
    }
    return count;
}



- (BOOL)getVisibleRect:(NXRect *)theRect
{
 /*
  * If we're printing, cut the recursion at the top view in the printing
  * chain, and return the current rect we are trying to print.  This is
  * analogous to returning the window's bounds in the non-printing case. 
  */
    if (NXDrawingStatus != NX_DRAWING) {
	PrivatePrintInfo   *privPInfo = [[NXApp printInfo] _privateData];

	if (self == privPInfo->currentPrintingView) {
	    *theRect = privPInfo->currentPrintingRect;
	    return YES;
	}
    }
    if (superview == nil) {
	*theRect = bounds;	/* start from window's view.bounds */
	return YES;
    } else if ([superview getVisibleRect:theRect])
	return ([self _convertRectFromSuperview:theRect test:YES]);
    else
	return NO;
}


- _display:(const NXRect *)rects :(int)rectCount
 /*
  * You never call this method.  This method is called by display:: with
  * rects an array of rectangles in self coordinates and rectCount, the
  * number of valid rectangles in rects (1 or 3).  
  */
{
    NX_PSDEBUG();
    [self drawSelf:rects :rectCount];
    if (subviews) {
	register int        s;
	register id        *vp;
	NXRect              svClips[3];
	int                 svNumClips;

	if (s = [subviews count])
	    for (vp = NX_ADDRESS(subviews); s--; vp++)
		if (svNumClips = [*vp _clip:rects :rectCount :svClips]) {
		    [*vp lockFocus];
		    [*vp _display:svClips :svNumClips];
		    [*vp unlockFocus];
		}
    }
    vFlags.needsDisplay = NO;
    vFlags._hasDirtySubview = NO;
    return self;
}


static BOOL
hasDirtySubview(View *view)
{
    int                 s;
    id                 *subviews;

    view->vFlags._hasDirtySubview = view->vFlags.needsDisplay;
    
    if (!view->vFlags._hasDirtySubview) 
	if (s = [view->subviews count]) 
	    for (subviews = NX_ADDRESS(view->subviews); s--; subviews++) 
		if (hasDirtySubview(*subviews))
		    view->vFlags._hasDirtySubview = YES;
		    
    return (view->vFlags._hasDirtySubview);
}


static void
displayIfNeeded(View *view)
{
    int                 s;
    View              **subviews;
    NXRect		visRect;

    if (view->vFlags.needsDisplay) {
	if ([view getVisibleRect:&visRect]);
	    [view _display:&visRect :1]; 
    } else 
	if (s = [view->subviews count]) 
	    for (subviews = NX_ADDRESS(view->subviews); s--; subviews++)
		if ((*subviews)->vFlags._hasDirtySubview) {
		    [*subviews lockFocus];
		    displayIfNeeded(*subviews);
		    [*subviews unlockFocus];
		}
}

static BOOL optimizeDrawingDefault = 0;
#define NEVER_OPTIMIZE -1
#define ALWAYS_OPTIMIZE 2

- (BOOL)_canOptimizeDrawing
{
    if (!optimizeDrawingDefault) {
	const char *defValue = NXGetDefaultValue([NXApp appName], "NXOptimizeDrawing");
	optimizeDrawingDefault = defValue ? (!strcmp(defValue, "NEVER") ? NEVER_OPTIMIZE : (!strcmp(defValue, "ALWAYS") ? ALWAYS_OPTIMIZE : 1)) : 1;
    }
    return (optimizeDrawingDefault != NEVER_OPTIMIZE && !vFlags.rotatedOrScaledFromBase &&
	    (optimizeDrawingDefault == ALWAYS_OPTIMIZE || [window _canOptimizeDrawing]));
}

- (BOOL)_optimizedRectFill:(const NXRect *)rect gray:(float)gray
{
    NXRect newRect;

    if (!rect || (gray != (float)NX_DKGRAY && gray != (float)NX_WHITE && gray != (float)NX_BLACK && gray != (float)NX_LTGRAY) ||
	![self _canOptimizeDrawing]) {
	return NO;
    } else {
	newRect = *rect;
	[self convertRectToSuperview:&newRect];
	return [superview _optimizedRectFill:&newRect gray:gray];
    }
}

- (BOOL)_optimizedXYShow:(const char *)text numChars:(int)numChars at:(NXCoord)x :(NXCoord)y
{
    NXPoint p;

    if (!text || ![self _canOptimizeDrawing]) {
	return NO;
    } else {
	p.x = x; p.y = y;
	[self convertPointToSuperview:&p];
	return [superview _optimizedXYShow:text numChars:numChars at:p.x :p.y];
    }
}

- (BOOL)_setOptimizedXYShowFont:font gray:(float)gray
{
    if (!font || ![self _canOptimizeDrawing]) {
	return NO;
    } else {
	return [superview _setOptimizedXYShowFont:font gray:gray];
    }
}


- displayIfNeeded
{
    if ([self canDraw]) {
	if (hasDirtySubview(self)) {
	    [self lockFocus];
	    displayIfNeeded(self);
	    [window flushWindow];
	    [self unlockFocus];
	}
    }
    return self;
}


- display:(const NXRect *)rects :(int)rectCount :(BOOL)clipFlag
{
    int                 i;
    NXRect              newClips[3];
    NXRect		visRect;

    if (![self getVisibleRect:&visRect])
	return self;
	
    if (!window || ((WINNUMBER(window)) < 0)
	|| (![window isDisplayEnabled] && (NXDrawingStatus == NX_DRAWING))) {
	vFlags.needsDisplay = YES;
	return self;
    }
    if (!rects || !rectCount) {
	newClips[0] = visRect;
	rectCount = 1;
	if (vFlags.noClip)
	    clipFlag = NO;
    } else {
	if (rectCount == 1) {
	    newClips[0] = rects[0];
	    if (!NXIntersectionRect(&visRect, &newClips[0]))
		return self;
	} else {
	    rectCount = (rectCount > 3) ? 3 : rectCount;
	    for (i = 1; i < rectCount; i++) {
		newClips[i] = rects[i];
		NXIntersectionRect(&visRect, &newClips[i]);
		if (i == 1)
		    newClips[0] = newClips[1];
		else
		    NXUnionRect(&newClips[i], &newClips[0]);
	    }
	    if (NXEmptyRect(&newClips[0]))
		return self;
	}
    }
    [self lockFocus];
    
    if (clipFlag) {
	if (rectCount == 1)
	    NXRectClip(newClips);
	else
	    NXRectClipList(&newClips[1], rectCount - 1);
    }
    [self _display:newClips :rectCount];

    [window flushWindow];	/* flush inside lockFocus for less ps */
    [self unlockFocus];

    return self;
}


- displayFromOpaqueAncestor:(const NXRect *)rects :(int)rectCount :(BOOL)clipFlag
{
    NXRect              _tRect[3];
    NXRect             *tRect = _tRect;
    int                 i;
    register id         dView;

    if (!rects || !rectCount) {
	if (![self getVisibleRect:&_tRect[0]])
	    return self;
	    
	rects = tRect;
	rectCount = 1;
	if (vFlags.noClip)
	    clipFlag = NO;
    }
    dView = [self opaqueAncestor];
    if (dView != self) {
	clipFlag = YES;
	if (rectCount == 1) {
	    tRect[0] = rects[0];
	    [dView convertRect:&tRect[0] fromView:self];
	} else {
	    if (rectCount > 3)
		rectCount = 3;
	    for (i = 1; i < rectCount; i++) {
		tRect[i] = rects[i];
		[dView convertRect:&tRect[i] fromView:self];
		if (i == 1)
		    tRect[0] = tRect[1];
		else
		    NXUnionRect(&tRect[i], &tRect[0]);
	    }
	}
	rects = tRect;
    } 
    return ([dView display:rects :rectCount :clipFlag]);
}


- display:(const NXRect *)rects :(int)rectCount
{
    return ([self display:rects :rectCount :NO]);
}


- display
{
    return ([self display:(NXRect *)0 :0 :NO]);
}


- drawSelf:(const NXRect *)rects :(int)rectCount
{
    return self;
}


 /* Scrolling Support */

- (float)backgroundGray
{
    return (-1.0);
}


/* ??? Backwards Compatibility Methods - leave out of spec sheets - rip out soon */
- scrollPoint:(const NXPoint *)aPoint fromView:aView
{
    return ([superview _scrollPoint:aPoint fromView:aView]);
}


- _scrollPoint:(const NXPoint *)aPoint fromView:aView
{
    return ([superview _scrollPoint:aPoint fromView:aView]);
}


- scrollPoint:(const NXPoint *)aPoint
{
    return ([self _scrollPoint:aPoint fromView:self]);
}

/* ??? Backwards Compatibility Methods - leave out of spec sheets - rip out soon */
- scrollRectToVisible:(const NXRect *)aRect fromView:aView
{
    return ([superview _scrollRectToVisible:aRect fromView:aView]);
}


- _scrollRectToVisible:(const NXRect *)aRect fromView:aView
{
    return ([superview _scrollRectToVisible:aRect fromView:aView]);
}


- scrollRectToVisible:(const NXRect *)aRect
{
    return ([self _scrollRectToVisible:(NXRect *)aRect fromView:self]);
}


- autoscroll:(NXEvent *)theEvent
{
    return ([superview autoscroll:theEvent]);
}


- adjustScroll:(NXRect *)newVisible
{
    return self;
}


- (BOOL)calcUpdateRects:(NXRect *)rects :(int *)rectCount :(NXRect *)enclRect :(NXRect *)goodRectArg
{
    NXRect goodRectSpace;
    NXRect *goodRect;
    BOOL minx, maxx, miny, maxy;	/* TRUE if edge aligns */

    rects[0] = *enclRect;
    *rectCount = 1;

    if (_NXGetShlibVersion() <= MINOR_VERS_1_0)
	goodRect = goodRectArg;		/* smash that parameter, a 1.0 bug */
    else {
	goodRect = &goodRectSpace;
	*goodRect = *goodRectArg;
    }
    if (!NXIntersectionRect(enclRect, goodRect))
	return YES;

    minx = NX_X(enclRect) == NX_X(goodRect);
    maxx = NX_MAXX(enclRect) == NX_MAXX(goodRect);
    miny = NX_Y(enclRect) == NX_Y(goodRect);
    maxy = NX_MAXY(enclRect) == NX_MAXY(goodRect);
  /* make sure goodRect and enclRect share a horiz and vert edge */
    if ((!minx && !maxx) || (!miny && !maxy))
	return YES;

    if (minx && maxx && miny && maxy) {	/* rects equal */
	*rectCount = 0;
	return NO;	/* goodRectArg covers enclRect */
    }

  /* calc full width piece */
    rects[1].origin.x = NX_X(enclRect);
    rects[1].origin.y = maxy ? NX_Y(enclRect) : NX_MAXY(goodRect);
    rects[1].size.width = NX_WIDTH(enclRect);
    rects[1].size.height = NX_HEIGHT(enclRect) - NX_HEIGHT(goodRect);

  /* calc partial width piece */
    rects[2].origin.x = maxx ? NX_X(enclRect) : NX_MAXX(goodRect);
    rects[2].origin.y = NX_Y(goodRect);
    rects[2].size.height = NX_HEIGHT(goodRect);
    rects[2].size.width = NX_WIDTH(enclRect) - NX_WIDTH(goodRect);

    AK_ASSERT(rects[1].size.height != 0.0 || rects[2].size.width != 0.0, "Two unexpected empty rects in calcUpdateRects::::");
    if (rects[1].size.height == 0.0) {
	rects[0] = rects[2];
	*rectCount = 1;
    } else if (rects[2].size.width == 0.0) {
	rects[0] = rects[1];
	*rectCount = 1;
    } else
	*rectCount = 3;
    return YES;
}


- invalidate:(const NXRect *)rects :(int)rectCount
{
    if (subviews) {
	register int        i, s;
	register id        *vp;
	NXRect              svClips[3];
	int                 svNumClips;

	if (s = [subviews count])
	    for (i = 0, vp = (id *)NX_ADDRESS(subviews); i < s; i++, vp++)
		if (svNumClips = [*vp _clip:rects :rectCount :svClips])
		    [*vp invalidate:svClips :svNumClips];
    }
    return self;
}


- scrollRect:(const NXRect *)aRect by:(const NXPoint *)delta
{
 /*
  * could rotated case scroll outside view ??? 
  */
    NXRect              sRect, dRect, vRect;
    register NXRect    *sr = &sRect, *dr = &dRect, *vr = &vRect;

    if (![self getVisibleRect:vr])
	return self;

    *sr = *dr = *aRect;
    dr->origin.x += delta->x;
    dr->origin.y += delta->y;

    if (!NXIntersectionRect(vr, sr))
	return self;

    if (!NXIntersectionRect(vr, dr))
	return self;

    dr->origin.x -= delta->x;
    dr->origin.y -= delta->y;

    NXIntersectionRect(dr, sr);

    *dr = *sr;
    dr->origin.x += delta->x;
    dr->origin.y += delta->y;

 /* centerScan source and dest to compensate for postscript over-scanning */
    [self centerScanRect:sr];
    [self _centerScanPoint:&dr->origin];

    if ([self canDraw]) {
	[self lockFocus];
	NXCopyBits(NXNullObject, sr, &dr->origin);
	[self unlockFocus];
    }
    return self;
}


/** Event Processing **/

- hitTest:(NXPoint *)aPoint
{
    id                  hitView = nil;
    NXPoint             tPoint;
    register int        i, s;
    register id        *vp;

    tPoint = *aPoint;
    if ([self _convertPointFromSuperview:&tPoint test:YES]) {
	if (subviews && (s = [subviews count]))
	    for (i = 0, vp = (id *)NX_ADDRESS(subviews) + s - 1; i < s; i++, vp--)
		if ((hitView = [*vp hitTest:&tPoint]) != nil)
		    break;
	if (!hitView)
	    hitView = self;
    }
    return hitView;
}


- (BOOL) mouse:(NXPoint *)aPoint inRect:(NXRect *)aRect
{
    return (NXMouseInRect(aPoint, aRect, vFlags.needsFlipped));
}	    

	    
- findViewWithTag:(int)aTag
{
    register int        index, size;
    register id         tempView;
    register id         taggedView;

    if ([self tag] == aTag)
	return self;

    size = [subviews count];	/* size = 0 if subviews == nil */
    for (index = 0; index < size; index++) {
	tempView = [subviews objectAt:index];
	if (taggedView = [tempView findViewWithTag:aTag])
	    return taggedView;
    }
    return nil;
}


- (int)tag
{
    return (-1);
}


- (BOOL)performKeyEquivalent:(NXEvent *)theEvent
{
    register int        index, size;
    register id         tempView;

    size = [subviews count];	/* size = 0 if subviews == nil */
    for (index = 0; index < size; index++) {
	tempView = [subviews objectAt:index];
	if ([tempView performKeyEquivalent:theEvent])
	    return YES;
    }
    return NO;
}


- (BOOL)acceptsFirstMouse
{
    return NO;
}

/*** printing and copying PS ***/

/* NOTE:  There are two types of page numbers in this code.  Ordinal page
	    numbers (and sheet numbers) always start at one.  Label page
	    numbers start at whatever the first page number of the first
	    ordinal page is.  For example, the first page of a document
	    could be 43, since its the start of the second chapter.
 */

- copyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream
{
    return [self _realCopyPSCodeInside:rect to:stream helpedBy:self];
}


- _realCopyPSCodeInside:(const NXRect *)rect to:(NXStream *)stream helpedBy:target
/*
 * Guts of copyPSCode, with target parametrized.  Target is the object who
 * is sent most printing methods, and thus participates in generating the
 * conforming PostScript comments.
 */
{
    NXRect              bbox;	/* bounding box, in superview's coords, but */
				/* origined at this view's lower left corner */
    NXRect              innerBox;	/* box to draw, in this view's coords */
    DPSContext          ctxt;
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo    privPInfo;
    NXHandler           exception;

    exception.code = 0;		/* reset to real err code when error raised */
    bzero(&privPInfo, sizeof(PrivatePrintInfo));
    [pInfo _setPrivateData:&privPInfo];
    privPInfo.printStream = stream;
    privPInfo.printPanel = nil;
    ctxt = DPSCreateStreamContext(stream, 0, dps_ascii, dps_strings, NULL);
    NX_DURING {
	[pInfo setContext:ctxt];
	[Font _clearDocFontsUsed];
	[target beginPSOutput];
	NXDrawingStatus = NX_COPYING;
	if (rect)
	    bbox = innerBox = *rect;
	else
	    bbox = innerBox = bounds;
	[self convertRect:&bbox toView:superview];
      /* The bbox goes in the conforming bbox comment.  Since the includer
	 of this illustration will provide a default PS coord system, we
	 must determine the illustration's bbox with respect to its lower
	 left corner (which is different based on flippedness).
       */
	bbox.origin.x -= frame.origin.x;
	if ([superview isFlipped])
	    bbox.origin.y = NX_MAXY(&bbox) - NX_MAXY(&frame);
	else
	    bbox.origin.y -= frame.origin.y;
	privPInfo.currentPrintingRect = innerBox;
	[target beginPrologueBBox:&bbox creationDate:NULL createdBy:NULL
	 fonts:NULL forWhom:NULL pages:0 title:NULL];
	[target endHeaderComments];
	[target endPrologue];
	[target beginSetup];
	[target endSetup];
	privPInfo.specialPrintFocus = YES;
	[self display:&innerBox:1 :YES];
	[target beginTrailer];
	[target endTrailer];
    } NX_HANDLER {
	exception = NXLocalHandler;	/* save to re-raise later */
    } NX_ENDHANDLER
    [target endPSOutput];
    [pInfo _setPrivateData:NULL];
    NXDrawingStatus = NX_DRAWING;
    if (exception.code)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return self;
}


- printPSCode:sender
{
    return [self _realPrintPSCode:sender helpedBy:self doPanel:YES forFax:NO];
}

- faxPSCode:sender
{
    return [self _realPrintPSCode:sender helpedBy:self doPanel:YES forFax:YES];
}

static void runFaxModal(volatile PrivatePrintInfo *privPInfo)
{
    privPInfo->faxPanel = privPInfo->printPanel = [NXFaxPanel new];
    if (!privPInfo->faxPanel)
	privPInfo->mode = NX_CANCELTAG;
    else {    
	switch([privPInfo->faxPanel runModal]) {
	case _NX_FAXCANCEL:
	    privPInfo->mode = NX_CANCELTAG;
	    break;
	case _NX_FAXPREVIEWWITHCOVER:
	    privPInfo->coverSheet = 1;
	    privPInfo->mode = NX_PREVIEWTAG;
	    break;
	case _NX_FAXWITHCOVER:
	    privPInfo->coverSheet = 1;
	    privPInfo->mode = NX_OKTAG;
	    break;
	case _NX_FAXNOCOVER:
	    privPInfo->mode = NX_OKTAG;
	    break;
	case _NX_FAXPREVIEWNOCOVER:
	    privPInfo->mode = NX_PREVIEWTAG;
	    break;
	}
    }
}

/*
 * Guts of printPSCode, with target parametrized.  Target is the object who
 * is sent most printing methods, and thus participates in generating the
 * conforming PostScript comments.  If panelFlag is YES, we will ask the
 * sender about putting up a panel, else the panel is blocked.
 */
- _realPrintPSCode:sender helpedBy:target doPanel:(BOOL)panelFlag forFax:(BOOL)faxFlag
{
    volatile BOOL	faxPanelUp = faxFlag;   /* did we have fax panel up */
    volatile BOOL	rerunOnError = NO;	/* For some errors, we set */
						/* set this to rerun panel */
    volatile BOOL	anyError = NO;		/* set on all errors */
    volatile const char	*lastStatMsg;	/* last message seen in PrintPanel */
    volatile PrivatePrintInfo privPInfo;
    volatile id         pInfo = [NXApp printInfo];
    char                spoolFile[MAXPATHLEN];	/* name of spoolFile we use */
    int                 docFirstLabel;	/* page # of first page in doc */
    int                 docLastLabel;	/* page # of last page in doc */
    BOOL                knowsPages;	/* does view know where its own pages
					 * fall? */
    const NXSize       *defPaperSize;	/* default size for paper type */
    NXHandler           exception;
    volatile int	pagesPerSheet;
    int			firstLabelPage;	/* labels of first/last page to */
    int			lastLabelPage;	/* print.  valid iff knowsPages */
  /* button picked last time print panel was run */
    static char         lastPanelChoice = NX_OKTAG;
  /* ditto for cover sheet and faxPanel */
    static id	        lastFaxPanel = NULL;
    static int 		lastCoverSheet = 0;
    volatile BOOL	usePanel;	/* do we put up panel for user? */

  /* I know this code has become a bit of a fortran-like monster.  The
     first label putUpPanel is for restarting the user interaction with
     the panel, such as after doing some pagination we realize that the
     user selected a range of pages not in the document.  The second label
     ErrorBailout is for exits on numerous error conditions.
   */

    exception.code = 0;		/* reset to real err code when error raised */
    pagesPerSheet = [pInfo pagesPerSheet];

  /*
   * put up panel, if appropriate.  privPInfo.mode gets set, determining
   * what we will be doing (saving, printing or previewing).
   */
    usePanel = panelFlag &&
		(![sender respondsTo:@selector(shouldRunPrintPanel:)] ||
		  [sender shouldRunPrintPanel:self]);
putUpPanel:
    bzero(&(PrivatePrintInfo)privPInfo, sizeof(PrivatePrintInfo));
    lastStatMsg = KitString(Printing, "Done.", "Message to user when printing is finished.");
    if (usePanel) {
        if (faxPanelUp) {
	    NX_DURING {
	        runFaxModal(&privPInfo);
	    } NX_HANDLER {
		anyError = YES;
		exception = NXLocalHandler;	/* save to re-raise later */
		privPInfo.mode = NX_CANCELTAG;
	    } NX_ENDHANDLER
        } else {
	    privPInfo.printPanel = [PrintPanel new];
	    if (!privPInfo.printPanel)
		privPInfo.mode = NX_CANCELTAG;
	    else {
		[privPInfo.printPanel setHideOnDeactivate:NO];
		privPInfo.coverSheet = 0;
		NX_DURING {
		    privPInfo.mode = [privPInfo.printPanel runModal];
		} NX_HANDLER {
		    anyError = YES;
		    exception = NXLocalHandler;	/* save to re-raise later */
		    privPInfo.mode = NX_CANCELTAG;
		} NX_ENDHANDLER
		if (privPInfo.mode == NX_FAXTAG) {
		    faxPanelUp = YES;
		    NX_DURING {
			runFaxModal(&privPInfo);
		    } NX_HANDLER {
			anyError = YES;
			exception = NXLocalHandler;	/* save to re-raise later */
			privPInfo.mode = NX_CANCELTAG;
		    } NX_ENDHANDLER
		}
	    }
	}
    } else {
	privPInfo.mode = lastPanelChoice;
	privPInfo.faxPanel = lastFaxPanel;
	privPInfo.coverSheet = lastCoverSheet;
    }
    if (privPInfo.mode == NX_CANCELTAG)
	goto ErrorBailout;
    lastPanelChoice = privPInfo.mode;
    lastFaxPanel = privPInfo.faxPanel;
    lastCoverSheet = privPInfo.coverSheet;
    [privPInfo.printPanel _setControlsEnabled:NO];
    _NXInitialPStat(privPInfo.printPanel, privPInfo.mode);

  /*
   * figure out how many and which pages to print
   */
    docFirstLabel = 1;		/* defaults if the app doesn't know */
    docLastLabel = MAXINT;	/* important for _printPages:::: */
    knowsPages = [target knowsPagesFirst:&docFirstLabel last:&docLastLabel];
    if (knowsPages) {
	if (!_NXCalcPageSubset(pInfo, docFirstLabel, docLastLabel,
		&firstLabelPage, &lastLabelPage, &(int)privPInfo.numPages)) {
	    _NXKitAlert("Printing", "Print", "No pages from the document were selected to be printed", NULL, NULL, NULL);
	    anyError = YES;
	    rerunOnError = YES;
	    goto ErrorBailout;
	}
    } else {
	privPInfo.numPages = -1;	/* set after pagination */
	if (![pInfo isAllPages] && [pInfo lastPage] < [pInfo firstPage]) {
	    _NXKitAlert("Printing", "Print", "No pages from the document were selected to be printed", NULL, NULL, NULL);
	    anyError = YES;
	    rerunOnError = YES;
	    goto ErrorBailout;
	}
    }

  /*
   * set up privPInfo
   */
    [pInfo _setPrivateData:&(PrivatePrintInfo)privPInfo];
    defPaperSize = NXFindPaperSize([pInfo paperType]);
    privPInfo.paperOrientation =
      (!defPaperSize || defPaperSize->width <= defPaperSize->height) ?
      NX_PORTRAIT : NX_LANDSCAPE;
    privPInfo.extraTurn = _NXNeedsExtraTurn(pagesPerSheet);
    _NXGetSpoolFileName(spoolFile, pInfo);

  /*
   * generate PostScript code
   */
    NX_DURING {
	NXModalSession session;

    	if (usePanel)
	    privPInfo.session = [NXApp beginModalSession:&session
					for:privPInfo.printPanel];
	if ([target openSpoolFile:spoolFile]) {
	    [Font _clearDocFontsUsed];
	    [pInfo setPageOrder:(privPInfo.mode == NX_OKTAG &&
				!(knowsPages && docLastLabel == MAXINT) &&
				!privPInfo.faxPanel) ?
				NX_DESCENDINGORDER : NX_ASCENDINGORDER];
	    [target beginPSOutput];
	    NXDrawingStatus = NX_PRINTING;
	    [target beginPrologueBBox:NULL creationDate:NULL createdBy:NULL
	     fonts:NULL forWhom:NULL
	     pages:privPInfo.numPages > 0 ?	/* # sheets to print */
			(privPInfo.numPages - 1) / pagesPerSheet + 1 : -1
	     title:NULL];
	    [target endHeaderComments];
	    [target endPrologue];
	    [target beginSetup];
	    [target endSetup];
#ifndef OLD_SPOOLER
	    if (privPInfo.spoolPort)
		_NXNpdSend(NPD_SEND_HEADER, pInfo);
#endif

	    if (knowsPages)
		[self _printPages:firstLabelPage :lastLabelPage printInfo:pInfo
		 helpedBy:target];
	    else if (![self _printAndPaginate:docFirstLabel printInfo:pInfo
			helpedBy:target]) {
		[target endPSOutput];
		NXDrawingStatus = NX_DRAWING;
		_NXKitAlert("Printing", "Print", "No pages from the document were selected to be printed", NULL, NULL, NULL);
		if (privPInfo.session)
		    [NXApp endModalSession:privPInfo.session];
		rerunOnError = YES;
		anyError = YES;
		_NXRemoveHandler(&NXLocalHandler);
		goto ErrorBailout;
	    }
	    [self _generateFaxCoverSheet];

	    [target beginTrailer];
	    [target endTrailer];
	    [target endPSOutput];
	    NXDrawingStatus = NX_DRAWING;

	  /* spool file to printer or previewer */
	    if (privPInfo.mode == NX_OKTAG)
		[target spoolFile:spoolFile];
	    else if (privPInfo.mode == NX_PREVIEWTAG) {
		port_t              workSpacePort;
		id                  speaker;
		int                 ok;
    
		[privPInfo.printPanel _updatePrintStat:KitString(Printing, "Messaging Previewer...", "Message displayed while the user's file is sent to a PostScript previewer.")
		 label:NULL];
		workSpacePort = NXPortFromName(NX_WORKSPACEREQUEST, NULL);
		if (workSpacePort == PORT_NULL) {
		    anyError = YES;
		    _NXKitAlert("Printing", "Print", "The Workspace Manager cannot be contacted to start up the previewing application.", NULL, NULL, NULL);
		} else {
		    speaker = [[Speaker allocFromZone:[self zone]] init];
		    [speaker setSendPort:workSpacePort];
		    if ([speaker openTempFile:spoolFile ok:&ok] || !ok) {
			anyError = YES;
			_NXKitAlert("Printing", "Print", "The Workspace Manager is unable to start up the previewing application.", NULL, NULL, NULL);
		    }
		    [speaker free];
		}
	    }
	} else {
#ifndef OLD_SPOOLER
	    anyError = YES;
	    if (!(spoolFile && spoolFile[0]))
		_NXKitAlert("Printing", "Print", "The application could not connect to the printing daemon.", NULL, NULL, NULL);
	    else {
#endif
		_NXKitAlert("Printing", "Print", "The file %s could not be created.", NULL, NULL, NULL, spoolFile);
#ifndef OLD_SPOOLER
	    }
#endif
	}
	if (privPInfo.session)
	    [NXApp endModalSession:privPInfo.session];
    } NX_HANDLER {
	exception = NXLocalHandler;	/* save to re-raise later */
	if (exception.code == NX_printPackageError) {
	    anyError = YES;
	    lastStatMsg = KitString(Printing, "Could not find printPackage!", "Error condition while printing.");
	} else if (exception.code != NX_abortPrinting) {
	    anyError = YES;
	    lastStatMsg = KitString(Printing, "Error while printing!", "Unknown error condition while printing.");
	} else {
	    if (privPInfo.faxPanel)
	        lastStatMsg = KitString(Printing, "Fax request canceled.","Message to user when he cancel his fax request.");
	    else 
	        lastStatMsg = KitString(Printing, "Print request canceled.","Message to user when he cancel his print request.");
	}
	[target endPSOutput];
    } NX_ENDHANDLER
    [(((PrivatePrintInfo)privPInfo).printPanel)
			_updatePrintStat:(char *)lastStatMsg label:NULL];
    sleep(1);	/* leave message up in panel */
ErrorBailout:
#ifndef OLD_SPOOLER
    if (privPInfo.spoolPort) {
	(void)port_deallocate(task_self(), privPInfo.replyPort);
	(void)port_deallocate(task_self(), privPInfo.spoolPort);
	NXCloseMemory(privPInfo.printStream, NX_FREEBUFFER);
    }
#endif
    if (rerunOnError) {
	anyError = NO;
	rerunOnError = NO;
	goto putUpPanel;
    }
    NXDrawingStatus = NX_DRAWING;
    [pInfo _setPrivateData:NULL];
    [privPInfo.printPanel orderOut:self];
    if (exception.code && exception.code != NX_abortPrinting)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    if (privPInfo.mode == NX_CANCELTAG ||
    		exception.code == NX_abortPrinting || anyError)
	return nil;
    else
	return self;
}


- (BOOL)knowsPagesFirst:(int *)firstPageNum last:(int *)lastPageNum
{
    return NO;
}


- openSpoolFile:(char *)filename
{
    DPSContext          ctxt;
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

#ifdef OLD_SPOOLER
    if (!_NXMakeFileStream(pInfo, filename))
	return NULL;
#else
    if (privPInfo->mode == NX_OKTAG && !(filename && *filename)) {
	if (!_NXMakeMemStream(pInfo))
	    return NULL;
    } else {
	if (!_NXMakeFileStream(pInfo, filename))
	    return NULL;
    }
#endif
    ctxt = DPSCreateStreamContext(privPInfo->printStream, 0, dps_ascii,
				  dps_strings, NULL);
    if (!ctxt)
	return NULL;
    [pInfo setContext:ctxt];
    return self;
}


- beginPSOutput
{
    id                  pInfo = [NXApp printInfo];

    [pInfo _privateData]->currentPrintingView = self;
    DPSSetContext([pInfo context]);
    return self;
}


- beginPrologueBBox:(const NXRect *)boundingBox
    creationDate:(const char *)dateCreated
    createdBy:(const char *)anApplication
    fonts:(const char *)fontNames
    forWhom:(const char *)user
    pages:(int)numPages
    title:(const char *)aTitle
{
#define MAX_CHARS 512
    char                timeBuffer[MAX_CHARS];
    const char         *timeString;
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    const char         *paperName;
    NXRect		coverBBox,bbox;
    int			newTitleLen;
    unsigned char	*newTitle;	/* Holder for a new title */
    unsigned char	newTitleSpace[MAX_CHARS];
    unsigned char	*src, *dest, *newFrag;
    
    if (NXDrawingStatus == NX_PRINTING)
	DPSPrintf(DPSGetCurrentContext(), "%%!PS-Adobe-2.0\n");
    else
	DPSPrintf(DPSGetCurrentContext(), "%%!PS-Adobe-2.0 EPSF-2.0\n");
    if (!aTitle)
	if (!(aTitle = [window title]))
	    aTitle = "Untitled";

  /* convert non-ASCII characters to ascii */
    newTitleLen = strlen(aTitle) * 3;
    newTitle = (newTitleLen > MAX_CHARS-1) ? NXZoneMalloc([self zone], newTitleLen+1) : newTitleSpace;
    for (src = (unsigned char *)aTitle, dest = newTitle; *src; src++)
	if (newFrag = NXToAscii(*src))
	    while (*newFrag)
		*dest++ = *newFrag++;
    *dest = '\0';
    DPSPrintf(DPSGetCurrentContext(), "%%%%Title: %s\n", newTitle);
    if (newTitleLen > MAX_CHARS-1)
	free(newTitle);

    if (!anApplication)
	if (!(anApplication = [NXApp appName])) {
	    anApplication = strrchr(NXArgv[0], '/');
	    anApplication = anApplication ? anApplication + 1 : NXArgv[0];
	}
    DPSPrintf(DPSGetCurrentContext(), "%%%%Creator: %s\n", anApplication);

    if (!dateCreated) {
	time_t theTime;

	theTime = time(0);
	strncpy(timeBuffer, ctime(&theTime), MAX_CHARS);
	timeBuffer[MAX_CHARS - 1] = '\0';
	timeBuffer[strlen(timeBuffer) - 1] = '\0';	/* lop off \n */
	timeString = timeBuffer;
    } else
	timeString = dateCreated;
    DPSPrintf(DPSGetCurrentContext(), "%%%%CreationDate: %s\n", timeString);

    if (!user)
	user = NXUserName();
    DPSPrintf(DPSGetCurrentContext(), "%%%%For: %s\n", user);

    privPInfo->docFontsAtEnd = !fontNames;
    _NXWriteLongComment("%%DocumentFonts: ",
			fontNames ? fontNames : "(atend)");
    if (privPInfo->pagesAtEnd = (numPages < 0))
	DPSPrintf(DPSGetCurrentContext(), "%%%%Pages: (atend) %d\n",
		  [pInfo pageOrder]);
    else
	DPSPrintf(DPSGetCurrentContext(), "%%%%Pages: %d %d\n", 
	          numPages + privPInfo->coverSheet,
		  [pInfo pageOrder]);
    if (privPInfo->docBBoxAtEnd = !boundingBox) {
	DPSPrintf(DPSGetCurrentContext(), "%%%%BoundingBox: (atend)\n");
	NX_X(&privPInfo->docUnionBBox) = NX_Y(&privPInfo->docUnionBBox) = 0.0;
	NX_WIDTH(&privPInfo->docUnionBBox) =
	  NX_HEIGHT(&privPInfo->docUnionBBox) = -1.0;
    } else {
	if (privPInfo->coverSheet) {
	    [privPInfo->faxPanel coverBBox : &coverBBox];
	    _NXComputeRealBBox(&bbox,&coverBBox,pInfo,privPInfo);
	    NXUnionRect(boundingBox,&bbox);
	} else
	    bbox = *boundingBox;
	DPSPrintf(DPSGetCurrentContext(), "%%%%BoundingBox: %d %d %d %d\n",
		  (int)floor(NX_X(&bbox)),
		  (int)floor(NX_Y(&bbox)),
		  (int)ceil(NX_MAXX(&bbox)),
		  (int)ceil(NX_MAXY(&bbox)));
    }
    if (NXDrawingStatus == NX_PRINTING) {
	if ((paperName = [pInfo paperType]) && *paperName)
	    DPSPrintf(DPSGetCurrentContext(),
		      "%%%%DocumentPaperSizes: %s\n", paperName);
	DPSPrintf(DPSGetCurrentContext(), "%%%%Orientation: %s\n",
	     ([pInfo orientation] == NX_PORTRAIT) == !privPInfo->extraTurn ?
		  "Portrait" : "Landscape");
    }
    if (privPInfo->faxPanel) {
	[privPInfo->faxPanel writeFaxNumberList:"%%%%NXFaxNumber: %s\n"];
	[privPInfo->faxPanel writeFaxToList:"%%%%NXFaxTo: %s\n"];
        DPSPrintf(DPSGetCurrentContext(), 
		  "%%%%NXFaxNotify: %s\n",
		  [privPInfo->faxPanel notifyIsChecked] ? "YES" : "NO");
        DPSPrintf(DPSGetCurrentContext(), 
		  "%%%%NXFaxHires: %s\n",
		  [privPInfo->faxPanel hiresIsChecked] ? "YES" : "NO");
    }

    return self;
}


- endHeaderComments
{
    int ret;
    char *file;
    id pInfo = [NXApp printInfo];
    PrivatePrintInfo *privPInfo = [pInfo _privateData];
    const char *type;

    type = [pInfo printerType];
    if (type && !strcmp(type, "NeXT 400 dpi Laser Printer") && privPInfo->mode == NX_OKTAG)
	file = NEXT_PRINT_PACKAGE;
    else
	file = PRINT_PACKAGE;
    DPSPrintf(DPSGetCurrentContext(), "%%%%EndComments\n\n%%%%BeginDocument: %s\n", file);
    ret = _NXSendFileToPS(file);
    DPSPrintf(DPSGetCurrentContext(), "%%%%EndDocument\n\n");
    if (ret < 0)
	NX_RAISE(NX_printPackageError, 0, 0);
    return self;
}


- endPrologue
{
    [self _genBaseMatrix];
    DPSPrintf(DPSGetCurrentContext(), "%%%%EndProlog\n");
    return self;
}


- beginSetup
{
    id                  pInfo = [NXApp printInfo];
    DPSContext          ctxt = DPSGetCurrentContext();
    int			res;

    DPSPrintf(ctxt, "%%%%BeginSetup\n");
    if (NXDrawingStatus == NX_PRINTING) {
	_NXPaperSizeComment(pInfo);
	DPSPrintf(ctxt, "%%%%Feature: *ManualFeed %s\n",
		  [pInfo isManualFeed] ? "True" : "False");
	if (res = [pInfo resolution]) {
	    DPSPrintf(ctxt, "%%%%Feature: *Resolution %d\n", res);
	}
    }
    return self;
}


- endSetup
{
    DPSPrintf(DPSGetCurrentContext(), "%%%%EndSetup\n");
    return self;
}

- _generateFaxCoverSheet {
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    NXRect		bbox;
    const NXRect       *paperRect;
    id 			faxPanel;
    DPSContext          ctxt = DPSGetCurrentContext();
    int                 finalOrientation;	/* orientation all things
						 * considered */
    float		scalingFactor;
    
    if (privPInfo->coverSheet) {
	scalingFactor = [pInfo scalingFactor];
	
	[privPInfo->printPanel _updatePrintStat:KitString(Printing, "Writing Cover Sheet", "Status message displayed while generating Fax cover sheet.") label:NULL];

        faxPanel = privPInfo->faxPanel;
    
    /*  beginPage... stuff */
    
	DPSPrintf(ctxt, "\n%%%%Page: Coversheet %d\n",
		    1 + (privPInfo->numPages >= 0 ? privPInfo->numPages :
					            privPInfo->ordSheet));
	DPSPrintf(ctxt, "%%%%PageBoundingBox: ");
	[faxPanel coverBBox : &bbox];
	bbox.size.width *= scalingFactor;
	bbox.size.height *= scalingFactor;
	DPSPrintf(ctxt, "%d %d %d %d\n",
		  (int)NX_X(&bbox), (int)NX_Y(&bbox),
		  (int)NX_MAXX(&bbox), (int)NX_MAXY(&bbox));
	DPSPrintf(ctxt,"%%PageFonts: (atend)\n");
	
	[Font _clearSheetFontsUsed];
	[Font _clearPageFontsUsed];
	_NXUpdateBBox(privPInfo->pageBBoxAtEnd, &privPInfo->pageUnionBBox,
		      &bbox);
	_NXUpdateBBox(privPInfo->docBBoxAtEnd, &privPInfo->docUnionBBox,
		      &bbox);
	
    /*  Page Setup */
    	      
        paperRect = [pInfo paperRect];
	DPSPrintf(ctxt, "%%%%BeginPageSetup\n");
	_NXPaperSizeComment(pInfo);
	DPSPrintf(ctxt, "/__NXsheetsavetoken save def\n");

    /* if mult pages/sheet requires a turn, switch orientation */
	finalOrientation = [pInfo orientation];

    /* setup portrait or landscape mode */
	if (finalOrientation == NX_LANDSCAPE &&
	    privPInfo->paperOrientation == NX_PORTRAIT) {
	    PSrotate(-90.0);
	    PStranslate(-NX_WIDTH(paperRect), 0.0);
	} else if (finalOrientation == NX_PORTRAIT &&
		   privPInfo->paperOrientation == NX_LANDSCAPE) {
	    PStranslate(NX_WIDTH(paperRect), 0.0);
	    PSrotate(90.0);
	}
	
     /* do reduction factor */
	if (scalingFactor != 1.0)
	    PSscale(scalingFactor, scalingFactor);
	
	DPSPrintf(ctxt, "%%%%EndPageSetup\n");
	
    /*  Generate Cover Sheet by displaying kludge view */
    
	[faxPanel writeCoverSheet];
	PSshowpage();
	
    /*  Page Trailer stuff */
    
	DPSPrintf(ctxt,"__NXsheetsavetoken restore\n%%%%PageTrailer\n");
	[Font _writePageFontsUsed];
    }
    return self;
}

- beginPage:(int)ordinalNum label:(const char *)aString
	bBox:(const NXRect *)pageRect fonts:(const char *)fontNames
{
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    char		panelLabel[20];
 
    sprintf(panelLabel, "%d", privPInfo->firstLabelPage +
				(([pInfo pageOrder] != NX_DESCENDINGORDER) ?
				(privPInfo->ordPage - 1) :
				(privPInfo->numPages - privPInfo->ordPage)));
    _NXPagePStat(privPInfo->printPanel, privPInfo->mode, panelLabel);
    if (privPInfo->firstPageOfSheet) {
	DPSPrintf(DPSGetCurrentContext(), "\n%%%%Page: ");
	if (!aString)
	    DPSPrintf(DPSGetCurrentContext(), "%d", ordinalNum);
	else
	    DPSPrintf(DPSGetCurrentContext(), "%s", aString);
	DPSPrintf(DPSGetCurrentContext(), " %d\n", ordinalNum);

	DPSPrintf(DPSGetCurrentContext(), "%%%%PageBoundingBox: ");
	if (privPInfo->pageBBoxAtEnd = !pageRect)
	    DPSPrintf(DPSGetCurrentContext(), "(atend)\n");
	else
	    DPSPrintf(DPSGetCurrentContext(), "%d %d %d %d\n",
		      (int)floor(NX_X(pageRect)),
		      (int)floor(NX_Y(pageRect)),
		      (int)ceil(NX_MAXX(pageRect)),
		      (int)ceil(NX_MAXY(pageRect)));
	_NXWriteLongComment("%%PageFonts: ", fontNames ? fontNames : "(atend)");
	privPInfo->pageFontsAtEnd = !fontNames;
    }
    return self;
}


- beginPageSetupRect:(const NXRect *)aRect placement:(const NXPoint *)location
{
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    const NXRect       *paperRect;
    int                 finalOrientation;	/* orientation all things
						 * considered */
    DPSContext          ctxt = DPSGetCurrentContext();

    paperRect = [pInfo paperRect];
    if (privPInfo->firstPageOfSheet) {
	DPSPrintf(ctxt, "%%%%BeginPageSetup\n");
	_NXPaperSizeComment(pInfo);
	DPSPrintf(ctxt, "/__NXsheetsavetoken save def\n");

      /* if mult pages/sheet requires a turn, switch orientation */
	finalOrientation = [pInfo orientation];
	if (privPInfo->extraTurn)
	    finalOrientation = (finalOrientation == NX_LANDSCAPE ?
				NX_PORTRAIT : NX_LANDSCAPE);

      /* setup portrait or landscape mode */
	if (finalOrientation == NX_LANDSCAPE &&
	    privPInfo->paperOrientation == NX_PORTRAIT) {
	    PSrotate(-90.0);
	    PStranslate(privPInfo->extraTurn ? -NX_HEIGHT(paperRect) :
			-NX_WIDTH(paperRect), 0.0);
	} else if (finalOrientation == NX_PORTRAIT &&
		   privPInfo->paperOrientation == NX_LANDSCAPE) {
	    PStranslate(privPInfo->extraTurn ? NX_HEIGHT(paperRect) :
			NX_WIDTH(paperRect), 0.0);
	    PSrotate(90.0);
	}

      /* draw border for this sheet */
	if (privPInfo->extraTurn)
	    [self drawSheetBorder:NX_HEIGHT(paperRect) :NX_WIDTH(paperRect)];
	else
	    [self drawSheetBorder:NX_WIDTH(paperRect) :NX_HEIGHT(paperRect)];
    }
 /* if more than one page/sheet, each page still gets a save-restore */
    if ([pInfo pagesPerSheet] > 1) {
	DPSPrintf(ctxt, "/__NXpagesavetoken save def\n");
	PStranslate(privPInfo->sheetOffset.x, privPInfo->sheetOffset.y);
	PSscale(privPInfo->sheetScale, privPInfo->sheetScale);
    }

  /* draw border for this page */
    [self drawPageBorder:NX_WIDTH(paperRect) :NX_HEIGHT(paperRect)];

 /* place localRect within the page */
    PStranslate(location->x, location->y);

 /* do reduction factor */
    if (privPInfo->totalScale != 1.0)
	PSscale(privPInfo->totalScale, privPInfo->totalScale);

 /* allow app a crack at the page setup - to be used mostly to implement
    PageLayout's scale factor by apps that do their own pagination.
  */
    [self addToPageSetup];

    [self _genBaseMatrix];

 /*
  * focus on view with special printing focus, which skips xforms from the
  * View's parents. 
  */
    privPInfo->specialPrintFocus = YES;
    [self lockFocus];

 /* move localRect of the view onto page */
    PStranslate(NX_X(&bounds) - NX_X(aRect),
		[self isFlipped] ? NX_MAXY(&bounds) - NX_MAXY(aRect) :
		NX_Y(&bounds) - NX_Y(aRect));
    return self;
}


- addToPageSetup
{
    return self;
}


- endPageSetup
{
    if ([[NXApp printInfo] _privateData]->firstPageOfSheet)
	DPSPrintf(DPSGetCurrentContext(), "%%%%EndPageSetup\n");
    return self;
}


- endPage
{
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    int                 pagesPerSheet = [pInfo pagesPerSheet];

    [self unlockFocus];
 /* if more than one page/sheet, each page still gets a save-restore */
    if (pagesPerSheet > 1)
	DPSPrintf(DPSGetCurrentContext(), "__NXpagesavetoken restore\n");
 /* if last page written on a sheet */
    if (privPInfo->lastPageOfSheet || privPInfo->doingLastPage) {
	PSshowpage();
	DPSPrintf(DPSGetCurrentContext(), "__NXsheetsavetoken restore\n%%%%PageTrailer\n");
	if (privPInfo->pageFontsAtEnd)
	    [Font _writePageFontsUsed];
	if (privPInfo->pageBBoxAtEnd)
	    _NXWriteBBoxComment("PageBoundingBox", &privPInfo->pageUnionBBox,
				pInfo, privPInfo);
    }
    return self;
}


- beginTrailer
{
    DPSPrintf(DPSGetCurrentContext(), "%%%%Trailer\n");
    return self;
}


- endTrailer
{
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

    if (privPInfo->docFontsAtEnd)
	[Font _writeDocFontsUsed];

  /* privPInfo->ordSheet works because if we didnt know the total pages,
     we print in ascending order.
   */
    if (privPInfo->pagesAtEnd)
	DPSPrintf(DPSGetCurrentContext(), "%%%%Pages: %d %d\n",
		  privPInfo->ordSheet + privPInfo->coverSheet,
		  [pInfo pageOrder]);

    if (privPInfo->docBBoxAtEnd)
	_NXWriteBBoxComment("BoundingBox", &privPInfo->docUnionBBox,
			    pInfo, privPInfo);
    return self;
}


- endPSOutput
{
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

    DPSDestroyContext([pInfo context]);
#ifndef OLD_SPOOLER
    if (NXDrawingStatus == NX_PRINTING && privPInfo->printStream &&
	!privPInfo->spoolPort) {
#else
    if (NXDrawingStatus == NX_PRINTING && privPInfo->printStream) {
#endif
	NXClose(privPInfo->printStream);
	close(privPInfo->printFD);
    }
    [pInfo setContext:NULL];
    DPSSetContext([NXApp context]);
    return self;
}


- spoolFile:(const char *)filename
{
    char                commandNamePtr[MAXPATHLEN + 100];
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    const char         *printerName;

#ifndef OLD_SPOOLER
    if (privPInfo->spoolPort)
	_NXNpdSend(NPD_SEND_TRAILER, pInfo);
    else
#endif
    {
	[privPInfo->printPanel _updatePrintStat:KitString(Printing, "Spooling file...", "Status message informing user that the print job is being spooled.") label:NULL];
	printerName = [pInfo printerName];
	if (printerName)
	    sprintf(commandNamePtr, "lpr -p%s -#%d -r -s %s",
			[pInfo printerName], [pInfo copies], filename);
	else
	    sprintf(commandNamePtr, "lpr -#%d -r -s %s", [pInfo copies],
			filename);
	(void)system(commandNamePtr);
    }
    return self;
}


/*
 * Prints all pages of a given job in the case where the application knows
 * where its own pages lie.  firstLabel and lastLabel are the first and
 * last pages in the document (no the first and last pages requested).
 *
 * If the number of pages is not known, lastLabel should be passed in as
 * MAXINT.  If this is the case, we dont know when we've printed the last
 * page until we ask for one too many, and thus we must to the [endPage]
 * ourselves.  This is to be able to finish up a sheet that is only
 * partially filled with pages.
 */
- _printPages:(int)firstLabel :(int)lastLabel printInfo:pInfo helpedBy:target
{
    int                 labelPage;	/* current page number within
					 * document */
    NXRect              currRect;	/* next rect we will image in local
					 * coords */
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    BOOL                morePages;	/* Is their another page to print? */
    int			pageIncrement;	/* +1 or -1 */
    BOOL		knowsTotalPages;/* Know which is the last page? */
    BOOL		reversePages;

    knowsTotalPages = !(privPInfo->numPages == -1);
    reversePages = [pInfo pageOrder] == NX_DESCENDINGORDER;
    if (!reversePages) {
	labelPage = firstLabel;
      /* -1 to compensate for below loop structure */
	privPInfo->ordSheet = privPInfo->ordPage = 1 - 1;
	pageIncrement = 1;
    } else {
	AK_ASSERT(knowsTotalPages, "Cant reverse pages without knowing the last one");
	labelPage = lastLabel;
      /* +1 to compensate for below loop structure */
	privPInfo->ordPage = privPInfo->numPages + 1;
      /* num sheets (+1 to compensate for below loop structure) */
	privPInfo->ordSheet = ((privPInfo->numPages - 1) /
				[pInfo pagesPerSheet] + 1) + 1;
	pageIncrement = -1;
    }
    privPInfo->doingLastPage = NO;
    privPInfo->totalScale = 1.0;
    privPInfo->firstLabelPage = firstLabel;
    morePages = YES;
    while (morePages && (!reversePages ? (labelPage <= lastLabel) :
					(labelPage >= firstLabel))) {
	[pInfo _setCurrentPage:labelPage];
	morePages = [target getRect:&currRect forPage:labelPage];
      /* if app didnt know how many pages it has, we always call endPage
         instead of letting _doPageArea:finishPage:helpedBy:pageLabel:.
	 privPInfo->doingLastPage is an implicit parameter that says
	 to finish up the page regardless if the sheet is really filled.
       */
	if (!knowsTotalPages) {
	    if (!morePages)
		privPInfo->doingLastPage = YES;
	    if (privPInfo->ordPage) {
		[target endPage];
#ifndef OLD_SPOOLER
		if (privPInfo->lastPageOfSheet || privPInfo->doingLastPage)
		    if (privPInfo->spoolPort)
			_NXNpdSend(NPD_SEND_PAGE, pInfo);
#endif
	    }
	}
	if (morePages) {
	    privPInfo->ordPage += pageIncrement;
	    [self _doPageArea:&currRect finishPage:knowsTotalPages
					helpedBy:target pageLabel:labelPage];
	    labelPage += pageIncrement;
	}
    }
    return self;
}


/*
 * Prints all pages of a given job in the case where we don't know
 * where pages lie.  This routine holds the pagination loop.
 */
- _printAndPaginate:(int)docFirstLabel printInfo:pInfo helpedBy:target
{
    BOOL                flipped;	/* this view's orientation */
    NXRect              currRect;	/* current rect we're printing */
    int                 labelPage;	/* current document page number */
    int                 row, col;	/* position of current page in grid */

#define DEF_ARRAY_SIZE  20
    float               cwStuff[DEF_ARRAY_SIZE];/* intial stack storage for
						 * colWidths */
    float              *colWidths = cwStuff;	/* widths of all columns */
    int                 numCols = DEF_ARRAY_SIZE;
    float               rhStuff[DEF_ARRAY_SIZE];/* intial stack storage for
						 * rowHeights */
    float              *rowHeights = rhStuff;	/* height of all rows */
    int                 numRows = DEF_ARRAY_SIZE;
    BOOL                allPages;
    int			docLastLabel;		/* label of last page in doc */
    int                 firstLabel, lastLabel;	/* labels of first and last
						 * pages to print */
    NXSize              marginSize;
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

    [self _calcMarginSize:&marginSize printInfo:pInfo];
    if (marginSize.width <= 0.0 || marginSize.height <= 0.0)
	return self;
    flipped = [self isFlipped];
    allPages = [pInfo isAllPages];
    [self _calcWidths:&colWidths num:&numCols margin:&marginSize printInfo:pInfo helpedBy:target];
    [self _calcHeights:&rowHeights num:&numRows margin:&marginSize printInfo:pInfo helpedBy:target];
    privPInfo->cols = numCols;
    privPInfo->rows = numRows;
    docLastLabel = docFirstLabel + numCols * numRows - 1;
    if (!_NXCalcPageSubset(pInfo, docFirstLabel, docLastLabel,
				&firstLabel, &lastLabel, &privPInfo->numPages))
	return nil;

  /* doingLastPage not used in this case since we know total num of pages */
    privPInfo->doingLastPage = NO;
    privPInfo->firstLabelPage = firstLabel;
    if ([pInfo pageOrder] != NX_DESCENDINGORDER) {
	labelPage = docFirstLabel;
      /* -1 to compensate for loop structure */
	privPInfo->ordSheet = privPInfo->ordPage = 1 - 1;
      /* start rect at upper edge */
      	NX_Y(&currRect) = flipped ? NX_Y(&bounds) : NX_MAXY(&bounds);
	for (row = 0; labelPage <= lastLabel; row++) {
	  /* start rect at left edge */
	    NX_X(&currRect) = NX_X(&bounds);
	    NX_HEIGHT(&currRect) = rowHeights[row];
	    if (!flipped)
		NX_Y(&currRect) -= NX_HEIGHT(&currRect);
	    for (col = 0; col < numCols && labelPage <= lastLabel; col++) {
		NX_WIDTH(&currRect) = colWidths[col];
		NX_ASSERT(NXIntersectionRect(&bounds, &currRect), "Printing: empty intersection of currRect and bounds\n");
	      /* skip pages before first requested */
		if (labelPage >= firstLabel) {
		    privPInfo->ordPage++;
		    [pInfo _setCurrentPage:labelPage];
		    [self _doPageArea:&currRect finishPage:YES helpedBy:target
						        pageLabel:labelPage];
		}
		labelPage++;
		NX_X(&currRect) += NX_WIDTH(&currRect);
	    }
	    if (flipped)
		NX_Y(&currRect) += NX_HEIGHT(&currRect);
	}
    } else {
	labelPage = docLastLabel;
      /* +1 to compensate for loop structure */
	privPInfo->ordPage = privPInfo->numPages + 1;
      /* num sheets (+1 to compensate for loop structure) */
	privPInfo->ordSheet = ((privPInfo->numPages - 1) /
				[pInfo pagesPerSheet] + 1) + 1;
      /* start rect at lower edge of paginated area */
	NX_Y(&currRect) = privPInfo->bottomEdge;
	for (row = numRows - 1; labelPage >= firstLabel; row--) {
	  /* start rect at right edge of paginated area */
	    NX_X(&currRect) = privPInfo->rightEdge;
	    NX_HEIGHT(&currRect) = rowHeights[row];
	    if (flipped)
		NX_Y(&currRect) -= NX_HEIGHT(&currRect);
	    for (col = numCols; --col >= 0 && labelPage >= firstLabel; ) {
		NX_WIDTH(&currRect) = colWidths[col];
		NX_X(&currRect) -= NX_WIDTH(&currRect);
		NX_ASSERT(NXIntersectionRect(&bounds, &currRect), "Printing: empty intersection of currRect and bounds\n");
	      /* skip pages after last requested */
		if (labelPage <= lastLabel) {
		    privPInfo->ordPage--;
		    [pInfo _setCurrentPage:labelPage];
		    [self _doPageArea:&currRect finishPage:YES helpedBy:target
						        pageLabel:labelPage];
		}
		labelPage--;
	    }
	    if (!flipped)
		NX_Y(&currRect) += NX_HEIGHT(&currRect);
	}
    }
    if (numCols > DEF_ARRAY_SIZE)
	NX_FREE(colWidths);
    if (numRows > DEF_ARRAY_SIZE)
	NX_FREE(rowHeights);
    return self;
}


/*
 * Calculates the size of the box in the printing View's coords that will
 * be used to dice up the View into pages.  This takes into account the
 * reduction scale factor, and if the View is being force fitted onto the
 * page.  This method also sets the total scale in the private printInfo.
 * If any force fitting is in effect, that scale is applied first;  any
 * reduction specified in the Page Layout Panel is applied after that.
 *
 * The smaller the scaling factor, the larger the margin box (to get more
 * one a page) and the smaller the images (to make more stuff fit).
 * "Shrink factors" are the recipricol of scaling factors.
 *
 * sets size, privPInfo->(totalScale,rightEdge,bottomEdge) 
 */
- (void)_calcMarginSize:(NXSize *)size printInfo:pInfo
{
    NXCoord             lm, rm, tm, bm;
    NXSize              paperSize;
    float               horizShrinkFactor;	/* for force fitting View */
    float               vertShrinkFactor;
    float		finalShrinkFactor;	/* max of previous two */
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    float               panelScaleFactor;
    char		horizPag = [pInfo horizPagination];
    char		vertPag = [pInfo vertPagination];
    BOOL		flipped = [self isFlipped];
    float		viewPiece;

    panelScaleFactor = [pInfo scalingFactor];
    paperSize = ([pInfo paperRect])->size;
    [pInfo getMarginLeft:&lm right:&rm top:&tm bottom:&bm];
    size->width = paperSize.width - lm - rm;
    size->height = paperSize.height - tm - bm;
    [self convertSize:size fromView:superview];

  /* figure out scale factors due to force-fitting */
    if (horizPag == NX_FITPAGINATION && NX_WIDTH(&bounds) > size->width)
	horizShrinkFactor = NX_WIDTH(&bounds) / size->width;
    else
	horizShrinkFactor = 1.0;
    if (vertPag == NX_FITPAGINATION && NX_HEIGHT(&bounds) > size->height)
	vertShrinkFactor = NX_HEIGHT(&bounds) / size->height;
    else
	vertShrinkFactor = 1.0;
    finalShrinkFactor = MAX(horizShrinkFactor, vertShrinkFactor);

  /* figure out bounds of the part of the view that we will paginate */
    if (horizPag == NX_CLIPPAGINATION) {
	viewPiece = size->width * finalShrinkFactor;
	if (panelScaleFactor < 1.0)
	    viewPiece /= panelScaleFactor;
	if (viewPiece > NX_WIDTH(&bounds))
	    viewPiece = NX_WIDTH(&bounds);
	privPInfo->rightEdge = NX_X(&bounds) + viewPiece;
    } else
	privPInfo->rightEdge = NX_MAXX(&bounds);
    if (vertPag == NX_CLIPPAGINATION) {
	viewPiece = size->height * finalShrinkFactor;
	if (panelScaleFactor < 1.0)
	    viewPiece /= panelScaleFactor;
	if (viewPiece > NX_HEIGHT(&bounds))
	    viewPiece = NX_HEIGHT(&bounds);
	privPInfo->bottomEdge = flipped ? (NX_Y(&bounds) + viewPiece) : (NX_MAXY(&bounds) - viewPiece);
    } else
	privPInfo->bottomEdge = flipped ? NX_MAXY(&bounds) : NX_Y(&bounds);

  /* apply scale factor in PageLayout panel */
    privPInfo->totalScale = panelScaleFactor / finalShrinkFactor;
    size->width /= privPInfo->totalScale;
    size->height /= privPInfo->totalScale;
}


/* error strings for asserts */
static const char calcWidthsError[] = "*new not set or increased in adjustPageWidthNew:left:right:limit:";
static const char calcHeightsError[] = "*new not set or increased in adjustPageHeightNew:top:bottom:limit:";

/* Fraction of marginRect we will toss instead of having a page with a very thin strip of View on it.  Approx 1/2 a pixel on letter paper at 400 dpi */
#define PAG_SLACK	(1.0/8800.0)


/*
 * Calculates the vertical lines of the page grid over the view we are trying
 * to print.  defaultSpace is an array of floats that the widths are put
 * into.  numWidths should start as the size of default space.  If the
 * defaultSpace turns out not to be big enough, then it will return
 * pointing to some malloc'ed space, which the caller is responsible to
 * free.  numWidths always returns holding the number of columns found.
 * marginSize is in this view's coord system, as provided by the caller.
 */
- (void)_calcWidths:(float **)defaultSpace num:(int *)numWidths
    margin:(const NXSize *)marginSize printInfo:pInfo helpedBy:target
{
    int                 col;		/* #column we're on now   */
    float               left, right;	/* boundary of current strip we're
					 * considering */
    float               new;		/* adjusted value */
    float              *results = *defaultSpace;
    int                 colsAlloced = *numWidths;
    float               minWidth;	/* minimum width of any page */
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    char		pagination = [pInfo horizPagination];
    NXZone	       *zone = [self zone];
  /* we force this value to be on the stack instead of in a register,
     because of compiler/68882 damage.  When NX_MAXX is evaluated, the
     expression ends up in a floating point reg, with extended precision.
     If this is compared to a float variable which has been assigned the
     same value, they are seen as unequal.  Yuck!
   */
    volatile float	maxX = privPInfo->rightEdge;
    float pagSlack;

    AK_ASSERT(*numWidths >= 1, "not enough initial widths in calcWidths");
    minWidth = [target widthAdjustLimit];
    NX_ASSERT(minWidth >= 0.0 && minWidth <= 1.0,
		"Value returned by widthAdjustLimit is not a percentage");
    minWidth = marginSize->width * (1.0 - minWidth);
    pagSlack = marginSize->width * PAG_SLACK;
    for (left = NX_X(&bounds), col = 0; maxX - left > pagSlack; col++, left = new) {
	right = left + marginSize->width;
	if (right > maxX)
	    right = maxX;
	if (pagination == NX_AUTOPAGINATION) {
	    new = MAXINT;	/* assume MAXINT non-sensical for debugging */
	    [self adjustPageWidthNew:&new left:left right:right
		limit:left + minWidth];
	    NX_ASSERT(new != MAXINT, calcWidthsError);
	    NX_ASSERT(new <= right, calcWidthsError);
	} else
	    new = right;
	if (col == *numWidths) {
	    NX_ZONEMALLOC(zone, results, float, colsAlloced *= 2);
	    bcopy(*defaultSpace, results, col*sizeof(float));
	} else if (col == colsAlloced)
	    NX_ZONEREALLOC(zone, results, float, colsAlloced *= 2);
	if (new > right || new < left + minWidth)
	    new = right;
	results[col] = new - left;
    }
    *numWidths = col;
    *defaultSpace = results;
}


/*
 * Calculates one horizontal line of the page grid over the view we are trying
 * to print.  The rectangle passed in has its height (and maybe origin.y)
 * changed to reflect the new height of the print rectangle.  minHeight is
 * the minimum height of the resulting rect, which gets calculated in the
 * caller (and also cached there for the life of the job).
 */
- (void)_calcHeights:(float **)defaultSpace num:(int *)numHeights
    margin:(const NXSize *)marginSize printInfo:pInfo helpedBy:target
{
    float               top, bottom;	/* boundary of current strip we're
					 * considering */
    float               new;		/* adjusted y-value for page break */
    int                 row;
    float              *results = *defaultSpace;
    int                 rowsAlloced = *numHeights;
    float               minHeight;	/* minimum height of any page */
    BOOL		flipped = [self isFlipped];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    char		pagination = [pInfo vertPagination];
    NXZone	       *zone = [self zone];
  /* we force this value to be on the stack instead of in a register,
     because of compiler/68882 damage.  When NX_MAXY is evaluated, the
     expression ends up in a floating point reg, with extended precision.
     If this is compared to a float variable which has been assigned the
     same value, they are seen as unequal.  Yuck!
   */
    volatile float	maxY = privPInfo->bottomEdge;
    float pagSlack;

    minHeight = [target heightAdjustLimit];
    NX_ASSERT(minHeight >= 0.0 && minHeight <= 1.0,
		"Value returned by heightAdjustLimit is not a percentage");
    minHeight = marginSize->height * (1.0 - minHeight);
    pagSlack = marginSize->height * PAG_SLACK;
    if (flipped)
	for (top = NX_Y(&bounds), row = 0; maxY - top > pagSlack; row++, top = new) {
	    bottom = top + marginSize->height;
	    if (bottom > maxY)
		bottom = maxY;
	    if (pagination == NX_AUTOPAGINATION) {
		new = MAXINT;	/* assume MAXINT non-sensical for debugging */
		[self adjustPageHeightNew:&new top:top bottom:bottom
		    limit:top + minHeight];
		NX_ASSERT(new != MAXINT, calcHeightsError);
		NX_ASSERT(new <= bottom, calcHeightsError);
	    } else
		new = bottom;
	    if (row == *numHeights) {
		NX_ZONEMALLOC(zone, results, float, rowsAlloced *= 2);
		bcopy(*defaultSpace, results, row*sizeof(float));
	    } else if (row == rowsAlloced)
		NX_ZONEREALLOC(zone, results, float, rowsAlloced *= 2);
	    if (new > bottom || new < top + minHeight)
		new = bottom;
	    results[row] = new - top;
	}
    else
	for (top = NX_MAXY(&bounds), row = 0; top - maxY > pagSlack; row++, top = new) {
	    bottom = top - marginSize->height;
	    if (bottom < maxY)
		bottom = maxY;
	    if (pagination == NX_AUTOPAGINATION) {
		new = MAXINT;	/* assume MAXINT non-sensical for debugging */
		[self adjustPageHeightNew:&new top:top bottom:bottom
		    limit:top - minHeight];
		NX_ASSERT(new != MAXINT, calcHeightsError);
		NX_ASSERT(new >= bottom, calcHeightsError);
	    } else
		new = bottom;
	    if (row == *numHeights) {
		NX_ZONEMALLOC(zone, results, float, rowsAlloced *= 2);
		bcopy(*defaultSpace, results, row*sizeof(float));
	    } else if (row == rowsAlloced)
		NX_ZONEREALLOC(zone, results, float, rowsAlloced *= 2);
	    if (new < bottom && new > top - minHeight)
		new = bottom;
	    results[row] = top - new;
	}
    *numHeights = row;
    *defaultSpace = results;
}


- (float)heightAdjustLimit
{
    return 0.2;			/* ??? whats a good default? */
}


- (float)widthAdjustLimit
{
    return 0.2;			/* ??? whats a good default? */
}


- adjustPageWidthNew:(float *)newRight left:(float)oldLeft
    right:(float)oldRight limit:(float)rightLimit
{
    int                 numSubs;/* #subviews */
    int                 i;
    View	       *subv;	/* current subview */
    float               currNew;/* current value for newRight, in our coords */
    float               oldNew;	/* previous value for newRight, in our coords */
    NXPoint             subLeft, subNew,	/* params in subview's coords */
                        subLimit;
    float               subRight;
  /* we force this value to be on the stack instead of in a register,
     because of compiler/68882 damage.  When NX_MAXX is evaluated, the
     expression ends up in a floating point reg, with extended precision.
     If this is compared to a float variable which has been assigned the
     same value, they are seen as unequal.  Yuck!
   */
    volatile float	maxX;
    volatile float	maxSubX;

    maxX = NX_MAXX(&bounds);
    if (oldRight > NX_X(&bounds) && oldRight < maxX)
	if (subviews && (numSubs = [subviews count])) {
	    currNew = oldRight;
	    do {
		oldNew = currNew;
		for (i = 0; i < numSubs; i++) {
		    subv = [subviews objectAt:i];
		/* quick test to minimize coord conversions */
		    maxSubX = NX_MAXX(&(subv->frame));
		    if (currNew > NX_X(&(subv->frame)) && currNew < maxSubX) {
			subLeft.x = oldLeft;
			subLimit.x = rightLimit;
			subNew.x = currNew;
			subLeft.y = subLimit.y = subNew.y = 0.0;
			[subv convertPointFromSuperview:&subLeft];
			[subv convertPointFromSuperview:&subLimit];
			[subv convertPointFromSuperview:&subNew];
			subRight = subNew.x;
			subNew.x = MAXINT;
			[subv adjustPageWidthNew:&subNew.x left:subLeft.x
			 right:subRight limit:subLimit.x];
			NX_ASSERT(subNew.x != MAXINT, calcWidthsError);
			NX_ASSERT(subNew.x <= subRight, calcWidthsError);
		    /* if subview made an adjustment, and still within limit */
			if (subNew.x < subRight && subNew.x >= subLimit.x) {
			    [subv convertPointToSuperview:&subNew];
			    currNew = subNew.x;	/* update currNew */
			}
		    }
		}
	    } while (oldNew != currNew);	/* until everyone agrees */
	    *newRight = currNew;
	} else
	    *newRight = oldLeft >= NX_X(&bounds) ? oldRight : NX_X(&bounds);
    else
	*newRight = oldRight;
    return self;
}


- adjustPageHeightNew:(float *)newBottom top:(float)oldTop
    bottom:(float)oldBottom limit:(float)bottomLimit
{
    BOOL                flipped;/* this view's orientation */
    int                 numSubs;/* #subviews */
    int                 i;
    View	       *subv;	/* current subview */
    float               currNew;/* current value for new, in our coords */
    float               oldNew;	/* previous value for new, in our coords */
    NXPoint             subTop, subNew,	/* values in subview's coords */
                        subLimit;
    float               subBottom;
  /* we force this value to be on the stack instead of in a register,
     because of compiler/68882 damage.  When NX_MAXY is evaluated, the
     expression ends up in a floating point reg, with extended precision.
     If this is compared to a float variable which has been assigned the
     same value, they are seen as unequal.  Yuck!
   */
    volatile float	maxY;
    volatile float	maxSubY;

    flipped = [self isFlipped];
    maxY = NX_MAXY(&bounds);
    if (oldBottom > NX_Y(&bounds) && oldBottom < maxY) {
	if (subviews && (numSubs = [subviews count])) {
	    currNew = oldBottom;
	    do {
		oldNew = currNew;
		for (i = 0; i < numSubs; i++) {
		    subv = [subviews objectAt:i];
		/* quick test to minimize coord conversions */
		    maxSubY = NX_MAXY(&(subv->frame));
		    if (currNew > NX_Y(&(subv->frame)) && currNew < maxSubY) {
			subTop.y = oldTop;
			subLimit.y = bottomLimit;
			subNew.y = currNew;
			subTop.x = subLimit.x = subNew.x = 0.0;
			[subv convertPointFromSuperview:&subTop];
			[subv convertPointFromSuperview:&subLimit];
			[subv convertPointFromSuperview:&subNew];
			subBottom = subNew.y;
			subNew.y = MAXINT;
			[subv adjustPageHeightNew:&subNew.y top:subTop.y
			 bottom:subBottom limit:subLimit.y];
			NX_ASSERT(subNew.y != MAXINT, calcHeightsError);
			NX_ASSERT([subv isFlipped] ? subNew.y <= subBottom
				  : subNew.y >= subBottom, calcHeightsError);
		    /* if subview made an adjustment, and still within limit */
			if ([subv isFlipped] ? (subNew.y < subBottom &&
						subNew.y >= subLimit.y)
			    : (subNew.y > subBottom &&
			       subNew.y <= subLimit.y)) {
			    [subv convertPointToSuperview:&subNew];
			    currNew = subNew.y;	/* update currNew */
			}
		    }
		}
	    } while (oldNew != currNew);	/* until everyone agrees */
	    *newBottom = currNew;
	} else if (flipped)
	    *newBottom = oldTop >= NX_Y(&bounds) ? oldBottom : NX_Y(&bounds);
	else
	    *newBottom = oldTop <= maxY ? oldBottom : maxY;
    } else
	*newBottom = oldBottom;
    return self;
}


/*
 * Prints a single page, given the rect to print in view coords.
 */
- (void)_doPageArea:(const NXRect *)localRect finishPage:(BOOL)finishFlag
    helpedBy:target pageLabel:(int)label
{
    NXRect              baseRect;	/* next rect we will image in base
					 * coords */
    NXPoint             placement;	/* offset used to place printRect on
					 * a page */
    id                  pInfo = [NXApp printInfo];
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    int                 pagesPerSheet = [pInfo pagesPerSheet];
    char                labelText[20];	/* string for the label */
    BOOL		reversePages;
    short		oldStatus;

    reversePages = [pInfo pageOrder] == NX_DESCENDINGORDER;
  /* factor next two lines */
    privPInfo->firstPageOfSheet = pagesPerSheet == 1 ||
	(reversePages ? (privPInfo->ordPage == privPInfo->numPages ||
				privPInfo->ordPage % pagesPerSheet == 0)
		      : (privPInfo->ordPage % pagesPerSheet == 1));
    privPInfo->lastPageOfSheet = pagesPerSheet == 1 ||
	(reversePages ? (privPInfo->ordPage % pagesPerSheet == 1)
		      : (privPInfo->ordPage == privPInfo->numPages ||
				privPInfo->ordPage % pagesPerSheet == 0));
    sprintf(labelText, "%d", label);
    if (privPInfo->firstPageOfSheet) {
	NX_X(&privPInfo->pageUnionBBox) =
	  NX_Y(&privPInfo->pageUnionBBox) = 0.0;
	NX_WIDTH(&privPInfo->pageUnionBBox) =
	  NX_HEIGHT(&privPInfo->pageUnionBBox) = -1.0;
	if (reversePages)
	    privPInfo->ordSheet--;
	else
	    privPInfo->ordSheet++;
	[Font _clearSheetFontsUsed];
    }
    [Font _clearPageFontsUsed];

 /* convert local printing rect into page coords */
    privPInfo->currentPrintingRect = *localRect;
    baseRect = *localRect;
    [self convertRectToSuperview:&baseRect];
    baseRect.origin.x = baseRect.origin.y = 0.0;
    baseRect.size.width *= privPInfo->totalScale;
    baseRect.size.height *= privPInfo->totalScale;
    [target placePrintRect:&baseRect offset:&placement];
    baseRect.origin = placement;

 /* generate xform to place page on sheet */
    _NXCalcPageOffsetOnSheet(pInfo, privPInfo);

 /* translate baseRect into sheet coord from page coords */
    if (pagesPerSheet > 1)
	_NXRectPageToSheet(&baseRect, pInfo, privPInfo);

 /* sets privPInfo->pageBBoxAtEnd */
    [target beginPage:privPInfo->ordSheet label:labelText
     bBox:(pagesPerSheet == 1 ? &baseRect : NULL) fonts:NULL];

    _NXUpdateBBox(privPInfo->pageBBoxAtEnd, &privPInfo->pageUnionBBox,
		  &baseRect);
    _NXUpdateBBox(privPInfo->docBBoxAtEnd, &privPInfo->docUnionBBox,
		  &baseRect);

    DPSSetContext([NXApp context]);		/* talk to the server */
    oldStatus = NXDrawingStatus;
    NXDrawingStatus = NX_DRAWING;
    if (privPInfo->session)
	[NXApp runModalSession:privPInfo->session];
    DPSSetContext([pInfo context]);		/* talk to printer */
    NXDrawingStatus = oldStatus;

    [target beginPageSetupRect:localRect placement:&placement];
    [target endPageSetup];
    [self display:localRect :1 :YES];
    if (finishFlag) {
	[target endPage];
#ifndef OLD_SPOOLER
	if (privPInfo->lastPageOfSheet || privPInfo->doingLastPage)
	    if (privPInfo->spoolPort)
		_NXNpdSend(NPD_SEND_PAGE, pInfo);
#endif
    }
}


- (BOOL)getRect:(NXRect *)theRect forPage:(int)page
{
    return NO;
}


/* By default this method either centers the image or aligns it to the top and left.  When centering, we only center if there is one row/col in that direction.
This keeps subsequent pages from being little strips in the middle of a page, instead of being aligned on the edge near what they are connected to  on the adjoining page. */
- placePrintRect:(const NXRect *)aRect offset:(NXPoint *)location
{
    id                  pInfo = [NXApp printInfo];
    NXCoord             lm, rm, tm, bm;
    NXRect              marginArea;
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

    marginArea = *[pInfo paperRect];
    [pInfo getMarginLeft:&lm right:&rm top:&tm bottom:&bm];
    NX_X(&marginArea) += lm;
    NX_Y(&marginArea) += bm;
    NX_WIDTH(&marginArea) -= lm + rm;
    NX_HEIGHT(&marginArea) -= tm + bm;
    location->x = NX_X(&marginArea);
  /* rows or cols 0 means app is paginating.  In this case we stay compatible with 1.0.  Also, unlike when the kit paginates, aRect can be bigger than marginRect, so we could crop info. */
    if ([pInfo isHorizCentered] && privPInfo->cols <= 1)
	location->x += (NX_WIDTH(&marginArea) - NX_WIDTH(aRect)) / 2;
    location->y = NX_MAXY(&marginArea) - NX_HEIGHT(aRect);
    if ([pInfo isVertCentered] && privPInfo->rows <= 1)
	location->y -= (NX_HEIGHT(&marginArea) - NX_HEIGHT(aRect)) / 2;
    return self;
}


- drawSheetBorder:(float)width :(float)height
{
#ifdef SPECIAL_DEBUG
    PSgsave();
    PSsetgray(0.2);
    PSrectfill(0.0, 0.0, width, height);
    PSgrestore();
#endif
    return self;
}


- drawPageBorder:(float)width :(float)height;
{
#ifdef SPECIAL_DEBUG
    PSgsave();
    PSsetgray(0.9);
    PSrectfill(0.0, 0.0, width, height);
    PSgrestore();
#endif
    return self;
}


- addCursorRect:(const NXRect *)aRect cursor:anObj
{
    NXRect              baseRect = *aRect;

    [self convertRect:&baseRect toView:nil];
    [window addCursorRect:&baseRect cursor:anObj forView:self];
    return self;
}

- removeCursorRect:(const NXRect *)aRect cursor:anObj
{
    NXRect              baseRect = *aRect;

    [self convertRect:&baseRect toView:nil];
    [window removeCursorRect:&baseRect cursor:anObj forView:self];
    return self;
}

- discardCursorRects
{
    [window _discardCursorRectsForView:self];
    return self;
}

- resetCursorRects
{
    return self;
}

- _resetCursorRects:(BOOL)flag		/* ??? wrp - what is the flag for? */
{
    [self resetCursorRects];
    if (subviews)
	subviewsPerform(self, @selector(_resetCursorRects:), flag);
    return self;
}

- (BOOL)shouldDrawColor
{
    return (NXDrawingStatus == NX_DRAWING) ? [window canStoreColor] : YES;
}

/*
 * ??? Rip this method out after Warp-4; it's already out of the header file.
 * -Ali
 */
- (BOOL)canStoreColor
{
    NXLogError ("Don't use -[View canStoreColor]");
    return [self shouldDrawColor];
}

- write:(NXTypedStream *) stream
{
    NXCoord             frameAngle;

    frameAngle = _frameMatrix ?[_frameMatrix getRotationAngle] : 0.0;
    [super write:stream];
    NXWriteTypes(stream, "f", &frameAngle);
    NXWriteRect(stream, &frame);
    NXWriteRect(stream, &bounds);
    NXWriteObjectReference(stream, superview);
    NXWriteObjectReference(stream, window);
    NXWriteTypes(stream, "@ss@", &subviews, &vFlags, &_vFlags, &_drawMatrix);

    return self;
}


- read:(NXTypedStream *) stream
{
    NXCoord             frameAngle;

    [super read:stream];
    NXReadTypes(stream, "f", &frameAngle);
    NXReadRect(stream, &frame);
    NXReadRect(stream, &bounds);
    superview = NXReadObject(stream);
    window = NXReadObject(stream);
    NXReadTypes(stream, "@ss@", &subviews, &vFlags, &_vFlags, &_drawMatrix);
    if (frameAngle != 0.0) {
	_frameMatrix = [[PSMatrix allocFromZone:[self zone]] init];
	[_frameMatrix _doRotationOnly];
	[_frameMatrix rotateTo:frameAngle];
    }
    return self;
}


@end

/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
12/12/88 trey	Added support for registration marks in
		 beginPageSetupRect
12/12/88 trey	Removed redundant lockFocus/unlockFocucs from display:::
		Moved blockFocusForPrint=NO to lockFocus for printing fix
01/12/89 trey	moved printing statics into PrivatePrintInfo struct
		prototyped all static routines
		changed the static NXAutoSizeView() to autoSizeView()
		unimplemented convertPastebordData nuked
		_real{Copy,Print}PSCode added to support Window's modifying
		  the printing process
		fixed temp filename we spool to to be real path, and fixed
		  error message with the name
		App name is written in conforming comments as %%Creator by
		  default
		multiple pages per sheet of paper supported
		resolution interpretation has been changed so that scaling
		  is applied before pagination instead of after
		new Pagination modes in PrintInfo supported
		all BoundingBox comments are fixed so dimensions corresspond
		  to default paper coord system
		added calls to clear font list on a per sheet basis
01/17/89 trey	printing outputs a %%Title: Untitled if no title is known
1/27/89	 bs	added read: write:
02/05/89 bgy	added support for cursor tracking. the following are new 
 		 methods:
		    - addCursorRect:(NXRect *)aRect cursor:anObj;
		    - removeCursorRect:(NXRect *)aRect cursor:anObj;
		    - discardCursorRects;
		    - resetCursorRects;
		    - _resetCursorRects:(BOOL)flag; 
02/17/89 trey	convertSize forces size to have positive components
		updates Print Panel with status lines
		supports save and preview options while printing
		conditional support for new spooling scheme
		puts out extra ps comment for resolution
02/09/89 wrp	_focusFromTo declared static
02/10/89 pah	add API for displayIfNeeded
		make vFlags.needsDisplay get set if display is called and
		 we can't draw because there is no window or display disabled
02/15/89 wrp	implemented: 
		 - replaceSubview:oldView with:newView
		 - addSubview:aView :(int)place relativeTo:otherView
		removed:
		 - (int)_gState
		 - (BOOL)_flipState
02/17/89 wrp	added -(BOOL)canDraw
  
0.83
----
03/17/89 trey	fixed bug in outputting bbox in copyPSCode:
		remember last button chosen in print panel, and use that
		 choice if no panel is presented to user
		fixed bug in outputting %%Orientation tag
03/10/89 wrp	fixed displayFromOpaqueAncestor to not change rects passed in
		(Bug fix)
 3/12/89 pah	implement displayIfNeeded:
03/15/89 wrp	fixed bug in _scroll. If noCopyOnScroll is set, the update
		 area is now the entire bounds, where before, it was just as
		 if noCopyOnScroll was clear (ie. a slice).
03/21/89 wrp	added check in scrollRect: to see if View canDraw before doing lockFocus.
03/31/89 trey	fixed bug convertSize with srcView == destView->superview
04/02/89 wrp	added _centerScanRect method
04/02/89 wrp	made _scroll respect window disabled flag by early exit if no canDraw.

0.91
----
04/19/89 wrp	fixed displayFromOpaqueAncestor for case where self is opaque (broken on 3/10 fix)
04/26/89 wrp	added ASSERT to unlockFocus to check self == topOfStack
05/09/89 wrp	added check to _focusFromTo to setgstate on null device if no window
05/10/89 wrp	added FocusState object to knothole focusing code
05/19/89 trey	minimized static data
		-free disconnects all subviews, then frees the list
		-free invalidates cursorrects for that view
		-removeFromSuperView calls [window _viewFreeing] (needs a
		 little more work)
		speaker used to message Previewer while printing now freed
		Document title defaults to window title when printing
		used an explicit name for save objects generated during
		 printing (per page and per doc)

0.92
-----
05/23/89 trey	made Printing page... message reflect doc page number instead
		 of ordinal number
		fixed infinite loop in pagination due to floating point
		 accumulation error.  currRect is now calc'ed from scratch
		 everytime instead of incrementally moved.
		page reversal implemented
		better error checking of page ranges
		foundation laid to pass hostname to npd as part of protocol

0.93
----
 6/15/89 pah	change NXAlert() to NXRunAlertPanel()
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps
 6/16/89 wrp	made display(fromOpaqueAncestor):0:0 use the visibleRect rather 
 		 than the bounds
 6/16/89 wrp	_centerScanRect public
 6/17/89 wrp	scrollPoint:fromView: and scrollRectToVisible:fromView:
 		 made private
 6/17/89 wrp	moved _vFlags.thumbing to Scroller. Removed _setThumbing:
 6/17/89 trey	made cancel button work during printing
		intermediate page ranges fixed when kit paginates
 6/25/89 wrp	modified hitTesting to use NXMouseInRect() rather than 
 		 NXPointInRect()
 6/25/89 wrp	added new method mouse:inRect: which uses NXMouseInRect()
 6/25/89 wrp	converted all use of NXPointInRect() to NXMouseInRect()
 6/25/89 trey	support added for setting current page in printInfo

0.94
----
 7/08/89 trey	fixed %%BoundingBox comments generated by copyPSCode
 7/08/89 trey	if we are printing to a local NeXT printer, we check its
		 existence by opening the printer device
 7/10/89 trey	fixed bug where modal sessions were run even when we were
		 printing without using a print panel
		added check for existence of local printer
		added _NXPureConvertSize
 7/10/89 wrp	added _invalidateGStates method and called from _removeSubview
 7/10/89 wrp	changed _focusStack from List to Storage to support error 
 		 recovery
 7/11/89 trey	previewing tmp files unlinked
 7/14/89 wrp	fixed bug in suspendNotifyAncestorWhenFrameChanged:. Changed ne 
 		 test to eq test.  This should never have worked before.
 7/18/89 trey	fixed off by one error in page labels of %%Page comments
 7/20/89 trey	fixed storage allocation probs when paginating large docs
 8/02/89 wrp	fixed freeGState to call _NXTermGState so that gState actually 
 		 freed.  This fix was needed for one-shot fix in window of same 
		 date.
 8/12/89 trey	_NXNpdSend calls that send page data are made outside of
		 -endPage, so that this happens even if someone overrides
 8/18/89 wrp	fixed centerScanRect to not return a zero height or width 
 		 rectangle when positive values passed in.  This fixes bug 
		 #2225
 8/23/89 wrp	addded uniRound function to call instead of rint().  This 
 		 function is more useful for graphics as it always rounds in 
		 the same graphical direction.  uniRound called in 
		 {centerScan,crack}{Rect,Point} This fixes bug #2233

76
--
 1/03/90 trey	plugged stream memory leak when printing

77
--
 2/28/90 king	fixed output of %%BoundingBox to be integers instead of floats
 3/02/90 trey	implemented incremental flushing of drawing
 3/09/90 trey	incremental flushing only happens after drawing leaf views

79
--
 3/21/90 trey	renamed _setGState to _setBorderViewGState
		deallocs gstate userobject with DPSUndefineUserObject
		added -renewGState
 3/21/90 king	nuked localPrinterExists().  The new npd will set an ignore
 		flag in the printer database if the local printer is not
		connected.
 3/21/90 cmf	added _generateFaxCoverSheet and modified various printing
 		routines to support Fax printing.
 3/30/90 king	fixed boundingbox comments to use ceil() and floor().

80
--
 4/8/90 trey	added drawSheetBorder::, drawPageBorder::
 4/8/90 aozer	Added getScreenInfo:rects:count:forRect:. 
 4/8/90 aozer	Added isOnColorScreen and fractionOnColorScreen.

82
--
 4/17/90 chris	changes _NXFAX* constants as per trey's request.
 4/20/90 aozer  renamed getScreenInfo:rects:count:forRect: to
		getScreens:andRects:count:forRect: 
83
--
 4/26/90 chris	added code to write out conforming comment for whether user
 		should be notified on fax completion.
 4/24/90 aozer	Added __NXbasematrix generation in 
		beginPageSetupRect:placement:
84
--
 5/13/90 trey	nuked incremental flush code

85
--
 5/20/90 trey	nuked useless public macros NX_XSIZING and NX_YSIZING
 6/4/90 pah	Added optimized rectfill and moveto/show drawing.
		 _canOptimizeDrawing returns what it says
		 _optimizedRectFill:gray: batches up a rectfill
		 _optimizedXYShow:at:: batches up a moveto/show
		 _setOptimizedXYShowFont:gray: batches up font/gray of show

86
--
 6/6/90 king	Ensure %%Title comments only contain printeable ASCII.
 		Only output %%Feature: *Resolution if it is non-zero.
		Fixed %%PageTrailer to have two percent signs.
 6/6/90 aozer	Cleaned up the color routines now that things are stabilized.
 6/9/90 trey	redid print package loading - checks for errors

87
--
 6/26/90 king	%%Title comments now escape non-ascii rather than replacing
 		with '#' which was my first solution.
 6/27/90 chris	modified to treat fax and printer as separate in printPSCode:

89
--
 7/19/90 chris	added code for faxPSCode: and support in _realPrintPSCode
 7/23/90 chris	added support for fax hires/lowres
 7/20/90 aozer	Removed isOnColorScreen & getScreens:andRects:count:forRect:

91
--
 8/6/90 trey	fixed pagination bugs with scaling views with non-auto
		 pagination modes (still using kit pagination)
		fixed default placePrintRect: to center a little less often
 8/8/90 trey	replaceSubview:withView: is a NOP if args are the same
 8/11/90 glc	fixed bug in moveto. Clear gstate to fix scrollview sizeto bug	
 8/12/90 aozer	On Don's suggestion, canStoreColor -> shouldDrawColor.

92
--
 8/13/90 trey	calcUpdateRects returns 3 rects with all the easy cases
 		calcUpdateRects does not smash good size parameter (unless 1.0)
 8/18/90 aozer	Cleared wantsGState flag after dearchive to workaround
		bug 7712.

93
--
 9/4/90 trey	fixed pagination bug, adding PAG_SLACK to prevent unwanted
		 horizontal pagination

94
--
 9/20/90 chris	fixed orphaned fax panel bug (9412)
 9/25/90 trey	fixed pagination bug where we didnt take advantage of extra
		 space due to a small Page Layout scale factor in CLIP mode

95
--
 9/27/90 chris	added support for multiple fax phone numbers as well as
 		writing out list of To: fields for better notification 
		messages
 10/1/90 trey	added -addToPageSetup
 10/1/90 aozer	Added _genBaseMatrix to generate the basematrix correctly.

96
--
 10/3/90 trey	fixed beginProlog to not emit octal in the document title

100
--
 10/24/90 chris	fixed coversheet page ordinal for case where last doc page
 		is not known (numPages = -1)
*/

/*
	Scroller.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Greg Cockroft
  
	Modifications:
  
	08/13/88 wrp	changed curValue to floatValue
			changed setCurValue to setFloatValue
	08/15/88 pah	Changed to use Control's enabled flag
	21Nov88  trey	fixed bouncy buttons with timer events
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "nextstd.h"
#import "Scroller.h"
#import "NXImage.h"
#import "Window.h"
#import "Application.h"
#import <defaults.h>
#import "timer.h"
#import <dpsclient/wraps.h>
#import <math.h>
#import "Window_Private.h"

#define DEFMIN		(0.0)
#define DEFMAX		(1.0)

#define	SCROLLEVENTMASK (NX_LMOUSEDOWNMASK|NX_LMOUSEUPMASK|NX_MOUSEDRAGGEDMASK)
#define	KEYMASKS	(NX_KEYDOWNMASK|NX_KEYUPMASK)
#define	MOUSEMASKS	(NX_MOUSEMOVEDMASK)

#define ARROWHEIGHT	(16.0)
#define KNOBHEIGHTMIN	(16.0)
#define	ARROWSLEN	((2.0*ARROWHEIGHT)+2.0)

#define	NUMARROWS	(8)

#define	X	origin.x
#define	Y	origin.y
#define	W	size.width
#define	H	size.height

static id	arrows[NUMARROWS] = {nil, nil, nil, nil, nil, nil, nil, nil};
static id	scrollKnob = nil;
static float	buttonDelay = -1.0;
static float	buttonPeriod = -1.0;
static float	knobDragDelay = -1.0;
static int	knobDragCount = 0;

#define HORIZ		(4)
#define DEC		(2)
#define LIT		(1)

static const char * const arrowName[] = {"NXscrollDown", "NXscrollDownH",
				   "NXscrollUp", "NXscrollUpH",
				   "NXscrollRight", "NXscrollRightH",
				   "NXscrollLeft", "NXscrollLeftH"
};

#define BACKCOLOR  NX_LTGRAY
#define SLOTCOLOR  (0.5)

@implementation Scroller:Control

+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}   

- initFrame:(const NXRect *)frameRect
{
    NXRect              fRect;
    BOOL                ih;

    ih = frameRect->W > frameRect->H;
    fRect = *frameRect;
    if (ih)
	fRect.H = NX_SCROLLERWIDTH;
    else
	fRect.W = NX_SCROLLERWIDTH;
    [super initFrame:&fRect];
    conFlags.enabled = 0;
    [self setFlipped:YES];
    [self setClipping:NO];
    if (!scrollKnob)
	[self drawParts];
    curValue = DEFMIN;
    sFlags.isHoriz = ih;
    [self checkSpaceForParts];	/* needs to go here because newFrame doesnt
				 * call sizeTo:: */
    return self;
}

- drawParts
{
    int                 i;

    for (i = 0; i < NUMARROWS; i++)
	arrows[i] = [NXImage findImageNamed:arrowName[i]];

    scrollKnob = [NXImage findImageNamed:"NXscrollKnob"];

    return self;
}


- awake
{
    [super awake];
    if (!scrollKnob)
	[self drawParts];
    return self;
}


- (NXRect *)calcRect:(NXRect *)aRect forPart:(int)partCode
{
    NXCoord             base;
    NXCoord             length;

    if (sFlags.partsUsable == NX_SCROLLERNOPARTS ||
	((sFlags.partsUsable == NX_SCROLLERONLYARROWS) && !((partCode == NX_DECLINE) || (partCode == NX_INCLINE))) || (sFlags.arrowsLoc == NX_SCROLLARROWSNONE && ((partCode == NX_DECLINE) || (partCode == NX_INCLINE)))) {
	NX_X(aRect) = NX_Y(aRect) = NX_WIDTH(aRect) = NX_HEIGHT(aRect) = 0.0;
	return (NXRect *)0;
    }
    *aRect = bounds;
    NXInsetRect(aRect, 1.0, 1.0);

    if (sFlags.isHoriz) {
	base = aRect->X;
	length = aRect->W;
    } else {
	base = aRect->Y;
	length = aRect->H;
    }

    switch (partCode) {
    case NX_DECLINE:
	if (sFlags.arrowsLoc == NX_SCROLLARROWSMAXEND)
	    base += length - ARROWSLEN + 1.0;
	length = ARROWHEIGHT;
	break;

    case NX_INCLINE:
	base += (sFlags.arrowsLoc == NX_SCROLLARROWSMAXEND) ? (length - ARROWHEIGHT) : ARROWHEIGHT + 1.0;
	length = ARROWHEIGHT;
	break;

    case NX_KNOB:
	if (sFlags.arrowsLoc != NX_SCROLLARROWSNONE)
	    length -= ARROWSLEN;
	_knobSize = floor(perCent * length);
	_knobSize = (_knobSize > KNOBHEIGHTMIN) ? _knobSize : KNOBHEIGHTMIN;
	base += floor(curValue * (length - _knobSize));
	length = _knobSize;
	if (sFlags.arrowsLoc == NX_SCROLLARROWSMINEND)
	    base += ARROWSLEN;
	break;

    case NX_KNOBSLOT:
	if (sFlags.arrowsLoc != NX_SCROLLARROWSNONE)
	    length -= ARROWSLEN;
	if (sFlags.arrowsLoc == NX_SCROLLARROWSMINEND)
	    base += ARROWSLEN;
	break;

    default:
	break;
    }

    if (sFlags.isHoriz) {
	aRect->X = base;
	aRect->W = length;
    } else {
	aRect->Y = base;
	aRect->H = length;
    }

    return aRect;
}

- checkSpaceForParts
{
    register NXCoord    length;

    length = (sFlags.isHoriz ? bounds.W : bounds.H);

    if (sFlags.arrowsLoc == NX_SCROLLARROWSNONE)
	sFlags.partsUsable = (length < KNOBHEIGHTMIN + 3.0) ? NX_SCROLLERNOPARTS : NX_SCROLLERALLPARTS;
    else
	sFlags.partsUsable = (length < ARROWSLEN) ? NX_SCROLLERNOPARTS :
	  (length < ARROWSLEN + KNOBHEIGHTMIN + 3.0) ? NX_SCROLLERONLYARROWS : NX_SCROLLERALLPARTS;

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


- (SEL)action
{
    return action;
}


- setAction:(SEL)aSelector
{
    _NXCheckSelector(aSelector, "[Scroller -setAction:]");
    action = aSelector;
    return self;
}



- sizeTo:(NXCoord)width :(NXCoord)height
{
    [super sizeTo:width :height];
    [self checkSpaceForParts];
    return self;
}


- setArrowsPosition:(int)where
{
    sFlags.arrowsLoc = where;
    return self;
}


- setEnabled:(BOOL)flag
{
    if (sFlags._thumbing)
	return self;
    return ([super setEnabled:flag]);
}

- drawArrow:(BOOL)whichButton :(BOOL)flag
{
    id                  bm;
    NXRect              displayRect;

    bm = arrows[(sFlags.isHoriz ? HORIZ : 0) + (whichButton ? DEC : 0) + (flag ? LIT : 0)];

    if ([self calcRect:&displayRect forPart:(whichButton ? NX_DECLINE : NX_INCLINE)]) {
	NX_Y(&displayRect) += ([self isFlipped] ? NX_HEIGHT(&displayRect) :0.);
	[bm composite:NX_COPY toPoint:&displayRect.origin]; 
    }

    return self;
}


- drawKnob
{
    static const NXRect dimpleRect = {{4.0, 4.0}, {8.0, 8.0}};
    NXRect              displayRect;
    NXPoint             dimplePoint;

    if ([self calcRect:&displayRect forPart:NX_KNOB]) {
	_NXDrawButton(&displayRect, (NXRect *)0, self);
	dimplePoint.x = NX_MIDX(&displayRect) - 4.;
	dimplePoint.y = NX_MIDY(&displayRect) + ([self isFlipped] ? 4. : -4.);
	[scrollKnob composite:NX_COPY
		    fromRect:&dimpleRect toPoint:&dimplePoint];
    }
    sFlags._knobDrawn = YES;
    return self;
}


- drawSelf:(const NXRect *)rects :(int)rectCount
{
    NXRect              displayRect;
    register NXRect    *dp = &displayRect;
    register int        i, j;

    if (!sFlags._slotDrawn) {
	NXRect tRect;
	
	if (sFlags.partsUsable == NX_SCROLLERALLPARTS) {
	    [self calcRect:&tRect forPart:NX_KNOBSLOT];
	    sFlags._slotDrawn = NXContainsRect(rects, &tRect);
	} else
	    sFlags._slotDrawn = YES;
    }
	
    PSsetgray(BACKCOLOR);
    NXRectFill(rects);

    if (!conFlags.enabled) {
        *dp = bounds;
	NXInsetRect(dp, 1.0, 1.0);
	if (NXIntersectionRect(rects, dp)) {
	    _NXSetGrayUsingPattern (SLOTCOLOR);
	    NXRectFill(dp);
	}
	return self;
    }
    for (j = 0; j < 4; j++) {
	static const int  displayOrder[] = {NX_KNOBSLOT, NX_KNOB, NX_DECLINE, NX_INCLINE};

	i = displayOrder[j];
	if ([self calcRect:dp forPart:i])
	    if (NXIntersectionRect(rects, dp)) {

		switch (i) {
		case NX_DECLINE:
		    [self drawArrow:YES :NO];
		    break;

		case NX_INCLINE:
		    [self drawArrow:NO :NO];
		    break;

		case NX_KNOB:
		    [self drawKnob];
		    break;

		case NX_KNOBSLOT:
		    _NXSetGrayUsingPattern (SLOTCOLOR);
		    NXRectFill(dp);
		    break;
		}
	    }
    }
    return self;
}


- highlight:(BOOL)flag
{
    if (hitPart != NX_INCLINE && hitPart != NX_DECLINE)
	return self;

    [self lockFocus];
    [self drawArrow:(hitPart == NX_DECLINE) :flag];
    [window flushWindow];
    [self unlockFocus];

    return self;
}


- (int)testPart:(const NXPoint *)thePoint
{
/* INDENT OFF */
    static const struct _partStruct {
	int                 code;
	NXCoord             size;
    } partTemplate[] = {{NX_DECLINE, ARROWHEIGHT},
			{NX_NOPART, 1.0},
			{NX_INCLINE, ARROWHEIGHT},
			{NX_NOPART, 1.0},
			{NX_DECPAGE, 0.0},
			{NX_KNOB, KNOBHEIGHTMIN},
			{NX_INCPAGE, 0.0},
			{NX_NOPART, 1.0},
			{NX_DECLINE, ARROWHEIGHT},
			{NX_NOPART, 1.0},
			{NX_INCLINE, ARROWHEIGHT}
    };
/* INDENT ON */
#define NUM_PARTS	(sizeof(partTemplate)/sizeof(struct _partStruct))
    struct _partStruct part[NUM_PARTS];
    register NXCoord    base;
    register NXCoord    length;
    register NXCoord    pos;
    register            i, count;
    NXPoint             localPoint;
    NXRect              aRect;
    float               arrowLen;

    if (sFlags.partsUsable == NX_SCROLLERNOPARTS)
	return NX_NOPART;

    bcopy(partTemplate, part, sizeof(struct _partStruct)*NUM_PARTS);
    localPoint = *thePoint;

    [self convertPoint:&localPoint fromView:nil];
    aRect = bounds;
    NXInsetRect(&aRect, 1.0, 1.0);
    if (!NXMouseInRect(&localPoint, &aRect, [self isFlipped]))
	return NX_NOPART;

    if (sFlags.isHoriz) {
	pos = localPoint.x;
	base = aRect.X;
	length = aRect.W;
    } else {
	pos = localPoint.y;
	base = aRect.Y;
	length = aRect.H;
    }

    arrowLen = sFlags.arrowsLoc == NX_SCROLLARROWSNONE ? 0.0 : ARROWSLEN;
    if (sFlags.partsUsable == NX_SCROLLERONLYARROWS) {
	part[4].size = (length  - arrowLen);
	part[5].size = part[6].size = 0.0;
	part[4].code = part[5].code = part[6].code = NX_NOPART;
    } else {
	part[4].size = floor(curValue * (length - _knobSize - arrowLen));
	part[5].size = _knobSize;
	part[6].size = ceil((1.0 - curValue) * (length - _knobSize - arrowLen));
    }

    i = sFlags.arrowsLoc == NX_SCROLLARROWSMINEND ? 0 : 4;
    count = sFlags.arrowsLoc == NX_SCROLLARROWSNONE ? 3 : 7;
    for (; count--; i++) {
	base += part[i].size;
	if (pos < base)
	    return (part[i].code);
    }
    return NX_NOPART;
}

- sendAction:(SEL)theAction to:theTarget
{
    sFlags._thumbing = (hitPart == NX_KNOB);
    [window disableFlushWindow];
    sFlags._needsEnableFlush = YES;
    [super sendAction:theAction to:theTarget];
    if (sFlags._needsEnableFlush) {
	[window reenableFlushWindow];
	sFlags._needsEnableFlush = NO;
    }
    [window flushWindowIfNeeded];
    sFlags._thumbing = NO;
    return self;
}


- trackKnob:(NXEvent *)theEvent
 /* theEvent->location is in global coordinates */
{
    NXCoord             range, delta, total, prev, p, offset;
    NXPoint             P;
    NXRect              r;
    float               newVal, oldVal;

    if (knobDragDelay == -1.0) {
    	knobDragDelay = atof(NXGetDefaultValue(NXSystemDomainName, "ScrollerKnobDelay"));
    	knobDragCount = atoi(NXGetDefaultValue(NXSystemDomainName, "ScrollerKnobCount"));
    }

    total = (sFlags.isHoriz ? bounds.W : bounds.H) / perCent;

    [self calcRect:&r forPart:NX_KNOBSLOT];
    range = (sFlags.isHoriz ? r.W : r.H) - _knobSize;

    [self calcRect:&r forPart:NX_KNOB];
    P = theEvent->location;
    [self convertPoint:&P fromView:nil];
    prev = (sFlags.isHoriz ? P.x : P.y);
    offset = prev - (sFlags.isHoriz ? r.X : r.Y) + (sFlags.arrowsLoc == NX_SCROLLARROWSMINEND ? ARROWSLEN : 0.);

    while (1) {
	if (NX_EVENTCODEMASK(theEvent->type) & KEYMASKS) {
	    /* pitch keys in this loop */;
	} else {
	    p = (sFlags.isHoriz ? P.x : P.y);
	    delta = p - prev;
	    if (delta != 0.) {
		if (theEvent->flags & NX_ALTERNATEMASK) {
		    float               s;
    
		    s = delta == 1.|| delta == -1.? delta :
		      delta == 2.|| delta == -2.? delta :
		      delta == 3.|| delta == -3.? delta * 2.:
		      delta * 4.;
    
		    newVal = curValue + s / total;
		} else
		    newVal = (p - offset) / range;
    
		[window disableFlushWindow];
		oldVal = curValue;
		[self setFloatValue:newVal :perCent];
		[window reenableFlushWindow];
		if (oldVal != curValue)
		    [self sendAction:action to:target];
    
		prev = p;
	    }
	    if (theEvent->type == NX_LMOUSEUP)
		break;		/* break out of while loop */
	    NXPing();
	}
	theEvent = [NXApp getNextEvent:SCROLLEVENTMASK | KEYMASKS];
	if (!(theEvent->flags & NX_ALTERNATEMASK)) {
	    NXEvent *tempEvent;
	    int i = 0;
	    
	    while (i++ < knobDragCount && theEvent->type == NX_LMOUSEDRAGGED && 
	    		(tempEvent = [NXApp getNextEvent:SCROLLEVENTMASK | KEYMASKS waitFor:knobDragDelay threshold:NX_MODALRESPTHRESHOLD]))
			theEvent = tempEvent;
	}
	P = theEvent->location;
	[self convertPoint:&P fromView:nil];
    }
    hitPart = NX_JUMP;
    [self sendAction:action to:target];

    return self;
}



- trackScrollButtons:(NXEvent *)theEvent
{
    int                 newPart;
    int                 origPart;
    NXHandler           exception;
    NXTrackingTimer     timer;
    
    if (buttonDelay == -1.0) {
	buttonDelay = atof(NXGetDefaultValue(NXSystemDomainName, "ScrollerButtonDelay"));
	buttonPeriod = atof(NXGetDefaultValue(NXSystemDomainName, "ScrollerButtonPeriod"));
    }
    
    origPart = hitPart;
    exception.code = 0;
    [self highlight:YES];
    [window addToEventMask:NX_FLAGSCHANGEDMASK];
    NX_DURING {
	(void)NXBeginTimer(&timer, buttonDelay, buttonPeriod);
	sFlags._fine = (theEvent->flags & NX_ALTERNATEMASK) ? 0 : 1;
	[self sendAction:action to:target];
	while (1) {
	    NXPing();
	    if (theEvent = [NXApp getNextEvent:SCROLLEVENTMASK | KEYMASKS |
					NX_FLAGSCHANGEDMASK | NX_TIMERMASK]) {
    
		if (theEvent->type == NX_TIMER) {
		    if (hitPart == NX_DECLINE || hitPart == NX_INCLINE)
			[self sendAction:action to:target];
    
		} else if (NX_EVENTCODEMASK(theEvent->type) & KEYMASKS) {
		    /* pitch these */;
		} else {		/* else its a real server event */
		    sFlags._fine = (theEvent->flags & NX_ALTERNATEMASK) ? 0:1;
		    if (theEvent->type == NX_LMOUSEUP)
			break;	/* break out of while */
    
		    else if (theEvent->type == NX_MOUSEDRAGGED) {
			newPart = [self testPart:&(theEvent->location)];
			if (newPart != hitPart) {
			    if (newPart != origPart)
				if (!((origPart == NX_DECLINE ||
				       origPart == NX_INCLINE) &&
				      (newPart == NX_DECLINE ||
				       newPart == NX_INCLINE)))
				    newPart = NX_NOPART;
			    if (newPart != hitPart) {
				if (newPart != NX_NOPART)
				    [window disableFlushWindow];
				[self highlight:NO];
				hitPart = newPart;
				if (newPart != NX_NOPART)
				    [window reenableFlushWindow];
				[self highlight:YES];
			    }
			}
		    } /* else eat a mouse down or flags-changed */
		}
	    }
	}
	NXEndTimer(&timer);
    } NX_HANDLER {
	exception = NXLocalHandler;
    } NX_ENDHANDLER
    [self highlight:NO];
    if (exception.code)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return self;
}


- mouseDown:(NXEvent *)theEvent
{
    int         oldMask;
    BOOL	hasCursorRects;

    if (!conFlags.enabled || (perCent == 1.0)) {
	[NXApp getNextEvent:NX_LMOUSEUPMASK];
	return self;
    }
    hitPart = [self testPart:&theEvent->location];

    if (hitPart == NX_NOPART)
	return self;

    hasCursorRects = [window _hasCursorRects];
    if(hasCursorRects)
	[window disableCursorRects];
    oldMask = [window addToEventMask:SCROLLEVENTMASK];

    if (hitPart == NX_KNOB)
	[self trackKnob:theEvent];
    else if (hitPart == NX_DECPAGE || hitPart == NX_INCPAGE) {
    /* jump knob to location, then drag */
	NXCoord             range, p;
	NXRect              r;
	NXPoint             P;

	[self calcRect:&r forPart:NX_KNOBSLOT];
	range = (sFlags.isHoriz ? r.W : r.H) - _knobSize;
	P = theEvent->location;
	[self convertPoint:&P fromView:nil];
	p = (sFlags.isHoriz ? P.x - r.X : P.y - r.Y);
	p -= _knobSize / 2.0;
	hitPart = NX_KNOB;
	[window disableFlushWindow];
	[self setFloatValue:p / range:perCent];
	[window reenableFlushWindow];
	[self sendAction:action to:target];
	[self trackKnob:theEvent];
    } else
	[self trackScrollButtons:theEvent];

    [window setEventMask:oldMask];
    if(hasCursorRects)
	[window enableCursorRects];

    return self;
}

- (int)hitPart
{
    if (!sFlags._fine) {
	if (hitPart == NX_DECLINE)
	    return NX_DECPAGE;
	else if (hitPart == NX_INCLINE)
	    return NX_INCPAGE;
    }
    return hitPart;
}


- (float)floatValue
{
    return curValue;
}


/* try to reduce bRect by portion of aRect that covers it. */
/* aRect and bRect are assumed to be the same size to make this faster!!! */
/* aRect is either horizontally or vertically, (not both) offset from bRect */
static void
uncoveredRect(aRect, bRect)
    register NXRect    *aRect, *bRect;
{
    if (!NXIntersectsRect(aRect, bRect))
	return;
    if (aRect->Y < bRect->Y) {	/* move to up */
	bRect->H = bRect->Y - aRect->Y;
	bRect->Y = aRect->Y + aRect->H;
    } else if (aRect->Y > bRect->Y) {	/* move to down */
	bRect->H = aRect->Y - bRect->Y;
    } else if (aRect->X < bRect->X) {	/* move to left */
	bRect->W = bRect->X - aRect->X;
	bRect->X = aRect->X + aRect->W;
    } else if (aRect->X > bRect->X) {	/* move to right */
	bRect->W = aRect->X - bRect->X;
    }
}


- setFloatValue:(float)aFloat :(float)percent
{
    NXRect              udRect[4];
    BOOL                copyKnob;

    if (sFlags._thumbing)
	return self;
	
    if (aFloat < 0.0)
	aFloat = 0.0;
    else if (aFloat > 1.0)
	aFloat = 1.0;
    if (percent < 0.0)
	percent = 0.0;
    else if (percent > 1.0)
	percent = 1.0;

	/*
	 * I'm disabling the copyKnob code below. It is an optimization which does not
	 * work when the scroller is not completely visible. This happens in the case
	 * of nested scroll views.   glc
	 */
    /*copyKnob = (percent == perCent) && sFlags._knobDrawn && sFlags._slotDrawn;*/
    copyKnob = 0;    
    [self calcRect:&udRect[1] forPart:NX_KNOB];

    curValue = aFloat;
    perCent = percent;

    [self calcRect:&udRect[2] forPart:(sFlags._slotDrawn ? NX_KNOB : NX_KNOBSLOT)];
    
    if (!NXEqualRect(&udRect[2], &udRect[1]) || !sFlags._slotDrawn) {
	if ([self canDraw]) {
	    if (sFlags._needsEnableFlush) {
		[window reenableFlushWindow];
		sFlags._needsEnableFlush = NO;
	    }
	    if (copyKnob) {
		[self lockFocus];
		PScomposite(NX_X(&udRect[1]),
			    NX_Y(&udRect[1]),
			    NX_WIDTH(&udRect[1]),
			    NX_HEIGHT(&udRect[1]),
			    NXNullObject,
			    NX_X(&udRect[2]),
			    NX_Y(&udRect[2]),
			    NX_COPY);
		uncoveredRect(&udRect[2], &udRect[1]);
		[self drawSelf:&udRect[1] :1];
		[window flushWindow];	/* changed display to drawSelf */
		[self unlockFocus];
	    } else {
		udRect[0] = udRect[1];
		NXUnionRect(&udRect[2], &udRect[0]);
		[self display:udRect :1];
	    }
	} else
	    sFlags._slotDrawn = NO;
    }
    return self;
}


- setFloatValue:(float)aFloat
{
    return ([self setFloatValue:aFloat :perCent]);
}


- (BOOL)acceptsFirstMouse
{
    return YES;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteObjectReference(stream, target);
    NXWriteTypes(stream, "ff:s", &curValue, &perCent, &action, &sFlags);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    target = NXReadObject(stream);
    NXReadTypes(stream, "ff:s", &curValue, &perCent, &action, &sFlags);
    return self;
}


@end


/*
  
Modifications (starting at 0.8):
01/05/88 bgy	made it so that mouse entered & exited event didn't get
		 thrown away during the tracking. 
1/27/89  bs	added read: write:
 3/21/89 wrp	added const to declarations
 4/02/89 wrp	removed bug in mouseDown for case where arrows are at minEnd (Horiz) where jump
 		 to location was off by the arrowsLen.

0.91
----
 5/19/89 trey	minimized static data

0.93
----
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps
 6/18/89 wrp	moved thumbing state into Scroller.  Used to prevent clients 
  		 from setting enabled or setting value during thumbing.
 6/18/89 wrp	added sFlags._slotDrawn to fix bug where the percent was being 
  		 changed when display was disabled.  When display was 
		 re-enabled, the scroller was not being drawn. Subsequent use 
		 then appeared to break the knob.  
 6/25/89 wrp	fixed bug where you get a knob the entire size of the slot
 		 by changing calc of knob to floor rather than ceil. Also,
		 disabled effect of mouseDown if perCent == 1.0 which should 
		 never be the case except when misused
 6/25/89 wrp	used flushWindowIfNeeded to avoid extra flushgraphics calls
 7/24/89 wrp	fixed bug # 1634 where scroll button wasn't being dehilighted 
 		when you dragged outside of the button.  This bug was 
		introduced by an earlier flush optimization.  The fix was to 
		only disableFlush when both buttons would be redrawn and 
		reenable before the second button would be drawn.
 7/31/89 wrp	fixed bug # 1880, 'testPart:' fixed to give right results when 
 		 only arrows showing (no knob).

0.99
----
 8/18/89 trey	fixed tracking loop to pitch key events in the loop,
		 not using DPSDiscardEvents
 8/22/89 wrp	fixed bug # 2233, in trackKnob:, if the value is not changed by 
 		 dragging, do not do the 'sendAction'
 8/24/89 wrp	fixed bug # 2186, in sendAction:to:, moved flushIfNeeded 
 		 outside of the test for needsEnableFlush.  Also, in 
		 drawSelf::, set sFlags._slotDrawn if no knob.

84
--
 5/7/90 aozer	Converted from Bitmap to NXImage.

88
--
 7/19/90 aozer	Made drawSelf:: call setpattern instead of setgray
 
91
--
 8/11/90 glc	disabled copyKnob code so nested scrollviews will work.	

105
---
 11/6/90 glc	Check for cursorrects before disabling and enabling.
*/




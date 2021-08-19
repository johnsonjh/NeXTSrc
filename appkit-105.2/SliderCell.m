/*
	SliderCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Cell_Private.h"
#import "SliderCell.h"
#import "Slider.h"
#import "NXImage.h"
#import "Window.h"
#import "View.h"
#import "nextstd.h"
#import <dpsclient/dpsNeXT.h>
#import <dpsclient/wraps.h>
#import <math.h>
#import <zone.h>

#define BARGRAY (cFlags1.disabled ? NX_LTGRAY : 0.5)

#define LEFTOFFSET 1.0
#define RIGHTOFFSET 1.0
#define TOPOFFSET 1.0
#define BOTTOMOFFSET 1.0

#define AUTODISPLAY(condition) if (condition) [[self controlView] updateCellInside:self]

#define fixValue(val) val=MIN(maxValue,val); val=MAX(minValue,val);

static id vertKnob, horizKnob = nil;


@implementation SliderCell:ActionCell


static void drawcellinside(SliderCell *self, double v)
{
    double minValue, maxValue;

    minValue = self->minValue;
    maxValue = self->maxValue;
    fixValue(v);
    if (self->value != v) {
	self->value = v;
	[[self controlView] updateCellInside:self];
    }
}

static void initKnobs()
{
    NXSize horizKnobSize, vertKnobSize;

    if (!horizKnob) {
	horizKnob = [NXImage findImageNamed:"NXhSliderKnob"];
	vertKnob = [NXImage findImageNamed:"NXvSliderKnob"];
	[horizKnob getSize:&horizKnobSize];
	[vertKnob getSize:&vertKnobSize];
	AK_ASSERT(horizKnobSize.height == vertKnobSize.width && horizKnobSize.width == vertKnobSize.height, "Horizontal and vertical knob size mismatch in SliderCell");
    }
}

+ new
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

+ (BOOL)prefersTrackingUntilMouseUp
{
    return YES;
}

- init
{
    [super init];
    value = 0.0;
    minValue = 0.0;
    maxValue = 1.0;
    [self setContinuous:YES];
    if (!horizKnob) initKnobs();
    return self;
}


- awake
{
    if (!horizKnob) initKnobs();
    return[super awake];
}


- (double)minValue
{
    return minValue;
}


- setMinValue:(double)aDouble
{
    double oldMinValue = minValue;

    minValue = aDouble;
    fixValue(value);
    AUTODISPLAY(oldMinValue != aDouble);

    return self;
}


- (double)maxValue
{
    return maxValue;
}


- setMaxValue:(double)aDouble
{
    double oldMaxValue = maxValue;

    maxValue = aDouble;
    fixValue(value);
    AUTODISPLAY(oldMaxValue != aDouble);

    return self;
}


- (const char *)stringValue
{
    static char *cachedStringValue = NULL;
    char workString[256];

    bzero(workString, 256);
    sprintf(workString, "%g", value);
    workString[255] = '\0';
    if (cachedStringValue) free(cachedStringValue);
    cachedStringValue = NXCopyStringBufferFromZone(workString, [self zone]);

    return cachedStringValue;
}


- setStringValue:(const char *)aString
{
    double d;
    if (_NXExtractDouble(aString, &d)) drawcellinside(self, d);
    return self;
}


- (int)intValue
{
    return (int)value;
}


- setIntValue:(int)anInt
{
    drawcellinside(self, (double)anInt);
    return self;
}


- (float)floatValue
{
    return (float)value;
}


- setFloatValue:(float)aFloat
{
    drawcellinside(self, (double)aFloat);
    return self;
}


- (double)doubleValue
{
    return value;
}


- setDoubleValue:(double)aDouble
{
    drawcellinside(self, aDouble);
    return self;
}


- setContinuous:(BOOL)flag
{
    if (flag) {
	cFlags2.dontActOnMouseUp = NO;
	cFlags2.actOnMouseDragged = YES;
    } else {
	cFlags2.actOnMouseDragged = NO;
    }
    return self;
}


- (BOOL)isContinuous
{
    return cFlags2.actOnMouseDragged ? YES : NO;
}


- (BOOL)isOpaque
{
    return YES;
}


static void insetTrackRect(NXRect *trackRect, BOOL flipped)
{
    trackRect->origin.x += LEFTOFFSET;
    trackRect->size.width -= LEFTOFFSET + RIGHTOFFSET;
    trackRect->origin.y += flipped ? TOPOFFSET : BOTTOMOFFSET;
    trackRect->size.height -= TOPOFFSET + BOTTOMOFFSET;
}

static void drawBar(NXRect *barRect, BOOL flipped, BOOL disabled, id view)
{
    int psSides[4];
    static const float grays[] = { NX_BLACK, NX_BLACK, NX_WHITE, NX_WHITE };

    psSides[1] = NX_XMIN;
    psSides[2] = NX_XMAX;
    if (flipped) {
	psSides[0] = NX_YMIN;
	psSides[3] = NX_YMAX;
    } else {
	psSides[0] = NX_YMAX;
	psSides[3] = NX_YMIN;
    }
    _NXDrawTiledRects(barRect, (NXRect *)0, psSides, grays, 4, view);
}


- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect
{
    NXSize knobSize;

    [vertKnob getSize:&knobSize];

    if (aRect->size.width > aRect->size.height) {
	theSize->height = knobSize.width + TOPOFFSET + BOTTOMOFFSET;
	if (aRect->size.width >= knobSize.height + LEFTOFFSET + RIGHTOFFSET &&
	    aRect->size.height >= theSize->height) {
	    theSize->width = floor(aRect->size.width);
	    return self;
	}
    } else {
	theSize->width = knobSize.width + LEFTOFFSET + RIGHTOFFSET;
	if (aRect->size.height >= knobSize.height + TOPOFFSET + RIGHTOFFSET &&
	    aRect->size.width >= theSize->width) {
	    theSize->height = floor(aRect->size.height);
	    return self;
	}
    }

    theSize->width = 0.0;
    theSize->height = 0.0;

    return self;
}


- getKnobRect:(NXRect *)knobRect flipped:(BOOL)flipped
{
    NXSize knobSize;
    double percent, offset;

    [vertKnob getSize:&knobSize];

    percent = (maxValue <= minValue) ? 0.0 : (value - minValue) / (maxValue - minValue);
    if (trackRect.size.width > trackRect.size.height) {
	offset = floor((trackRect.size.width - knobSize.height) * percent);
	knobRect->origin.x = trackRect.origin.x + offset;
	knobRect->origin.y = trackRect.origin.y;
	knobRect->size.width = knobSize.height;
	knobRect->size.height = knobSize.width;
    } else {
	offset = floor((trackRect.size.height - knobSize.height) * percent);
	knobRect->origin.x = trackRect.origin.x;
	knobRect->size.width = knobSize.width;
	knobRect->size.height = knobSize.height;
	if (flipped) {
	    knobRect->origin.y = trackRect.origin.y + trackRect.size.height - knobRect->size.height - offset;
	} else {
	    knobRect->origin.y = knobRect->origin.y + offset;
	}
    }

    return self;
}


- drawKnob:(const NXRect *)knobRect
{
    NXPoint knobPoint = knobRect->origin;

    if ([NXApp _flipState]) knobPoint.y += NX_HEIGHT(knobRect);

    if (NX_WIDTH(knobRect) > NX_HEIGHT(knobRect)) {
	[horizKnob composite:NX_COPY toPoint:&knobPoint];
    } else {
	[vertKnob composite:NX_COPY toPoint:&knobPoint];
    }

    return self;
}


- drawKnob
{
    NXRect knobRect;

    [self getKnobRect:&knobRect flipped:[NXApp _flipState]];
    [self drawKnob:&knobRect];

    return self;
}


- (BOOL)_calcTrackRect:(NXRect *)aRect
{
    NXSize size;

    [self calcCellSize:&size inRect:aRect];
    if (size.width && size.height) {
	aRect->origin.x += floor((aRect->size.width - size.width) / 2.0);
	aRect->origin.y += floor((aRect->size.height - size.height) / 2.0);
	return YES;
    } else {
	return NO;
    }
}


- drawBarInside:(const NXRect *)aRect flipped:(BOOL)flipped
{
    _NXSetGrayUsingPattern (BARGRAY);
    NXRectFill(aRect);
    return self;
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    if ([self controlView] != controlView) [self _setView:controlView];

    trackRect = *cellFrame;
    if ([self _calcTrackRect:&trackRect]) {
	drawBar(&trackRect, [controlView isFlipped], cFlags1.disabled, controlView);
	[self drawBarInside:&trackRect flipped:[controlView isFlipped]];
	[self drawKnob];
    }

    return self;
}


- drawInside:(const NXRect *)cellFrame inView:controlView
{
    if ([self controlView] != controlView) [self _setView:controlView];

    trackRect = *cellFrame;
    if ([self _calcTrackRect:&trackRect]) {
	insetTrackRect(&trackRect, [controlView isFlipped]);
	[self drawBarInside:&trackRect flipped:[controlView isFlipped]];
	[self drawKnob];
    }

    return self;
}


- (BOOL)startTrackingAt:(const NXPoint *)startPoint inView:controlView
{
    return[self continueTracking:NULL at:startPoint inView:controlView];
}


- (BOOL)continueTracking:(const NXPoint *)lastPoint at:(const NXPoint *)currentPoint inView:controlView
{
    NXRect              knobRect;
    NXPoint             p = *currentPoint;
    double              percent;
    NXCoord             knobLength;
    static int          count = 0;

    if ([NXApp currentEvent]->type == NX_MOUSEDOWN) {
	count = 0;
    } else if ([NXApp currentEvent]->type == NX_MOUSEUP && !count) {
	_offset = 0.0;
    } else {
	count++;
    }

    [self getKnobRect:&knobRect flipped:[controlView isFlipped]];
    if (trackRect.size.width > trackRect.size.height) {
	knobLength = knobRect.size.width;
	p.x -= trackRect.origin.x + knobLength / 2.0 - _offset;
	percent = p.x / (trackRect.size.width - knobLength);
    } else {
	knobLength = knobRect.size.height;
	p.y -= trackRect.origin.y + knobLength / 2.0 - _offset;
	percent = p.y / (trackRect.size.height - knobLength);
    }
    percent = MIN(percent, 1.0);
    percent = MAX(percent, 0.0);
    if (trackRect.size.height > trackRect.size.width &&
	[controlView isFlipped])
	percent = 1.0 - percent;
    value = (minValue + percent * (maxValue - minValue));
    value = MAX(value, minValue);
    [self drawBarInside:&knobRect flipped:[controlView isFlipped]];
    [self drawKnob];
    [[controlView window] flushWindow];

    return YES;
}

- stopTracking:(const NXPoint *)lastPoint at:(const NXPoint *)stopPoint inView:controlView mouseIsUp:(BOOL)flag
{
    [self continueTracking:stopPoint at:lastPoint inView:controlView];
    return self;
}


- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView
{
    NXRect              r;
    NXPoint             p;
    NXCoord		x;
    BOOL                retval;
    BOOL                oldDisabled = cFlags1.disabled;
    BOOL                flipped = [controlView isFlipped];

    if (cellFrame) {
	r = *cellFrame;
	if ([self _calcTrackRect:&r]) {
	    insetTrackRect(&r, flipped);
	    if (!NXEqualRect(&r, &trackRect)) {
		if (r.size.width == trackRect.size.width &&
		    r.size.height == trackRect.size.height) {
		    NXOffsetRect(&trackRect,
				 r.origin.x - trackRect.origin.x,
				 r.origin.y - trackRect.origin.y);
		} else {
		    cFlags1.disabled = YES;
		}
	    }
	} else {
	    cFlags1.disabled = YES;
	}
    }
    _offset = 0.0;
    if (!cFlags1.disabled && theEvent->type == NX_MOUSEDOWN) {
	[self getKnobRect:&r flipped:[controlView isFlipped]];
	p = theEvent->location;
	[controlView convertPoint:&p fromView:nil];
	if (NXMouseInRect(&p, &r, [controlView isFlipped])) {
	    if (trackRect.size.width > trackRect.size.height) {
		x = (trackRect.size.width - r.size.width) *
		    ((value - minValue) / (maxValue - minValue));
		x += trackRect.origin.x + r.size.width / 2.0;
		_offset = x - p.x;
	    } else {
		x = (trackRect.size.height - r.size.height) * (flipped ?
		     (1.0 - ((value - minValue) / (maxValue - minValue))) :
		     ((value - minValue) / (maxValue - minValue)));
		x += trackRect.origin.y + r.size.height / 2.0;
		_offset = x - p.y;
	    }
	}
    }
    retval = [super trackMouse:theEvent inRect:cellFrame ofView:controlView];

    cFlags1.disabled = oldDisabled;

    return retval;
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteTypes(stream, "ddd", &maxValue, &minValue, &value);
    return self;
}


- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadTypes(stream, "ddd", &maxValue, &minValue, &value);
    return self;
}


@end

/*
  
Modifications (since 0.8):
  
11/21/88 pah	eliminated cFlags2.flipped support (had to change
		 _calcKnobPosition: to _calcKnobPosition:flipped:)
11/22/88 pah	changed meaning of setContinuous: to mean send the
		 action whenever the mouse moves instead of sending
		 periodically whether the mouse moves or not (by
		 overriding setContinous: and having it do a
		 sendActionOn:NX_MOUSEDRAGGEDMASK|NX_MOUSEUPMASK)
11/23/88 pah	eliminate _isInsideOpaque
12/11/88 pah	added readSelf:archiver: and writeSelf:archiver: to
		 provide compatibility for new meaning of
		 setContinuous:
0.82
----
  1/6/89 pah	added drawKnob:, getKnobRect:flipped:, and drawKnob interface
		elminated updateKnobPosition:
 1/10/89 pah	made value a double
		eliminated _lastValue
		eliminated _calcKnobPosition:
		added getKnobRect:flipped:, drawKnob: and drawKnob
		added _calcTrackRect (cleaned up drawSelf:inView:)
 1/25/89 trey	made setString* methods take const params
 1/28/89 bs	added read: write:
 2/07/89 pah	make max and minValue be doubles

0.92
----
 6/12/89 pah	fixed jumping Slider bug
 6/12/89 pah	made clicks on the knob jump the knob

0.93
----
 6/15/89 pah	stringValue now returns const char *
		make things autodisplay

84
--
 5/8/90 aozer	Converted from Bitmap to NXImage; changed drawKnob:

88
--
 7/19/90 aozer	Made drawBarInside: call setpattern instead of setgray

95
--
 10/3/90 aozer	Overrode prefersTrackingUntilMouseUp to return YES to make
		Matrix of Sliders behave reasonably (bugs 9622, 10076).

 
*/




/*
	Slider.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Slider.h"
#import "SliderCell.h"
#import "Application.h"
#import "Window.h"
#import <math.h>
#import <zone.h>

static const NXRect defaultFrameRect = {{0.0, 0.0}, {30.0, 100.0}};
static id sliderCellClass = nil;

@implementation Slider:Control

+ setCellClass:factoryId
{
    sliderCellClass = factoryId;
    return self;
}


+ newFrame:(const NXRect *)frameRect
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:frameRect];
}


- initFrame:(const NXRect *)frameRect
{
    NXRect frect;
    NXZone *zone = [self zone];

    if (frameRect) {
	frect = *frameRect;
	frect.origin.x = floor(frect.origin.x + 0.5);
	frect.origin.y = floor(frect.origin.y + 0.5);
    } else {
	frect = defaultFrameRect;
    }
    [super initFrame:&frect];
    if (!sliderCellClass) sliderCellClass = [SliderCell class];
    if (_NXCanAllocInZone(sliderCellClass, @selector(new), @selector(init))) {
	cell = [[sliderCellClass allocFromZone:zone] init];
    } else {
	cell = [sliderCellClass new];
    }
    vFlags.opaque = NO;
    [self setClipping:NO];
    [self setFlipped:YES];

    return self;
}


- sizeToFit
{
    NXSize theSize;
    [cell calcCellSize:&theSize inRect:&bounds];
    return [self sizeTo:theSize.width:theSize.height];
}

- (double)minValue
{
    return[cell minValue];
}

- setMinValue:(double)aDouble
{
    [cell setMinValue:aDouble];
    return self;
}

- (double)maxValue
{
    return[cell maxValue];
}

- setMaxValue:(double)aDouble
{
    [cell setMaxValue:aDouble];
    return self;
}

- setEnabled:(BOOL)flag
{
    [cell setEnabled:flag];
    return self;
}


static BOOL mouseHit(id view, NXRect *r, NXPoint *p)
{
    NXPoint localPoint = *p;
    [view convertPoint:&localPoint fromView:nil];
    return (NXMouseInRect(&localPoint, r, [view isFlipped]));
}


#define ACTIVESLIDERMASK	(NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK)

- mouseDown:(NXEvent *)theEvent
{
    int mouseUp, oldMask;

    oldMask = [window addToEventMask:ACTIVESLIDERMASK];

    if ([cell isEnabled]) {
	[self lockFocus];
	for (;;) {
	    if (mouseHit(self, &bounds, &theEvent->location)) {
		mouseUp = [cell trackMouse:theEvent inRect:(NXRect *)0 ofView:self];
		if (mouseUp) {
		    DPSFlush();
		    break;
		}
	    }
	    theEvent = [NXApp getNextEvent:ACTIVESLIDERMASK];
	    if (theEvent->type == NX_MOUSEUP) break;
	}
	[window flushWindow];
	[self unlockFocus];
    } else {
	theEvent = [NXApp getNextEvent:NX_MOUSEUPMASK];
    }

    [window setEventMask:oldMask];

    return self;
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}

@end

/*
  
Modifications (since 0.8):
  
12/5/88  pah	add factory method setCellClass:
  
0.82
----
  1/6/80 pah	added autodisplay to setMinValue:, setMaxValue: and overrode
		setEnabled: to autodisplay the inside only

0.93
----
 6/15/89 pah	remove redundant autodisplay calls
 6/16/89 wrp	put flushWindow inside focus lock to reduce ps

*/

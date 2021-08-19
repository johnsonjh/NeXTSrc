/*
	NXColorWell.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Box_Private.h"
#import "NXColorWell.h"
#import "NXColorPanel.h"
#import "NXColorPicker.h"
#import "Application.h"
#import "publicWraps.h"
#import "privateWraps.h"
#import "colorPickerPrivate.h"
#import "nextstd.h"
#import <objc/List.h>
#import <dpsclient/wraps.h>
#import <libc.h>
#import <math.h>

#define WELLBORDER 6.0

static id wellList = nil;

@implementation NXColorWell

- _takeColorFromIfContinuous:sender
{
   if ([self isContinuous]) [self takeColorFrom:sender];
   return self;
}

+ deactivateAllWells
{
    id tempList;

    tempList = wellList;
    wellList = nil;
    [tempList makeObjectsPerform:@selector(deactivate)];
    [tempList empty];
    wellList = tempList;

    return self;
}

- (BOOL)isBordered
{
    return _isBordered;
}

- setBordered:(BOOL)flag
{
    _isBordered = flag ? YES : NO;
    return self;
}

- initFrame:(NXRect const *)theFrame
{
    [super initFrame:theFrame];
    [self setBordered:YES];
    color = NX_COLORWHITE;
    if (wellList == nil) wellList = [[List allocFromZone:[self zone]] init];
    return self;
}

+ activeWellsTakeColorFrom:sender
{
    [self activeWellsTakeColorFrom:sender continuous:NO];
    return self;
}

+ activeWellsTakeColorFrom:sender continuous:(BOOL)flag
{
    if(flag) {
	[wellList makeObjectsPerform:@selector(_takeColorFromIfContinuous:) with:sender];
    } else {
	[wellList makeObjectsPerform:@selector(takeColorFrom:) with:sender];
    }
    return self;
}

- setEnabled:(BOOL)flag
{
    [super setEnabled:flag];
    [self update];
    return self;
}

- awake
{
    if (wellList == nil) wellList = [[List allocFromZone:[self zone]] init];
    return [super awake];
}

- drawColor
{
    NXCoord inset;
    NXRect colorRect;

    if (_cantDraw) return NO;

    colorRect = bounds;
    inset = 2.0 + (_isBordered ? WELLBORDER : 0.0);
    NXInsetRect(&colorRect, inset, inset);

    return [self drawWellInside:&colorRect];
}

- drawWellInside:(const NXRect *)insideRect
{
    BOOL lockedFocus = [self isFocusView];

    if (insideRect && [self canDraw]) {
	if (!lockedFocus) [self lockFocus];
	NXSetColor(NX_COLORWHITE);
	if (NXDrawingStatus == NX_DRAWING) {
	    PScompositerect(insideRect->origin.x, insideRect->origin.y, insideRect->size.width, insideRect->size.height, NX_COPY);
	}
	// No need for an else part; we'll overwrite this area below.
	// ??? We could show the alpha better on the printer by premultipling
	// towards black & white like it happens on the screen but after 2.0.
	if (NXAlphaComponent(color) != 1.0 && NXAlphaComponent(color) != NX_NOALPHA) {
	    PSsetgray(NX_BLACK);
	    PSmoveto(insideRect->origin.x, insideRect->origin.y);
	    PSrlineto(0.0, insideRect->size.height);
	    PSrlineto(insideRect->size.width, 0.0);
	    PSclosepath();
	    PSfill();
	}
	NXSetColor(color);
	if (NXDrawingStatus == NX_DRAWING) {
	    PScompositerect(insideRect->origin.x, insideRect->origin.y, insideRect->size.width, insideRect->size.height, NX_SOVER);
	} else {
	    NXRectFill (insideRect);
	}
	if (!lockedFocus) [self unlockFocus];
    }

    return self;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
    static const int    psSides[] = {   NX_YMAX,   NX_XMIN,   NX_XMAX,  NX_YMIN,  NX_YMAX,  NX_XMIN,  NX_XMAX,   NX_YMIN };
    static const int    flipSides[] = { NX_YMIN,   NX_XMIN,   NX_XMAX,  NX_YMAX,  NX_YMIN,  NX_XMIN,  NX_XMAX,   NX_YMAX };
    static const float  grays[] = {     NX_DKGRAY, NX_DKGRAY, NX_WHITE, NX_WHITE, NX_BLACK, NX_BLACK, NX_LTGRAY, NX_LTGRAY };
    static const float  disabledgrays[] = {     NX_LTGRAY, NX_LTGRAY, NX_WHITE, NX_WHITE, NX_DKGRAY, NX_DKGRAY, NX_LTGRAY, NX_LTGRAY };
    static const float  disabledbuttongrays[] = {     .833, .833, NX_DKGRAY, NX_DKGRAY, NX_LTGRAY, NX_LTGRAY, NX_LTGRAY, NX_LTGRAY };
    NXRect bezel = bounds;

    if (rects->size.width < 20.0 || rects->size.height < 20.0) return self;
    if(conFlags.enabled){
	if (_isBordered) {
	    NXDrawButton(&bounds, &bounds);
	    if (_isActive) {
		if (NXDrawingStatus == NX_DRAWING) {
		    PScompositerect(bounds.origin.x + 1.0, 
				bounds.origin.y + 1.0,
				bounds.size.width - 2.0, 
				bounds.size.height - 2.0,
				NX_HIGHLIGHT);
		} else {
		    PSsetgray (NX_WHITE);
		    PSrectfill(bounds.origin.x + 1.0, 
				bounds.origin.y + 1.0,
				bounds.size.width - 2.0, 
				bounds.size.height - 2.0);
		}
	    }
	    NXInsetRect(&bezel, WELLBORDER, WELLBORDER);
	}
	NXDrawTiledRects(&bezel, &bounds, [self isFlipped] ? flipSides : psSides, grays, 8);
    } else {
	if (_isBordered) {
	    NXDrawTiledRects(&bezel, &bounds, [self isFlipped] ? flipSides : psSides, disabledbuttongrays, 8);
	    PSsetgray(NX_LTGRAY);
	    PSrectfill(bounds.origin.x + 2.0, 
				bounds.origin.y + 2.0,
				bounds.size.width - 4.0, 
				bounds.size.height - 4.0);
	    NXInsetRect(&bezel, WELLBORDER, WELLBORDER);
	}
	NXDrawTiledRects(&bezel, &bounds, [self isFlipped] ? flipSides : psSides, disabledgrays, 8);
    }
    [self drawColor];

    return self;
}

- deactivate
{
    [wellList removeObject:self];
    _isActive = NO;
    return [self display];
}

- takeColorFrom:sender
{
   if(!conFlags.enabled)return self;
   [self setColor:[sender color]];
   [self sendAction:_action to:_target];
   return self;
}

- (BOOL)isActive
{
    return _isActive;
}

- (BOOL)isContinuous
{
    return _isNotContinuous ? NO : YES;
}

- setContinuous:(BOOL)flag
{
    _isNotContinuous = flag ? NO : YES;
    return self;
}

- (int)activate:(int)exclusive
{
    int retval;
    id tempList;

    if (exclusive) {
	tempList = wellList;
	wellList = nil;
	[tempList removeObject:self];
	[tempList makeObjectsPerform:@selector(deactivate)];
        [tempList empty];
	wellList = tempList;
    }

    [wellList addObjectIfAbsent:self];
    if (_isActive) return [wellList count];
    _isActive = YES;

    _cantDraw = YES;
    retval = [wellList count];
    if (retval > 1) {
	[self takeColorFrom:[wellList objectAt:0]];
    } else if ([NXColorPanel sharedInstance:NO]) {
	[[NXColorPanel sharedInstance:NO] setColor:color];
    }
    _cantDraw = NO;

    [self display];

    return retval;
}

- acceptColor:(NXColor)aColor atPoint:(NXPoint *)aPoint
{
   if(!conFlags.enabled)return self;
   if (!NXEqualColor(aColor, color)) {
	if (_isActive && [NXColorPanel sharedInstance:NO]) {
	    [[NXColorPanel sharedInstance:NO] setColor:aColor];
	} else {
	    [self setColor:aColor];
	    [self sendAction:_action to:_target];
	}
    }
    return self;
}

- (NXColor)color
{
    return color;
}

- setColor:(NXColor)aColor
{
   color = aColor;
   [self drawColor];
   [window flushWindow];
   return self;
}

- (BOOL)acceptsFirstMouse
{
   return YES;
}

- mouseDown:(NXEvent *)theEvent
{
    NXPoint mouseLocation;
    BOOL exclusive = YES;
    NXRect colorRect;
    NXCoord inset;
 
    mouseLocation = theEvent->location;
    if(!conFlags.enabled)return self;
    [self convertPoint:&mouseLocation fromView:nil];

    colorRect = bounds;
    inset = 2.0 + (_isBordered ? WELLBORDER : 0.0);
    NXInsetRect(&colorRect, inset, inset);
    if (NXPointInRect(&mouseLocation, &colorRect)) {
	[NXColorPanel dragColor:color withEvent:theEvent fromView:self];
    } else if (_isBordered) {
        exclusive = (theEvent->flags & NX_SHIFTMASK) ? NO : YES;
	if (_isActive) {
	    if (exclusive) {
		if ([wellList count] == 1) {
		    [self deactivate];
		} else {
		    [self activate:YES];
		    [NXApp orderFrontColorPanel:NXApp];
		}
	    } else {
		[self deactivate];
	    }
	} else {
	    [self activate:exclusive];
	    [NXApp orderFrontColorPanel:NXApp];
	}
    }
    [window flushWindow];
    return self;
}

- target
{
    return _target;
}

- setTarget:anObject
{
    _target=anObject;
    return self;
}

- (SEL)action
{
    return _action;
}

- setAction:(SEL)aSelector
{
    _action = aSelector;
    return self;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteColor(stream, color);
    NXWriteTypes(stream, "cc", &_isBordered, &_isActive);
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read:stream];
    color = NXReadColor(stream);
    NXReadTypes(stream, "cc", &_isBordered, &_isActive);
    if (_isActive) {
	if (wellList == nil) wellList = [[List allocFromZone:[self zone]] init];
	[wellList addObject:self];
    }
    return self;
}

@end

/*

appkit-85
---------
6/2/90 kro	new control for keeping track of a particular color and cummunicating with the color panel.
	
appkit-89
---------
7/29/90 kro	added read and write.

94
--
 9/19/90 aozer	Changed [NXColorPanel new] to [NXApp orderFrontColorPanel:...]
		in mouseDown: to make mouse down on border bring the color
		panel up...

95
--
 10/2/90 keith	Added setContinuous and isNotContinuous...

96
--
 10/4/90 aozer	Made [NXApp orderFrontColorPanel:...] happen only on activate.
 10/4/90 aozer	Renamed setNotContinuous: & isNotContinuous to 
		setContinuous: & isContinuous.

98
--
 10/16/90 aozer	Made NXColorWell print; it was generating compositerects.

*/

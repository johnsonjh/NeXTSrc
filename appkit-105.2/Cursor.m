/*
	NXCursor.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Ali Ozer
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Application_Private.h"
#import "Cursor.h"
#import "NXCursor.h"
#import "View.h"
#import "cursorprivate.h"
#import "packagesWraps.h"
#import "graphics.h"
#import <dpsclient/wraps.h>
#import <dpsclient/dpsNeXT.h>
#import <objc/List.h>
#import <zone.h>

extern id NXWait;

static id cursorStack = nil, currentCursor = nil;

@interface Cursor(Obsolete)
- (BOOL)lockFocus;
- unlockFocus;
@end

@implementation NXCursor

+ newFromImage:newImage
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFromImage:newImage];
}

- initFromImage:newImage
{
    [super init];
    [self setImage:newImage];
    return self;
}

- init
{
    return [self initFromImage:nil];
}

- image
{
    return image;
}

- setImage:newImage
{
    image = newImage;
    return self;
}

- setHotSpot:(const NXPoint *)spot
{
    hotSpot = *spot;
    return self;
}

- setOnMouseExited:(BOOL)flag
{
    cFlags.onMouseExited = flag;
    return self;
}

- setOnMouseEntered:(BOOL)flag
{
    cFlags.onMouseEntered = flag;
    return self;
}

+ currentCursor
{
    return currentCursor;
}

- set
{
    if (self != NXWait && NXDrawingStatus == NX_DRAWING && [image lockFocus]) {
	float absY = hotSpot.y < 0 ? -hotSpot.y : hotSpot.y;
	BOOL flipped = [image isFlipped];
	_NXSetCursor([NXApp _currentActivation],
		0.0, flipped ? 0.0 : 16.0, hotSpot.x, flipped ? absY : -absY);
	[image unlockFocus];
	NXPing();
	currentCursor = self;
    }
    return self;
}

/*
 * Don't know if this should be public or not... Workspace could use it for sure. 
 */
- forceSet
{
    if (self != NXWait && NXDrawingStatus == NX_DRAWING && [image lockFocus]) {
	float absY = hotSpot.y < 0 ? -hotSpot.y : hotSpot.y;
	BOOL flipped = [image isFlipped];
	PSsetcursor (0.0, flipped ? 0.0 : 16.0, hotSpot.x, flipped ? absY : -absY);
	[image unlockFocus];
	NXPing();
    }
    return self;
}

- mouseEntered:(NXEvent *)theEvent
{
    if (cFlags.onMouseEntered)
	[self set];
    return self;
}


- mouseExited:(NXEvent *)theEvent
{
    if (cFlags.onMouseExited)
	[self set];
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadPoint(stream, &hotSpot);
    NXReadTypes(stream, "s@", &cFlags, &image);
    return self;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWritePoint(stream, &hotSpot);
    NXWriteTypes(stream, "s@", &cFlags, &image);
    return self;
};

- push
{
    if (!cursorStack) {
	cursorStack = [List new];
    }
    [cursorStack addObject:self];
    [self set];
    return self;
}

+ pop
{
    id current;
    if (!cursorStack || ![cursorStack count])
	return self;
    [cursorStack removeLastObject];
    current = [cursorStack lastObject];
    if (current)
	[current set];
    else 
	[NXApp _restoreCursor];
    return self;
}

- pop
{
    [[self class] pop];
    return self;
}
   
+ _buildCursor:(const char *)imageName cursorData:(const NXCursorData *)data
{
    if (self = [self newFromImage:[NXImage findImageNamed:imageName]]) {
	NXPoint theHotSpot = {(float)data->hotX, (float)data->hotY};
	[self setHotSpot:&theHotSpot];
    }
    return self;
}

static const NXCursorData arrowData = {{0., 48.}, 1, -1};
static const NXCursorData iBeamData = {{32., 48.}, 4, -8};
static const NXCursorData waitData = {{48., 48.}, 8, -8};

+ _makeCursors
 /*
  * TYPE: Creating 
  *
  * Private factory method used to create the cursors when the application
  * starts. 
  */
{
    NXArrow = [self _buildCursor:"NXarrow" cursorData:&arrowData];
    NXIBeam = [self _buildCursor:"NXibeam" cursorData:&iBeamData];
    NXWait =  [self _buildCursor:"NXwait"  cursorData:&waitData];
    return self;
}

@end


@implementation Cursor:Bitmap

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ new
{
    self = [super newSize:CURSORSIZE :CURSORSIZE type:NX_ALPHABITMAP];
    return self;
}


- setHotSpot:(const NXPoint *)spot
{
    hotSpot = *spot;
    return self;
}


- setOnMouseExited:(BOOL)flag
{
    cFlags.onMouseExited = flag;
    return self;
}


- setOnMouseEntered:(BOOL)flag
{
    cFlags.onMouseEntered = flag;
    return self;
}


- set
{
    if (self != NXWait && NXDrawingStatus == NX_DRAWING) {
	[self lockFocus];
	_NXSetCursor([NXApp _currentActivation], 0.0, 0.0, hotSpot.x, hotSpot.y); 
	[self unlockFocus];
	NXPing();
	currentCursor = self;
    }
    return self;
}


- mouseEntered:(NXEvent *)theEvent
{
    if (cFlags.onMouseEntered)
	[self set];
    return self;
}


- mouseExited:(NXEvent *)theEvent
{
    if (cFlags.onMouseExited)
	[self set];
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadPoint(stream, &hotSpot);
    NXReadTypes(stream, "s", &cFlags);
    return self;
}

- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWritePoint(stream, &hotSpot);
    NXWriteTypes(stream, "s", &cFlags);
    return self;
};

- push
{
    if (!cursorStack)
	cursorStack = [List new];
    [cursorStack addObject:self];
    [self set];
    return self;
}

- pop
{
    id current;
    if (!cursorStack || ![cursorStack count])
	return self;
    [cursorStack removeLastObject];
    current = [cursorStack lastObject];
    if (current)
	[current set];
    else 
	[NXApp _restoreCursor];
    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/07/88 trey	fixed hot spot of I-beam cursor.
03/28/89 wrp	changed hotspot of arrow cursor to be 1 pixel higher

0.91
----
 5/19/89 trey	minimized static data
 8/16/89 bgy	made set not draw when printing

77
--
 2/18/89 trey	got rid of lockFocus in -set

81
--
 4/12/90 aozer	Created NXCursor; eventually will get rid of Cursor.
		Made NXArrow etc instances of NXCursor.

85
--
 5/19/90 trey	Special hack in -set to prevent ever setting the wait cursor.

91
--
 8/13/90 aozer	NXCursor does not free its image anymore (so its more like
		Button & ButtonCell). setImage: returns self instead of old
		image; also added method to return image.
 8/16/90 aozer	Changed NXCursor set to check flipped status on determining 
		hotspot. With NXCursor, the hotspot is specified from the TOP 
		LEFT corner of the image, whether or not the image is flipped. 
		If a negative y hotspot is provided, its absolute value is used 
		to make all old Cursor users happy and to keep NXarrow & 
		friends compatible.

*/



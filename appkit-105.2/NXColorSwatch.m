/*
	NXColorSwatch.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "NXColorSwatch.h"
#import "NXColorPicker.h"
#import "NXColorPanel.h"
#import "NXColorWheel.h"
#import "NXColorWell.h"
#import "Application.h"
#import "Window.h"
#import "color.h"
#import "colorPickerPrivate.h"
#import <dpsclient/wraps.h>
#import <sys/dir.h>
#import <fcntl.h>
#import <libc.h>
#import <math.h>
#import <mach.h>
#import <zone.h>

#define SWATCHW 12.0
#define SWATCHH 12.0
#define DRAGDISTANCE 3.0

@implementation NXColorSwatch:View

+ newFrame:(NXRect const *)theFrame
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFrame:theFrame];
}

- (BOOL)acceptsFirstMouse
{
    return YES;
}

- drawSelf:(const NXRect *)rects :(int)rectCount
{
   int x, y, cnt=0;
   NXRect nFrame = bounds;
   
   PSgsave();
   NXDrawGrayBezel(&bounds, &bounds);
   NXInsetRect(&nFrame, 2.0, 2.0);
   NXRectClip(&nFrame);
   PSsetgray(NX_DKGRAY);
   NXRectFill(&nFrame);
   for (y = 0; y < bounds.size.height; y += SWATCHH + 1) {
       for (x = 0; x < bounds.size.width; x += SWATCHW + 1) {
   	   NXSetColor(colors[cnt++]);
   	   PSrectfill(x + 2.0, y + 2.0, SWATCHW, SWATCHH);
	   if (cnt >= NUMCOLORS) break;
       }
       if (cnt >= NUMCOLORS) break;
   }
   PSgrestore();
   
   return self;
}

- drawColor:(int)num
{
    float x, y;
    NXRect nFrame = bounds;
    
    if (![self canDraw]) return self;

    [self lockFocus];
    PSgsave();
    NXInsetRect(&nFrame,2.0,2.0);
    NXRectClip(&nFrame);
    NXSetColor(colors[num]);
    x = (num % ((int)(frame.size.width / (SWATCHW + 1)) + 1)) * (SWATCHW + 1);
    y = floor(num / (floor(frame.size.width / (SWATCHW + 1)) + 1.0)) * (SWATCHH + 1);
    PSrectfill(x + 2.0, y + 2.0, SWATCHW, SWATCHH);
    PSgrestore();
    [self unlockFocus];
    
    return self;
}

- highlightColor:(int)num
{
    float x, y;
    NXRect nFrame = bounds;
    static int currentHighlight = 0;
    
    if (![self canDraw]) return self;

    [self lockFocus];
    PSgsave();
    NXInsetRect(&nFrame, 2.0, 2.0);
    NXRectClip(&nFrame);
    x = (currentHighlight % ((int)(frame.size.width / (SWATCHW + 1)) + 1)) * (SWATCHW + 1);
    y = floor(currentHighlight / (floor(frame.size.width / (SWATCHW + 1)) + 1.0)) * (SWATCHH + 1);
    if (currentHighlight != -1) {
	PSsetgray(NX_DKGRAY);
	PSrectfill(x + 1.0, y + 1.0, SWATCHW + 2.0, SWATCHH + 2.0);
	NXSetColor(colors[currentHighlight]);
	PSrectfill(x + 2.0, y + 2.0, SWATCHW, SWATCHH);
    }
    if (num != -1) {
	x = (num % ((int)(frame.size.width / (SWATCHW + 1)) + 1)) * (SWATCHW + 1);
	y = floor(num/(floor(frame.size.width / (SWATCHW + 1)) + 1.0)) * (SWATCHH + 1);
	PSsetgray(NX_WHITE);
	PSrectfill(x + 1.0, y + 1.0, SWATCHW + 2.0, SWATCHH + 2.0);
	NXSetColor(colors[num]);
	PSrectfill(x + 2.0, y + 2.0, SWATCHW, SWATCHH);
    }
    PSgrestore();
    [self unlockFocus];
    
    currentHighlight = num;

    return self;
}

- setColorPicker:anObject;
{
    colorPicker = anObject;
    return self;
}

static int indexFromPoint(const NXPoint *p, const NXRect *frame)
{
    return floor((p->x / (SWATCHW + 1.0))) + 
    	  floor(p->y / (SWATCHH + 1)) * 
	  (floor(frame->size.width / (SWATCHW + 1)) + 1.0);
}

- acceptColor:(NXColor)col atPoint:(NXPoint *)pt
{
    int num;
    
    [self convertPoint:pt fromView: nil];
    pt->x -= 2.0; pt->y -= 2.0;
    if (!NXPointInRect(pt, &bounds)) return self;
    num = indexFromPoint(pt, &frame);
    colors[num] = col;
    [self drawColor:num];
    [window flushWindow];
    [self writeColors];

    return self;
}

- (NXColor)color 
{
    return colors[mousedColor];
}

- mouseDown:(NXEvent *)theEvent
{
    int num, tnum;
    BOOL loop = YES;
    NXPoint mL1, mL, pt;
    NXEvent *nextEvent, downEvent;
    
    mL1 = theEvent->location; 
    downEvent = *theEvent;
    [[self window] addToEventMask:NX_MOUSEDRAGGEDMASK];
    [self convertPoint: &mL1 fromView: nil];
    num = indexFromPoint(&mL1, &frame);
    [self highlightColor:num];
    mousedColor = num;
    [window flushWindow];

    while (loop) {
	nextEvent = [NXApp getNextEvent:NX_MOUSEUPMASK|NX_MOUSEDRAGGEDMASK];
	switch (nextEvent->type) {
	case NX_LMOUSEUP:
	    loop = NO;
	    pt = nextEvent->location;
	    [self convertPoint:&pt fromView:nil];
	    pt.x -= 2.0;
	    pt.y -= 2.0;
	    if (NXPointInRect(&pt, &bounds)){
		num = indexFromPoint(&pt, &frame);
		[colorPicker setAndSendColorValues:colors[num]:1];
		[[colorPicker wheelView] showColor:&colors[num]];
	    }
	    [self highlightColor:-1];
	    [window flushWindow];
	    break;
	default:
	    mL = nextEvent->location; 
	    [self convertPoint: &mL fromView: nil];
	    tnum = indexFromPoint(&mL, &frame);
	    if ((abs(mL.x - mL1.x) > DRAGDISTANCE) || (abs(mL.y - mL1.y) > DRAGDISTANCE) || tnum != num) {
		if ([NXColorPanel dragColor:colors[num] withEvent:&downEvent fromView:self]) [self highlightColor: -1];
		loop = NO;
	    } else {
		num = tnum;
		[self highlightColor:num];
	    }
	    [window flushWindow];
	    NXPing();
	}
    }
    
    return self;
}	

- setFilename:(char *)aFile
{
    if (aFile) {
	free(filename);
	filename = NXCopyStringBufferFromZone(aFile, [self zone]);
    }
    return self;
}

- writeColors
{
    FILE *outFile;

    if (filename && ((outFile = fopen(filename,"w")) != NULL)) {
        fwrite(colors, sizeof(NXColor), NUMCOLORS, outFile);
        fclose(outFile);
    }

    return self;
}

- readColors
{
    FILE *inFile;
    
    if (!filename || access(filename, F_OK)) return self;

    if ((inFile = fopen(filename,"r")) != NULL){
        fread(colors, sizeof(NXColor), NUMCOLORS, inFile);
        fclose(inFile);
    }

    return self;
}

@end

/*
    
4/1/90 kro	new class for beginner mode of color panel

appkit-80
---------
 4/8/90 keith	changed the way highlighting works.	
 
appkit-85
---------
 6/2/90 keith	added NXColorWell support for rearranging custom colors.	
 
appkit-89
---------
7/27/90 keith stripped color history stuff out.

appkit-90
----------
8/4/90		Greg added zone stuff. Code was finessed after paul and keith design review

*/

/*
	XYShow.m
	Responsibility: Paul Hegarty
	Copyright (c) 1988, 1989, NeXT, Inc.  All rights reserved. 
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "NXXYShow.h"
#import <stdlib.h>
#import "Font.h"
#import "View.h"
#import "privateWraps.h"
#import "textWraps.h"
#import <dpsclient/wraps.h>
#import <dpsclient/dpsNeXT.h>
#import <string.h>
#import <mach.h>
#import <zone.h>

@implementation NXXYShow

#define byte char

+ newFont:aFont gray:(NXCoord)aGray
{
    return [[self allocFromZone:NXDefaultMallocZone()] initFont:aFont gray:aGray];
}

- newCopy
{
    NXCoord x, y;
    id retval;
    
    retval = [[NXXYShow allocFromZone:[self zone]] initFont:font gray:gray];
    [self currentPoint:&x :&y];
    [retval moveTo:x :y];

    return retval;
}

- reset:aFont gray:(float)aGray
{
    int i;
    float fontSize;

    if (!text) {
	bufferSize = 24;
	text = (char *)NXZoneMalloc([self zone], bufferSize);
	pos = (float *)NXZoneMalloc([self zone], bufferSize * 2 * sizeof(float) + sizeof(float));
    }

    curPos = pos+1;
    curText = text;
    first = YES;
    curX = curY = 0.0;
    gray = aGray;

    if (aFont != font) {
	if (freeWidths) free(screenWidths);
	screenFont = [aFont screenFont];
	if (screenFont == aFont) {
	    font = nil;
	} else {
	    font = aFont;
	    if (!screenFont) screenFont = font;
	}
	if (screenFont == font) {
	    float *widths = ([font metrics])->widths;
	    freeWidths = YES;
	    fontSize = [font pointSize];
	    screenWidths = (float *)NXZoneMalloc([self zone], sizeof(float) * 256);
	    for (i = 0; i < 256; i++) screenWidths[i] = widths[i] * fontSize;
	} else {
	    freeWidths = NO;
	    screenWidths = ([screenFont metrics])->widths;
	}
    }

    if (NXDrawingStatus != NX_DRAWING) {
	[font set];
	PSsetgray(gray);
    }

    return self;
}

- initFont:aFont gray:(NXCoord)aGray
{
    [super init];

    return [self reset:aFont gray:aGray];
}

- free
{
    free(text);
    free(pos);
    if (freeWidths) free(screenWidths);
    return [super free];
}

- finalize
{
    int total;
    byte *header;

    if (NXDrawingStatus == NX_DRAWING) {
	[screenFont set];
	*curText = 0;
	*(curPos++) = 0.0;
	*(curPos++) = 0.0;
	total = curPos - pos - 1;
	PSsetgray(gray);
	PSmoveto(startX, startY);
	header = (byte *)pos;
	*header++ = 149;
	*header++ = dps_float;
	*((short *)header) = total;
	_NXstringxyshow(text, total * 4 + 4, (char *)pos);
    }

    return self;
}

- currentPoint:(NXCoord *)x :(NXCoord *)y
{
    if (first) {
	if (x) *x = startX;
	if (y) *y = startY;
    } else {
	if (x) *x = curX + *(curPos-2);
	if (y) *y = curY + *(curPos-1);
    }
    return self;
}

- moveTo:(NXCoord)x :(NXCoord)y
{
    if (NXDrawingStatus == NX_DRAWING) {
	if (first) {
	    startX = x; startY = y;
	} else {
	    curPos -= 2;
	    *(curPos++) = x - curX;
	    *(curPos++) = y - curY;   
	}
    } else {
	PSmoveto(x, y);
    }
    return self;
}


- show:(const char *)string count:(int)count
{
    int offset;
    unsigned char ch;
    NXCoord w = 0.0, width = 0.0;

    if (!count) return self;

    offset = curText - text;
    if (offset + count >= bufferSize) {
	while (offset + count >= bufferSize) bufferSize *= 2;
	text = (char *)NXZoneRealloc([self zone], text, bufferSize);
	curText = text + offset;
	pos = (float *)NXZoneRealloc([self zone], pos, bufferSize * 2 * sizeof(float) + sizeof(float));
	curPos = pos + offset * 2 + 1;
    }

    if (NXDrawingStatus == NX_DRAWING) {
	if (first) {
	    first = NO;
	    curX = startX; curY = startY;
	} else {
	    curX += *(curPos-2); curY += *(curPos-1);
	}
	for (; count--; string++) {
	    ch = *((unsigned char *)string);
	    w = screenWidths[ch];
	    width += w;
	    *(curPos++) = w;
	    *(curPos++) = 0.0;
	    *(curText++) = (char)ch;
	}
	curX += width - w;
    } else {
	[font set];
	PSsetgray(gray);
	_NXshow(count, (char *)string);
    }

    return self;
}

- show:(const char *)string
{
    if (NXDrawingStatus == NX_DRAWING) {
	return [self show:string count:string ? strlen(string) : 0];
    } else {
	[font set];
	PSsetgray(gray);
	PSshow((char *)string);
    }
    return self;
}

@end

/*

Created by: Bryan Yamamoto
  
Modifications (since 86):

86
-- 
 6/7/90 aozer	In show:count: made ch unsigned chars rather than char; 
		screenWidth array was being accessed with negative indexes.

*/

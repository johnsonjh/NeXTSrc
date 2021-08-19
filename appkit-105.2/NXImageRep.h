/*
	NXImageRep.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "graphics.h"

/*
 * NX_MATCHESDEVICE indicates the value is variable, depending on the output
 * device. It can be the passed in (or received back) as the value of 
 * numColors, bitsPerSample, or pixelsWide & pixelsHigh.
 */

#define NX_MATCHESDEVICE	(0)

/*
 * Names of segments for EPS and TIFF files and app & document icons.
 */

#define NX_EPSSEGMENT "__EPS"
#define NX_TIFFSEGMENT "__TIFF"
#define NX_ICONSEGMENT "__ICON"

@interface NXImageRep : Object
{
    struct __repFlags {
	unsigned int hasAlpha:1;
	unsigned int numColors:3;
	unsigned int bitsPerSample:6;
	unsigned int dataSource:3;
	unsigned int dataLoaded:1;
	unsigned int :0;
    }                   _repFlags;
    NXSize              size;
    int                 _pixelsWide;
    int                 _pixelsHigh;
    int                 _reservedRepInt;
}

- (BOOL)drawAt:(const NXPoint *)point;
- (BOOL)drawIn:(const NXRect *)rect;
- (BOOL)draw;
- setSize:(const NXSize *)aSize;
- getSize:(NXSize *)aSize;
- setAlpha:(BOOL)flag;
- (BOOL)hasAlpha;
- setNumColors:(int)anInt;
- (int)numColors;
- setBitsPerSample:(int)anInt;
- (int)bitsPerSample;
- setPixelsWide:(int)anInt;
- (int)pixelsWide;
- setPixelsHigh:(int)anInt;
- (int)pixelsHigh;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;

@end



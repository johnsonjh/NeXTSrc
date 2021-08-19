/*
	obsoleteBitmap.h
	Application Kit, Release 1.0
	Copyright (c) 1988, 1989, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "graphics.h"

/* NOTE: The Bitmap class has been obsoleted, use NXImage instead.   */
/*       To ease conversion, this file contains the old declarations */
/*       of the Bitmap methods.                                      */

/* Bitmap Types */

#define	NX_UNIQUEBITMAP		(-1)
#define	NX_NOALPHABITMAP	(0)
#define	NX_ALPHABITMAP		(1)
#define	NX_UNIQUEALPHABITMAP	(2)

@interface Bitmap : Object
{
    NXRect 	    frame;
    char	   *iconName;
    int             type;
    int		    _builtIn;
    void	   *_manager;		
    struct __bFlags{
	unsigned int    _flipDraw:1;
	unsigned int    _systemBitmap:1;
	unsigned int    _nibBitmap:1;
	unsigned int    _willFree:1;
	unsigned int    _RESERVED:12;
    }               _bFlags;
}
  
+ findBitmapFor:(const char *)name;
+ getSize:(NXSize *) size for:(const char *)name;
+ (BOOL)addName:(const char *)name data:(void *)bitmapData
    width:(int)samplesWide height:(int)samplesHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel;
+ (BOOL)addName :(const char *)name fromTIFF:(const char *)filename;
+ (BOOL)addName :(const char *)name fromMachO:(const char *)sectionName;
+ (BOOL)addName :(const char *)name bitmap:bitmapObj;
+ newFromStream:(NXStream *)stream;
+ newFromTIFF:(const char *)filename;
+ newFromMachO:(const char *)sectionName;
+ newSize:(NXCoord) width :(NXCoord) height type:(int)aType;
+ newRect:(const NXRect *) aRect type:(int) aType window: window;
+ compact;
- finishUnarchiving;
- free;
- setFlip:(BOOL)flag;
- _getFrame:(NXRect *) aRect;
- (const char *)name;
- window;
- getSize:(NXSize *)theSize;
- (BOOL)lockFocus;
- unlockFocus;
- (int)type;
- writeTIFF:(NXStream *) stream;
- image:(void *)data withAlpha:(void *)alpha
    width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample;
- image:(void *)data width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel;
- image:(void *)data toRect:(const NXRect *) rect
    width:(int)samplesWide height:(int)sampleHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel;
- imageSize:(int *)sizeInBytes 
    width:(int *)ptrWidth height:(int *)ptrHeight
    bps:(int *)ptrBps spp:(int *)ptrSpp inRect:(const NXRect *)rect;
- readImage:(void *)image;
- readImage:(void *)image inRect:(const NXRect *)rect;
- readImage:(void *)image withAlpha:(void *)alpha;
- readImage:(void *)image withAlpha:(void *)alpha inRect:(NXRect *)rect;
- composite:(int)op toPoint:(const NXPoint *)aPoint;
- composite:(int)op fromRect:(const NXRect *)aRect toPoint:(const NXPoint *)aPoint;
- write:(NXTypedStream *)s;
- read:(NXTypedStream *)s;
- resize:(NXCoord)theWidth :(NXCoord)theHeight;
- image:(void *)data toRect:(const NXRect *)rect 
	width:(int)samplesWide height:(int)samplesHigh
    bps:(int)bitsPerSample spp:(int)samplesPerPixel 
	config:(int)planarConfig interp:(int)photoInterp;
- imageSize:(int *)sizeInBytes
    width:(int *)ptrWidth height:(int *)ptrHeight
    bps:(int *)ptrBps spp:(int *)ptrSpp 
	config:(int *)ptrConfig interp:(int *)ptrInterp
	inRect:(const NXRect *)rect;
	
@end

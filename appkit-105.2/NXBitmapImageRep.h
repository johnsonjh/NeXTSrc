/*
	NXBitmapImageRep.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "NXImageRep.h"
#import <objc/List.h>
#import "graphics.h"
#import <streams/streams.h>

@interface NXBitmapImageRep : NXImageRep
{
    unsigned int        _bytesPerRow;
    unsigned short      _imageNumber;
    short		_colorSpace;
    struct __bitmapRepFlags {
	unsigned int        isPlanar:1;
	unsigned int	    explicitPlanes:1;
	unsigned int:0;
    }                   _moreRepFlags;
    unsigned short      _memoryPages;
    char               *_fileName;
    unsigned char      *_memory;
    unsigned char      *_data;
    char	       *_otherName;
    int                 _reservedInt;
}

- initFromSection:(const char *)fileName;
- initFromFile:(const char *)fileName;
- initFromStream:(NXStream *)stream;
- initData:(unsigned char *)data fromRect:(const NXRect *)rect;
- initData:(unsigned char *)data pixelsWide:(int)width pixelsHigh:(int)height bitsPerSample:(int)bps samplesPerPixel:(int)spp hasAlpha:(BOOL)alpha isPlanar:(BOOL)isPlanar colorSpace:(NXColorSpace)colorSpace bytesPerRow:(int)rBytes bitsPerPixel:(int)pBits;
- initDataPlanes:(unsigned char **)planes pixelsWide:(int)width pixelsHigh:(int)height bitsPerSample:(int)bps samplesPerPixel:(int)spp hasAlpha:(BOOL)alpha isPlanar:(BOOL)isPlanar colorSpace:(NXColorSpace)colorSpace bytesPerRow:(int)rBytes bitsPerPixel:(int)pBits;

+(int)sizeImage:(const NXRect *)rect;
+(int)sizeImage:(const NXRect *)rect pixelsWide:(int *)width pixelsHigh:(int *)height bitsPerSample:(int *)bps samplesPerPixel:(int *)spp hasAlpha:(BOOL *)hasAlpha isPlanar:(BOOL *)isPlanar colorSpace:(NXColorSpace *)colorSpace;

+ (List *)newListFromSection:(const char *)fileName;
+ (List *)newListFromFile:(const char *)fileName;
+ (List *)newListFromStream:(NXStream *)stream;
+ (List *)newListFromSection:(const char *)fileName zone:(NXZone *)zone;
+ (List *)newListFromFile:(const char *)fileName zone:(NXZone *)zone;
+ (List *)newListFromStream:(NXStream *)stream zone:(NXZone *)zone;

- (unsigned char *)data;
- getDataPlanes:(unsigned char **)data;
- (BOOL)isPlanar;
- (int)samplesPerPixel;
- (int)bitsPerPixel;
- (int)bytesPerRow;
- (int)bytesPerPlane;
- (int)numPlanes;
- (NXColorSpace)colorSpace;
- (BOOL)drawIn:(const NXRect *)rect;
- (BOOL)draw;
- writeTIFF:(NXStream *)stream;
- writeTIFF:(NXStream *)stream usingCompression:(int)compression;
- writeTIFF:(NXStream *)stream usingCompression:(int)compression andFactor:(float)compressionFactor;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;
- copy;
- free;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFromSection:(const char *)fileName;
+ newFromFile:(const char *)fileName;
+ newFromStream:(NXStream *)stream;
+ newData:(unsigned char *)data pixelsWide:(int)width pixelsHigh:(int)height bitsPerSample:(int)bps samplesPerPixel:(int)spp hasAlpha:(BOOL)alpha isPlanar:(BOOL)isPlanar colorSpace:(NXColorSpace)colorSpace;
+ newData:(unsigned char *)data pixelsWide:(int)width pixelsHigh:(int)height bitsPerSample:(int)bps samplesPerPixel:(int)spp hasAlpha:(BOOL)alpha isPlanar:(BOOL)isPlanar colorSpace:(NXColorSpace)colorSpace bytesPerRow:(int)rBytes bitsPerPixel:(int)pBits;
+ newDataPlanes:(unsigned char **)planes pixelsWide:(int)width pixelsHigh:(int)height bitsPerSample:(int)bps samplesPerPixel:(int)spp hasAlpha:(BOOL)alpha isPlanar:(BOOL)isPlanar colorSpace:(NXColorSpace)colorSpace bytesPerRow:(int)rBytes bitsPerPixel:(int)pBits;
+ readImage:(const NXRect *)rect into:(unsigned char *)data;

@end

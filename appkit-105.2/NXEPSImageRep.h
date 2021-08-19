/*
	NXEPSImageRep.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "NXImageRep.h"
#import <objc/List.h>
#import <streams/streams.h>

@interface NXEPSImageRep : NXImageRep
{
    char               *_fileName;
    NXPoint             _bBoxOrigin;
    char               *_memory;
    int                 _epsLen;
    short               _epsOffset;
    short		_reservedShort;
    char               *_otherName;
    int                 _reservedInt;
}

- initFromSection:(const char *)fileName;
- initFromFile:(const char *)fileName;
- initFromStream:(NXStream *)stream;

+ (List *)newListFromSection:(const char *)fileName;
+ (List *)newListFromFile:(const char *)fileName;
+ (List *)newListFromStream:(NXStream *)stream;
+ (List *)newListFromSection:(const char *)fileName zone:(NXZone *)zone;
+ (List *)newListFromFile:(const char *)fileName zone:(NXZone *)zone;
+ (List *)newListFromStream:(NXStream *)stream zone:(NXZone *)zone;

- (BOOL)drawIn:(const NXRect *)rect;
- (BOOL)draw;
- prepareGState;
- getEPS:(char **)epsString length:(int *)length;
- getBoundingBox:(NXRect *)rect;
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

@end


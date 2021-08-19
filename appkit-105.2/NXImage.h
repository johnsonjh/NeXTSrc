/*
	NXImage.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "color.h"
#import "graphics.h"
#import "NXImageRep.h"
#import <objc/List.h>

@interface NXImage : Object
{
    char               *name;
    NXSize              _size;
    struct __imageFlags {
	unsigned int        scalable:1;
	unsigned int        dataRetained:1;
	unsigned int        flipDraw:1;
	unsigned int        uniqueWindow:1;
	unsigned int        _unused1:1;
	unsigned int        sizeWasExplicitlySet:1;
	unsigned int        builtIn:1;
	unsigned int        needsToExpand:1;
	unsigned int        useEPSOnResolutionMismatch:1;
	unsigned int        colorMatchPreferred:1;
	unsigned int        multipleResolutionMatching:1;
	unsigned int        _unused2:1;
	unsigned int        subImage:1;
	unsigned int	    aSynch:1;
	unsigned int	    archiveByName:1;
	unsigned int	    unboundedCacheDepth:1;
    }                   _flags;
    short		_reservedShort;
    void               *_reps;
    List               *_repList;
    NXColor            *_color;
    int			_reservedInt;
}

+ findImageNamed:(const char *)name;

- init;
- initSize:(const NXSize *)aSize;
- initFromFile:(const char *)fileName;
- initFromSection:(const char *)fileName;
- initFromStream:(NXStream *)stream;
- initFromImage:(NXImage *)image rect:(const NXRect *)rect;

- getImage:(NXImage **)image rect:(NXRect *)rect;
- setSize:(const NXSize *)aSize;
- getSize:(NXSize *)aSize;
- free;
- (BOOL)setName:(const char *)string;
- (const char *)name;
- setFlipped:(BOOL)flag;
- (BOOL)isFlipped;
- setScalable:(BOOL)flag;
- (BOOL)isScalable;
- setDataRetained:(BOOL)flag;
- (BOOL)isDataRetained;
- setUnique:(BOOL)flag;
- (BOOL)isUnique;
- setCacheDepthBounded:(BOOL)flag;
- (BOOL)isCacheDepthBounded;
- setBackgroundColor:(NXColor)aColor;
- (NXColor)backgroundColor;
- setEPSUsedOnResolutionMismatch:(BOOL)flag;
- (BOOL)isEPSUsedOnResolutionMismatch;
- setColorMatchPreferred:(BOOL)flag;
- (BOOL)isColorMatchPreferred;
- setMatchedOnMultipleResolution:(BOOL)flag;
- (BOOL)isMatchedOnMultipleResolution;
- dissolve:(float)delta toPoint:(const NXPoint *)point;
- dissolve:(float)delta	fromRect:(const NXRect *)rect toPoint:(const NXPoint *)point;
- composite:(int)op toPoint:(const NXPoint *)point;
- composite:(int)op fromRect:(const NXRect *)rect toPoint:(const NXPoint *)point;
- (BOOL)drawRepresentation:(NXImageRep *)imageRep inRect:(const NXRect *)rect;
- recache;
- writeTIFF:(NXStream *)stream;
- writeTIFF:(NXStream *)stream allRepresentations:(BOOL)flag;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
- finishUnarchiving;
- (BOOL)loadFromStream:(NXStream *)stream;
- (BOOL)useFromFile:(const char *)fileName;
- (BOOL)useFromSection:(const char *)fileName;
- (BOOL)useDrawMethod:(SEL)drawMethod inObject:anObject;
- (BOOL)useRepresentation:(NXImageRep *)imageRepresentation;
- (BOOL)useCacheWithDepth:(NXWindowDepth)depth;
- removeRepresentation:(NXImageRep *)imageRepresentation;
- (BOOL)lockFocus;
- (BOOL)lockFocusOn:(NXImageRep *)imageRepresentation;
- unlockFocus;
- (NXImageRep *)lastRepresentation;
- (NXImageRep *)bestRepresentation;
- (List *)representationList;
- setDelegate:(id)anObject;
- delegate;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newSize:(const NXSize *)aSize;
+ newFromFile:(const char *)fileName;
+ newFromSection:(const char *)fileName;
+ newFromStream:(NXStream *)stream;
+ newFromImage:(NXImage *)image rect:(const NXRect *)rect;

@end

@interface Object(NXImageDelegate)
- (NXImage *)imageDidNotDraw:sender inRect:(const NXRect *)aRect;
@end

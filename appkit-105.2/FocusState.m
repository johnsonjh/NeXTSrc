/*
	FocusState.m
  	Copyright 1989, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "FocusState.h"
#import "nextstd.h"

@implementation FocusState


- clip:(const NXRect *)aRect
{
    NXRect tRect = *aRect;
    
    if (mFlags.rotated)
	[self flush];
    
    [self transformRect:&tRect];
    if (clipSet && !clipEmpty)
	clipEmpty = NXIntersectionRect(&tRect, &theClip) ? NO : YES;
    else if (!clipSet)
	theClip = tRect;
	
    clipSet = YES;
    return self;
}


- flush
{
    static const NXRect emptyRect = {{0.0, 0.0},{0.0, 0.0}};
    
    if (clipSet)
	NXRectClip(clipEmpty ? &emptyRect : &theClip);
    if (!clipEmpty)
	[self send];
    [self reset];
    return self;
}


- reset
{
    [self makeIdentity];
    clipSet = NO;
    clipEmpty = NO;
    return self;
}


/* cover up methods that can not work */
- scaleTo:(NXCoord)sx :(NXCoord)sy
{
    AK_ASSERT(NO, "concat can invalidate SX, SY; don't use this method");
    return (self);
}

- rotateTo:(NXCoord)angle
{
    AK_ASSERT(NO, "concat can invalidate SX, SY; don't use this method");
    return (self);
}


@end



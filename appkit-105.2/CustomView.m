/*
	CustomView.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "CustomView.h"
#import "Text.h"
#import "ScrollView.h"
#import <objc/List.h>
#import "nextstd.h"

@implementation CustomView

- free
{
    free(className);
    return [super free];
}

- initFrame:(const NXRect *)aFrame
{
    [super initFrame:aFrame];
    free(className);
    className = NXCopyStringBufferFromZone("View", [self zone]);
    [self setOpaque:YES];
    return self;
}

- nibInstantiate
{
    id                  theClass, theSuperview, theList;
    int                 theOffset;

    if (realObject)
	return realObject;
    theClass = objc_getClass(className);

    if (!theClass || ![theClass isKindOf:(id)(((CustomView *)[View class])->isa)]) {
	NXLogError("Unknown View class %s in Interface Builder file,\n\t creating generic View instead", className);
	theClass = [View class];
    }
    theSuperview = superview;
    theOffset = [[theSuperview subviews] indexOf:self];
    [self removeFromSuperview];
    if (_NXCanAllocInZone(theClass, @selector(newFrame:), @selector(initFrame:))) {
	realObject = [[theClass allocFromZone:[self zone]] initFrame:&frame];
    } else {
	realObject = [theClass newFrame:&frame];
    }
    [theSuperview addSubview:realObject];
    theList = [theSuperview subviews];
    [theList removeLastObject];
    [theList insertObject:realObject at:theOffset];
    [realObject setAutosizing:_vFlags.autosizing];
    return realObject;
}

- awake
{
    [NXApp delayedFree:self];
    return self;
}

- write:(NXTypedStream *) s
{
    [super write:s];
    NXWriteTypes(s, "*@", &className, &extension);
    return self;
}

- read:(NXTypedStream *) s
{
    [super read:s];
    NXReadTypes(s, "*@", &className, &extension);
    return self;
}

@end
/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
1/27/89  bs	added read: write:;
  
  
  
*/


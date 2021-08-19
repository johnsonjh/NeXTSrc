/*
	CustomObject.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "CustomObject.h"
#import "nextstd.h"

@implementation CustomObject

- free
{
    free(className);
    return [super free];
}

- init
{
    [super init];
    className = NXCopyStringBufferFromZone("Object", [self zone]);
    return self;
}

- nibInstantiate
{
    id                  theClass;

    if (realObject)
	return realObject;
    theClass = objc_getClass(className);
    if (!theClass) {
	NXLogError("Unknown class %s in Interface Builder file,\n\t creating generic Object instead", className);
	theClass = [Object class];
    }
    if ((theClass == [Object class]) || _NXCanAllocInZone(theClass, @selector(new), @selector(init))) {
	realObject = [[theClass allocFromZone:[self zone]] init];
    } else {
	realObject = [theClass new];
    }
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
  
1/27/88	bs	added read: write:;
  
  
*/


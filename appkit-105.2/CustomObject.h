/*

	CustomObject.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import <objc/Object.h>

#import <objc/typedstream.h>

@interface CustomObject : Object
{
    char	*className;	
    id		realObject;	
    id		extension;
}

- nibInstantiate;
- write:(NXTypedStream *) s;
- read:(NXTypedStream *) s;
@end

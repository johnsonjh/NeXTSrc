/*

	NibData.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import <objc/Object.h>

#import <objc/typedstream.h>


@interface  NibData:Object
{
    id		objectList;		
    id		connectList;		
    id		visibleWindows;		
    id		extension;		
    struct _nFlags {
    	unsigned int isList:1;		
	unsigned int _PADDING:15;	
    } nFlags;
}

- free;
- nibInstantiateIn:nameTable owner:owner;
- write:(NXTypedStream *) s;
- read:(NXTypedStream *) s;
@end


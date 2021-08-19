/*

	CustomView.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import "View.h"

@interface CustomView : View
{
    char	*className;	
    id		realObject;	
    id		extension;
}

- nibInstantiate;
- write:(NXTypedStream *) s;
- read:(NXTypedStream *) s;
@end

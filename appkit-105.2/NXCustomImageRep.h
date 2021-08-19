/*
	NXCustomImageRep.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "NXImageRep.h"

@interface NXCustomImageRep : NXImageRep
{
    SEL                 drawMethod;
    id                  drawObject;
    int			_reservedInt;
}

- initDrawMethod:(SEL)aMethod inObject:anObject;

- (BOOL)draw;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newDrawMethod:(SEL)aMethod inObject:anObject;

@end


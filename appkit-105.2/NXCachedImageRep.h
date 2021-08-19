/*
	NXCachedImageRep.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "NXImageRep.h"
#import "Window.h"

@interface NXCachedImageRep : NXImageRep
{
    NXPoint             _origin;
    int                 _gState;
    Window	       *_window;
    void	       *_cache;
    int			_reservedInt;
}

- initFromWindow:(Window *)win rect:(const NXRect *)rect;

- getWindow:(Window **)win andRect:(NXRect *)rect;
- (BOOL)draw;
- read:(NXTypedStream *)stream;
- write:(NXTypedStream *)stream;
- free;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFromWindow:(Window *)win rect:(const NXRect *)rect;

@end

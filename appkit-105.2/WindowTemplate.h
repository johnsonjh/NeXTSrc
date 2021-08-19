/*

	WindowTemplate.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import <objc/Object.h>
#ifndef GRAPHICS_H
#import "graphics.h"
#endif GRAPHICS_H

@interface WindowTemplate : Object
{
    NXRect	windowRect;		
    int		windowStyle;		
    int		windowBacking;		
    int		windowButtonMask;	
    int		windowEventMask;	
    char	*windowTitle;		
    char	*viewClass;		
    char	*windowClass;		
    id		windowView;		
    id		realObject;		
    struct _windowFlags {
	unsigned int    hideOnDeactivate:1;
	unsigned int    dontFreeWhenClosed:1;
	unsigned int    defer:1;
	unsigned int    oneShot:1;
	unsigned int    visible:1;
	unsigned int	wantsToBeColor:1;
	unsigned int    dynamicDepthLimit:1;
	unsigned int    _PADDING:9; 
    }		windowFlags;
    id		extension;
}

- free;
- nibInstantiate;
- write:(NXTypedStream *)s;
- read:(NXTypedStream *)s;
@end

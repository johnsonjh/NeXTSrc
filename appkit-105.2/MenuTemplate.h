/*

	MenuTemplate.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#import <objc/Object.h>
#ifndef GRAPHICS_H
#import "graphics.h"
#endif GRAPHICS_H

@interface MenuTemplate : Object
{
    char	*title;		
    NXPoint	location;	
    id		view;		
    char	*menuClassName;	
    id		supermenu;	
    id		realObject;	
    id		extension;
    BOOL	isWindowsMenu;
    BOOL	isRequestMenu;
    BOOL	isFontMenu;
    BOOL	_reservedBOOL;
}

- free;
- nibInstantiate;
- write:(NXTypedStream *) s;
- read:(NXTypedStream *) s;
@end

@interface MenuTemplateCallee : Object
- setMenu:anObject;
@end


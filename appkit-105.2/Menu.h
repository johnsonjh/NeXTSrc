/*
	Menu.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"
#import <zone.h>

@interface Menu : Panel
{
    id                  supermenu;
    id                  matrix;
    id                  attachedMenu;
    NXPoint             lastLocation;
    id                  reserved;
    struct _menuFlags {
	unsigned int        sizeFitted:1;
	unsigned int        autoupdate:1;
	unsigned int        attached:1;
	unsigned int        tornOff:1;
	unsigned int        wasAttached:1;
	unsigned int        wasTornOff:1;
	unsigned int        _RESERVED:8;
	unsigned int        _isServicesMenu:1;
	unsigned int        _changeTitle:1;
    }                   menuFlags;
}

+ setMenuZone:(NXZone *)aZone;
+ (NXZone *)menuZone;

- init;
- initTitle:(const char *)aTitle;

- addItem:(const char *)aString action:(SEL)aSelector keyEquivalent:(unsigned short)charCode;
- setSubmenu:aMenu forItem:aCell;
- itemList;
- setItemList:aMatrix;
- display;
- sizeToFit;
- moveTopLeftTo:(NXCoord)x :(NXCoord)y;
- windowMoved:(NXEvent *)theEvent;
- close;
- update;
- setAutoupdate:(BOOL)flag;
- findCellWithTag:(int)aTag;
- getLocation:(NXPoint *)theLocation forSubmenu:aSubmenu;
- mouseDown:(NXEvent *)theEvent;
- rightMouseDown:(NXEvent *)theEvent;
- awake;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTitle:(const char *)aTitle;

@end

@interface Menu(SubmenuDummyAction)
- submenuAction:sender;
@end

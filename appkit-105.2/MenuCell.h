/*
	MenuCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "ButtonCell.h"

@interface MenuCell : ButtonCell
{
    SEL                 updateAction;
}

- initTextCell:(const char *)aString;
- init;

+ useUserKeyEquivalents:(BOOL)flag;

- setUpdateAction:(SEL)aSelector forMenu:aMenu;
- (unsigned short)userKeyEquivalent;
- (SEL)updateAction;
- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView;
- (BOOL)hasSubmenu;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newTextCell:(const char *)aString;
+ newTextCell;
+ new;

@end

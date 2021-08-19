/*
	PopUpList.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Menu.h"

@interface  PopUpList : Menu
{
}

extern void NXAttachPopUpList(id button, PopUpList *popuplist);
extern id NXCreatePopUpListButton(PopUpList *popuplist);

- init;

- popUp:sender;
- addItem:(const char *)title;
- insertItem:(const char *)title at:(unsigned int)index;
- removeItem:(const char *)title;
- removeItemAt:(unsigned int)index;
- (unsigned int)count;
- changeButtonTitle:(BOOL)flag;
- (int)indexOfItem:(const char *)title;
- font;
- setFont:fontId;
- (const char *)selectedItem;
- getButtonFrame:(NXRect *)bframe;
- (SEL)action;
- setAction:(SEL)aSelector;
- target;
- setTarget:anObject;
- sizeWindow:(NXCoord)width :(NXCoord)height;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;

@end

/*
	Button.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

@interface Button : Control
{
}

+ setCellClass:factoryId;

- init;
- initFrame:(const NXRect *)frameRect;
- initFrame:(const NXRect *)frameRect title:(const char *)aString tag:(int)anInt target:anObject action:(SEL)aSelector key:(unsigned short)charCode enabled:(BOOL)flag;
- initFrame:(const NXRect *)frameRect icon:(const char *)aString tag:(int)anInt target:anObject action:(SEL)aSelector key:(unsigned short)charCode enabled:(BOOL)flag;

- (const char *)title;
- setTitle:(const char *)aString;
- setTitleNoCopy:(const char *)aString;
- (const char *)altTitle;
- setAltTitle:(const char *)aString;
- (const char *)icon;
- setIcon:(const char *)iconName;
- (const char *)altIcon;
- setAltIcon:(const char *)iconName;
- image;
- setImage:image;
- altImage;
- setAltImage:image;
- (int)iconPosition;
- setIconPosition:(int)aPosition;
- setIcon:(const char *)iconName position:(int)aPosition;
- setType:(int)aType;
- (int)state;
- setState:(int)value;
- (BOOL)isBordered;
- setBordered:(BOOL)flag;
- (BOOL)isTransparent;
- setTransparent:(BOOL)flag;
- setPeriodicDelay:(float)delay andInterval:(float)interval;
- getPeriodicDelay:(float *)delay andInterval:(float *)interval;
- (unsigned short)keyEquivalent;
- setKeyEquivalent:(unsigned short)charCode;
- sound;
- setSound:soundObj;
- display;
- highlight:(BOOL)flag;
- (BOOL)performKeyEquivalent:(NXEvent *)theEvent;
- performClick:sender;
- (BOOL)acceptsFirstMouse;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newFrame:(const NXRect *)frameRect;
+ newFrame:(const NXRect *)frameRect title:(const char *)aString tag:(int)anInt target:anObject action:(SEL)aSelector key:(unsigned short)charCode enabled:(BOOL)flag;
+ newFrame:(const NXRect *)frameRect icon:(const char *)aString tag:(int)anInt target:anObject action:(SEL)aSelector key:(unsigned short)charCode enabled:(BOOL)flag;


@end

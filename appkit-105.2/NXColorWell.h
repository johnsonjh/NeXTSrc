/*
	NXColorWell.h
	Application Kit, Release 2.0
	Copyright (c) 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

@interface NXColorWell : Control
{
     NXColor color;
     id _target;
     SEL _action;
     BOOL _isActive, _isBordered, _cantDraw, _isNotContinuous;
     void *_reservedPtr;
}

+ deactivateAllWells;
+ activeWellsTakeColorFrom:sender;
+ activeWellsTakeColorFrom:sender continuous:(BOOL)flag;

- initFrame:(const NXRect *)theFrame;

- deactivate;
- (int)activate:(int)exclusive;
- (BOOL)isActive;
- (BOOL)isContinuous;
- setContinuous:(BOOL)flag;

- acceptColor:(NXColor)color atPoint:(NXPoint *)aPoint;

- drawWellInside:(const NXRect *)insideRect;
- drawSelf:(const NXRect *)rects :(int)rectCount;

- setEnabled:(BOOL)flag;
- awake;
- (BOOL)acceptsFirstMouse;
- mouseDown:(NXEvent *)theEvent;

- takeColorFrom:sender;
- setColor:(NXColor)color;
- (NXColor)color;

- target;
- setTarget:anObject;
- (SEL)action;
- setAction:(SEL)aSelector;

@end

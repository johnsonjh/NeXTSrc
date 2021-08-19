/*
	TextField.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

@interface TextField : Control
{
    id                  nextText;
    id                  previousText;
    id                  textDelegate;
    SEL                 errorAction;
    unsigned int        _reservedTFint1;
}

+ setCellClass:factoryId;

- initFrame:(const NXRect *)frameRect;

- sizeTo:(float)width :(float)height;
- setBackgroundGray:(float)value;
- (float)backgroundGray;
- setTextGray:(float)value;
- (float)textGray;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- setBackgroundTransparent:(BOOL)flag;
- (BOOL)isBackgroundTransparent;
- setTextColor:(NXColor)color;
- (NXColor)textColor;
- setEnabled:(BOOL)flag;
- (BOOL)isBordered;
- setBordered:(BOOL)flag;
- (BOOL)isBezeled;
- setBezeled:(BOOL)flag;
- (BOOL)isEditable;
- setEditable:(BOOL)flag;
- (BOOL)isSelectable;
- setSelectable:(BOOL)flag;
- setPreviousText:anObject;
- setNextText:anObject;
- mouseDown:(NXEvent *)theEvent;
- (SEL)errorAction;
- setErrorAction:(SEL)aSelector;
- selectText:sender;
- textDelegate;
- setTextDelegate:anObject;
- (BOOL)textWillEnd:textObject;
- textDidEnd:textObject endChar:(unsigned short)whyEnd;
- (BOOL)textWillChange:textObject;
- textDidChange:textObject;
- textDidGetKeys:textObject isEmpty:(BOOL)flag;
- (BOOL)acceptsFirstResponder;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

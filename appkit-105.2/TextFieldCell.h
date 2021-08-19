/*
	TextFieldCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "ActionCell.h"
#import "color.h"

@interface TextFieldCell : ActionCell
{
    float               backgroundGray;
    float               textGray;
    void	        *_private;
}

- init;
- initTextCell:(const char *)aString;

- copy;
- (BOOL)isOpaque;
- (float)backgroundGray;
- setBezeled:(BOOL)flag;
- setBackgroundGray:(float)value;
- (float)textGray;
- setTextGray:(float)value;
- setBackgroundColor:(NXColor)color;
- (NXColor)backgroundColor;
- setBackgroundTransparent:(BOOL)flag;
- (BOOL)isBackgroundTransparent;
- setTextColor:(NXColor)color;
- (NXColor)textColor;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- setTextAttributes:textObj;
- (BOOL)trackMouse:(NXEvent *)event inRect:(const NXRect *)aRect ofView:controlView;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTextCell;
+ newTextCell:(const char *)aString;

@end

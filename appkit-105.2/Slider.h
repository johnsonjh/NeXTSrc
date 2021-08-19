/*
	Slider.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Control.h"

@interface Slider : Control
{
}

+ setCellClass:factoryId;

- initFrame:(const NXRect *)frameRect;

- sizeToFit;
- (double)minValue;
- setMinValue:(double)aDouble;
- (double)maxValue;
- setMaxValue:(double)aDouble;
- setEnabled:(BOOL)flag;
- mouseDown:(NXEvent *)theEvent;
- (BOOL)acceptsFirstMouse;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newFrame:(const NXRect *)frameRect;

@end

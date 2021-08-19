/*
	SliderCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "ActionCell.h"

@interface SliderCell : ActionCell
{
    float               _reservedSCfloat1;
    float               _reservedSCfloat2;
    NXCoord             _offset;
    double              value;
    double              maxValue;
    double              minValue;
    NXRect              trackRect;
}


+ (BOOL)prefersTrackingUntilMouseUp;

- init;
- awake;

- (double)minValue;
- setMinValue:(double)aDouble;
- (double)maxValue;
- setMaxValue:(double)aDouble;
- (const char *)stringValue;
- setStringValue:(const char *)aString;
- (int)intValue;
- setIntValue:(int)anInt;
- (float)floatValue;
- setFloatValue:(float)aFloat;
- (double)doubleValue;
- setDoubleValue:(double)aDouble;
- setContinuous:(BOOL)flag;
- (BOOL)isContinuous;
- (BOOL)isOpaque;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- getKnobRect:(NXRect*)knobRect flipped:(BOOL)flipped;
- drawKnob:(const NXRect*)knobRect;
- drawKnob;
- drawBarInside:(const NXRect *)aRect flipped:(BOOL)flipped;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- (BOOL)startTrackingAt:(const NXPoint *)startPoint inView:controlView;
- (BOOL)continueTracking:(const NXPoint *)lastPoint at:(const NXPoint *)currentPoint inView:controlView;
- stopTracking:(const NXPoint *)lastPoint at:(const NXPoint *)stopPoint inView:controlView mouseIsUp:(BOOL)flag;
- (BOOL)trackMouse:(NXEvent *)theEvent inRect:(const NXRect *)cellFrame ofView:controlView;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;

@end

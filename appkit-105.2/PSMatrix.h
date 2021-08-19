/*

	PSMatrix.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
*/

#import <objc/Object.h>
#ifndef GRAPHICS_H
#import "graphics.h"
#endif GRAPHICS_H

@interface PSMatrix : Object
{
    NXCoord           matrixElements[12];	
    struct _psmFlags{
	unsigned int    identity:1;
	unsigned int    rotated:1;
	unsigned int    rotationOnly:1;
	unsigned int    _RESERVED:11;
    }               mFlags;
}

+ new;
- init;
- _doRotationOnly;
- (NXCoord)getRotationAngle;
- scale:(NXCoord)sx :(NXCoord)sy;
- translate:(NXCoord)tx :(NXCoord)ty;
- rotate:(NXCoord)angle;
- scaleTo:(NXCoord)sx :(NXCoord)sy;
- translateTo:(NXCoord)tx :(NXCoord)ty;
- rotateTo:(NXCoord)angle;
- concat:(PSMatrix *)aMatrix;
- send;
- sendInv;
- transform:(NXPoint *) aPoint;
- invTransform:(NXPoint *) aPoint;
- makeIdentity;
- (BOOL)identity;
- (BOOL)rotated;
- transformRect:(NXRect *) aRect;
- invTransformRect:(NXRect *) aRect;
- _computeInv;
- write:(NXTypedStream *) stream ;
- read:(NXTypedStream *) stream ;
@end


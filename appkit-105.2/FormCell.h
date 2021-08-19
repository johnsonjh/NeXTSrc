/*
	FormCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "ActionCell.h"

@interface FormCell : ActionCell
{
    NXCoord             titleWidth;
    id                  titleCell;
    NXCoord             titleEndPoint;
    unsigned int        _reservedFCint1;
}

- init;
- initTextCell:(const char *)aString;

- free;
- copy;
- (NXCoord)titleWidth:(const NXSize *)aSize;
- (NXCoord)titleWidth;
- setTitleWidth:(NXCoord)width;
- (const char *)title;
- setTitle:(const char *)aString;
- titleFont;
- setTitleFont:fontObj;
- (int)titleAlignment;
- setTitleAlignment:(int)mode;
- setEnabled:(BOOL)flag;
- (BOOL)isOpaque;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- (BOOL)trackMouse:(NXEvent*)event inRect:(const NXRect*)aRect ofView:controlView;
- resetCursorRect:(const NXRect *)cellFrame inView:controlView;
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

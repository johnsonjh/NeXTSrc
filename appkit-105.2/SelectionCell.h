/*
	SelectionCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Cell.h"

@interface SelectionCell : Cell
{
}

- init;
- initTextCell:(const char *)aString;

- awake;
- (BOOL)isOpaque;
- setLeaf:(BOOL)flag;
- (BOOL)isLeaf;
- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTextCell;
+ newTextCell:(const char *)aString;

@end

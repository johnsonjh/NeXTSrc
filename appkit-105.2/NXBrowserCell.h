/*
	NXBrowserCell.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Cell.h"

@interface NXBrowserCell : Cell
{
}

+ branchIcon;
+ branchIconH;

- init;
- initTextCell:(const char *)aString;

- (BOOL)isLeaf;
- setLeaf:(BOOL)flag;
- (BOOL)isLoaded;
- setLoaded:(BOOL)flag;
- reset;

- (BOOL)isOpaque;

- calcCellSize:(NXSize *)theSize inRect:(const NXRect *)aRect;

- drawInside:(const NXRect *)cellFrame inView:controlView;
- drawSelf:(const NXRect *)cellFrame inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)lit;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;
+ newTextCell;
+ newTextCell:(const char *)aString;

@end

/*

	QueryCell.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#import "SelectionCell.h"

@interface QueryCell : SelectionCell

- drawInside:(const NXRect *)cellFrame inView:controlView;

@end

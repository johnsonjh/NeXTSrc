/*

	TaggedCell.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#import "Cell.h"

@interface TaggedCell : Cell
{
    int tag;
}

+ new;
- init;
+ newTextCell:(const char *)aString;
- initTextCell:(const char *)aString;

- (BOOL)isOpaque;
- drawInside:(const NXRect *)cellFrame inView:controlView;
- highlight:(const NXRect *)cellFrame inView:controlView lit:(BOOL)flag;
- setTag:(int)anInt;
- (int)tag;

@end


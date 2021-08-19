/*

	PaperView.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/

#import "View.h"

@interface PaperView : View
{
}

- drawSelf:(const NXRect *)rects :(int)rectCount;
@end

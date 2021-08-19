/*
	QueryCell.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
  
	DEFINED IN: The Application Kit
	HEADER FILES: QueryCell.h
*/

#import "QueryCell.h"
#import "SelectionCell_Private.h"
#import <dpsclient/wraps.h>

@implementation QueryCell

- drawInside:(const NXRect *)cellFrame inView:controlView
{
    if (cFlags1.state || cFlags1.highlighted) {
	PSsetgray(NX_WHITE);
	NXRectFill(cellFrame);
	cFlags1.entryType = 1;
    } else if (cFlags1.entryType) {
	PSsetgray(NX_LTGRAY);
	NXRectFill(cellFrame);
	cFlags1.entryType = 0;
    }

    return [self _drawInside:cellFrame inView:controlView];
}

@end

/*
  
Modifications (since 0.8):
  
 2/14/89 pah	totally rewritten for 0.82
  
*/



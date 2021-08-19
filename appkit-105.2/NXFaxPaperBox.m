/*
	NXFaxPaperBox.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <dpsclient/dpsclient.h>
#import <dpsclient/wraps.h>


#import "NXFaxPaperBox.h"

@implementation NXFaxPaperBox

- drawSelf:(const NXRect *)rects :(int)rectCount
{
/*  this guy exists just to show the black edge to right of page in preview.
 *  edging conforms to size of NXFaxCoverView in box.
 */
    NXRect frects[2];
    NXRect cFrame;
    
    [super drawSelf:rects:rectCount];
    [contentView getFrame : &cFrame];
    frects[0] = cFrame;
    frects[0].origin.x += cFrame.size.width;
    frects[0].origin.y -= 3.0;
    frects[0].size.width = 3.0;
    frects[1] = cFrame;
    frects[1].origin.x += 3.0;
    frects[1].origin.y -= 3.0;
    frects[1].size.height = 3.0;
    PSsetgray(NX_BLACK);
    NXRectFillList(frects,2);
    return self;
}

/*
  appkit.79  Created by cmf  
*/

@end

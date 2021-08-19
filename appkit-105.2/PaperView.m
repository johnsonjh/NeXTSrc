/*
	PaperView.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
  
	DEFINED IN: The Application Kit
	HEADER FILES: PaperView.h
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "PaperView.h"
#import <dpsclient/wraps.h>

@implementation PaperView:View


- drawSelf: (const NXRect *)rects:(int)rectCount
{
    PSsetgray(NX_WHITE);
    while (rectCount) {
	NXRectFill(rects++);
	rectCount--;
    }
    return self;
}


@end



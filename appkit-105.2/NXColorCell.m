

/*
	NXColorCell.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Keith Ohlfs
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "NXColorCell.h"
#import "NXBrowser.h"
#import "Cell.h"
#import "Application.h"
#import "nextstd.h"
#import <dpsclient/wraps.h>
#import <libc.h>
#import <math.h>

#define COLORWIDTH 20.0


@implementation NXColorCell:NXBrowserCell

- setSwatchColor:(NXColor)col
{
    swatchcolor = col;
    return self;
}

- (BOOL)isLeaf
{  
    return YES;
}

- (NXColor)swatchColor
{
    return swatchcolor;
}

- setColorType:(int)atype
{
    colortype = atype;
    return self;
}

- (int)colorType
{
    return colortype;
}

- drawSelf:(const NXRect *)cellFrame inView:controlView
{
    return [self drawInside: cellFrame inView: controlView];
}

- drawInside:(const NXRect *)cellFrame inView:controlView
{
    NXRect colRect = *cellFrame;
    NXRect              _inBounds = *cellFrame;
    register NXRect    *inBounds = &_inBounds;

    colRect.size.width = COLORWIDTH;
    NXSetColor(swatchcolor);
    NXRectFill(&colRect);
    
    inBounds->origin.x += COLORWIDTH;
    inBounds->size.width -= COLORWIDTH;
    [super drawInside: inBounds inView: controlView];
    return self;
}


@end

/*
    
4/1/90 kro	changed to work with NXColor

91
--
 8/12/90 keith	cleaned up a little

*/




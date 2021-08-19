/*
	NXLazyBrowserCell.m
  	Copyright 1990, NeXT, Inc.
	Responsibility: Paul Hegarty
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "NXLazyBrowserCell.h"
#import "Application.h"
#import "Matrix_Private.h"
#import "NXBrowser.h"

@implementation NXLazyBrowserCell

static id shared = nil;

+ (BOOL)_canAlloc { return NO; }

+ allocFromZone:(NXZone *)zone { return [self doesNotRecognize:_cmd]; }

+ alloc { return [self doesNotRecognize:_cmd]; }


+ new
{
    if (!shared) {
	NXZone *zone = [NXApp zone];
	if (!zone) zone = NXDefaultMallocZone();
	shared = [[super allocFromZone:zone] init];
    }
    return shared;
}

- free
{
    return self;
}

- setLoaded:(BOOL)flag
{
    return self;
}

- reset
{
    return self;
}

- (BOOL)isLoaded
{
    return NO;
}

- (BOOL)_checkLoaded:matrix rect:(const NXRect *)cellFrame flushWindow:(BOOL *)flushWindow
{
    int row, col;
    id cell, browser;

    if ([matrix isKindOf:[Matrix class]]) {
	browser = [matrix superview];
	while (browser && [browser class] != [NXBrowser class]) {
	    browser = [browser superview];
	}
	if (browser) {
	    [matrix _getDrawingRow:&row andCol:&col];
	    [[matrix window] disableDisplay];
	    [matrix putCell:[[browser cellPrototype] copy] at:row :col];
	    [[matrix window] reenableDisplay];
	    cell = [matrix cellAt:row :col];
	    if (![cell isKindOf:[NXLazyBrowserCell class]]) {
		[cell setLoaded:NO];
		[cell drawInside:cellFrame inView:matrix];
	    }
	}
    }

    *flushWindow = NO;

    return NO;
}

@end

/*

Modifications:

80
--
  4/9/90 pah	New for 2.0.  Used to support super-lazy NXBrowser mode.

84
--
  5/14/90 pah	Added setLoaded: and reset cover methods
		_checkLoaded: must call _drawingRow:andCol:

*/

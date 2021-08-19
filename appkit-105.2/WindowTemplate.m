/*
	WindowTemplate.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "WindowTemplate.h"
#import "Window.h"
#import "Panel.h"
#import "ChoosePrinter.h"
#import "FontPanel.h"
#import "SavePanel.h"
#import "NXColorPanel.h"
#import "NXFaxPanel.h"
#import "OpenPanel.h"
#import "PageLayout.h"
#import "nextstd.h"

@implementation WindowTemplate

- free
{
    if (windowTitle)
	free(windowTitle);
    if (windowClass)
	free(windowClass);
    if (viewClass)
	free(viewClass);
    return [super free];
}


- nibInstantiate
{
    id                  theWindowClass;

    if (realObject)
	return realObject;
    theWindowClass = objc_getClass(windowClass);
    if (!theWindowClass || ![theWindowClass isKindOf:(id)(((WindowTemplate *)[Window class])->isa)]) {
	NXLogError("Unknown Window class %s in Interface Builder file,\n\t creating generic Window instead", windowClass);
	theWindowClass = [Window class];
    }
    if (windowFlags.wantsToBeColor) {
	if (_NXCanAllocInZone(theWindowClass, @selector(newContent:style:backing:buttonMask:defer:screen:), @selector(initContent:style:backing:buttonMask:defer:screen:))) {
	    realObject = [[theWindowClass allocFromZone:[self zone]] initContent:&windowRect style:windowStyle backing:windowBacking buttonMask:windowButtonMask defer:windowFlags.defer screen:[NXApp colorScreen]];
	} else {
	    realObject = [theWindowClass newContent:&windowRect style:windowStyle backing:windowBacking buttonMask:windowButtonMask defer:windowFlags.defer screen:[NXApp colorScreen]];
	}
    } else {
	if (_NXCanAllocInZone(theWindowClass, @selector(newContent:style:backing:buttonMask:defer:), @selector(initContent:style:backing:buttonMask:defer:))) {
	    realObject = [[theWindowClass allocFromZone:[self zone]] initContent:&windowRect style:windowStyle backing:windowBacking buttonMask:windowButtonMask defer:windowFlags.defer];
	} else {
	    realObject = [theWindowClass newContent:&windowRect style:windowStyle backing:windowBacking buttonMask:windowButtonMask defer:windowFlags.defer];
	}
    }
    [realObject disableDisplay];
    [realObject setHideOnDeactivate:windowFlags.hideOnDeactivate];
    [realObject setFreeWhenClosed:!windowFlags.dontFreeWhenClosed];
    [realObject setDynamicDepthLimit:windowFlags.dynamicDepthLimit];
    [[realObject setContentView:windowView] free];
    [realObject setTitle:windowTitle];
    [realObject setOneShot:windowFlags.oneShot];
    [realObject reenableDisplay];
    return realObject;
}

- awake
{
    [NXApp delayedFree:self];
    return self;
}

- write:(NXTypedStream *) s
{
    [super write:s];
    NXWriteRect(s, &windowRect);
    NXWriteTypes(s, "iiii***@s@", &windowStyle, &windowBacking,
	      &windowButtonMask, &windowEventMask, &windowTitle, &viewClass,
		 &windowClass, &windowView, &windowFlags, &extension);
    return self;
}

- read:(NXTypedStream *) s
{
    [super read:s];
    NXReadRect(s, &windowRect);
    NXReadTypes(s, "iiii***@s@", &windowStyle, &windowBacking,
	      &windowButtonMask, &windowEventMask, &windowTitle, &viewClass,
		&windowClass, &windowView, &windowFlags, &extension);
    return self;
}

@end


/*
  
Modifications (starting at 0.8):
  
1/27/88 bs	added read: write:;
02/18/89 avie	call disableDisplay and reenableDisplay around setTitle
		in nibInstantiate method to prevent extraneous displays
		of the window.  There is still one inefficiency... when
		newContent is called, it does an unnecessary display
		of the window.

83
--
 4/26/90 aozer	Added wantsToBeColor flag and the code to support it.

86
--
 6/8/90 aozer	Added dontDisplayOnScreenChange flag; no by default.

87
--
 7/4/90 trey	windowView no longer freed in -free

89
--
 7/23/90 aozer	added dynamicDepthLimit

*/


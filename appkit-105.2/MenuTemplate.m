/*
	MenuTemplate.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Window_Private.h"
#import "Menu_Private.h"
#import "Cell_Private.h"
#import "MenuTemplate.h"
#import "Menu.h"
#import "MenuCell.h"
#import "Matrix.h"
#import <objc/List.h>
#import "Application.h"
#import "FontManager.h"
#import "PopUpList.h"
#import "FrameView.h"
#import "perfTimer.h"
#import "string.h"
#import <stdlib.h>
#import <zone.h>

typedef struct {
    @defs (Menu)
}                  *menuId;

@implementation MenuTemplate

#ifdef DEBUG
static int ProfileCount = 0;

#define DOPROFILEMARK(flag)	doProfileMark(flag)

static void doProfileMark(BOOL flag)
{
    if (flag) {
	if (!ProfileCount++)
	    PROFILEMARK('M', YES);
    } else {
	if (!--ProfileCount)
	    PROFILEMARK('M', NO);
    }
}

#else
#define DOPROFILEMARK(flag)		/* a NOP when we ship */
#endif


+ initialize
{
   [self setVersion:2];
   return self;
}

- free
{
    free(title);
    free(menuClassName);
    return [super free];
}


- nibInstantiate
{
    int                 i;
    id                  cells;
    id		       *cellPtr;
    NXPoint             initLoc;
    BOOL		popupType, popup = NO;

    if (menuClassName && !strcmp("PopUpList", menuClassName)) popup = YES;
    if (location.x) popupType = YES; else popupType = NO;
    if (realObject)
	return realObject;
    if (!popup)
	DOPROFILEMARK(YES);
    if (supermenu && [supermenu respondsTo:@selector(nibInstantiate)]) {
	supermenu = [supermenu nibInstantiate];
    }
    if (!popup) {
	if (view) {
	    realObject = [Menu _newTitleFromNibSection:title withMatrix:view];
	} else {
	    realObject = [Menu newTitle:title];
	    [[realObject setItemList:view] free];
	}
	[realObject moveTo:location.x:location.y];
	((menuId) realObject)->supermenu = supermenu;
	if ([view prototype]) {
	    [[view prototype] _useUserKeyEquivalent];
	} else {
	    [view setPrototype:[[view makeCellAt:0:0] _useUserKeyEquivalent]];
	}
    } else {
	realObject = [[PopUpList allocFromZone:[self zone]] init];
	[[realObject setItemList:view] free];
    }
    [view setAutodisplay:YES];
    cells = [view cellList];
    if (cells) {
	cellPtr = NX_ADDRESS(cells);
	if (cellPtr)
	    for (i = [cells count]; i--; cellPtr++)
		if ([*cellPtr hasSubmenu])
		    [*cellPtr setTarget:[[*cellPtr target] nibInstantiate]];
		else if (!popup) {
		    [*cellPtr setKeyEquivalent:[*cellPtr userKeyEquivalent]];
		    [*cellPtr _useUserKeyEquivalent];
		}
    }

    if (isFontMenu && !isRequestMenu && !isWindowsMenu && ![[FontManager new] getFontMenu:NO])
        [[FontManager new] setMenu:realObject];
    else if (title && !strcmp(title, "Font") && ![[FontManager new] getFontMenu:NO])
        [[FontManager new] setMenu:realObject];
    if (!popup && !supermenu) {
	[NXApp setMainMenu:realObject];
	_NXGetMenuLocation(&initLoc);
	[realObject moveTopLeftTo:initLoc.x:initLoc.y];
    }
    if (popup) [realObject changeButtonTitle:popupType];
    if (isWindowsMenu && !isRequestMenu && ![NXApp windowsMenu]) [NXApp setWindowsMenu:realObject];
    if (isRequestMenu && !isWindowsMenu && ![NXApp servicesMenu]) [NXApp setServicesMenu:realObject];
    if (!popup)
	DOPROFILEMARK(NO);
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
    NXWriteObjectReference(s, supermenu);
    NXWritePoint(s, &location);
    NXWriteTypes(s, "*@*@ccc", &title, &view, &menuClassName, &extension, &isWindowsMenu, &isRequestMenu, &isFontMenu);
    return self;
}

- read:(NXTypedStream *) s
{
    int version;

    [super read:s];
    version = NXTypedStreamClassVersion(s, "MenuTemplate");
    supermenu = NXReadObject(s);
    NXReadPoint(s, &location);
    if (version < 1) {
	NXReadTypes(s, "*@*@", &title, &view, &menuClassName, &extension);
    } else if (version < 2) {
	NXReadTypes(s, "*@*@cc", &title, &view, &menuClassName, &extension, &isWindowsMenu, &isRequestMenu);
    } else {
	NXReadTypes(s, "*@*@ccc", &title, &view, &menuClassName, &extension, &isWindowsMenu, &isRequestMenu, &isFontMenu);
    }

    return self;
}

@end

/*
  
Modifications (starting at 0.8):
  
12/13/88 bgy	converted to the new List object;
 1/27/88 bs	added read: write:;
 2/10/89 jmh	fix code for menu instantiation
 3/15/89 trey	uses menu private function to position menus

77
--
 3/05/90 pah	Changed not to sizeToFit a menu if it's a PopUpList.

83
--
  5/3/90 pah	eliminated unnecessary sizeToFit during nibInstantiation

86
--
 6/12/90 pah	Added backward compatibility for NXCommandKey support
		Added hooks to set the Request and Windows menus from nib

89
--
 7/29/90 trey	nibInstantiate calls +_newTitleFromNibSection:withMatrix:

*/

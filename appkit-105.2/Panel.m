/*
	Panel.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
 */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Panel_Private.h"
#import "Application_Private.h"
#import "View_Private.h"
#import "Window_Private.h"
#import "Application.h"
#import "View.h"
#import "Menu.h"
#import "NXCursor.h"
#import "nextstd.h"
#import <defaults.h>
#import "privateWraps.h"
#import "packagesWraps.h"
#import <dpsclient/wraps.h>
#import <math.h>
#import <string.h>
#import <sys/param.h>
#import <objc/List.h>
#import <objc/objc-runtime.h>
#import <sys/time.h>
#import <mach.h>
#import <stdio.h>

@implementation Panel:Window


/* Creation */

id _NXCallPanelSuper(id factory, SEL factoryMethod,
		  const NXRect *contentRect, int aStyle, int bufferingType,
		  int mask, BOOL flag)
{
    struct objc_super _X;

    _X.receiver = factory;
    _X.class = (Class)&_OBJC_METACLASS_Panel;
    return objc_msgSendSuper(&_X, factoryMethod, contentRect, aStyle,
					    bufferingType, mask, flag);
}

id _NXCallPanelSuperFromZone(id factory, SEL factoryMethod, NXZone *zone)
{
    struct objc_super _X;

    _X.receiver = factory;
    _X.class = (Class)&_OBJC_METACLASS_Panel;
    return objc_msgSendSuper(&_X, factoryMethod, zone);
}

+ newContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    return [[self allocFromZone:NXDefaultMallocZone()] initContent:contentRect 
	style:aStyle backing:bufferingType buttonMask:mask defer:flag];
}

- _initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
    contentView:aView
{
    [super _initContent:contentRect
	    style:aStyle
	    backing:bufferingType
	    buttonMask:mask
	    defer:flag
	    contentView:aView];
    [self setBackgroundGray:NX_LTGRAY];
    [self setFreeWhenClosed:NO];
    [self setHideOnDeactivate:YES];
    return self;
}

- initContent:(const NXRect *)contentRect
    style:(int)aStyle
    backing:(int)bufferingType
    buttonMask:(int)mask
    defer:(BOOL)flag
{
    [super initContent:contentRect
	    style:aStyle
	    backing:bufferingType
	    buttonMask:mask
	    defer:flag];
    [self setBackgroundGray:NX_LTGRAY];
    [self setFreeWhenClosed:NO];
    [self setHideOnDeactivate:YES];
    return self;
}


+ new
{
    self = [self newContent:NULL
	    style:NX_TITLEDSTYLE
	    backing:NX_BUFFERED
	    buttonMask:NX_CLOSEBUTTONMASK
	    defer:YES];
    return self;
}

- init
{
    [self initContent:NULL
	    style:NX_TITLEDSTYLE
	    backing:NX_BUFFERED
	    buttonMask:NX_CLOSEBUTTONMASK
	    defer:YES];
    return self;
}

- (BOOL)isFloatingPanel
{
    return wFlags2._floatingPanel;
}

- setFloatingPanel:(BOOL)flag
{
    if ([self isKindOf:[Menu class]]) return self;

    if (flag) {
	wFlags2._floatingPanel = YES;
	if (windowNum > 0) [self _setWindowLevel:NX_FLOATINGLEVEL];
    } else {
	wFlags2._floatingPanel = NO;
	if (windowNum > 0) [self _setWindowLevel:NX_NORMALLEVEL];
    }
    return self;
}

- _doOrderWindow:(int)place relativeTo:(int)otherWin findKey:(BOOL)doKeyCalc level:(int)level forcounter:(BOOL)aCounter
{
    return [super _doOrderWindow:place relativeTo:otherWin findKey:doKeyCalc level:(wFlags2._floatingPanel ? NX_FLOATINGLEVEL : level) forcounter:aCounter];
}

+ _writeFrame:(const NXRect *)theFrame toDefaults:(const char *)panelName domain:(const char *)domain;
{
    char defName[100];
    char value[100];

    strcpy(defName, panelName);
    strcat(defName, "Frame");
    sprintf(value, "%d %d %d %d",
	(int)theFrame->origin.x, (int)theFrame->origin.y,
	(int)theFrame->size.width, (int)theFrame->size.height);
    NXWriteDefault(domain, defName, value);

    return self;
}

+ (BOOL)_readFrame:(NXRect *)theFrame fromDefaults:(const char *)panelName domain:(const char *)domain;
{
    char defName[100];
    const char *value;
    int matches, x, y, w, h;

    if (!theFrame) return NO;
    strcpy(defName, panelName);
    strcat(defName, "Frame");
    if (!(value = NXUpdateDefault(domain, defName))) {
	value = NXGetDefaultValue(domain, defName);
    }
    if (!value) return NO;
    matches = sscanf(value, "%d %d %d %d", &x, &y, &w, &h);
    if (matches != 4) return NO;
    theFrame->origin.x = (NXCoord)x;
    theFrame->origin.y = (NXCoord)y;
    theFrame->size.width = (NXCoord)w;
    theFrame->size.height = (NXCoord)h;

    return YES;
}

/* Event handling */

- (BOOL)commandKey:(NXEvent *)theEvent
{
    if (![self isVisible]) {
	[self update];		/* update those invisible guys */
    }
    if (theEvent->type == NX_KEYDOWN &&
	theEvent->data.key.charSet == NX_ASCIISET &&
	theEvent->data.key.charCode != 0) {
	return [contentView performKeyEquivalent:theEvent];
    } else {
	return NO;
    }
}


- keyDown:(NXEvent *)theEvent
{
  /* this is a gross hack (its how keyboard equivalents for buttons work) */
    [self commandKey:theEvent];
    return self;
}

- setBecomeKeyOnlyIfNeeded:(BOOL)flag
{
    wFlags2._limitedBecomeKey = flag ? YES : NO;
    return self;
}

- (BOOL)doesBecomeKeyOnlyIfNeeded
{
    return wFlags2._limitedBecomeKey ? YES : NO;
}

- setWorksWhenModal:(BOOL)flag
{
    wFlags2._worksWhenModal = flag ? YES : NO;
    return self;
}

- (BOOL)worksWhenModal
{
    return wFlags2._worksWhenModal ? YES : NO;
}

/* Internal methods */


- _setTempHidden:(BOOL)aFlag
{
    if (!wFlags.visible)
	wFlags2._tempHidden = aFlag; /* wot -- This used to be a unconditional YES */
    return self;
}


/* Used by panel subclasses to set accessory views. */
- _doSetAccessoryView:new topView:top bottomView:bottom oldView:(id *)old
{
    id                  oldAccView;

    if (*old != new)
	[self _replaceAccessoryView:*old with:new
			topView:top bottomView:bottom];
    oldAccView = *old;
    *old = new;
    return oldAccView;
}


/* inserts a custom view tree into a panel.  It creates a horizontal strip
   of space between the top and bottom views.
 */
- _replaceAccessoryView:curView with:newView topView:top bottomView:bottom
{
    int                 i;
    id                  subviews, *vptr;
    NXCoord             dw, dh, gapBottom, gapTop;
    NXRect              winFrame, curFrame, newFrame, tempRect;

    AK_ASSERT([contentView isFlipped] == NO, "Flipped contentView with Accessory View");
    [contentView _setNoVerticalAutosizing:YES];
    [newView setAutosizing:NX_MINXMARGINSIZABLE|NX_MAXXMARGINSIZABLE];
    [self disableDisplay];
    newFrame.size.width = newFrame.size.height = 0.0;	/* in case !newView */
    [newView getFrame:&newFrame];
    curFrame.size.width = curFrame.size.height = 0.0;	/* in case !curView */
    [curView getFrame:&curFrame];
    dw = floor(newFrame.size.width - frame.size.width);
    dw = dw < 0.0 ? 0.0 : dw;
    dh = floor(curFrame.size.height - newFrame.size.height);
    winFrame = frame;
    winFrame.size.width += dw;
    winFrame.size.height -= dh;
    [self placeWindow:&winFrame];
    [contentView translate:0.0:-dh];
    [curView removeFromSuperview];
    [top getFrame:&tempRect];
    gapTop = tempRect.origin.y;
    subviews = [contentView subviews];
    for (i = [subviews count], vptr = NX_ADDRESS(subviews); i--; vptr++) {
	[*vptr getFrame:&tempRect];
	if (tempRect.origin.y < gapTop)
	    [*vptr moveBy:0.0 :dh];
    }
    if (newView) {
	if (bottom) {
	    [bottom getFrame:&tempRect];	/* check after its been moved */
	    gapBottom = tempRect.origin.y + tempRect.size.height;
        } else {
	    [contentView getBounds:&tempRect];
	    gapBottom = tempRect.origin.y;
	}
	[newView moveTo:floor((frame.size.width - newFrame.size.width) / 2.0)
	  :gapBottom + floor((gapTop - gapBottom - newFrame.size.height) / 2.0)];
	[contentView addSubview:newView];
    }
    [self reenableDisplay];
    [self display];
    [contentView _setNoVerticalAutosizing:NO];
    return self;
}


/* Loads a nib file from specified segment & section of
 * the shared library.  This function is a slightly generalized version
 * of _NXLoadNibPanel().
 */
id _NXLoadNib(const char *segName, const char *sectName, id owner, NXZone *zone)
{
    id retval = nil;
    const char *const *languages;
    char path[MAXPATHLEN+1];
    BOOL zoneCreated;

    if (!zone) {
	zone = NXCreateZone(vm_page_size, vm_page_size, YES);
	zoneCreated = YES;
    } else
	zoneCreated = NO;

    languages = [NXApp systemLanguages];
    if (languages) {
	while (!retval && *languages) {
	    if (!strcmp(*languages, KIT_LANGUAGE)) break;
	    sprintf(path, "%s/Languages/%s/%s.nib", _NXAppKitFilesPath, *languages, sectName);
	    retval = [NXApp loadNibFile:path owner:owner withNames:NO fromZone:zone];
	    languages++;
	}
    }
    if (!retval) {
	retval = [NXApp _loadNibSegment:segName section:sectName owner:owner
			      withNames:NO fromShlib:YES fromHeader:NULL fromZone:zone];
    }
    if (!retval) {
	strcpy(path, _NXAppKitFilesPath);
	strcat(path, sectName);
	strcat(path, ".nib");
	retval = [NXApp loadNibFile:path owner:owner withNames:NO fromZone:zone];
    }

    if (!retval) {
	NXDestroyZone(zone);
	if (_NXKitAlert("Error", NULL, "The Interface Builder file %s cannot be loaded.", NULL, NULL, NULL, path) == NX_ALERTERROR) {
	    NXLogError("Cannot load Interface Builder file: %s\n", path);
	}
    } else if (zoneCreated)
	NXNameZone(zone, [retval name]);

    return retval;
}

/* Loads an appkit panel for the given class name.  The following conventions
   are assumed:  1) The segment name in the appkit shlib is __APPKIT_PANELS.
   2) The section name is the name parameter (should be the class name).
   3) In case an appkit shlib is not being used, the nib data has been
   installed in the file /usr/lib/NextStep/<name>.nib.
 */
id _NXLoadNibPanel(const char *name)
{
    return _NXLoadNib ("__APPKIT_PANELS", name, NXApp, NULL);
}

@end

/*
  
Modifications (starting at 0.8):
  
 3/17/89 trey	-_insertAccessoryView:topView:bottomView: and
		 -_replaceAccessoryView:with: added to support customization
		 of panels
 3/20/89 pah	check to be sure we could load nib file, and if we couldn't,
 		 put up an Alert
 3/21/89 wrp	added const to declarations

0.92
----
 6/12/89 trey	combined _replaceAccessoryView: and _insertAccessoryView:
		_replaceAccessoryView: enforces no X margin around inserted
		 view
		wait cursor used while loading panels

0.93
----
 6/16/89 pah	change NXAlert to NXRunAlertPanel()
 
0.96
----
 7/21/89 wrp	changed commandKey to only 'update' if Panel not visible

77
--
 2/07/90 trey	nuked -_tempHide:
		put Menu specific stuff of -showPanel in Menu
		-_hidePanel bypasses -orderOut: so it doesnt affect which
		 window is key
		_NXLoadNibPanel sets incrementalFlush bit on kit panels

78
--
 3/8/90 aozer	added _NXLoadNibFromShlib() to allow loading nib files from a 
		specified segment in libNeXT with a specified owner		

79
--
 3/21/90 trey	fixed bug in _NXLoadNibPanel to not hardwire owner as NXApp
		 if search of MachO fails		
   4/3/90 pah	added setBecomeKeyOnlyWhenHitAcceptsFirstResponder:
		 (a method which needs a new name)
		 it sets a bit in the window which causes it to become
		  the key window only if the user mouses down on a view which
		  acceptsFirstResponder (limitedBecomeKey)
		added setWorksWhenModal: and worksWhenModal
		 allows a panel to operate even when a modal panel is up
		 useful when you want a panel (such as the FontPanel) to
		  operate on a modal panel

80
--
  4/8/90 pah	add setFloatPanel: support for panels that float

84
--
 5/13/90 trey	nuked incremental flush code

85
--
 5/31/90 trey	nuked use of NXWait
 5/31/90 aozer	Put in a check for zero charCode in commandKey:. (Fix #5059)

89
--
 7/29/90 trey	addded +_newContent:style:backing:buttonMask:defer:contentView:

91
--
 8/12/90 trey	nuked _showPanel/_hidePanel dead code

92
--
 8/20/90 gcockrft  Named the panel malloc zones.

98
--
 10/13/90 aozer	Fixed 10710, where deferred floating panels weren't floating.
		Turned out _doOrder...level: method in window grew to
		_doOrder...level:forCounter:, but Panel was still calling the
		old one.


*/

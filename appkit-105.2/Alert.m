/*
	Alert.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Paul Hegarty
  
	DEFINED IN: The Application Kit
	HEADER FILES: Alert.h Panel.h
*/

#ifdef SHLIB
#import "shlib.h"
#endif

#import "appkitPrivate.h"
#import "Alert.h"
#import "Application_Private.h"
#import "Button.h"
#import "Cell_Private.h"
#import "Font.h"
#import "Panel.h"
#import "nextstd.h"
#import "errors.h"
#import <stdarg.h>
#import <zone.h>

@implementation Alert

static id cachedAlert = nil;
static BOOL haveCachedAnAlert = NO;
static const char *defaultTitle = NULL;
static const char *defaultButton = NULL;

+ new
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    char path[MAXPATHLEN+15];

    [super init];
    panel = [NXApp _loadNibSegment:"__APPKIT_PANELS" section:"AlertPanel"
				owner:self withNames:NO fromShlib:YES
			fromHeader:NULL fromZone:[self zone]];
    if (!panel) {
	strcpy(path, _NXAppKitFilesPath);
	strcat(path, "AlertPanel.nib");
	panel = [NXApp loadNibFile:path owner:self withNames:NO fromZone:[self zone]];
    }
    if (!panel) {
	NXLogError("Cannot load Interface Builder file: %s/AlertPanel.nib", _NXAppKitFilesPath);
	[self free];
	self = nil;
    } else {
	[panel setOneShot:YES];
	[panel useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
    }
    if (!defaultTitle) {
        defaultTitle = KitString(Common, "Alert", "Default title of an Alert panel.");
	defaultButton = KitString(Common, "OK", "Default button on an Alert panel.");
    }

    return self;
}

- free
{
    if (![first superview]) [first free];
    if (![second superview]) [second free];
    if (![third superview]) [third free];
    [panel free];
    return [super free];
}

- setFirst:anObject
{
    NXRect bframe, wframe;

    first = anObject;
    [first getFrame:&bframe];
    [[first window] getFrame:&wframe];
    buttonSpacing = wframe.size.width - bframe.origin.x - bframe.size.width;
    buttonHeight = bframe.size.height;

    return self;
}

- setPanel:anObject
{
    NXRect wframe;

    panel = anObject;
    [panel getFrame:&wframe];
    defaultPanelSize = wframe.size;

    return self;
}

- setIconButton:anObject
{
    [anObject setIcon:"app"];
    return self;
}

- setMsg:anObject
{
    msg = anObject;
    [msg setFont:[Font newFont:NXSystemFont size:[[msg font] pointSize]]];
    [[msg cell] _setCentered:YES];
    return self;
}

- buttonPressed:sender
{
    int exitValue;

    if (sender == first) {
	exitValue = NX_ALERTDEFAULT;
    } else if (sender == second) {
	exitValue = NX_ALERTALTERNATE;
    } else if (sender == third) {
	exitValue = NX_ALERTOTHER;
    } else {
	AK_ASSERT(NO, "Alert - Got buttonPressed: from unknown source");
	exitValue = NX_ALERTERROR;
    }

    [NXApp stopModal:exitValue];

    return self;
}

static NXCoord sizeButton(id button)
{
    NXRect bframe;

    [button sizeToFit];
    [button getFrame:&bframe];

    return bframe.size.width;
}

- placeButtons:(int)count
{
    NXRect bframe, wframe;
    NXCoord width, maxWidth = 0.0;

    width = sizeButton(first);
    maxWidth = MAX(width, maxWidth);
    if (count > 1) {
	width = sizeButton(second);
	maxWidth = MAX(width, maxWidth);
	if (count > 2) {
	    width = sizeButton(third);
	    maxWidth = MAX(width, maxWidth);
	}
    }

    [panel getFrame:&wframe];
    width = (maxWidth + buttonSpacing) * 3 + buttonSpacing;
    width = MAX(width, defaultPanelSize.width);
    if (width != wframe.size.width) {
	wframe.size.width = width;
	[panel placeWindow:&wframe];
    }

    bframe.origin.y = buttonSpacing;
    bframe.size.width = maxWidth;
    bframe.size.height = buttonHeight;
    bframe.origin.x = wframe.size.width - buttonSpacing - maxWidth;
    [first setFrame:&bframe];
    if (count > 1) {
	bframe.origin.x -= buttonSpacing + maxWidth;
	[second setFrame:&bframe];
	if (count > 2) {
	    bframe.origin.x -= buttonSpacing + maxWidth;
	    [third setFrame:&bframe];
	}
    }

    return self;
}

- setMessage:(const char *)message
{
    id cell;
    NXSize msgSize;
    NXCoord panelHeight;
    NXRect mframe, wframe;

    [panel getFrame:&wframe];
    [msg getFrame:&mframe];
    mframe.size.height = 10000.0;
    [msg setStringValue:message];
    cell = [msg cell];
    [cell calcCellSize:&msgSize inRect:&mframe];
    [msg getFrame:&mframe];
    panelHeight = wframe.size.height + msgSize.height - mframe.size.height;
    panelHeight = MAX(panelHeight, defaultPanelSize.height);
    if (wframe.size.height != panelHeight) {
	wframe.size.height = panelHeight;
	[panel placeWindow:&wframe];
    }

    return self;
}

#define MAXMSGLENGTH 1024

static id buildAlert(Alert *alert, const char *title, const char *s, const char *first, const char *second, const char *third, va_list ap)
{
    const char *t, *msg;
    char msgbuf[MAXMSGLENGTH];

    if (s && strlen(s) < (MAXMSGLENGTH / 3)) {
	vsprintf(msgbuf, s, ap);
	msg = msgbuf;
    } else {
	msg = s;
    }

    if (first) {
	[alert->first setTitle:first];
	if (!title || !*title) {
	    [alert->title setStringValueNoCopy:""];
	} else {
	    t = [alert->title stringValue];
	    if (!t || strcmp(t, title)) [alert->title setStringValue:title];
	}
	if (second) {
	    [[alert->panel contentView] addSubview:alert->second];
	    [alert->second setTitle:second];
	    if (third) {
		[[alert->panel contentView] addSubview:alert->third];
		[alert->third setTitle:third];
		[alert placeButtons:3];
	    } else {
		[alert->third removeFromSuperview];
		[alert placeButtons:2];
	    }
	} else {
	    [alert->second removeFromSuperview];
	    [alert->third removeFromSuperview];
	    [alert placeButtons:1];
	}
    } else {
	[alert->first removeFromSuperview];
	[alert->second removeFromSuperview];
	[alert->third removeFromSuperview];
    }

    [alert setMessage:msg];

    return alert->panel;
}

id NXGetAlertPanel(const char *title, const char *s, const char *first, const char *second, const char *third, ...)
{
    id retval;
    va_list ap;
    Alert *alert;

    alert = [Alert new];

    if (!alert) {
	NXLogError("Cannot load Interface Builder file: %s/AlertPanel.nib", _NXAppKitFilesPath);
	return nil;
    }

    va_start(ap, third);
    title = title ? title : defaultTitle;
    retval = buildAlert(alert, title, s, first, second, third, ap);
    va_end(ap);

    return retval;
}

void NXFreeAlertPanel(id alertPanel)
{
    [NXApp delayedFree:[alertPanel delegate]];
}

int NXRunAlertPanel(const char *title, const char *s, const char *first, const char *second, const char *third, ...)
{
    id panel;
    va_list ap;
    NXZone *zone;
    Alert *newAlert;
    volatile NXHandler handler;
    volatile int exitValue = NX_ALERTERROR;
    volatile BOOL gotCached = NO;

    if (cachedAlert) {
	newAlert = cachedAlert;
	cachedAlert = nil;
	gotCached = YES;
    } else {
	zone = [NXApp zone];
	if (!zone) zone = NXDefaultMallocZone();
	zone = NXCreateChildZone(zone, 1024, 1024, YES);
	newAlert = [[Alert allocFromZone:zone] init];
	NXMergeZone(zone);
	if (!newAlert) return NX_ALERTERROR;
	gotCached = !haveCachedAnAlert;
	haveCachedAnAlert = YES;
    }

    va_start(ap, third);
    title = (title && *title) ? title : defaultTitle;
    first = (first && *first) ? first : defaultButton;
    panel = buildAlert(newAlert, title , s, first, second, third, ap);
    va_end(ap);

    NXPing();

    NX_DURING {
	handler.code = 0;
	exitValue = [NXApp runModalFor:panel];
    } NX_HANDLER {
	handler = NXLocalHandler;
	if (handler.code == dps_err_ps) NXReportError((NXHandler *)(&handler));
    } NX_ENDHANDLER

    [panel orderOut:panel];

    if (gotCached) {
	cachedAlert = [panel delegate];
    } else {
	NXFreeAlertPanel(panel);
    }

    if (handler.code && handler.code != dps_err_ps) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return exitValue;
}


int _NXKitAlert(const char *stringTable, const char *title, const char *s, const char *first, const char *second, const char *third, ...)
{
    id panel;
    va_list ap;
    NXZone *zone;
    Alert *newAlert;
    volatile NXHandler handler;
    volatile int exitValue = NX_ALERTERROR;
    volatile BOOL gotCached = NO;

    if (cachedAlert) {
	newAlert = cachedAlert;
	cachedAlert = nil;
	gotCached = YES;
    } else {
	zone = [NXApp zone];
	if (!zone) zone = NXDefaultMallocZone();
	zone = NXCreateChildZone(zone, 1024, 1024, YES);
	newAlert = [[Alert allocFromZone:zone] init];
	NXMergeZone(zone);
	if (!newAlert) return NX_ALERTERROR;
	gotCached = !haveCachedAnAlert;
	haveCachedAnAlert = YES;
    }

    va_start(ap, third);
    title = (title && *title) ? _NXKitString(stringTable, title) : defaultTitle;
    first = (first && *first) ? _NXKitString(stringTable, first) : defaultButton;
    if (s) s = _NXKitString(stringTable, s);
    if (second) second = _NXKitString(stringTable, second);
    if (third) third = _NXKitString(stringTable, third);
    panel = buildAlert(newAlert, title , s, first, second, third, ap);
    va_end(ap);

    NXPing();

    NX_DURING {
	handler.code = 0;
	exitValue = [NXApp runModalFor:panel];
    } NX_HANDLER {
	handler = NXLocalHandler;
	if (handler.code == dps_err_ps) NXReportError((NXHandler *)(&handler));
    } NX_ENDHANDLER

    [panel orderOut:panel];

    if (gotCached) {
	cachedAlert = [panel delegate];
    } else {
	NXFreeAlertPanel(panel);
    }

    if (handler.code && handler.code != dps_err_ps) {
	NX_RAISE(handler.code, handler.data1, handler.data2);
    }

    return exitValue;
}


int NXAlert(const char *s, const char *first, const char *second, const char *third)
{
    return NXRunAlertPanel(NULL, s, first, second, third);
}

int NXTitledAlert(const char *title, const char *s, const char *first, const char *second, const char *third)
{
    return NXRunAlertPanel(title, s, first, second, third);
}

@end

/*
  
Modifications (since 0.8):
  
 2/18/89 pah	new for 0.82 (private class)
  
0.83
----
 3/15/89 pah	Fix bug whereby button was not properly placed near right edge
 3/17/89 pah	Error handling around runModalFor:
 3/20/89 pah	Better error checking for non-existent .nib file

0.91
----
 5/28/89 pah	support NXTitledAlert()

0.93
----
 6/14/89 pah	add free method
 6/15/89 pah	eliminate NXTitledAlert() and NXAlert()
		 add methods NXRunAlertPanel(), NXGetAlertPanel(),
		 and NXFreeAlertPanel()
 6/16/89 pah	support stopModal: scheme
 6/19/89 pah	add varargs support to NX*AlertPanel()
 6/27/89 trey	add checking for loadNibFile failure

0.94
----
 7/13/89 pah	fixed bug whereby buildAlert barfed on a NULL msg field
 7/19/89 trey	alert's nibfile read from shlib's MachO

77
--
 3/02/90 trey	makes panel incrementally flush drawing

84
--
 5/13/90 trey	nuked incremental flush code

91
--
  8/9/91 pah	general cleanup and implement autosizing

*/

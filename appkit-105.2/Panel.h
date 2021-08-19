/*
	Panel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Window.h"

/*
 * In the following two functions, msg may be a printf-like message with
 * the arguments tacked onto the end.  Thus, to get a '%' in your message,
 * you must use '%%'
 */

extern int NXRunAlertPanel(const char *title, const char *msg, const char *defaultButton, const char *alternateButton, const char *otherButton, ...);

extern id NXGetAlertPanel(const char *title, const char *msg, const char *defaultButton, const char *alternateButton, const char *otherButton, ...);

extern void NXFreeAlertPanel(id panel);

/*
 * NXRunAlertPanel() return values (also returned by runModalSession: when
 * the modal session is run with a panel returned by NXGetAlertPanel()).
 */

#define NX_ALERTDEFAULT		1
#define NX_ALERTALTERNATE	0
#define NX_ALERTOTHER		-1
#define NX_ALERTERROR		-2

/* Tags for common controls in Panels */

#define NX_OKTAG	1
#define NX_CANCELTAG	0

@interface Panel : Window
{
}

- initContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
- init;
- (BOOL)commandKey:(NXEvent *)theEvent;
- keyDown:(NXEvent *)theEvent;
- (BOOL)isFloatingPanel;
- setFloatingPanel:(BOOL)flag;
- (BOOL)doesBecomeKeyOnlyIfNeeded;
- setBecomeKeyOnlyIfNeeded:(BOOL)flag;
- (BOOL)worksWhenModal;
- setWorksWhenModal:(BOOL)flag;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
+ new;

@end

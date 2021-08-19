/*
	screens.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "graphics.h"

/*
 * Information about the various screens hooked up to the machine
 * on which the window server is running. This information can be
 * accessed with the getScreens:count: (and various other methods)
 * in Application, Window, and View.
 *
 * You may access the fields of this structure when you get them from
 * these methods.
 */

typedef struct _NXScreen {

    int screenNumber;		/* Screen number (may be used as */
				/* argument to framebuffer op). */
    NXRect screenBounds;	/* Bounds of the screen. */

    short _reservedShort[6];	/* Don't use these. */

    NXWindowDepth depth;	/* Depth of the frame buffer */

    int _reserved[3];		/* Don't use these either. */

} NXScreen;

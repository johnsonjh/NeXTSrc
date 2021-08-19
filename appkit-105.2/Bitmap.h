/*
	Bitmap.h
	Application Kit, Release 1.0
	Copyright (c) 1988, 1989, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "graphics.h"

/* NOTE: This class has been obsoleted, use NXImage instead.    */
/*       To ease conversion, the file obsoleteBitmap.h contains */
/*       The old declarations of the Bitmap methods.            */

/* Bitmap Types */

#define	NX_UNIQUEBITMAP		(-1)
#define	NX_NOALPHABITMAP	(0)
#define	NX_ALPHABITMAP		(1)
#define	NX_UNIQUEALPHABITMAP	(2)

@interface Bitmap : Object
{
    NXRect 	    frame;	
    char	   *iconName;	
    int             type;	
    int		    _builtIn;
    void	   *_manager;		
    struct __bFlags{
	unsigned int    _flipDraw:1;
	unsigned int    _systemBitmap:1;
	unsigned int    _nibBitmap:1;
	unsigned int    _willFree:1;
	unsigned int    _RESERVED:12;
    }               _bFlags;
}

/* These remain since the prototypes are necessary for correctness. */

+ newSize:(NXCoord)width :(NXCoord)height type:(int)aType;
- resize:(NXCoord)theWidth :(NXCoord)theHeight;

@end

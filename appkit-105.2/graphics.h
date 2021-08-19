/*
	graphics.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

/* This file defines the most primitive client graphic structures. */

#ifndef GRAPHICS_H
#define GRAPHICS_H

#ifndef EVENT_H
#import <dpsclient/dpsfriends.h>
#import <dpsclient/event.h>
#endif EVENT_H

#ifndef OBJC_INCL
#import <objc/objc.h>
#endif OBJC_INCL

#import <objc/typedstream.h>

#define	NX_WHITE	(1.0)
#define	NX_LTGRAY	(2.0/3.0)
#define	NX_DKGRAY	(1.0/3.0)
#define	NX_BLACK	(0.0)

typedef struct _NXRect {	/* rectangle */
    NXPoint         origin;
    NXSize          size;
} NXRect;

#define	NX_X(aRect)	((aRect)->origin.x)
#define	NX_Y(aRect)	((aRect)->origin.y)
#define	NX_WIDTH(aRect)	((aRect)->size.width)
#define	NX_HEIGHT(aRect)	((aRect)->size.height)
#define	NX_MAXX(aRect)	((aRect)->origin.x + (aRect)->size.width)
#define	NX_MAXY(aRect)	((aRect)->origin.y + (aRect)->size.height)
#define	NX_MIDX(aRect)	(NX_X(aRect)+NX_WIDTH(aRect)/2.0)
#define	NX_MIDY(aRect)	(NX_Y(aRect)+NX_HEIGHT(aRect)/2.0)

/* The sides of a rectangle */
#define NX_XMIN	0
#define NX_YMIN	1
#define NX_XMAX	2
#define NX_YMAX	3

extern void NXCopyBits(int sgnum, const NXRect *sRect, const NXPoint *dPoint);
extern void NXDrawButton(const NXRect *aRect, const NXRect *clipRect);
extern void NXDrawGrayBezel(const NXRect *aRect, const NXRect *clipRect);
extern void NXDrawGroove(const NXRect *aRect, const NXRect *clipRect);
extern NXRect *NXDrawTiledRects(NXRect *boundsRect, const NXRect *clipRect,
		const int *sides, const float *grays, int count);
extern void NXDrawWhiteBezel(const NXRect *aRect, const NXRect *clipRect);
extern void NXEraseRect(const NXRect *aRect);
extern void NXFrameRect(const NXRect *aRect);
extern void NXFrameRectWithWidth(const NXRect *aRect, NXCoord frameWidth);
extern void NXRectClip(const NXRect *aRect);
extern void NXRectClipList(const NXRect *rects, int count);
extern void NXRectFill(const NXRect *aRect);
extern void NXRectFillList(const NXRect *rects, int count);
extern void NXHighlightRect(const NXRect *aRect);
extern void NXRectFillListWithGrays(const NXRect *rects, const float *grays, int count);

extern BOOL NXContainsRect(const NXRect *a, const NXRect *b);
extern NXRect *NXDivideRect(NXRect *aRect, NXRect *bRect, NXCoord slice, int edge);
extern BOOL NXEmptyRect(const NXRect *aRect);
extern BOOL NXEqualRect(const NXRect *aRect, const NXRect *bRect);
extern void NXInsetRect(NXRect *aRect, NXCoord dX, NXCoord dY);
extern void NXIntegralRect(NXRect *aRect);
extern NXRect *NXIntersectionRect(const NXRect *aRect, NXRect *bRect);
extern BOOL NXIntersectsRect(const NXRect *aRect, const NXRect *bRect);
extern void NXOffsetRect(NXRect *aRect, NXCoord dX, NXCoord dY);
extern BOOL NXPointInRect(const NXPoint *aPoint, const NXRect *aRect);
extern BOOL NXMouseInRect(const NXPoint *aPoint, const NXRect *aRect, BOOL flipped);
extern void NXSetRect(NXRect *aRect, NXCoord x, NXCoord y, NXCoord w, NXCoord h);
extern NXRect *NXUnionRect(const NXRect *aRect, NXRect *bRect);

/* Obsolete, do not use */
extern int NXContainRect(NXRect *aRect, const NXRect *bRect);


extern void NXPing(void);


/*
 * The following four constants are obsolete and used in the 1.0 API. 
 * Use NXColorSpace instead.
 */
#define NX_MONOTONICMASK	1
#define NX_COLORMASK		2
#define NX_ALPHAMASK		4
#define NX_PALETTEMASK		8

/*
 * The following two constants are obsolete and used in the 1.0 API to
 * describe planar configuration of bitmaps. 2.0 API uses a boolean (isPlanar) 
 * instead.
 */
#define NX_MESHED		1
#define NX_PLANAR		2	

/*
 * The following values should be used in describing color space of bitmaps.
 */
typedef enum _NXColorSpace { 
    NX_OneIsBlackColorSpace = 0,	/* monochrome, 1 is black */
    NX_OneIsWhiteColorSpace = 1,	/* monochrome, 1 is white */
    NX_RGBColorSpace = 2,			
    NX_CMYKColorSpace = 5
} NXColorSpace;

/*
 * NXWindowDepth defines the values used in setting window depth limits.
 * Use the functions NXBPSFromDepth() and NXColorSpaceFromDepth()
 * to extract colorspace/bps info from an NXWindowDepth.
 */
typedef enum _NXWindowDepth { 
    NX_DefaultDepth = 0,
    NX_TwoBitGrayDepth = 258,
    NX_EightBitGrayDepth = 264,			
    NX_TwelveBitRGBDepth = 516,
    NX_TwentyFourBitRGBDepth = 520
} NXWindowDepth;

/*
 * #defines for historical reasons; don't use these in new applications.
 */
#define NXWindowDepthType	NXWindowDepth
#define NXColorSpaceType	NXColorSpace

extern NXColorSpace NXColorSpaceFromDepth (NXWindowDepth depth);
extern int NXBPSFromDepth (NXWindowDepth depth);
extern int NXNumberOfColorComponents (NXColorSpace colorSpace);
extern BOOL NXGetBestDepth (NXWindowDepth *depth, int numColors, int bps);

extern void NXWritePoint (NXTypedStream *s, const NXPoint *aPoint);
	/* Equivalent to NXWriteTypes (s, "ff", &aPoint.x, &aPoint.y) */
extern void NXReadPoint (NXTypedStream *s, NXPoint *aPoint);

extern void NXWriteSize (NXTypedStream *s, const NXSize *aSize);
	/* Equivalent to NXWriteTypes (s, "ff", &aSize.width, &aSize.height) */
extern void NXReadSize (NXTypedStream *s, NXSize *aSize);

extern void NXWriteRect (NXTypedStream *s, const NXRect *aRect);
	/* Equivalent to NXWriteArray (s, "f", 4, &aRect) */
extern void NXReadRect (NXTypedStream *s, NXRect *aRect);

#endif GRAPHICS_H

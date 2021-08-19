/*
	rect.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bill Parkhurst
	  
	DEFINED IN: The Application Kit
	HEADER FILES: appkit.h
*/

/*	This file contains rectangle arithmetic functions */


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "graphics.h"
#import <math.h>
#import "nextstd.h"

#define ZERO_RECT(r) ((r)->origin.x = (r)->origin.y = (r)->size.width = (r)->size.height = 0.)
#define EMPTY_RECT(r)	(((r)->size.width <= 0.0) || ((r)->size.height <= 0.0))

/* Testing */

BOOL
NXPointInRect(const NXPoint *aPoint, const NXRect *aRect)
{
    return (aPoint->x >= NX_X(aRect) &&
	    aPoint->x < NX_MAXX(aRect) &&
	    aPoint->y >= NX_Y(aRect) &&
	    aPoint->y < NX_MAXY(aRect));
}


BOOL
NXMouseInRect(const NXPoint *aPoint, const NXRect *aRect, BOOL flipped)
{
    return (aPoint->x >= NX_X(aRect) &&
	    aPoint->x < NX_MAXX(aRect) &&
	    (flipped ? (aPoint->y >= NX_Y(aRect) && aPoint->y < NX_MAXY(aRect))
	    	  : (aPoint->y > NX_Y(aRect) && aPoint->y <= NX_MAXY(aRect))));
}


BOOL
NXIntersectsRect(const NXRect *aRect, const NXRect *bRect)
{
    return (!(EMPTY_RECT(aRect) ||
	      EMPTY_RECT(bRect) ||
	      (NX_X(aRect) >= NX_MAXX(bRect)) ||
	      (NX_MAXX(aRect) <= NX_X(bRect)) ||
	      (NX_Y(aRect) >= NX_MAXY(bRect)) ||
	      (NX_MAXY(aRect) <= NX_Y(bRect))));
}


BOOL
NXEmptyRect(const NXRect *aRect)
{
    return (EMPTY_RECT(aRect));
}


BOOL
NXEqualRect(const NXRect *aRect, const NXRect *bRect)
{
    return ((NX_X(aRect) == NX_X(bRect)) &&
	    (NX_Y(aRect) == NX_Y(bRect)) &&
	    (NX_WIDTH(aRect) == NX_WIDTH(bRect)) &&
	    (NX_HEIGHT(aRect) == NX_HEIGHT(bRect)));
}


/* Computation */

void
NXInsetRect(NXRect *aRect, NXCoord dX, NXCoord dY)
{
    NX_X(aRect) += dX;
    NX_Y(aRect) += dY;
    NX_WIDTH(aRect) -= dX;
    NX_WIDTH(aRect) -= dX;
    NX_HEIGHT(aRect) -= dY;
    NX_HEIGHT(aRect) -= dY;
}


void
NXOffsetRect(NXRect *aRect, NXCoord dX, NXCoord dY)
{
    NX_X(aRect) += dX;
    NX_Y(aRect) += dY;
}


NXRect             *
NXIntersectionRect(const NXRect *aRect, NXRect *bRect)
{
 /* bRect is replaced with the intersection of aRect and bRect */
    register NXCoord    tMaxX, tMaxY;

    if (!NXIntersectsRect(aRect, bRect)) {
	ZERO_RECT(bRect);
	return (NXRect *)0;
    }
    tMaxX = NX_MAXX(bRect);
    tMaxY = NX_MAXY(bRect);

    NX_X(bRect) = MAX(NX_X(aRect), NX_X(bRect));
    NX_Y(bRect) = MAX(NX_Y(aRect), NX_Y(bRect));

    NX_WIDTH(bRect) = MIN(NX_MAXX(aRect), tMaxX) - NX_X(bRect);
    NX_HEIGHT(bRect) = MIN(NX_MAXY(aRect), tMaxY) - NX_Y(bRect);

    return (bRect);
}


NXRect             *
NXUnionRect(const NXRect *aRect, NXRect *bRect)
{
 /* bRect is replaced with the union of aRect and bRect */
    register NXCoord    tMaxX, tMaxY;

    if (EMPTY_RECT(aRect)) {
	if (EMPTY_RECT(bRect))
	    bRect = (NXRect *)0;
    } else if (EMPTY_RECT(bRect)) {
	*bRect = *aRect;
    } else {
	tMaxX = NX_MAXX(bRect);
	tMaxY = NX_MAXY(bRect);
    
	NX_X(bRect) = MIN(NX_X(aRect), NX_X(bRect));
	NX_Y(bRect) = MIN(NX_Y(aRect), NX_Y(bRect));
    
	NX_WIDTH(bRect) = MAX(NX_MAXX(aRect), tMaxX) - NX_X(bRect);
	NX_HEIGHT(bRect) = MAX(NX_MAXY(aRect), tMaxY) - NX_Y(bRect);
    }
    return (bRect);
}

NXRect             *
NXDivideRect(NXRect *aRect, NXRect *bRect, NXCoord slice, int edge)
{
 /*
  * this function slices off a rectangle of 'slice' width on 'edge' side of
  * aRect.  This slice is returned by the function and put in bRect.  aRect
  * contains the remainder after the division. edge is  0, 1, 2, 3 for minX,
  * minY, maxX, maxY 
  */
    *bRect = *aRect;

    if (edge == 0 || edge == 2) {
	if (slice > NX_WIDTH(aRect))
	    slice = NX_WIDTH(aRect);
	NX_WIDTH(bRect) = slice;
	NX_WIDTH(aRect) -= NX_WIDTH(bRect);
	if (edge == 0)
	    NX_X(aRect) += NX_WIDTH(bRect);
	else
	    NX_X(bRect) += NX_WIDTH(aRect);
    } else {
	if (slice > NX_HEIGHT(aRect))
	    slice = NX_HEIGHT(aRect);
	NX_HEIGHT(bRect) = slice;
	NX_HEIGHT(aRect) -= NX_HEIGHT(bRect);
	if (edge == 1)
	    NX_Y(aRect) += NX_HEIGHT(bRect);
	else
	    NX_Y(bRect) += NX_HEIGHT(aRect);
    }
    return (bRect);
}


BOOL NXContainsRect(const NXRect *a, const NXRect *b)
{
    return (!(EMPTY_RECT(a) ||
	      EMPTY_RECT(b) ||
	      (NX_X(a) > NX_X(b)) ||
	      (NX_Y(a) > NX_Y(b)) ||
	      (NX_MAXX(a) < NX_MAXX(b)) ||
	      (NX_MAXY(a) < NX_MAXY(b))));
}



/* DO NOT DOCUMENT THIS FUNCTION.  IT IS BEING LEFT IN ONLY FOR COMPATIBILITY  - REMOVE AT VERSION 1.1 */
int
NXContainRect(NXRect *aRect, const NXRect *bRect)
{
    register NXCoord    delta;
    register NXCoord    deltaMax;
    register int        signX, signMaxX, signY, signMaxY;

    delta = NX_X(aRect) - NX_X(bRect);
    signX = (delta > 0.0);
    deltaMax = NX_MAXX(aRect) - NX_MAXX(bRect);
    signMaxX = (deltaMax > 0.0);
    if (signMaxX = (signX == signMaxX) && delta != 0.0 && deltaMax != 0.0)
	NX_X(aRect) -= (signX == (delta < deltaMax)) ? delta : deltaMax;

    delta = NX_Y(aRect) - NX_Y(bRect);
    signY = (delta > 0.0);
    deltaMax = NX_MAXY(aRect) - NX_MAXY(bRect);
    signMaxY = (deltaMax > 0.0);
    if (signMaxY = (signY == signMaxY) && delta != 0.0 && deltaMax != 0.0)
	NX_Y(aRect) -= (signY == (delta < deltaMax)) ? delta : deltaMax;

    return (signMaxX || signMaxY);
}

int
_NXCoverRect(NXRect *aRect, NXRect *bRect)
{
    if (NX_WIDTH(aRect) > NX_WIDTH(bRect))
	NX_WIDTH(bRect) = NX_WIDTH(aRect);
    if (NX_HEIGHT(aRect) > NX_HEIGHT(bRect))
	NX_HEIGHT(bRect) = NX_HEIGHT(aRect);

    return (NXContainRect(aRect, bRect));
}


void
NXIntegralRect(NXRect *aRect)
{
 /*
  * Forces aRect's edges to lie on the nearest integral values such that the
  * new aRect completely encloses the old aRect 
  */
    if (EMPTY_RECT(aRect)) {
	ZERO_RECT(aRect);
	return;
    }
    NX_WIDTH(aRect) += NX_X(aRect);
    NX_HEIGHT(aRect) += NX_Y(aRect);
    NX_X(aRect) = floor(NX_X(aRect));
    NX_Y(aRect) = floor(NX_Y(aRect));
    NX_WIDTH(aRect) = ceil(NX_WIDTH(aRect));
    NX_HEIGHT(aRect) = ceil(NX_HEIGHT(aRect));
    NX_WIDTH(aRect) -= NX_X(aRect);
    NX_HEIGHT(aRect) -= NX_Y(aRect);
}


void
NXSetRect(NXRect *aRect, NXCoord x, NXCoord y, NXCoord w, NXCoord h)
{
 /*
  * Sets the aRect origin and size. 
  */
    aRect->origin.x = x;
    aRect->origin.y = y;
    aRect->size.width = w;
    aRect->size.height = h;
}

/*
  
Modifications (starting at 0.8):
  
03/21/89 wrp	Made declarations const where needed.

06/16/89 wrp	Added code to check for empty rectangles
06/25/89 wrp	Added NXMouseInRect() to account for flipped coordinates
06/28/89 wrp	Fixed NXUnionRect to only return NULL if both rects are empty
06/28/89 wrp	Fixed NXIntersectsRect to check first for empty rectangles
06/28/89 wrp	Added NXContainsRect

*/


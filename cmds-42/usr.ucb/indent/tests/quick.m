/*
	Copyright 1987, NeXT, Inc.
	
	This file holds the simulation of a few QuickDraw routines
*/


#include <stdio.h>
#include "objc.h"
#include "nextstd.h"
#include "appkit.h"
//#include "macglobals.h"
#include "Windows.h"

extern GrafPtr  thePort;
extern void     _msSetupCurrentFont();
extern id       _msCurrentFontObject();
void            msPenPat();
Boolean         msSectRect();

/* macros to access pattern data as longs for speed */
#define PAT_LONG1(p)	(*(long *)(p).s)
#define PAT_LONG2(p)	(*(long *)((p).s+4))
#define PAT_EQUAL(p1,p2) (PAT_LONG1(p1) == PAT_LONG1(p2) &&\
			  PAT_LONG2(p1) == PAT_LONG2(p2) )

= (AppKit, Primitive)
    void
                    msClipRect(rectPtr)
    register Rect  *rectPtr;
{
    register int   *newData, *oldData;

    newData = (int *) rectPtr;
    oldData = (int *) &((*thePort->clipRgn)->rgnBBox);
    if ((*newData++ != *oldData++) || (*newData != *oldData)) {
	msRectRgn(thePort->clipRgn, rectPtr);
	msPScliprect((float) rectPtr->left, (float) rectPtr->top,
	       (float) rectPtr->right, (float) rectPtr->botRight.v);
    }
}


void
msDrawString(macString)
    StringPtr       macString;
{

    _msSetupCurrentFont();
    setupPattern(&black);
    setupAlpha(1.0);
    if (thePort->spExtra)
	msPSwidthdrawstring((float) thePort->pnLoc.h, (float) thePort->pnLoc.v,
	(float) thePort->spExtra / 65336, macString->s, macString->count);
    else
	msPSdrawstring((float) thePort->pnLoc.h, (float) thePort->pnLoc.v,
		       macString->s, macString->count);
}


Boolean
msEmptyRect(theRect)
    Rect           *theRect;
{
    return (theRect->left >= theRect->right ||
	    theRect->top >= theRect->botRight.v);
}


void
msEraseRect(r)
    Rect           *r;
{
    setupPattern(&white);
    setupAlpha(1.0);
    PScompositerect((float) r->left, (float) r->top, (float) (r->right - r->left),
		    (float) (r->bottom - r->top), NW_COPY);
}


void
msFillRect(r, p)
    Rect           *r;
    Pattern        *p;
{
    setupPattern(p);
    setupAlpha(1.0);
    msPSfillrect((float) r->left, (float) r->top,
		 (float) r->right, (float) r->botRight.v);
}


void
msFrameRect(r)
    Rect           *r;
{
    setupPattern(&thePort->pnPat);
    setupAlpha(1.0);
    msPSframerect((float) r->left, (float) r->top,
		  (float) r->right, (float) r->botRight.v);
}


void
msFrameRoundRect(r, ovalWidth, ovalHeight)
    Rect           *r;
    short           ovalWidth, ovalHeight;
{
    setupPattern(&thePort->pnPat);
    setupAlpha(1.0);
    msPSframeroundrect((float) r->left, (float) r->top,
		       (float) r->right, (float) r->botRight.v,
		       (float) ((ovalWidth + ovalHeight) / 2));
}


void
msGetFontInfo(info)
    FontInfo       *info;
{
    register id     currFontObj;
    register float  temp;
    register int    size;
    register NWFontMetrics *fontInfo;

    currFontObj = _msCurrentFontObject();
    fontInfo =[currFontObj getFontMetrics];
    size = thePort->txSize;
    info->ascent = size * fontInfo->fontBBox[3];
    info->descent = -size * fontInfo->fontBBox[1];
    info->widMax = size * (fontInfo->fontBBox[2] - fontInfo->fontBBox[0]);
    info->leading = 0;		/* ??? What do we do about leading? */
}


void
msGetClip(rgn)
    RgnHandle       rgn;
{
    msCopyRgn(thePort->clipRgn, rgn);
}


void
msGetPenState(ps)
    PenState       *ps;
{
    ps->pnLoc = thePort->pnLoc;
    ps->pnSize = thePort->pnSize;
    ps->pnMode = thePort->pnMode;
    ps->pnPat = thePort->pnPat;
}


void
msGlobalToLocal(pt)
    Point          *pt;
{
    NWPoint         tempPoint;

    tempPoint.x = (float) pt->h;
    tempPoint.y = (float) pt->v;
[WINDOW_DATA(thePort)->portObj baseToLocal:&tempPoint];
    pt->h = (short) tempPoint.x;
    pt->v = (short) tempPoint.y;
}


void
msInitGraf(globals)
    Ptr             globals;
{
}


void
msInsetRect(r, dh, dv)
    register Rect  *r;
    short         dh, dv;
{
    r->top += dv;
    r->left += dh;
    r->bottom -= dv;
    r->right -= dh;
}


void
msInvertRect(r)
    Rect           *r;
{
    int             winNum;

    winNum = WINDOW_DATA(thePort)->serverID;
    PSinitwindowalpha(NW_A_DATA, winNum);
    setupPattern(&black);
    setupAlpha(1.0);
    PScompositerect((float) r->left, (float) r->top, (float) (r->right - r->left),
		    (float) (r->bottom - r->top), NW_SOUTD);
    PSinitwindowalpha(NW_A_ONE, winNum);
}


void
msLine(dh, dv)
    short         dh, dv;
{
    setupPattern(&thePort->pnPat);
    setupAlpha(1.0);
    PSmoveto((float) thePort->pnLoc.h, (float) thePort->pnLoc.v);
    thePort->pnLoc.h += dh;
    thePort->pnLoc.v += dv;
    PSrlineto((float) dh, (float) dv);
    PSstroke();
}


void
msLocalToGlobal(pt)
    Point          *pt;

/* Instead of converting to global coords, this routine converts to the
   windows base coordinate system.
   */
{
    NWPoint         tempPoint;

    tempPoint.x = (float) pt->h;
    tempPoint.y = (float) pt->v;
[WINDOW_DATA(thePort)->portObj localToBase:&tempPoint];
    pt->h = (short) tempPoint.x;
    pt->v = (short) tempPoint.y;
}


void
msMove(h, v)
    short         h, v;
{
    thePort->pnLoc.h += h;
    thePort->pnLoc.v += v;
}


void
msMoveTo(h, v)
    short         h, v;
{
    thePort->pnLoc.h = h;
    thePort->pnLoc.v = v;
}


void
msOffsetRect(r, dh, dv)
    register Rect  *r;
    short         dh, dv;
{
    r->left += dh;
    r->top += dv;
    r->right += dh;
    r->botRight.v += dv;
}


void
msPenMode(mode)
    short         mode;
{
    thePort->pnMode = mode;
}

void
msPenNormal()
{
    thePort->pnSize.h = 1;
    thePort->pnSize.v = 1;
    thePort->pnMode = patCopy;
    thePort->pnPat = black;
}


void
msPenPat(newpat)
    Pattern        *newpat;
{
    thePort->pnPat = *newpat;
}


void
msPenSize(h, v)
    short         h, v;
{
    thePort->pnSize.h = h;
    thePort->pnSize.v = v;
}


Boolean
msPtInRect(p, r)
    Point           p;
    Rect           *r;
{
    return (!(p.h < r->left || p.h >= r->right ||
	      p.v < r->top || p.v >= r->botRight.v));
}


void
msScrollRect(r, dh, dv, rgn)
    register Rect  *r;
    register short dh, dv;
    RgnHandle       rgn;

/* blts a piece of the current window, and sets the passed region
   to the newly exposed stuff.  Right now, it only blts bits that it
   needs to to prevent copying bits outside our clip region,
   or outside the passed rectangle.
   This means to be blted, the bits must be in the clipRgn
   after the blt, and be within the source rectangle before and
   after the blt.
   If && is intersection, r is the source rect, and f() is the translation
   by (dh,dv), and g() is the inverse of f(), then the bits to blt are
   g(clipRgn && f(r) && r) = g(clipRgn && r) && r.

*/
{
    RgnHandle       tempRgn;
    extern RgnHandle msNewRgn();
    Rect            udRectStorage;	/* part of area needing update */
    register Rect  *udRect = &udRectStorage;
    Rect            scrollRectStorage;	/* part of to blt */
    register Rect  *sRect = &scrollRectStorage;

    msSetEmptyRgn(rgn);
    if (dv || dh) {
	*sRect = (*thePort->clipRgn)->rgnBBox;
	msSectRect(sRect, r, sRect);
	msOffsetRect(sRect, -dh, -dv);
	msSectRect(sRect, r, sRect);
	WindowBlt(sRect->left, sRect->top,
		  sRect->right - sRect->left,
		  sRect->botRight.v - sRect->top,
		  WINDOW_DATA(thePort)->serverID,
		  sRect->left + dh, sRect->top + dv);
	*udRect = *r;
	if (dv) {		/* if vertical or 2-way scroll */
	    if (dv > 0)
		udRect->botRight.v = udRect->top + dv;
	    else
		udRect->top = udRect->botRight.v + dv;
	    msRectRgn(rgn, udRect);
	} else if (dh && !dv) {	/* else if only horizontal scroll */
	    if (dh > 0)
		udRect->right = udRect->left + dh;
	    else
		udRect->left = udRect->right + dh;
	    msRectRgn(rgn, udRect);
	}
	if (dv && dh) {		/* 2-way scroll */
	    tempRgn = msNewRgn();
	    *udRect = *r;
	    if (dh > 0)
		udRect->right = udRect->left + dh;
	    else
		udRect->left = udRect->right + dh;
	    msRectRgn(rgn, udRect);
	    msUnionRgn(rgn, tempRgn, rgn);
	}
    }
}


Boolean
msSectRect(src1, src2, dest)
    register Rect  *src1, *src2, *dest;
{
    if ((dest->left = MAX(src1->left, src2->left)) >=
	(dest->right = MIN(src1->right, src2->right)) ||
	(dest->top = MAX(src1->top, src2->top)) >=
	(dest->botRight.v = MIN(src1->botRight.v, src2->botRight.v))) {
    /* we got an empty rect */
	dest->left = dest->top =
	      dest->right = dest->botRight.v = 0;
	return FALSE;
    }
    return TRUE;

}


void
msSetClip(rgn)
    RgnHandle       rgn;
{
    msClipRect(&(*rgn)->rgnBBox);
}


void
msSetOrigin(h, v)
    short           h, v;

/* sets the grafport's origin.  The given coordinate is made to
   be the coordinate of the upper-left corner of the screen.
   The Update region is kept in local coordinates instead of
   global coordinates as in the Macintosh.  This means that
   msSetOrigin must offset the updateRgn.  Unfortunately this
   means you must never use SetOrigin on anything but windows
   (i.e no solo GrafPorts).  NeXT decided not to have global
   coordinates.
   */
{
    short           dh, dv;

    dh = h - thePort->portRect.left;
    dv = v - thePort->portRect.top;
    [WINDOW_DATA(thePort)->portObj lockFocus];
    PStranslate((float) -dh, (float) -dv);
[WINDOW_DATA(thePort)->portObj translate: (float) -dh:(float) -dv];
    [WINDOW_DATA(thePort)->portObj unlockFocus];
    msOffsetRect(&(thePort->portRect), dh, dv);
    msOffsetRgn(thePort->visRgn, dh, dv);
    msOffsetRgn(((WindowPeek) thePort)->updateRgn, dh, dv);
}


void
msSetPenState(ps)
    PenState       *ps;
{
    thePort->pnLoc = ps->pnLoc;
    thePort->pnSize = ps->pnSize;
    thePort->pnMode = ps->pnMode;
    thePort->pnPat = ps->pnPat;
}


void
msSetPort(newPort)
    GrafPtr         newPort;
{
//    if (thePort = newPort)
//	[AKApp focusWindow:WINDOW_DATA(thePort)->windowObj];
}


void
msSpaceExtra(amount)
    Fixed           amount;
{
    thePort->spExtra = amount;
}


short
msStringWidth(str)
    Str255         *str;
{
    register id     currFontObj;
    register float  total = 0.0;
    register float *widths;
    register char  *theStr;
    register int    cnt;
    register NWFontMetrics *fontInfo;

    currFontObj = _msCurrentFontObject();
    fontInfo =[currFontObj getFontMetrics];
    widths = fontInfo->widths;
    total = 0.0;
    cnt = str->count;
    theStr = str->s;
    while (cnt--)
	total += widths[*theStr++];
    return (total * thePort->txSize);
}


void
msUnionRect(rectA, rectB, dest)
    Rect           *rectA, *rectB, *dest;
{
    Rect            result;
    register int    nonEmptyA, nonEmptyB;

    nonEmptyA = !msEmptyRect(rectA);
    nonEmptyB = !msEmptyRect(rectB);
    if (nonEmptyA && nonEmptyB) {
	result.left = MIN(rectA->left, rectB->left);
	result.right = MAX(rectA->right, rectB->right);
	result.top = MIN(rectA->top, rectB->top);
	result.botRight.v = MAX(rectA->botRight.v, rectB->botRight.v);
    } else if (nonEmptyA)
	result = *rectA;
    else if (nonEmptyB)
	result = *rectB;
    else {			/* both of them are empty */
	result.top = result.left = 0;
	result.botRight = result.topLeft;
    }
    *dest = result;
}


static float
convertPattern(pat)
    Pattern        *pat;
{
    if (PAT_EQUAL(*pat, black))
	return 0.0;
    if (PAT_EQUAL(*pat, dkGray))
	return 0.25;
    if (PAT_EQUAL(*pat, gray))
	return 0.5;
    if (PAT_EQUAL(*pat, ltGray))
	return 0.75;
    if (PAT_EQUAL(*pat, white))
	return 1.0;
#ifdef DEBUG
    fprintf(stderr, "no pattern matched\n");
#endif
    return 0.5;
}


static
setupPattern(newPat)
    Pattern        *newPat;

/* setupPattern keeps the given pattern in sync with the current
   gray in the server. Call it before anything that depends on the
   the drawing pattern.
   */
{
    if (!PAT_EQUAL(WINDOW_DATA(thePort)->lastPattern, *newPat)) {
	PSsetgray(convertPattern(newPat));
	WINDOW_DATA(thePort)->lastPattern = *newPat;
    }
}


static
setupAlpha(newVal)
    float           newVal;

/* setupAlpha ensures that the given alpha value is the same as the
   one in the server. Call it before anything that depends on the
   the current alpha value.
   */
{
    if (newVal != WINDOW_DATA(thePort)->lastAlpha) {
	PSsetalpha(newVal);
	WINDOW_DATA(thePort)->lastAlpha = newVal;
    }
}


static
setPattern(p, data1, data2)
    Pattern        *p;
    int             data1, data2;
{
    int            *stuffer;

    stuffer = (int *) p->s;
    stuffer = (int *) &p->s[0];
    *stuffer = data1;
    stuffer = (int *) (p->s + 4);
    *stuffer = data2;
}

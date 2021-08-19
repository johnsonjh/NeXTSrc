
/*
	textprivate.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Bryan Yamamoto
*/

#ifndef TEXT_H
#import "Text.h"
#endif TEXT_H

#import <appkit/Window.h>
#import <appkit/Font.h>
#import "super.h"
#import "textGlobalDefines.h"
#import "textps.h"

 /*
  * various font extraction macros, make them as fast or as
  * object-oriented as you want 
  */

typedef struct {
	@defs (Font)
} _IDFont ;

#define FONTTOMETRICS(fid) (((_IDFont *) fid)->faceInfo->fontMetrics)
#define FONTTOWIDTHS(fid) (((_IDFont *) fid)->faceInfo->fontMetrics->widths)
#define FONTTOSIZE(fid) (((_IDFont *) fid)->size)
#define FONTTOSTYLE(fid) (((_IDFont *) fid)->style)
#define FONTTONAME(fid) (((_IDFont *) fid)->name)
#define FONTTOMATRIX(fid) (((_IDFont *) fid)->matrix)
#define FONTTOSCREEN(fid) (((_IDFont *) fid)->otherFont)
#define ISSCREENFONT(fid) (((_IDFont *) fid)->fFlags.isScreenFont)

typedef struct {
	@defs (Window)
} WindowClass;

#define VALID_WINDOW(w) (w && (((WindowClass *) (w))->windowNum > 0))
#define FLUSHWINDOW(w) (VALID_WINDOW(w) && ((WindowClass *) w)->_flushDisabled <= 0 )
#define MUST_DRAW(v) ((!(v)->vFlags.disableAutodisplay) && VALID_WINDOW((v)->window))

#define TXYESCHANGE(LD) (LD | (int)0x8000)
#define TXNOCHANGE(LD) (LD & (int)0x7FFF)
#define TXISCHANGE(LD) (LD < 0)
#define TXMCHANGE 0x8000

#define TXYESPARA(LD) (LD | (int)0x4000)
#define TXNOPARA(LD) (LD & (int)0xBFFF)
#define TXISPARA(LD) (LD & (int)0x4000)
#define TXMPARA ((int)0x4000)

#define TXGETCHARS(LD) (LD & (int)0x3FFF)
#define TXSETCHARS(LD, NC) ((LD & (int)0xC000) | NC)
#define TXMCHARS (int)0x3FFF
#define TX_LINEAT(BASE, OFFSET) ((NXLineDesc *) ((char *) (BASE) + (OFFSET)))
#define TX_HTCHANGE(BASE, OFFSET) ((NXHeightChange *)((char *)(BASE) + (OFFSET)))
#define TX_HTINFO(BASE, OFFSET) ((NXHeightInfo *)((char *)(BASE) + (OFFSET)))
#define TX_LAYAT(BASE, OFFSET) ((NXLay *) ((char *)(BASE) + (OFFSET)))
#define TX_RUNAT(BASE, OFFSET) ((NXRun *) ((char *)(BASE) + (OFFSET)))
#define TX_NEXTSTATE(BASE, OFFSET) ((NXFSM *) ((char *)(BASE) + (OFFSET)))
#define TX_BYTES(PTR1, PTR2) ((char *)(PTR1) - (char *)(PTR2))
#define HTCHANGE(ps) ((NXHeightChange *)(ps))
/* targetMethods supported */
#define TXWILLGROW 		0
#define TXGROWACTION 		1
#define TXWILLCHANGE 		2
#define TXCHANGEACTION 		3
#define TXWILLEND 		4
#define TXENDACTION 		5
#define TXISEMPTY 		6

#define TXMWILLGROW		(1 << TXWILLGROW)
#define TXMGROWACTION		(1 << TXGROWACTION)
#define TXMWILLCHANGE		(1 << TXWILLCHANGE)
#define TXMCHANGEACTION		(1 << TXCHANGEACTION)
#define TXMWILLEND		(1 << TXWILLEND)
#define TXMENDACTION		(1 << TXENDACTION)
#define TXMISEMPTY		(1 << TXISEMPTY)


/* caret blinking states */
#define CARETDISABLED 0
#define CARETON 1
#define CARETOFF 2

/* left and right fudge factor */

#define HFUDGE 2.0

#define NX_MAXCPOS ((int)0x7FFFFFFF)
#define NX_YHYSTERESIS 3.0
#define NX_XHYSTERESIS 1.0

extern BOOL NXScreenDump;	/* Do we draw selection while printing? */

 /*
  * pingrate defines the number of lines per ping 
  */
#define PINGRATE 1

 /**********************************************************/

 /*
  * these are the default list of smart paste punctuation marks for left
  * and right of selection.  if we don't see one of these, we insert a
  * space. 
  */

extern const unsigned char NXSmartLeft[];
extern const unsigned char NXSmartRight[];
 /*
  * these are the default word definition tables. 
  *
  * the first table maps various characters to character classes the second
  * table is a finite state machine table for work breaks the third table
  * is a finite state machine table for double click words 
  *
  */

#define _CLNULL 0
#define _CLWHITE  1*sizeof(NXFSM)
#define _CLNL   2*sizeof(NXFSM)
#define _CLOP   3*sizeof(NXFSM)
#define _CLEQ   4*sizeof(NXFSM)
#define _CLDEF  5*sizeof(NXFSM)
#define _CLPUNCT 6*sizeof(NXFSM)

extern const NXFSM NXBreak[35];
extern const NXFSM NXNoBreak[14];
extern const NXFSM NXClick[42];			
extern const unsigned char NXCClasses[256];

#define DIACRITICAL(c) (self->tFlags.overstrikeDiacriticals && \
			(((c) >= 0301 && (c) <= 0317) || (c)=='\b'))
#define MOVECHAR(c) (c == '\t' || c == NX_FIGSPACE || DIACRITICAL(c))

#define GROWARRAYBY 100
#define INITARRAY 100
#define GROWLINESBY 10
#define SINotTracking 0
#define SIMove 1
#define VISBIGFUDGE 30.0
#define VISLITTLEFUDGE 5.0
#define READONLY 1
#define READWRITE 2
#define SELECTABLE 1
#define NOTSELECTABLE 2
#define GROWABLE 1
#define NOTGROWABLE 2
#define UPARROW 173
#define DOWNARROW 175
#define LEFTARROW 172
#define RIGHTARROW 174
#define SYMBOLFONT 1
#define NPBTYPES	2

#define ISNULLSEL ((sp0.cp >= 0) && (sp0.cp == spN.cp) && (sp0.line == spN.line) && (sp0.x == spN.x))

#define PROLOG(info) if (!(((NXTextInfo *)(info))->globals.refCount++)) \
    initTextInfo(info)
#define EPILOG(info) if (!(--(((NXTextInfo *)(info))->globals.refCount))) \
    finishTextInfo(info)
 
typedef struct {
    int		    runs;
    int             chars;
    int             words;
} NXRunPboardHeader;

typedef struct {
    int             chars;
    NXCoord         size;
    short	    fontStyle;
    short           namelen;
}               NXRunPboardFixed;

typedef struct {
    NXLayArray *lays;
    NXWidthArray *widths;
    NXCharArray *chars;
    NXBreakArray *savedBreaks;
    NXWidthArray *xPos;
} NXGlobalArrays;

typedef struct {
    NXRect hitRect;
    NXCoord lastCharWidth;
    NXBreakArray *savedBreaks;
    NXWidthArray *xPos;
    int tabLay;
    NXTabStop *tabs;
    NXTabStop *tabsEnd;
    int tabsPending;
    int refCount;
    NXGlobalArrays *arrays;
    unsigned short didEndChar;
    struct {
	unsigned int clipLeft:1;
	unsigned int clipRight:1;
	unsigned int vertOrHoriz:1;
	unsigned int plusMinus:1;
    } gFlags;
} NXTextGlobals;

typedef struct {
    NXLayInfo layInfo;
    NXTextGlobals globals;
} NXTextInfo;

/*
	PageLayout.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "Panel_Private.h"
#import "PageLayout.h"
#import "View.h"
#import "Bitmap.h"
#import "Button.h"
#import "ButtonCell.h"
#import "Form.h"
#import "FormCell.h"
#import "PopUpList.h"
#import "Application.h"
#import "PaperView.h"
#import "PrintInfo.h"
#import "Text.h"
#import "TextField.h"
#import "Box.h"
#import "publicWraps.h"
#import "errors.h"
#import "nextstd.h"
#import <dpsclient/dpsclient.h>

/* these methods will go away when LoadNibFile can set outlets by name */
@interface PageLayoutExtraMethods:Object
    - setAppIcon:obj;
    - setHeight:obj;
    - setWidth:obj;
    - setOk:obj;
    - setCancel:obj;
    - setOrientation:obj;
    - setScale:obj;
    - setPaperSizeList:obj;
    - setLayoutList:obj;
    - setUnitsList:obj;
@end

void _NXMakePopUpList(id button, const char *const * strings, const char *tableName);
static void initPaperTypes(id self);
static void fixPaperSize(id self, NXSize *inSize);
static void fillSizeFields(id self, const NXSize *data, int units);
static void updatePaperView(id self);

/* INDENT OFF */
/* strings for the Layout list */
static const char * const LayoutStrings[] = {"1 Up", "2 Up", "4 Up", "8 Up", "16 Up", NULL};

/* strings for the Units list */
static const char * const UnitsStrings[] = {"Inches", "Centimeters", "Points", "Picas", NULL};

/* label for custom paper sizes */
static const char *otherstring = NULL;

/* localization macros */

#define OtherString (otherstring ? otherstring : (otherstring = \
    KitString(Printing, "Other", "Name of an unknown paper size.")))

/*
    It would be better not to have to keep this in sync with the
    NXPaperList[], LayoutStrings[], and UnitsStrings[] arrays, but,
    for documentation purposes, we must enumerate the types here.

    KitString(Printing, "Letter", "A paper size: 612 x 792 (points).");
    KitString(Printing, "Tabloid", "A paper size: 792 x 1224 (points).");
    KitString(Printing, "Ledger", "A paper size: 1224 x 792 (points).");
    KitString(Printing, "Legal", "A paper size: 612 x 1008 (points).");
    KitString(Printing, "Executive", "A paper size: 540 x 720 (points).");
    KitString(Printing, "A3", "A paper size: 842 x 1190 (points).");
    KitString(Printing, "A4", "A paper size: 595 x 842 (points).");
    KitString(Printing, "A5", "A paper size: 420 x 595 (points).");
    KitString(Printing, "B4", "A paper size: 729 x 1032 (points).");
    KitString(Printing, "B5", "A paper size: 516 x 729 (points).");

    KitString(Units, "Inches", "A unit of measurement.")
    KitString(Units, "Centimeters", "A unit of measurement.")
    KitString(Units, "Points", "A unit of measurement.")
    KitString(Units, "Picas", "A unit of measurement.")
    
    KitString(Printing, "1 Up", "A way to lay out many pages on one page.")
    KitString(Printing, "2 Up", "A way to lay out many pages on one page.")
    KitString(Printing, "4 Up", "A way to lay out many pages on one page.")
    KitString(Printing, "8 Up", "A way to lay out many pages on one page.")
    KitString(Printing, "16 Up", "A way to lay out many pages on one page.")
*/

/* table of conversion factors.  This is a parallel array to UnitsStrings. */
static const float ConversionFactors[] = {
				1.0 / 72.0,		/* points to inches */
				1.0 / 72.0 * 2.54,	/* points to cm */
				1.0,			/* points to points */
				1.0 / 12.0		/* points to picas */
};
/* INDENT OFF */

/* the factory object will only create one instance of this panel */
static PageLayout *UniqueInstance = NULL;

/* factory used to create the instance */
static PageLayout *UniqueFactory = NULL;


@implementation PageLayout:Panel

+ (BOOL)_canAlloc { return UniqueFactory != nil; }

/* If someone just calls this out of the blue, its an error.  They must use a new method.  Else we're being called as part of nib loading, and we do a special hack to make sure an object of the right type is created in spite of the class stored in 
the nib file. */
+ allocFromZone:(NXZone *)zone
{
    if (UniqueFactory)
	return _NXCallPanelSuperFromZone(UniqueFactory, _cmd, zone);
    else
	return [self doesNotRecognize:_cmd];
}

/* If someone just calls this out of the blue, its an error.  They must use a new method.  We depend on nibInstantiate using allocFromZone:. */
+ alloc
{
    return [self doesNotRecognize:_cmd];
}


+ new
{
    return[self newContent:(NXRect *)0 style:0 backing:0
	   buttonMask:0 defer:NO];
}


+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag
{
    if (!UniqueInstance) {
	if (UniqueFactory)
	    return _NXCallPanelSuper(UniqueFactory,
		      @selector(newContent:style:backing:buttonMask:defer:),
			    contentRect, aStyle, bufferingType, mask, flag);
	else {
	    UniqueFactory = self;
	    UniqueInstance = _NXLoadNibPanel("PageLayout");
	    UniqueFactory = nil;
	    if (UniqueInstance) {
		initPaperTypes(UniqueInstance);
		_NXMakePopUpList(UniqueInstance->layoutList, LayoutStrings, "Printing");
		_NXMakePopUpList(UniqueInstance->unitsList, UnitsStrings, "Units");
		UniqueInstance->_currUnits = 0;	/* ??? should read a default */
		[UniqueInstance->ok setTag:NX_OKTAG];
		[UniqueInstance->cancel setTag:NX_CANCELTAG];
		[[UniqueInstance->height cell]
				setEntryType:NX_POSDOUBLETYPE];
		[[UniqueInstance->width cell]
				setEntryType:NX_POSDOUBLETYPE];
		[[UniqueInstance->scale cell] setEntryType:NX_POSDOUBLETYPE];
		{		/* ???this can go when nib lets me assign
				 * this name */
		    [UniqueInstance->appIcon setIcon:"app"];
		}
		[UniqueInstance setOneShot:NX_KITPANELSONESHOT];
		[UniqueInstance useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
	    }
	}
    }
    return UniqueInstance;
}


- free
{
    UniqueInstance = NULL;
    [appIcon free];
    [height free];
    [width free];
    [ok free];
    [cancel free];
    [orientation free];
    [scale free];
    [[paperSizeList target] free];	/* free buttons and their popUpLists */
    [paperSizeList free];
    [[layoutList target] free];
    [layoutList free];
    [[unitsList target] free];
    [unitsList free];
    [paperView free];
    return[super free];
}


- setAccessoryView:aView
{
    return [self _doSetAccessoryView:aView topView:orientation bottomView:ok
						oldView:&accessoryView];
}


- accessoryView
{
    return accessoryView;
}


- (BOOL)_entriesAreAcceptable
{
    id cell;

    if (![self makeFirstResponder:self]) {
	return NO;
    } else {
	cell = [width cell];
	if (![cell isEntryAcceptable:[cell stringValue]]) {
	    NXBeep();
	    [width selectText:self];
	    return NO;
	} else {
	    cell = [height cell];
	    if (![cell isEntryAcceptable:[cell stringValue]]) {
		NXBeep();
		[height selectText:self];
		return NO;
	    } else {
		cell = [scale cell];
		if (![cell isEntryAcceptable:[cell stringValue]]) {
		    NXBeep();
		    [scale selectText:self];
		    return NO;
		}
	    }
	}
    }

    return YES;
}


- pickedButton:sender
{
    int tag;

    tag = [sender tag];
    if (tag == NX_CANCELTAG || [self _entriesAreAcceptable]) {
	exitTag = tag;
	[NXApp stopModal];		/* exit modal run */
    }

    return self;
}


- pickedPaperSize:sender
{
    NXSize              pSize;
    const char         *paperName;

    paperName = [[sender selectedCell] title];
    if (strcmp(paperName, OtherString)) {
	pSize = *NXFindPaperSize(paperName);
	fillSizeFields(self, &pSize, _currUnits);
	[orientation selectCellAt:0 :(pSize.width <= pSize.height) ? 0 : 1];
	_otherPaper = NO;
    } else {
	if (!_otherPaper) {
	    [width setStringValue:NULL];
	    [height setStringValue:NULL];
	}
	[width selectTextAt:0];
	_otherPaper = YES;
    }
    updatePaperView(self);
    return self;
}


- pickedOrientation:sender
{
    NXSize              currSize;

    currSize.width = [width floatValueAt:0];
    currSize.height = [height floatValueAt:0];
    fixPaperSize(self, &currSize);
    fillSizeFields(self, &currSize, -1);
    updatePaperView(self);
    return self;
}


- pickedLayout:sender
{
    return self;		/* ??? update the display somehow? */
}


- pickedUnits:sender
{
    NXSize              paperSize;	/* paper size in points, including
					 * orientation */
    float               old, new;
    const char         *paperName;

    paperName = [paperSizeList title];
    if (strcmp(paperName, OtherString)) {	/* if known paper size */
	paperSize = *NXFindPaperSize(paperName);
	fixPaperSize(self, &paperSize);
    } else {
	[self convertOldFactor:&old newFactor:&new];
	paperSize.width = [width floatValue] / old;
	paperSize.height = [height floatValue] / old;
    }
    _currUnits = [sender selectedRow];
    fillSizeFields(self, &paperSize, _currUnits);
    return self;
}


- (BOOL)textWillChange:textObject
{
    [paperSizeList setTitle:OtherString];
    _otherPaper = YES;
    return 0;
}


- textDidEnd:textObject endChar:(unsigned short)theChar
{
    NXSize              pSize;
    const char	       *pName;

    pSize.width = [width floatValueAt:0] / ConversionFactors[_currUnits];
    pSize.height = [height floatValueAt:0] / ConversionFactors[_currUnits];
    [orientation selectCellAt:0 :(pSize.width <= pSize.height) ? 0 : 1];
    updatePaperView(self);
    pName = _NXFindPaperName(&pSize);
    if (pName) {
	[paperSizeList setTitle:pName];
	_otherPaper = NO;
    }
    return self;
}


- readPrintInfo
{
    id                  pInfo = [NXApp printInfo];
    const char         *paperName;
    const NXRect       *paperRect;
    int                 layoutIndex;
    int                 pagesPerSheet;

    paperName = [pInfo paperType];
    if (*paperName) {
	[paperSizeList setTitle:paperName];
	_otherPaper = NO;
    } else {
	[paperSizeList setTitle:OtherString];
	_otherPaper = YES;
    }
    paperRect = [pInfo paperRect];
    fillSizeFields(self, &paperRect->size, _currUnits);
    [scale setFloatValue:[pInfo scalingFactor] * 100];
    [orientation selectCellAt:0 :([pInfo orientation] == NX_PORTRAIT) ? 0 : 1];
    pagesPerSheet = [pInfo pagesPerSheet];
    layoutIndex = 0;
    while ((pagesPerSheet >>= 1) > 0)
	if (++layoutIndex == sizeof(LayoutStrings) / sizeof(char *) - 1)
	    break;
    [layoutList setTitle:LayoutStrings[layoutIndex]];
    updatePaperView(self);
    return self;
}


- writePrintInfo
{
    id                  pInfo = [NXApp printInfo];
    NXRect              paperRect;
    const char         *paperName;

/* ??? international units */

    paperName = [paperSizeList title];
    if (strcmp(paperName, OtherString)) {
	[pInfo setPaperType:paperName andAdjust:NO];
	paperRect.size = *NXFindPaperSize(paperName);
	fixPaperSize(self, &paperRect.size);
    } else {
	[pInfo setPaperType:(char *)0 andAdjust:NO];
	paperRect.size.width = [width floatValueAt:0] /
	  ConversionFactors[_currUnits];
	paperRect.size.height = [height floatValueAt:0] /
	  ConversionFactors[_currUnits];
    }
    paperRect.origin.x = paperRect.origin.y = 0.0;
    [pInfo setPaperRect:&paperRect andAdjust:NO];
    [pInfo setOrientation:([orientation selectedCol] == 0 ?
			   NX_PORTRAIT : NX_LANDSCAPE) andAdjust :NO];
    [pInfo setScalingFactor:[scale floatValue] / 100];
    [pInfo setPagesPerSheet:1 <<
     [[layoutList target] indexOfItem:[layoutList title]]];
    return self;
}


- (int)runModal
{
    NXHandler           exception;

    exception.code = 0;
    [self readPrintInfo];
    [scale selectText:self];
    NXPing();
    NX_DURING
      [NXApp runModalFor:self];
    NX_HANDLER
      exception = NXLocalHandler;
    if (exception.code == dps_err_ps)
	NXReportError(&NXLocalHandler);
    [NXApp stopModal];
    exitTag = NX_CANCELTAG;
    NX_ENDHANDLER
      if (exitTag == NX_OKTAG)
	[self writePrintInfo];
    [self orderOut:self];
    if (exception.code && exception.code != dps_err_ps)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return exitTag;
}


- convertOldFactor:(float *)old newFactor:(float *)new
{
    int                 row;

    *old = ConversionFactors[_currUnits];
    row = [[[unitsList target] itemList] selectedRow];
    *new = (row >= 0) ? ConversionFactors[row] : *old;
    return self;
}


/* INDENT OFF */
static const struct _NXPaper {	/* ??? temp */
    char               *name;
    NXSize              size;
} NXPaperList[] = {	{"Letter", {612.0, 792.0}},
			{"Tabloid", {792.0, 1224.0}},
			{"Ledger", {1224.0, 792.0}},
			{"Legal", {612.0, 1008.0}},
			{"Executive", {540.0, 720.0}},
			{"A3", {842.0, 1190.0}},
			{"A4", {595.0, 842.0}},
			{"A5", {420.0, 595.0}},
			{"B4", {729.0, 1032.0}},
			{"B5", {516.0, 729.0}}
};
/* INDENT ON */

#define NUM_PAPERS (sizeof(NXPaperList)/sizeof(struct _NXPaper))

static void
initPaperTypes(PageLayout *self)
{
    id                  list;
    int                 i;
    NXRect              buttonFrame;

    list = [[PopUpList allocFromZone:[self zone]] init];
    for (i = 0; i <= NUM_PAPERS; i++)
	[list addItem:(i < NUM_PAPERS ? _NXKitString("Printing", NXPaperList[i].name) : OtherString)];
    [self->paperSizeList getFrame:&buttonFrame];
    NXAttachPopUpList(self->paperSizeList, list);
    [self->paperSizeList setFrame:&buttonFrame];
    self->_otherPaper = NO;
}


/* finds the size of a given type of paper */
const NXSize *NXFindPaperSize(const char *paperName)
{
    int                 i;

    if (!paperName)
	return NULL;
    for (i = 0; i < NUM_PAPERS; i++)
	if (!strcmp(paperName, _NXKitString("Printing", NXPaperList[i].name)))
	    break;
    return i == NUM_PAPERS ? NULL : &NXPaperList[i].size;
}


/* finds the name of a given size of paper */
const char *_NXFindPaperName(const NXSize *paperSize)
{
    int                 i;

    if (!paperSize)
	return NULL;
    for (i = 0; i < NUM_PAPERS; i++)
	if (paperSize->width == NXPaperList[i].size.width && 
	    paperSize->height == NXPaperList[i].size.height)
	    break;
    return i == NUM_PAPERS ? NULL : _NXKitString("Printing", NXPaperList[i].name);
}


/* creates a PopUpList containing strings and attaches it to the button.
   strings should be a NULL terminated array of strings.  Maintains the
   original size of the button, in spite of NXAttachPopUpList.
 */
void _NXMakePopUpList(id button, const char *const * strings, const char *tableName)
{
    id                  list;
    NXRect              buttonFrame;

    list = [[PopUpList allocFromZone:[button zone]] init];
    while (*strings)
	[list addItem:_NXKitString(tableName, *strings++)];
    [button getFrame:&buttonFrame];
    NXAttachPopUpList(button, list);
    [button setFrame:&buttonFrame];
}


/* fixes the paper size to be in sync with the current orientation setting. */
static void fixPaperSize(PageLayout *self, NXSize *inSize)
{
    if ((inSize->width > inSize->height) ==
	([self->orientation selectedCol] == 0)) {
	float               swapper;

	swapper = inSize->width;
	inSize->width = inSize->height;
	inSize->height = swapper;
    }
}


/* sets the width and height fields, converting the passed in values from
   points to the given units.  Units is an index into the ConversionFactors
   array.  Units of -1 means no conversion, just set the fields.
 */
static void fillSizeFields(PageLayout *self, const NXSize *data, int units)
{
    float               w, h;

    if (units >= 0) {
	AK_ASSERT(units < sizeof(ConversionFactors) / sizeof(float), "Bad units value in fillSizeFields");
	w = data->width * ConversionFactors[units];
	h = data->height * ConversionFactors[units];
    } else {
	w = data->width;
	h = data->height;
    }
    [self->width setFloatValue:w at:0];
    [self->height setFloatValue:h at:0];
}


static void updatePaperView(PageLayout *self)
{
#define MAX_BOX_X	  7.0
#define MAX_BOX_Y	127.0
#define MAX_BOX_W	161.0
#define MAX_BOX_H	101.0
#define MIN_BOX_SIZE	MAX_BOX_H
#define MAX_PAPER_SIZE	(17.0*72.0)
    static const NXRect maxRect = {{MAX_BOX_X - 2, MAX_BOX_Y - 4},
				   {MAX_BOX_W + 6, MAX_BOX_H + 6}};
    NXRect              eraseRect;	/* rect to erase in borderView coords */
    NXRect              paperRect;
    NXSize              pSize;
    float               old, new;
    float               xscale, yscale;

    [self convertOldFactor:&old newFactor:&new];
    pSize.width = [self->width floatValueAt:0] / old;
    pSize.height = [self->height floatValueAt:0] / old;

    paperRect.origin.x = paperRect.origin.y = 0.0;
    paperRect.size.width = pSize.width * MIN_BOX_SIZE / MAX_PAPER_SIZE;
    paperRect.size.height = pSize.height * MIN_BOX_SIZE / MAX_PAPER_SIZE;
    xscale = yscale = 1.0;
    if (paperRect.size.width > MAX_BOX_W)
	xscale = MAX_BOX_W / paperRect.size.width;
    if (paperRect.size.height > MAX_BOX_H)
	yscale = MAX_BOX_H / paperRect.size.height;
    xscale = MIN(xscale, yscale);
    paperRect.size.width *= xscale;
    paperRect.size.height *= xscale;
    paperRect.origin.x = MAX_BOX_X + (MAX_BOX_W - paperRect.size.width) / 2;
    paperRect.origin.y = MAX_BOX_Y + (MAX_BOX_H - paperRect.size.height) / 2;

    [self disableFlushWindow];
    [self->paperView setFrame:&paperRect];
    paperRect.origin.x += 2;
    paperRect.origin.y -= 2;
    [self->_paperViewShadow setFrame:&paperRect];
    eraseRect = maxRect;
    [self->contentView convertRectToSuperview:&eraseRect];
    [self->_borderView display:(NXRect *)&eraseRect:1];
    [self reenableFlushWindow];
    [self flushWindow];
}


#ifndef SPECULATE
/* INDENT OFF */
- setAppIcon:obj	{ appIcon = obj; return self; }
- setHeight:obj		{ height = obj; return self; }
- setWidth:obj		{ width = obj; return self; }
- setOk:obj		{ ok = obj; return self; }
- setCancel:obj		{ cancel = obj; return self; }
- setOrientation:obj	{ orientation = obj; return self; }
- setScale:obj		{ scale = obj; return self; }
- setPaperSizeList:obj	{ paperSizeList = obj; return self; }
- setLayoutList:obj	{ layoutList = obj; return self; }
- setUnitsList:obj	{ unitsList = obj; return self; }
- setPaperView:obj	{ paperView = obj; return self; }
- set_paperViewShadow:obj { _paperViewShadow = obj; return self; }
/* INDENT ON */
#endif

@end

/*
  
Modifications (starting at 0.8):
  
 1/25/89 trey	major rewrite to load data from nib file
 2/17/89 trey	added support for paper view
 3/14/89 trey	added instance var and methods for customization
		added error handling to take panel down
		fixed erasing of paper view area to work for subclasses

0.91
----
 5/19/89 trey	minimized static data
		typing in a known paper size will be reflected in paper
		 types list
		added _NXFindPaperName

0.92
----
 6/12/89 trey	Repeated selection of Other no longer clear width and
		 height fields
		override orderWindow:relativeTo: to select the scale
		 field when we come up
		entry type for fields set

0.93
----
 6/16/89 pah	verify validity of fields when button is pressed to dismiss
 6/19/89 pah	panel is not dismissed of selection wont give up being
		 first responder

0.94
----
 7/12/89 trey	removed redundant drawing of paper view

0.94
----
 8/8/90 trey	changed pica conversion factor from 6 to 12

*/

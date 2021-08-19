/*
	PageLayout.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"

/* Returns the size of a given type of paper */

extern const NXSize *NXFindPaperSize(const char *paperName);

/* Tags of Controls in the Page Layout panel */

#define NX_PLICONBUTTON		50
#define NX_PLTITLEFIELD		51
#define NX_PLPAPERSIZEBUTTON	52
#define NX_PLLAYOUTBUTTON	53
#define NX_PLUNITSBUTTON	54
#define NX_PLWIDTHFORM		55
#define NX_PLHEIGHTFORM		56
#define NX_PLPORTLANDMATRIX	57
#define NX_PLSCALEFIELD		58
#define NX_PLCANCELBUTTON	NX_CANCELTAG
#define NX_PLOKBUTTON		NX_OKTAG

@interface PageLayout : Panel
{
    id                  appIcon;
    id                  height;
    id                  width;
    id                  ok;
    id                  cancel;
    id                  orientation;
    id                  scale;
    id                  paperSizeList;
    id                  layoutList;
    id                  unitsList;
    int                 exitTag;
    id                  paperView;
    id                  _paperViewShadow;
    id                  accessoryView;
    char                _currUnits;
    BOOL                _otherPaper;
    unsigned short      _reservedPageLayout1;
    unsigned int        _reservedPageLayout2;
    unsigned int        _reservedPageLayout3;
    unsigned int        _reservedPageLayout4;
    unsigned int        _reservedPageLayout5;
}

+ new;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;
+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- setAccessoryView:aView;
- accessoryView;
- pickedButton:sender;
- pickedPaperSize:sender;
- pickedOrientation:sender;
- pickedLayout:sender;
- pickedUnits:sender;
- (BOOL)textWillChange:textObject;
- textDidEnd:textObject endChar:(unsigned short)theChar;
- readPrintInfo;
- writePrintInfo;
- (int)runModal;
- convertOldFactor:(float *)old newFactor:(float *)new;

@end

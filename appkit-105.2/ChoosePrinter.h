/*
	ChoosePrinter.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"

/* Tags for Controls in the ChoosePrinter panel */

#define NX_CPPRINTERNAMEFIELD		110
#define NX_CPPRINTERTYPEFIELD		111
#define NX_CPPRINTERNOTEFIELD		112
#define NX_CPPRINTERNAMETITLE		113
#define NX_CPPRINTERTYPETITLE		114
#define NX_CPPRINTERNOTETITLE		115
#define NX_CPTITLEFIELD			116
#define NX_CPCANCELBUTTON		NX_CANCELTAG
#define NX_CPOKBUTTON			NX_OKTAG
#define NX_CPNAMETITLE			119
#define NX_CPTYPETITLE			120
#define NX_CPHOSTTITLE			121
#define NX_CPPORTTITLE			122
#define NX_CPICONBUTTON			123

@interface ChoosePrinter : Panel
{
    id                  appIcon;
    id                  ok;
    id                  cancel;
    id                  border;
    int                 exitTag;
    id                  name;
    id                  type;
    id                  note;
    const char        **_lastValues;
    id                  accessoryView;
    unsigned int        _reservedChoosePrinter1;
    unsigned int        _reservedChoosePrinter2;
    unsigned int        _reservedChoosePrinter3;
    unsigned int        _reservedChoosePrinter4;
}

+ new;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;

+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- setAccessoryView:aView;
- accessoryView;
- pickedButton:sender;
- pickedList:sender;
- readPrintInfo:(BOOL) printerFlag;
- writePrintInfo:(BOOL) printerFlag;
- (int)runModal;
- (int) runModalFax;
@end

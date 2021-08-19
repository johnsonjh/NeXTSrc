/*
	PrintPanel.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import "Panel.h"

/* These button tags are returned by runModal */

#define NX_SAVETAG	3	/* tag assigned to save button */
#define NX_PREVIEWTAG	4	/* tag assigned to preview button */
#define NX_FAXTAG	5	/* tag assigned to fax button */

/* Tags for Controls in the PrintPanel */

#define NX_PPTITLEFIELD		40
#define NX_PPICONBUTTON		41
#define NX_PPNAMETITLE		42
#define NX_PPNAMEFIELD		43
#define NX_PPNOTETITLE		44
#define NX_PPNOTEFIELD		45
#define NX_PPSTATUSTITLE	46
#define NX_PPSTATUSFIELD	47
#define NX_PPCOPIESFIELD	49
#define NX_PPPAGECHOICEMATRIX	50
#define NX_PPPAGERANGEFROM	51
#define NX_PPPAGERANGETO	52
#define NX_PPAUTOMANUALBUTTON	53
#define NX_PPRESOLUTIONBUTTON	54

@interface PrintPanel : Panel
{
    id                  appIcon;
    id                  pageMode;
    id                  firstPage;
    id                  lastPage;
    id                  copies;
    id                  ok;
    id                  cancel;
    id                  preview;
    id                  save;
    id                  printers;
    id                  feed;
    id                  resolutionList;
    id                  name;
    id                  note;
    id                  status;
    int                 exitTag;
    id                  accessoryView;
    unsigned short      _lastResolution;
    unsigned short      _reservedPrintPanel1;
    id			buttons;
    id			_reservedPrintPanel2[3];
}

+ new;
+ newContent:(const NXRect *)contentRect style:(int)aStyle backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag;

+ allocFromZone:(NXZone *)zone;
+ alloc;

- free;
- setAccessoryView:aView;
- accessoryView;
- pickedButton:sender;
- (BOOL)textWillChange:textObject;
- pickedAllPages:sender;
- changePrinter:sender;
- readPrintInfo;
- writePrintInfo;
- (int)runModal;

@end

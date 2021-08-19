/*
	PrintInfo.h
	Application Kit, Release 2.0
	Copyright (c) 1988, 1989, 1990, NeXT, Inc.  All rights reserved. 
*/

#import <objc/Object.h>
#import "graphics.h"
#import <dpsclient/dpsclient.h>

/* Page Order */

#define NX_DESCENDINGORDER	(-1)	/* descending order of pages */
#define NX_SPECIALORDER		0	/* special order. tells the spooler
					 * to not rearrange pages */
#define NX_ASCENDINGORDER	1	/* ascending order of pages */
#define NX_UNKNOWNORDER		2	/* no page order written out */

/* Page Orientation */

#define NX_LANDSCAPE		1	/* long side horizontal */
#define NX_PORTRAIT		0	/* long side vertical */

/* Pagination Modes */

#define NX_AUTOPAGINATION	0	/* auto pagination */
#define NX_FITPAGINATION	1	/* force image to fit on one page */
#define NX_CLIPPAGINATION	2	/* let image be clipped by page */

typedef struct _PrivatePrintInfo *NXPrivatePrintInfo;

@interface PrintInfo : Object
{
    char               *paperType;
    NXRect              paperRect;
    NXCoord             leftPageMargin;
    NXCoord             rightPageMargin;
    NXCoord             topPageMargin;
    NXCoord             bottomPageMargin;
    float               scalingFactor;
    char                pageOrder;
    struct _pInfoFlags {
	unsigned int        orientation:1;
	unsigned int        horizCentered:1;
	unsigned int        vertCentered:1;
	unsigned int        _RESERVEDA:2;
	unsigned int        manualFeed:1;
	unsigned int        allPages:1;
	unsigned int        _RESERVEDC:1;
	unsigned int        horizPagination:2;
	unsigned int        vertPagination:2;
	unsigned int        _RESERVEDB:4;
    }                   pInfoFlags;
    int                 firstPage;
    int                 lastPage;
    int                 currentPage;
    int                 copies;
    char               *outputFile;
    DPSContext          context;
    NXPrivatePrintInfo  _privateData;
    char               *printerName;
    char               *printerType;
    char               *printerHost;
    int                 resolution;
    short               pagesPerSheet;
    unsigned short      _reservedPrintInfo1;
    unsigned int        _reservedPrintInfo2;
    unsigned int        _reservedPrintInfo3;
    unsigned int        _reservedPrintInfo4;
    unsigned int        _reservedPrintInfo5;
}

- init;
- free;
- setPaperType:(const char *)type andAdjust:(BOOL)flag;
- (const char *)paperType;
- setPaperRect:(const NXRect *)aRect andAdjust:(BOOL)flag;
- (const NXRect *)paperRect;
- setMarginLeft:(NXCoord)leftMargin right:(NXCoord)rightMargin top:(NXCoord)topMargin bottom:(NXCoord)bottomMargin;
- getMarginLeft:(NXCoord *)leftMargin right:(NXCoord *)rightMargin top:(NXCoord *)topMargin bottom:(NXCoord *)bottomMargin;
- setScalingFactor:(float)aFloat;
- (float)scalingFactor;
- setOrientation:(char)mode andAdjust:(BOOL)flag;
- (char)orientation;
- setHorizCentered:(BOOL)flag;
- (BOOL)isHorizCentered;
- setVertCentered:(BOOL)flag;
- (BOOL)isVertCentered;
- setHorizPagination:(int)mode;
- (int)horizPagination;
- setVertPagination:(int)mode;
- (int)vertPagination;
- setOutputFile:(const char *)aString;
- (const char *)outputFile;
- setPageOrder:(char)mode;
- (char)pageOrder;
- setManualFeed:(BOOL)flag;
- (BOOL)isManualFeed;
- setAllPages:(BOOL)flag;
- (BOOL)isAllPages;
- setFirstPage:(int)anInt;
- (int)firstPage;
- setLastPage:(int)anInt;
- (int)lastPage;
- (int)currentPage;
- setCopies:(int)anInt;
- (int)copies;
- setContext:(DPSContext)aContext;
- (DPSContext)context;
- setPagesPerSheet:(short)aShort;
- (short)pagesPerSheet;
- setPrinterName:(const char *)aString;
- (const char *)printerName;
- setPrinterType:(const char *)aString;
- (const char *)printerType;
- setPrinterHost:(const char *)aString;
- (const char *)printerHost;
- setResolution:(int)anInt;
- (int)resolution;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* 
 * The following new... methods are now obsolete.  They remain in this  
 * interface file for backward compatibility only.  Use Object's alloc method  
 * and the init... methods defined in this class instead.
 */
+ new;

@end

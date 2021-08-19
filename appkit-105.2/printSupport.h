/*
	printSupport.h
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
  
	Some externs for private printing routines.
*/

#import "Application.h"
#import <mach.h>

/* info used over the lifetime of a single printing job. Actual data is
   stored in a kit-reserved pointer in the PrintInfo object
 */
typedef struct _PrivatePrintInfo {
    BOOL docBBoxAtEnd;		/* is the document BBox coming at the end? */
    BOOL pageBBoxAtEnd;		/* is the page BBox coming at the end? */
    NXRect docUnionBBox;	/* BBox we build up during printing doc */
    NXRect pageUnionBBox;	/* BBox we build up during printing page */
    BOOL docFontsAtEnd;		/* is the document font list at the end? */
    BOOL pageFontsAtEnd;	/* is the page font list written at the end? */
    BOOL pagesAtEnd;		/* is the number of pages at the end? */
    BOOL firstPageOfSheet;	/* first page printed on a sheet? */
				/* set in _doPage, always valid */
    BOOL lastPageOfSheet;	/* last page printed on a sheet? */
				/* set in _doPage, can be falsely NO if we */
				/* dont know the last page of the document */
    int ordSheet;		/* count of current sheet being printed */
    int ordPage;		/* count of current app page being printed */
				/* ranging from 1 to N */
    int numPages;		/* number of pages to print */
				/* if knows pages, set in _realPrintPSCode */
				/* else set in _printAndPaginate */
				/* -1 if we dont know the last doc page */
    int firstLabelPage;		/* label of first page to be printed */
    int printFD;		/* fd to spool to */
    NXStream *printStream;	/* stream to spool to */
    id currentPrintingView;	/* view told to make PS */
    NXRect currentPrintingRect;	/* rect of currentPrintingView that */
				/* we are currently imaging */
    BOOL specialPrintFocus;	/* indicates need for special focusing */
    BOOL doingLastPage;		/* current page is the last one we'll print */
    BOOL extraTurn;		/* Does multi pages/sheet require a switch */
    				/* of orientation? */
    char paperOrientation;	/* initial orientation of paper coord system */
    float totalScale;		/* cumulative scaling factor, due to scale */
    				/* factor and force fitting, but not mult */
				/* pages per sheet */
    float sheetScale;		/* scale to get from the sheet to a page */
    float rightEdge;		/* right boundary of paginated part of view */ 
    float bottomEdge;		/* bottom boundary of paginated part of view */
    int rows, cols;		/* results of pagination (0 = unknown) */
    NXPoint sheetOffset;	/* offset to get from sheet to page */
    id printPanel;		/* Print panel for this job */
    int mode;			/* tag of chosen button in Print panel */
#ifndef OLD_SPOOLER
    port_t spoolPort;		/* port to send page msgs to */
    port_t replyPort;		/* port to get npd messages on */
    int pageStart;		/* start of current page in Stream */
#endif
    NXModalSession *session;	/* used to keep panel running while printing */
    id faxPanel;		/* if we are faxing, NXFaxPanel for this job */
    int coverSheet;		/* 1 if we have fax cover sheet, 0 else */
} PrivatePrintInfo;


extern void _NXInitialPStat(id panel, int mode);
extern void _NXPagePStat(id panel, int mode, const char *pageLabel);
extern void _NXGetSpoolFileName(char *path, id pInfo);
extern void _NXWriteLongComment(const char *start, const char *body);
extern void _NXComputeRealBBox(NXRect *realBBox,const NXRect *bbox,
		               id pInfo, PrivatePrintInfo * privPInfo);
extern void _NXWriteBBoxComment(const char *label, const NXRect *bbox,
				id pInfo, PrivatePrintInfo *privPInfo);
extern void _NXPaperSizeComment(id pInfo);
extern BOOL _NXCalcPageSubset(id pInfo, int docFirstLabel, int docLastLabel,
			int *firstLabel, int *lastLabel, int *numPages);
extern BOOL _NXNeedsExtraTurn(int pagesPerSheet);
extern void _NXCalcPageOffsetOnSheet(id pInfo, PrivatePrintInfo *privPInfo);
extern void _NXFigureScale(float minorPap, float majorPap,
				int minorDivs, int majorDivs, float *scale,
				float *minorSlop, float *majorSlop);
extern void _NXUpdateBBox(BOOL flag, NXRect *bbox, const NXRect *baseRect);
extern void _NXRectPageToSheet(NXRect *r, id pInfo,
				PrivatePrintInfo *privPInfo);
extern NXStream *_NXMakeMemStream(id pInfo);
extern NXStream *_NXMakeFileStream(id pInfo, char *filename);



/*
	printSupport.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
  
	Here we have a set of private printing routines.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "PrintPanel_Private.h"
#import "PrintInfo_Private.h"
#import "Application.h"
#import "nextstd.h"
#import "SavePanel.h"
#import "printSupport.h"
#import <defaults.h>
#import "errors.h"
#ifndef OLD_SPOOLER
#import "npd.h"
#endif
#import <dpsclient/wraps.h>
#import <streams/streams.h>
#import <sys/time.h>
#import <sys/file.h>
#import <sys/message.h>
#import <servers/netname.h>


static int roundToPower(int num, int *log);
static port_t npdRendezVous(id pInfo, port_t ourPort);

/* prints out initial message on the Print Panel */
void
_NXInitialPStat(id panel, int mode)
{
    const char *initialStatMsg = NULL;

    switch (mode) {
    case NX_OKTAG:
	initialStatMsg = KitString(Printing, "Beginning printing...", "Status message indicating the start of printing.");
	break;
    case NX_SAVETAG:
	initialStatMsg = KitString(Printing, "Saving PostScript...", "Status message notify user that print output is now being saved to a file.");
	break;
    case NX_PREVIEWTAG:
	initialStatMsg = KitString(Printing, "Previewing...", "Status message notifying the user that a PostScript previewer is previewing their print output.");
	break;
    }
    [panel _updatePrintStat:initialStatMsg label:NULL];
}


/* prints out per page message on the Print Panel */
void
_NXPagePStat(id panel, int mode, const char *pageLabel)
{
    [panel _updatePrintStat:KitString(Printing, "Writing page...", "Status message telling the user what page is being imaged (the number immediately follows the ...") label:pageLabel];
}


/* figures out the name of the spool file. */
void
_NXGetSpoolFileName(char *path, id pInfo)
{
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    id                  sp;
    const char         *file;
    static const char   previewFileName[] = "/tmp/.previewXXXXXX.ps";

    if (privPInfo->mode == NX_OKTAG) {
	if (file = [pInfo outputFile])
	    strcpy(path, file);
	else
	    path[0] = '\0';
    } else if (privPInfo->mode == NX_PREVIEWTAG) {
	(void)strcpy(path, previewFileName);
	(void)NXGetTempFilename(path, 13);
    } else {
	sp = [SavePanel new];
	AK_ASSERT(sp, "No save panel even though save was chosen");
	strcpy(path, [sp filename]);
    }
}


/* writes a comment that may be greater then 256 chars long, properly
   separating it into multiple lines.
 */
void
_NXWriteLongComment(const char *start, const char *body)
{
#define LENGTH_LIMIT	256
    register const char *curr;
    register const char *end;
    register const char *pieceEnd;

    curr = body;
    end = curr + strlen(curr);
    while (curr < end) {
	if (curr == body) {	/* if first time */
	    if (start)
		DPSPrintf(DPSGetCurrentContext(), "%s", start);
	} else
	    DPSWriteData(DPSGetCurrentContext(), "%%%%+", 5);
	if (end - curr > LENGTH_LIMIT) {
	    for (pieceEnd = curr + LENGTH_LIMIT - 1; pieceEnd > curr; pieceEnd--)
		if (*pieceEnd == ' ' || *pieceEnd == '\t')
		    break;
	    if (pieceEnd == curr)
		pieceEnd += LENGTH_LIMIT;
	} else
	    pieceEnd = end;
	DPSWriteData(DPSGetCurrentContext(), curr, pieceEnd - curr);
	DPSWriteData(DPSGetCurrentContext(), "\n", 1);
	curr = pieceEnd;
    }
}


/* returns whether the number of pages per sheet forces us to rotate the
   page an extra turn.  This boils down to whether we have an odd number
   of magnifications
 */
BOOL
_NXNeedsExtraTurn(int pagesPerSheet)
{
    int                 mags;	/* # magnifications = logBase2(magnification) */

    (void)roundToPower(pagesPerSheet, &mags);
    return mags % 2;
}


/* generates the correct xform info to place a page on a sheet */
void
_NXCalcPageOffsetOnSheet(id pInfo, PrivatePrintInfo * privPInfo)
{
    int                 pagesPerSheet = [pInfo pagesPerSheet];
    int                 currPage = privPInfo->ordPage;
    int                 origOrientation = [pInfo orientation];
    const NXRect       *paperRect = [pInfo paperRect];
    const NXSize       *paperSize = &(paperRect->size);
    int                 mags;	/* # magnifications = logBase2(magnification) */
    int                 rows, cols;
    float               xslop, yslop;

    if (pagesPerSheet == 1) {	/* short-cut out */
	privPInfo->sheetOffset.x = privPInfo->sheetOffset.y = 0.0;
	privPInfo->sheetScale = 1.0;
	return;
    }
 /* gets currPage on a scale from 0 to n-1, mod'ing out previous sheets */
    currPage = (currPage - 1) % pagesPerSheet;
    pagesPerSheet = roundToPower(pagesPerSheet, &mags);

    rows = mags / 2;
    cols = 1 << (mags - rows);
    rows = 1 << rows;
    if (origOrientation == NX_LANDSCAPE) {
	int                 swapper = rows;

	rows = cols;
	cols = swapper;
    }
    if (!privPInfo->extraTurn) {
	privPInfo->sheetScale = 1.0 / cols;
	privPInfo->sheetOffset.x = paperSize->width;
	privPInfo->sheetOffset.y = paperSize->height;
	xslop = yslop = 0;
    } else {
	privPInfo->sheetOffset.x = paperSize->height;
	privPInfo->sheetOffset.y = paperSize->width;
	if (origOrientation == NX_PORTRAIT) {
	    _NXFigureScale(paperSize->width, paperSize->height, cols, rows,
			   &privPInfo->sheetScale, &xslop, &yslop);
#ifdef EQUIVALENT_CODE
	    widthScale = (paperSize->height / 2) / paperSize->width;
	    heightScale = paperSize->width / paperSize->height;
	    scale = MIN(widthScale, heightScale) / minRowsCols;
	    xslop = (paperSize->height / cols - paperSize->width * scale) / 2;
	    yslop = (paperSize->width / rows - paperSize->height * scale) / 2;
#endif
	} else {
	    _NXFigureScale(paperSize->height, paperSize->width, rows, cols,
			   &privPInfo->sheetScale, &yslop, &xslop);
#ifdef EQUIVALENT_CODE
	    heightScale = (paperSize->width / 2) / paperSize->height;
	    widthScale = paperSize->height / paperSize->width;
	    scale = MIN(widthScale, heightScale) / minRowsCols;
	    yslop = (paperSize->width / rows - paperSize->height * scale) / 2;
	    xslop = (paperSize->height / cols - paperSize->width * scale) / 2;
#endif
	}
    }
    privPInfo->sheetOffset.x *= (currPage % cols) / (float)cols;
    privPInfo->sheetOffset.x += xslop;
    privPInfo->sheetOffset.y *= (rows - 1 - currPage / cols) / (float)rows;
    privPInfo->sheetOffset.y += yslop;
}


/* figures out and positioning slop to center image in page quadrant */
void
_NXFigureScale(float minorPap, float majorPap, int minorDivs, int majorDivs,
	       float *scale, float *minorSlop, float *majorSlop)
{
    float               minorScale, majorScale;	/* scaling required in each
						 * dir */

    minorScale = (majorPap / 2) / minorPap;
    majorScale = minorPap / majorPap;
    *scale = MIN(minorScale, majorScale) / MIN(minorDivs, majorDivs);
    *minorSlop = (majorPap / minorDivs - minorPap * *scale) / 2;
    *majorSlop = (minorPap / majorDivs - majorPap * *scale) / 2;
}


/* rounds number up to a power of two.  Also returns the logarithm */
static int
roundToPower(int num, int *log)
{
    int                 result;

    AK_ASSERT(num > 0, "Non-positive arg to roundToPower");
    for (result = 1, *log = 0; result < num; result <<= 1, (*log)++);
    return result;
}


/* updates a bbox to include baseRect */
void
_NXUpdateBBox(BOOL flag, NXRect *bbox, const NXRect *baseRect)
{
    if (flag)
	if (NX_HEIGHT(bbox) < 0)
	    *bbox = *baseRect;	/* init for the first iteration */
	else
	    (void)NXUnionRect(baseRect, bbox);
}


/* transforms rect from page coords to sheet coords */
void
_NXRectPageToSheet(NXRect *r, id pInfo, PrivatePrintInfo * privPInfo)
{
    r->origin.x *= privPInfo->sheetScale;
    r->origin.y *= privPInfo->sheetScale;
    r->size.width *= privPInfo->sheetScale;
    r->size.height *= privPInfo->sheetScale;
    if (!privPInfo->extraTurn) {
	r->origin.x += privPInfo->sheetOffset.x;
	r->origin.y += privPInfo->sheetOffset.y;
    } else {
	int                 orientation = [pInfo orientation];
	const NXRect       *paperRect = [pInfo paperRect];
	NXRect              result;

	if (orientation == NX_PORTRAIT) {
	    result.origin.x = privPInfo->sheetOffset.y
	      + r->origin.y;
	    result.origin.y = paperRect->size.height
	      - privPInfo->sheetOffset.x
	      - r->origin.x
	      - r->size.width;
	} else {
	    result.origin.x = paperRect->size.width
	      - privPInfo->sheetOffset.y
	      - r->origin.y
	      - r->size.height;
	    result.origin.y = privPInfo->sheetOffset.x
	      + r->origin.x;
	}
	result.size.width = r->size.height;
	result.size.height = r->size.width;
	*r = result;
    }
}

/* computes a bbox, switching the x/y dimensions if necessary */
void
_NXComputeRealBBox(NXRect *realBBox,
		   const NXRect *bbox,id pInfo, PrivatePrintInfo * privPInfo)
{

    if ([pInfo orientation] == privPInfo->paperOrientation)
	*realBBox = *bbox;
    else {
	const NXRect       *paperRect = [pInfo paperRect];

	if (privPInfo->paperOrientation == NX_PORTRAIT) {
	    realBBox->origin.x = bbox->origin.y;
	    realBBox->origin.y = paperRect->size.width
	      - bbox->origin.x
	      - bbox->size.width;
	} else {
	    realBBox->origin.x = paperRect->size.height
	      - bbox->origin.y
	      - bbox->size.height;
	    realBBox->origin.y = bbox->origin.x;
	}
	realBBox->size.width = bbox->size.height;
	realBBox->size.height = bbox->size.width;
    }
}

/* writes out a bbox comment, switching the x/y dimensions if necessary */
void
_NXWriteBBoxComment(const char *label, const NXRect *bbox,
		    id pInfo, PrivatePrintInfo * privPInfo)
{
    NXRect              realBBox;

    _NXComputeRealBBox(&realBBox,bbox,pInfo,privPInfo);
    DPSPrintf(DPSGetCurrentContext(), "%%%%%s:%d %d %d %d\n\n", label,
	      (int)floor(NX_X(&realBBox)),
	      (int)floor(NX_Y(&realBBox)),
	      (int)ceil(NX_MAXX(&realBBox)),
	      (int)ceil(NX_MAXY(&realBBox)));
}



/* writes out a bbox comment, switching the x/y dimensions if necessary */
void
_NXPaperSizeComment(id pInfo)
{
    DPSContext          ctxt = DPSGetCurrentContext();
    const char         *paperName;
    const NXRect       *paperRect;
    float               minSize, maxSize;

    if ((paperName = [pInfo paperType]) && *paperName)
	DPSPrintf(ctxt, "%%%%PaperSize: %s\n", paperName);
    else {
	paperRect = [pInfo paperRect];
	if (paperRect->size.width < paperRect->size.height) {
	    minSize = paperRect->size.width;
	    maxSize = paperRect->size.height;
	} else {
	    maxSize = paperRect->size.width;
	    minSize = paperRect->size.height;
	}
	DPSPrintf(ctxt, "%%%%Feature: *CustomPaperSize %f %f\n",
		  minSize, maxSize);
    }
}


/* Given the first/last document pages, determines the labels of the
   pages to print.  Returns whether to print any pages.
 */
BOOL
_NXCalcPageSubset(id pInfo, int docFirstLabel, int docLastLabel,
			    int *firstLabel, int *lastLabel, int *numPages)
{
    if ([pInfo isAllPages]) {
	*firstLabel = docFirstLabel;
	*lastLabel = docLastLabel;
    } else {
	*firstLabel = [pInfo firstPage];
	if (docFirstLabel > *firstLabel)
	    *firstLabel = docFirstLabel;
	*lastLabel = [pInfo lastPage];
	if (docLastLabel < *lastLabel)
	    *lastLabel = docLastLabel;
    }
    if (*lastLabel != MAXINT)
      /* could be negative if firstLabel < 0 and lastLabel == MAXINT */
	*numPages = *lastLabel - *firstLabel + 1;
    else
	*numPages = -1;
    return *lastLabel >= *firstLabel;
}


/* creates a stream to a file for spooling, making up a name if passed
   a null ptr or empty string.
 */
NXStream           *
_NXMakeFileStream(id pInfo, char *filename)
{
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    int                 tempLen;

#define CREATE_MASK	(O_CREAT | O_WRONLY | O_TRUNC)
#define EXCREATE_MASK	(O_CREAT | O_WRONLY | O_TRUNC | O_EXCL)

    if (!filename || !*filename) {
	(void)strcpy(filename, _NX_SPOOLERPATH);
	(void)strcat(filename, "printOutput");
	tempLen = strlen(filename);
	(void)strcat(filename, "XXXXXX.ps");
	(void)NXGetTempFilename(filename, tempLen);
    }
    privPInfo->printFD = open(filename, (privPInfo->mode == NX_SAVETAG) ?
			      CREATE_MASK : EXCREATE_MASK, 0666);
    if (privPInfo->printFD < 0)
	return NULL;
    privPInfo->printStream = NXOpenFile(privPInfo->printFD, NX_WRITEONLY);
    return privPInfo->printStream;
}



#ifndef OLD_SPOOLER

/* sets up stream for memory spooling */
NXStream           *
_NXMakeMemStream(id pInfo)
{
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];

    privPInfo->printStream = NULL;
    if (port_allocate(task_self(), &privPInfo->replyPort) == KERN_SUCCESS) {
	privPInfo->spoolPort = npdRendezVous(pInfo, privPInfo->replyPort);
	if (privPInfo->spoolPort) {
	    privPInfo->printStream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
	    privPInfo->pageStart = 0;
	    if (!privPInfo->printStream) {
		(void)port_deallocate(task_self(), privPInfo->spoolPort);
		goto moreErrorCleanup;
	    }
	} else
moreErrorCleanup:
	    (void)port_deallocate(task_self(), privPInfo->replyPort);
    }
    return privPInfo->printStream;
}


extern port_t       name_server_port;

/* hooks up the the npd printing server */
static port_t
npdRendezVous(id pInfo, port_t ourPort)
{
    port_t              npd_port;
    int                 ret;			/* return val from sys calls */
    port_t		ourRet = PORT_NULL;	/* what we'll return */
    npd_con_msg         con_msg;
    const char		*printerInfo;
    const char		*altName;

#define NPD_CHAR_BUF_SIZE 1024
    static const msg_type_t charBufType =
    {MSG_TYPE_CHAR, 8, NPD_CHAR_BUF_SIZE, TRUE, FALSE, FALSE};
    static const msg_type_t intType =
    {MSG_TYPE_INTEGER_32, 32, 1, TRUE, FALSE, FALSE};

    altName = NXGetDefaultValue(_NXAppKitDomainName, "NPDName");
    if (altName) {
	ret = netname_look_up(name_server_port, "", (char *)altName,
			      &npd_port);
    } else {
	ret = netname_look_up(name_server_port, "", NPD_PUBLIC_PORT,
			      &npd_port);
    }
    if (ret != KERN_SUCCESS)
	return PORT_NULL;
    con_msg.head.msg_remote_port = npd_port;
    con_msg.head.msg_local_port = ourPort;
    con_msg.head.msg_size = sizeof(npd_con_msg);
    con_msg.head.msg_type = MSG_TYPE_NORMAL;
    con_msg.head.msg_simple = TRUE;
    con_msg.head.msg_id = NPD_CONNECT;

    bcopy(&charBufType, &con_msg.printer_type, sizeof(msg_type_t));
    printerInfo = [pInfo printerName];
    if (printerInfo) {
	strncpy(con_msg.printer, printerInfo, NPD_CHAR_BUF_SIZE);
	con_msg.printer[NPD_CHAR_BUF_SIZE - 1] = '\0';
    } else
	con_msg.printer[0] = '\0';

    bcopy(&charBufType, &con_msg.host_type, sizeof(msg_type_t));
    strncpy(con_msg.host, _NXHostName(), NPD_CHAR_BUF_SIZE);
    con_msg.host[NPD_CHAR_BUF_SIZE - 1] = '\0';

    bcopy(&charBufType, &con_msg.user_type, sizeof(msg_type_t));
    strncpy(con_msg.user, NXUserName(), NPD_CHAR_BUF_SIZE);
    con_msg.user[NPD_CHAR_BUF_SIZE - 1] = '\0';

    bcopy(&intType, &con_msg.copies_type, sizeof(msg_type_t));
    con_msg.copies = [pInfo copies];

    ret = msg_rpc((msg_header_t *)&con_msg, SEND_TIMEOUT | RCV_TIMEOUT,
		  sizeof(msg_header_t), 30000, 30000);
    if (ret == RCV_SUCCESS)
	ourRet = con_msg.head.msg_remote_port;
    else
	ourRet = PORT_NULL;
    port_deallocate(task_self(), npd_port);
    return ourRet;
}


void
_NXNpdSend(int msgID, id pInfo)
{
    npd_receive_msg     send_msg;
    char               *addr;
    int                 len, maxlen;
    int                 curr;
    msg_return_t        ret;
    PrivatePrintInfo   *privPInfo = [pInfo _privateData];
    BOOL		success;

    curr = NXTell(privPInfo->printStream);
    NXGetMemoryBuffer(privPInfo->printStream, &addr, &len, &maxlen);
    send_msg.head.msg_remote_port = privPInfo->spoolPort;
    send_msg.head.msg_local_port = privPInfo->replyPort;
    send_msg.head.msg_size = sizeof(npd_receive_msg);
    send_msg.head.msg_id = msgID;
    send_msg.head.msg_type = MSG_TYPE_NORMAL;
    send_msg.head.msg_simple = FALSE;
    send_msg.type.msg_type_long_name = MSG_TYPE_CHAR;
    send_msg.type.msg_type_long_size = 8;
    send_msg.type.msg_type_long_number = curr - privPInfo->pageStart;
    send_msg.type.msg_type_header.msg_type_inline = 0;
    send_msg.type.msg_type_header.msg_type_longform = 1;
    send_msg.type.msg_type_header.msg_type_deallocate = 0;
    send_msg.data = addr + privPInfo->pageStart;
    if (msgID == NPD_SEND_TRAILER) {
	ret = msg_rpc((msg_header_t *)&send_msg, MSG_OPTION_NONE, sizeof(msg_header_t), 0, 0);
	success = (ret == RPC_SUCCESS);
	port_deallocate(task_self(), privPInfo->replyPort);
    } else {
	ret = msg_send((msg_header_t *)&send_msg, MSG_OPTION_NONE, 0);
	success = (ret == SEND_SUCCESS);
    }
    if (!success)
	NX_RAISE(NX_printingComm, (void *)ret, 0);
    privPInfo->pageStart = curr;
}

#endif

/*
  
Modifications (starting at 0.8):
  
0.91
----
 5/19/89 trey	minimized static data
		fixed port leak in npdRendezVous
 5/28/89 trey	fixed bug where repeated saves to the same file would not
		 completely overwrite the entire old file
 6/16/89 trey	new protocol with npd:  we pass hostname of printer and
 		 retain our port so npd knows if we crash

appkit-77
---------
 2/28/90 king	fixed _NXWriteBBoxComment to output integers instead of floats
 		fixed _NXWriteLongComment to output %%+ instead of %+
		fixed npdRendezVous() to put local host name in connect packet
		 rather than the printer host.
 3/05/90 king	Added NPDName default for npd port name

79
--
 3/30/90 king	fixed _NXWriteBBoxComment to use ceil() and floor()
*/



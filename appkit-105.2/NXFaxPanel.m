/*
	NXFaxPanel.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Chris Franklin
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "PrintInfo_Private.h"
#import "PrintPanel_Private.h"
#import "Application_Private.h"
#import "Text.h"
#import "Button.h"
#import "ScrollView.h"
#import "Window.h"
#import "Application.h"
#import "Form.h"
#import "FormCell.h"
#import "Cursor.h"
#import "SelectionCell.h"
#import "PrintPanel.h"
#import "Font.h"
#import "NXBrowser.h"
#import "NXBrowserCell.h"
#import "ChoosePrinter.h"
#import <dpsclient/dpsclient.h>
#import <dpsclient/dpsfriends.h>

#import <objc/List.h>

#import "NXFaxPanel.h"
#import "NXFaxText.h"
#import "NXFaxCoverView.h"
#import "NXFaxPaperBox.h"

#import <objc/error.h>
#import <defaults.h>
#import "errors.h"
#import "graphics.h"
#import "nextstd.h"
#import "appkitPrivate.h"
#import "printSupport.h"

#import <streams/streams.h>
#import <sys/param.h>
#import <sys/stat.h>
#import <sys/errno.h>
#import <NXCType.h>
#import <pwd.h>

extern id NXApp;

@implementation NXFaxPanel

static void conformRect(NXRect *aRect,const NXRect *bounds);
static NXStream *mapByPath(const char *name,
			   int option,
			   char *mPath);
static const char *expandPath(const char *src,char *dst,const char *name);
static int createPath(const char *path);
static void useFonts(NXZone *zone, NXStream *stream);
static NXRect *computeScaledPaperRect(NXRect *scaledPaperRect);
static void loadSelectedData(NXFaxPanel *self);
static void updateTitlesAndOptions(id self);
static BOOL getFieldValue(Control *ctrl, const char *altString, int altValue,
			  int *resultValue);
typedef int (*matrixCellProc)( id matrix, id cell, int row, void *value);
static int forSelectedCellsDo(id matrix, void *value,matrixCellProc func);

/* defines and variables to do select by key. KEYINTERVAL is in vertical
 * retraces (same as event time field).
 */
#define KEYINTERVAL (30)
#define MAXMATCH (25)
static char keyBuffer[MAXMATCH+1];
static BOOL nowCreating = NO;	/* are we in the process of new'ing? */
static int keyPos = -1;
static long keyTime = 0;
static NXFaxPanel *_faxPanel = NULL;

/* names of files used by fax panel */

static const char faxCover[] = "Fax/Cover.rtf";
static const char faxBackground[] = "Fax/Background.eps";
static const char faxNumbers[] = "Fax/Numbers.text";

/* once NIB lets us set title, this would be unnecessary */

static const char *browserTitle = NULL;
#define BrowserTitle (browserTitle ? browserTitle : (browserTitle = KitString(Printing, "Phone List", NULL)))

/* duplicated from PrintPanel - should reference them directly */

static const char *firstString = NULL;
static const char *lastString = NULL;

#define FirstString (firstString ? firstString : (firstString = \
    KitString(Printing, "first", "Word that appears if the user is printing the document starting at the beginning.")))

#define LastString (lastString ? lastString : (lastString = \
    KitString(Printing, "last", "Word that appears if the user is printing the document all the way to the end.")))

+ (BOOL)_canAlloc { return nowCreating; }

/* If someone just calls this out of the blue, its an error.  They must use a new method.  Else we're being called as part of nib loading, and we do a special hack to make sure an object of the right type is created in spite of the class stored in 
the nib file. */
+ allocFromZone:(NXZone *)zone
{
    if (nowCreating)
      /* this wont work for subclassers, but this is a private class.  See one of the public panels for the appropriate magic. */
	return [super allocFromZone:zone];
    else
	return [self doesNotRecognize:_cmd];
}

/* If someone just calls this out of the blue, its an error.  They must use a new method.  We depend on nibInstantiate using allocFromZone:. */
+ alloc
{
    return [self doesNotRecognize:_cmd];
}


/*  Returns a faxPanel iff printerType is a fax modem printer, else NULL.  Like
 *  PrintPanel, there is only one set of fax panels, which are recycled.
 */
+ new {
    NXRect frameR;
    NXFaxPanel *fp;
    
    if (!(fp = _faxPanel)) {
	nowCreating = YES;
	_faxPanel = fp = _NXLoadNib("__APPKIT_PANELS","NXFaxPanel",self,NULL);
	nowCreating = NO;
	[fp setOneShot:NX_KITPANELSONESHOT];
	[fp useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
	fp->dataPS = NULL;
	fp->streamPS = NULL;
	fp->coverText = NULL;
	fp->numbersIno = 0;
	fp->numbersChanged = NO;
	fp->numberIsEmpty = YES;
	fp->nameIsEmpty = YES;
	[NXApp getScreenSize:&frameR.size];
	frameR.origin.x = frameR.size.width;
	frameR.origin.y = 0.0;
	frameR.size.width = 16.0;
	frameR.size.height = 16.0;
	fp->focusWindow = [Window allocFromZone:[fp zone]];
	[fp->focusWindow initContent:&frameR
			 style : NX_PLAINSTYLE
			 backing : NX_NONRETAINED
			 buttonMask : 0
			 defer : NO];
    
    }
    fp->faxExit = _NX_FAXCANCEL;
    return fp;
}

- free {
    if (streamPS) {
	NXCloseMemory(streamPS, NX_FREEBUFFER);
	streamPS = NULL;
	dataPS = NULL;
    }
    [focusWindow free];
    [coverWindow free];
    _faxPanel = NULL;
    return [super free];
}

/*  Fax Panel outlets */

/*
- setFaxWindow:anObject
{
    id  cview;
    
    cview = [anObject contentView];
    [self setNextResponder : [cview nextResponder]];
    [cview setNextResponder : self];
    faxWindow = anObject;
    return self;
}
*/

- setFaxIcon:anObject
{
    [anObject setIcon:"app"];
    faxIcon = anObject;
    return self;
}

- setFaxForm:anObject
{
    faxForm = anObject;
    [anObject setTextDelegate:self];
    return self;
}

- setFaxNumber:anObject
{
    faxNumber = anObject;
    return self;
}

- setFaxName:anObject
{
    faxName = anObject;
    return self;
}

- setAddButton:anObject
{
    addButton = anObject;
    return self;
}

- setDeleteButton:anObject
{
    deleteButton = anObject;
    return self;
}

- setReplaceButton:anObject
{
    replaceButton = anObject;
    return self;
}

- setFaxOkButton:anObject
{
    faxOkButton = anObject;
    return self;
}

- setCoverCheckBox:anObject
{
    coverCheckBox = anObject;
    return self;
}

- setNotifyCheckBox:anObject
{
    notifyCheckBox = anObject;
    return self;
}

- setHiresCheckBox:anObject
{
    hiresCheckBox = anObject;
    return self;
}

- setPhoneList:anObject
{
    
    phoneList = anObject;
    [anObject loadColumnZero];
    [phoneList setTitle:BrowserTitle ofColumn:0];
    numbersMatrix = [anObject matrixInColumn:0];
    return self;
}

- setFirstPage:anObject
{
    firstPage = anObject;
    return self;
}

- setLastPage:anObject
{
    lastPage = anObject;
    return self;
}

- setName:anObject
{
    name = anObject;
    return self;
}

- setPageMode:anObject
{
    pageMode = anObject;
    return self;
}

- setStatus:anObject
{
    status = anObject;
    return self;
}

- setPreview:anObject
{
    preview = anObject;
    return self;
}

- setModemButton:anObject
{
    modemButton = anObject;
    return self;
}

/*  Cover Sheet Panel outlets */

- setCoverScroller:anObject
{
    coverScroller = anObject;
    return self;
}

- setCoverIcon:anObject
{
    [anObject setIcon:"app"];
    coverIcon = anObject;
    return self;
}

- setCoverWindow:anObject
{
    coverWindow = anObject;
    [coverWindow setOneShot:NX_KITPANELSONESHOT];
    [coverWindow useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
    return self;
}

- setPaperBox : anObject
{
    NXRect rect;
    NXSize size;
    NXZone *zone = [self zone];
    
    [[anObject contentView] getFrame : &paperViewMax];
    [anObject getFrame : &rect];
    paperBox = [NXFaxPaperBox allocFromZone:zone];
    [paperBox initFrame : &rect];
    [paperBox setBorderType : [anObject borderType]];
    [paperBox getOffsets : &size];
    [paperBox setOffsets : size.width : size.height];
    [paperBox setFont : [anObject font]];
    [paperBox setTitle : [anObject title]];
    [paperBox setTitlePosition : [anObject titlePosition]];
    [[anObject superview] addSubview : paperBox];
    [anObject free];
    paperView = [NXFaxCoverView allocFromZone:zone];
    [paperView initFrame : &paperViewMax];
    [[paperBox setContentView : paperView] free];
    return self;
}


/*    Drive Cover Sheet Panel */

typedef struct _charLens {
    int totalChars;
    int maxChars;
} charLens;

static int sizeNumbersList( id matrix, id cell, int row, void *value)
{
    charLens *charParams;
    const char *stringValue;
    int sLen;
    
    charParams = (charLens *)value;
    stringValue = [cell stringValue];
    sLen = (stringValue ? strlen(stringValue) : 0) + 1;
    charParams->totalChars += sLen;
    if (sLen > charParams->maxChars)
        charParams->maxChars = sLen;
    return 0;
}

static int fillNumbersList( id matrix, id cell, int row, void *value)
{
    char **fillPoint;
    const char *stringValue;
    
    fillPoint = (char **)value;
    stringValue = [cell stringValue];
    if (stringValue) {
        strcpy(*fillPoint,stringValue);
	*fillPoint += strlen(stringValue)+1;
    } else {
        **fillPoint = '\0';
	*fillPoint += 1;
    }
    return 0;
}

static void swapFirstAndLast(char *person,char *swapBuffer)
{
    char *comma,*end;
    int  firstLen,lastLen;
    
    end = person;
    comma = NULL;
    while (*end && ((!comma) || *end != ',') && *end != '\n') {
	if ((!comma) && *end == ',')
	    comma = end;
	end++;
    }
    if (comma) {
	firstLen = end - comma - 1;
	lastLen = comma - person;
	bcopy(person,swapBuffer,lastLen);
	bcopy(person+lastLen+1,person,firstLen);
	person[firstLen] = ' ';
	bcopy(swapBuffer,person+firstLen+1,lastLen);
    }
}

- setCoverText {
    NXRect frect;
    NXStream *s;
    const char *formName;
    char *person;
    char *swapped = NULL;
    char *fillPoint;
    char *saveChars;
    int  len;
    struct passwd      *upwd;
    NXZone *zone = [self zone];
    charLens charParams;
    int nSel;
    
    frect.origin.x = frect.origin.y = 0.0;
    [coverScroller getContentSize : &frect.size];
    if (!coverText) {
        coverText = [NXFaxText allocFromZone:zone];
	[coverText initFrame:&frect];
        [coverText setAutodisplay : NO];
	[coverText setMonoFont:NO];
	[coverText setDelegate:self];
	[coverText setEditable:YES];
	[coverText setVertResizable : YES];
	[coverText setOpaque: NO];
	frect.size.height = 1000000.0;
	[coverText setMaxSize : &frect.size];
	[coverScroller setDocView:coverText];
	[coverScroller setVertScrollerRequired:YES];
	[coverScroller setDynamicScrolling:YES];
    } else
        [coverText setAutodisplay : NO];

    s = mapByPath(faxCover,NX_READONLY,NULL);
    if (s) {
	[coverText readRichText: s];
	NXCloseMemory(s, NX_FREEBUFFER);
    }
    charParams.totalChars = 0;
    charParams.maxChars = 0;
    nSel = forSelectedCellsDo(numbersMatrix,&charParams,sizeNumbersList);
    formName = [faxName stringValue];
    if (nSel && *formName == '\0') {
        swapped = NXZoneMalloc(zone, charParams.totalChars);
	saveChars = NXZoneMalloc(zone,charParams.maxChars);
	fillPoint = swapped;
	forSelectedCellsDo(numbersMatrix,&fillPoint,fillNumbersList);
    } else {
        len = strlen(formName)+1;
	swapped = NXZoneMalloc(zone, len);
	saveChars = NXZoneMalloc(zone,len);
	strcpy(swapped,formName);
        nSel = 1;
    }
    person = swapped;
    while (nSel--) {
	swapFirstAndLast(person,saveChars);
	if (nSel) {
	    person = person + strlen(person);
	    *person++ = '\n';
	}
    }
    [coverText setTo : swapped];
    free(swapped);
    free(saveChars);
        
    if ((upwd = getpwuid(getuid())) && upwd->pw_gecos) /* real user name */
	[coverText setFrom : upwd->pw_gecos];
    [coverText setDate : time(0)];
    [coverText setNeedsDisplay : NO];
    [coverText setAutodisplay : YES];
    return self;
}

- (void) setCoverBackground {
    NXStream *s;
    char path[MAXPATHLEN];
    struct stat st;
    
    s = mapByPath(faxBackground,NX_READONLY,path);
    if (s) {
        if (stat(path,&st) >= 0) {
	    if((!streamPS) ||
	       inoPS != st.st_ino ||
	       devPS != st.st_dev ||
	       mtimePS < st.st_mtime) {
	        if (streamPS) {
		    [paperView setBackgroundData: NULL length : 0 bbox : NULL];
		    NXCloseMemory(streamPS,NX_FREEBUFFER);
		}
		streamPS = s;
		s = NULL;
		[self readPSFromStream:streamPS];
		if (!dataPS) {
		    NXCloseMemory(streamPS, NX_FREEBUFFER);
		    streamPS = NULL;
		}
	    }
	    mtimePS = st.st_mtime;
	    devPS = st.st_dev;
	    inoPS = st.st_ino;
	}
	if (s)
	    NXCloseMemory(s, NX_FREEBUFFER);
    } else if (streamPS) {
	[paperView setBackgroundData: NULL length : 0 bbox : NULL];
	NXCloseMemory(streamPS,NX_FREEBUFFER);
	dataPS = NULL;
	streamPS = NULL;
    }

}

- (void) adjustPaperView {
    NXRect frect,scaledPaperRect;
    float heightScale,widthScale;
    static const NXRect empty = {{0.0,0.0},{0.0,0.0}};
    const char *s;
    char strRect[100];
    NXPoint textOrigin,graphicsOrigin;
    
    if (!(s = NXUpdateDefault(NXSystemDomainName,NX_FAXORIGINS)))
        s = NXGetDefaultValue(NXSystemDomainName,NX_FAXORIGINS);
    if (s) {
	strRect[99] = '\0';
	strncpy(strRect,s,99);
    }
    if (!s || sscanf(strRect,"%f%f%f%f",
		     &textOrigin.x,
		     &textOrigin.y,
		     &graphicsOrigin.x,
		     &graphicsOrigin.y) != 4) {
	textOrigin = empty.origin;
	graphicsOrigin = empty.origin;
    }
    [paperView getFrame:&frect];
    [paperView setDrawSize : frect.size.width : frect.size.height];

    (void) computeScaledPaperRect(&scaledPaperRect);
    heightScale = scaledPaperRect.size.height / paperViewMax.size.height;
    widthScale = scaledPaperRect.size.width / paperViewMax.size.width;
    if (heightScale < widthScale)
        heightScale = widthScale;
	
    frect.size.width = floor(scaledPaperRect.size.width / heightScale);
    frect.size.height = floor(scaledPaperRect.size.height / heightScale);
    frect.origin.x = 
        floor(paperViewMax.origin.x + 
    	       (paperViewMax.size.width-frect.size.width)/2.0);
    frect.origin.y = 
        floor(paperViewMax.origin.y + 
    	       (paperViewMax.size.height-frect.size.height)/2.0);
    [paperView moveTo:frect.origin.x:frect.origin.y];
    [paperView sizeTo:frect.size.width:frect.size.height];
    [paperView scale:frect.size.width/scaledPaperRect.size.width
    		    :frect.size.height/scaledPaperRect.size.height];
		    
    [coverText getBounds:&frect];
    frect.origin.x = textOrigin.x;
    frect.origin.y = textOrigin.y-frect.size.height;
    conformRect(&frect,&scaledPaperRect);
    [paperView setTextRect:&frect lineHeight : [coverText lineHeight]];
    if(dataPS) {
        frect = boundsPS;
	frect.origin.x = graphicsOrigin.x;
	frect.origin.y = graphicsOrigin.y-frect.size.height;
	conformRect(&frect,&scaledPaperRect);
	[paperView setGraphicsRect : &frect];
	[paperView setBackgroundData: dataPS length : lengthPS bbox : bboxPS];
    } else {
        [paperView setGraphicsRect : &empty];
	[paperView setBackgroundData: NULL length : 0 bbox : NULL];
    }
}

- cancelCover:sender
{
    faxExit = _NX_FAXCANCEL;
    [NXApp stopModal];
    return self;
}

- okCover:sender
{
    faxExit = (faxExit == _NX_FAXNOCOVER ? 
                  _NX_FAXWITHCOVER : _NX_FAXPREVIEWWITHCOVER);
    [NXApp stopModal];
    return self;
}

- coverSheet:sender
{
    id dw;
    NXHandler           exception;

    
    [self setCoverText];
    [self setCoverBackground];
    [self adjustPaperView];
	    
    exception.code = 0;
    dw = coverWindow;
    [dw display];
    [coverText selectFirstField];
    NX_DURING {
	[NXApp runModalFor:dw];
    } NX_HANDLER {
	exception = NXLocalHandler;
	if (exception.code == dps_err_ps)
	    NXReportError(&NXLocalHandler);
	[NXApp stopModal];
	faxExit = _NX_FAXCANCEL;
	dw = coverWindow;
    } NX_ENDHANDLER
    [dw close];
    if (exception.code && exception.code != dps_err_ps)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return self;
}

/*  Drive Fax Panel (Phone List) */

static void newCellAt(id matrix,int row,char *entry,int newCell)
{
    id theCell;
    
    theCell = [matrix cellAt:row :0];
    if (!newCell)
	free((char *) [theCell stringValue]);
    [theCell setStringValueNoCopy:entry];
    [theCell setParameter : NX_CELLSTATE to : 0];
    [theCell setParameter: NX_CELLHIGHLIGHTED to : 0];
    [theCell setLeaf: YES];
}

static void loadMatrix(NXZone *zone, id matrix,char *data,int length)
{
    char *person,*tab,*entry,*end;
    int bytes;
    int i,oldRowNum;
    id window;
    
    window = [matrix window];
    [window disableDisplay];
    
    oldRowNum = [[matrix cellList] count];

    i = 0;
    end = data+length;
    while(data < end) {
        tab = NULL;
	person = data;
	while(data < end && ((!tab) || *data != '\n')) {
	    if ((!tab) && *data == '\t')
	        tab = data;
	    data++;
	}
	data++;
	bytes = data-person;
	if (!tab) {
	    bytes++;
	    tab = data;
	}
	entry = NXZoneMalloc(zone, bytes);
	strncpy(entry,person,bytes-1);
	entry[tab-person] = '\0';
	entry[bytes-1] = '\0';
	
	if (i >= oldRowNum)
	    [matrix addRow];
	newCellAt(matrix,i,entry,i>=oldRowNum);	
        i++;
    }
    
    [matrix renewRows:i cols:1];
    [matrix clearSelectedCell];
    [matrix sizeToCells];
    [window reenableDisplay];
}

- loadNumbers {
    char path[MAXPATHLEN];
    struct stat st;
    int maxlen,length;
    char *dp;
    NXStream *s;
    
    s = mapByPath(faxNumbers,NX_READONLY,path);
    if (s) {
        if (stat(path,&st) >= 0) {
	    if(numbersIno != st.st_ino ||
	       numbersDev != st.st_dev ||
	       numbersMtime < st.st_mtime) {
               NXGetMemoryBuffer(s, &dp, &length, &maxlen);
               loadMatrix([self zone], numbersMatrix,dp,length);
	    }
	    numbersMtime = st.st_mtime;
	    numbersDev = st.st_dev;
	    numbersIno = st.st_ino;
	}
	NXCloseMemory(s, NX_FREEBUFFER);
    }
    return self;
}

- saveNumbers {
    NXStream *stream;
    char path[MAXPATHLEN];
    char *entry;
    int  row,numRows,numCols,nchars,ntab;
    id   cells;
    int oldMask;
    
    if (numbersChanged) {
	stream = NXOpenMemory(NULL, 0, NX_WRITEONLY);
	if (stream) {
	    cells = [numbersMatrix cellList];
	    [numbersMatrix getNumRows:&numRows numCols:&numCols];
	    for (row=0;row < numRows;row++) {
		entry = (char *)[[cells objectAt:row] stringValue];
		ntab = strlen(entry);
		nchars = ntab + 2 + strlen(entry+ntab+1);
		entry[ntab] = '\t';
		entry[nchars-1] = '\n';
		NXWrite(stream,entry,nchars);
		entry[ntab] = '\0';
		entry[nchars-1] = '\0';
	    }
	    (void) expandPath(NXGetDefaultValue(NXSystemDomainName,NX_LIBRARYPATH),
	    		      path,
			      faxNumbers);
	    oldMask= umask(0);
	    if ((createPath(path) < 0) || (NXSaveToFile(stream,path) < 0)) {
	    /*  !!! cmf !!! how does kit do error strings? */
		_NXKitAlert("Printing", NULL, "Unable to create %s.", NULL, NULL, NULL, path); /* Couldn't save the user's fax phone numbers to disk. */
	    };
	    umask(oldMask);
	    NXCloseMemory(stream, NX_FREEBUFFER);
	    numbersChanged = NO;
	    numbersMtime = time(0);
	}
    }
    return self;
}

static int CIprefix(const char *prefix, const char *string)
{
    int c1, c2;
    const char *cp1, *cp2;
    
    cp1 = prefix;
    cp2 = string;
    while (1) {
        c1 = *cp1++;
	c2 = *cp2++;
	c1 = NXToLower(c1);
	c2 = NXToLower(c2);
	if (!c1) return 0;
	if (c1 != c2) return 1;
    }
}

static int deleteANumber( id matrix, id cell, int row, void *value)
{
    free((char *)[cell stringValue]);
    [matrix removeRowAt:row andFree:YES];
    return 1;
}

static void deleteNumber(NXFaxPanel *faxPanel)
{
    id numbersMatrix;
    
    numbersMatrix = faxPanel->numbersMatrix;
    if (forSelectedCellsDo(numbersMatrix,NULL,deleteANumber)) {
	[numbersMatrix sizeToCells];
	faxPanel->numbersChanged = YES;
    }
}

- deleteNumber:sender
{
    int selectedRow,numRows,numCols;
    
    selectedRow = [numbersMatrix selectedRow];
    deleteNumber(self);
    [numbersMatrix getNumRows:&numRows numCols:&numCols];
    if (selectedRow >= numRows)
        selectedRow = numRows-1;
    [self disableFlushWindow];
    [numbersMatrix setAutodisplay:NO];
    [numbersMatrix selectCellAt:selectedRow:0];
    [numbersMatrix scrollCellToVisible:selectedRow:0];
    [numbersMatrix setNeedsDisplay : NO];
    [numbersMatrix setAutodisplay : YES];
    [self reenableFlushWindow];
    [phoneList display];
    keyPos = -1;
    [self makeFirstResponder: self];
    return self;
}

- addNumber:sender
{
    const char *person,*number;
    char *entry;
    int nameLen,row,numRows,numCols,i,count;
    id cells,priorCell;
    int rowAdded;
    
    if (sender == replaceButton)
        deleteNumber(self);
	
    person = [faxName stringValue];
    number = [faxNumber stringValue];
    
    cells = [numbersMatrix cellList];
    [numbersMatrix getNumRows:&numRows numCols:&numCols];
    for (row=0;row < numRows;row++) if (NXOrderStrings((unsigned char *)person, (unsigned char *)[[cells objectAt:row] stringValue], NO, -1, NULL) < 0) break;

    nameLen = strlen(person);
    entry = NXZoneMalloc([self zone], nameLen+strlen(number)+2);
    strcpy(entry,person);
    strcpy(entry+nameLen+1,number);
    
    [self disableFlushWindow];
    [numbersMatrix setAutodisplay:NO];

    rowAdded = NO;
    if ([cells count] <= numRows) {
	[numbersMatrix addRow];
	rowAdded = YES;
    }
    priorCell = [cells objectAt:row];
    count = [cells count];
    for (i = row+1;i < count;i++)
        priorCell = [cells replaceObjectAt:i with:priorCell];
    [cells replaceObjectAt:row with:priorCell];
    newCellAt(numbersMatrix,row,entry,rowAdded);
    [numbersMatrix renewRows:numRows+1 cols:1];
    [numbersMatrix sizeToCells];
    [numbersMatrix selectCellAt:row:0];
    [numbersMatrix scrollCellToVisible:row:0];
    [numbersMatrix setNeedsDisplay : NO];
    [numbersMatrix setAutodisplay : YES];
    [self reenableFlushWindow];
    [phoneList display];
    loadSelectedData(self);
    keyPos = -1;
    [self makeFirstResponder : self];
    numbersChanged = YES;
    return self;
}

static void selectByName(NXFaxPanel *faxPanel,char *person)
{
    int row,numRows,numCols;
    id cells;
    id numbersMatrix;
    
    numbersMatrix = faxPanel->numbersMatrix;
    cells = [numbersMatrix cellList];
    [numbersMatrix getNumRows:&numRows numCols:&numCols];
    for (row=0;row < numRows;row++) if (NXOrderStrings((unsigned char *)person, (unsigned char *)[[cells objectAt:row] stringValue], NO, -1, NULL) <= 0) break;
    if (row == numRows) row = numRows-1;

    [numbersMatrix selectCellAt:row:0];
    [numbersMatrix scrollCellToVisible:row:0];
    loadSelectedData(faxPanel);
}

static void loadSelectedData(NXFaxPanel *self)
{
    const char *entry;
    id cell;
    int nSel;
    
    nSel = forSelectedCellsDo(self->numbersMatrix,NULL,NULL);
    if (nSel == 1) {
	cell = [self->numbersMatrix selectedCell];
	if (cell) {
	    entry = [cell stringValue];
	    [self->faxName setStringValue:entry];
	    self->nameIsEmpty = !*entry;
	    while (*entry++)
		/* NULL */;
	    [self->faxNumber setStringValue:entry];
	    self->numberIsEmpty = !*entry;
	}
    } else {
        [self->faxName setStringValue:""];
	[self->faxNumber setStringValue:""];
	self->nameIsEmpty = YES;
	self->numberIsEmpty = YES;
    }
    [self enableButtonsIsMultipleSelection: nSel > 1 ? YES:NO];
}

- numberClicked:sender
{
    loadSelectedData(self);
    [self makeFirstResponder : self];
    return self;
}

- enableButtonsIsMultipleSelection: (BOOL) flag {
    int haveSelectedCell;
    int haveEntry;
    
    haveEntry = !(numberIsEmpty || nameIsEmpty);
    haveSelectedCell = [numbersMatrix selectedCell] ? 1 : 0; 
    [replaceButton setEnabled : haveSelectedCell && (!flag) && haveEntry];
    [deleteButton setEnabled : haveSelectedCell];
    [faxOkButton setEnabled : flag || !numberIsEmpty];
    [addButton setEnabled : haveEntry];
    return self;
}

- textDidGetKeys:textObject isEmpty:(BOOL)flag {
    id selectedCell;
    
    if (textObject != coverText && [textObject window] == self) {
	selectedCell = [faxForm selectedCell];
	if (selectedCell == faxName)
	    nameIsEmpty = flag;
	else if (selectedCell == faxNumber)
	    numberIsEmpty = flag;
	[self enableButtonsIsMultipleSelection:
	    forSelectedCellsDo(numbersMatrix,NULL,NULL) > 1 ? YES:NO];
    }
    return self;
}

- textDidEnd : textObject endChar : (unsigned short) whyEnd {
    const char *person,*entryName;

/*  Text is either in self or coverWindow  */

    if ([textObject window] == self && [faxForm selectedCell] == faxName) {
        person = [faxName stringValue];
	entryName = [[numbersMatrix selectedCell] stringValue];
	if (entryName && !CIprefix(person,entryName))
	    loadSelectedData(self);
    }
    return self;
}

- textDidResize:textObject oldBounds:(const NXRect *)oldBounds
    invalid:(NXRect *)invalidRect;
{
    NXRect objFrame,viewFrame;
    NXRect scaledPaperRect;
    
/*  Text is either in focusWindow,self or coverWindow */

    if ([textObject window] == coverWindow) { 
    /* resize text rect in preview to match */
	[textObject getBounds:&objFrame];
	[paperView textRect : &viewFrame];
	objFrame.origin.x = viewFrame.origin.x;
	objFrame.origin.y = viewFrame.origin.y + 
			    viewFrame.size.height - 
			    objFrame.size.height;
	conformRect(&objFrame,computeScaledPaperRect(&scaledPaperRect));
	[paperView setTextRect:&objFrame lineHeight : [textObject lineHeight]];
	[paperView display];
    }
    return self;
}

- (BOOL)textWillEnd:textObject
{
#define BUF_MAX		1024
    id control = nil;
    id cell = nil;
    char buffer[BUF_MAX];
    char *string;
    int length,blength;
    BOOL stringOK;
    const char *specialLabel = NULL;		/* first or last */

    if ([textObject window] == self) {
	if ([firstPage currentEditor])	{
	    control = firstPage;
	    specialLabel = FirstString;
	} else if ([lastPage currentEditor]) {
	    control = lastPage;
	    specialLabel = LastString;
	}
    }
    if (control) {
	cell = [control cell];
	length = [textObject textLength] + 1;
	blength = [textObject byteLength] + 1;
	if (blength > BUF_MAX)
	    NX_ZONEMALLOC([self zone], string, char, blength);
	else
	    string = buffer;
	[textObject getSubstring:string start:0 length:length];
	[cell setEntryType:NX_INTTYPE];
	stringOK = string && *string &&
	    ([cell isEntryAcceptable:string] || !strcmp(specialLabel, string));
	[cell setEntryType:NX_ANYTYPE];
	if (blength > BUF_MAX)
	    NX_FREE(string);
    } else
	stringOK = YES;
    return !stringOK;
}

- (BOOL)textWillChange:textObject
{
    if ([textObject window] == self &&
        ([firstPage currentEditor] == textObject ||
	 [lastPage currentEditor] == textObject)) {
	if ([pageMode selectedCol] == 0)	/* if needs to be changed */
	    [pageMode selectCellAt:0 :1];
	if (([firstPage currentEditor] != textObject) &&
	    (![firstPage stringValue] || !*[firstPage stringValue]))
	    [firstPage setStringValue:FirstString];
	if (([lastPage currentEditor] != textObject) &&
	    (![lastPage stringValue] || !*[lastPage stringValue]))
	    [lastPage setStringValue:LastString];
    }
    return 0;
}

- cancelFax:sender
{
    if (![modemButton isEnabled]) /* we are in modal session */
	NX_RAISE(NX_abortPrinting, NULL, NULL);
    faxExit = _NX_FAXCANCEL;
    [NXApp stopModal];
    return self;
}

- okFax:sender
{
    
    faxExit = _NX_FAXNOCOVER;
    [self makeFirstResponder:self];
    [NXApp stopModal];
    return self;
}

- modemHit:sender
{
/*  !!! we must enforce only fax modems at this point.
    !!! somehow we must not blast printInfo */
    [[ChoosePrinter new] runModalFax];
    updateTitlesAndOptions(self);
    return self;
}

- previewHit:sender
{

    faxExit = _NX_FAXPREVIEWNOCOVER;
    [NXApp stopModal];
    return self;
}

- pageModeHit:sender
{
    if ([pageMode selectedCol] == 0) {
	[firstPage setStringValue:NULL];
	[lastPage setStringValue:NULL];
    } else if (![firstPage stringValue] && ![lastPage stringValue]) {
	[firstPage setStringValue:FirstString];
	[lastPage setStringValue:LastString];
	[firstPage selectText:self];
    }
    return self;
}


- (BOOL)readPSFromStream:(NXStream *)stream
/*
 * Gets the bounding box from the conforming PostScript comments
 * being careful to skip over included PostScript files.
 */
{
    char *dp,*data;
    int subfiles = 0, matches = 0;
    int maxlen,length;
    float bbox[4];
    
    dataPS = NULL;
    NXGetMemoryBuffer(stream, &data, &length, &maxlen);

    if (length) {
	dp = data;
	while (dp && !matches) {
	    if (*dp++ == '%' && *dp++ == '%') {
		if (subfiles && *dp == 'E') {
		    if (!strncmp(dp, "EndFile", 7)) subfiles--;
		} else if (*dp == 'B') {
		    if (!strncmp(dp, "BeginFile:", 10)) {
			subfiles++;
		    } else if (!subfiles) {
			matches = sscanf(dp, "BoundingBox: %f %f %f %f\n",
			    &bbox[0], &bbox[1], &bbox[2], &bbox[3]);
			if ((matches && matches != 4) ||
			    (matches == 4 &&
			    (bbox[2] == bbox[0] || bbox[3] == bbox[1]))) {
			    /* error */;
			    matches = 1;
			}
		    }
		}
	    }
	    dp = strchr(dp, '\n');
	    if (dp) dp++;
	}
	if (!matches)
	    /* error */;
    }

    if (matches == 4) {
        dataPS = data;
	bcopy(bbox,bboxPS,sizeof(bbox));
	lengthPS = length;
	boundsPS.origin.x = boundsPS.origin.y = 0.0;
	boundsPS.size.width = floor(bbox[2] - bbox[0] + 0.99);
	boundsPS.size.height = floor(bbox[3] - bbox[1] + 0.99);
    }

    return dataPS ? YES : NO;
}

- coverBBox : (NXRect *) boundingBox
{
    (void) computeScaledPaperRect(boundingBox);
    return self;
}

- writeCoverSheet
{
    id view,saveView;
    NXRect frameR,objFrame,viewFrame;
    char strRect[100];
    NXPoint textOrigin,graphicsOrigin;
    PrivatePrintInfo   *privPInfo = [[NXApp printInfo] _privateData];
    	
    [focusWindow disableDisplay];
    [self coverBBox : &frameR];
    view = [NXFaxCoverView allocFromZone:[self zone]];
    [view initFrame : &frameR];
    [[focusWindow contentView] addSubview : view];
    [coverText setAutodisplay : NO];
    [coverText setPages : 1 + (privPInfo->numPages >= 0 ? 
    				privPInfo->numPages :
				privPInfo->ordSheet)];
    [coverText getBounds: &objFrame];
    [paperView textRect : &viewFrame];
    textOrigin.x = viewFrame.origin.x;
    textOrigin.y = viewFrame.origin.y + viewFrame.size.height;
    [view setCoverText : coverText 
	  at : textOrigin.x : textOrigin.y-objFrame.size.height];
    [paperView graphicsRect : &objFrame];
    graphicsOrigin.x = objFrame.origin.x;
    graphicsOrigin.y = objFrame.origin.y + objFrame.size.height;
    if (dataPS) {
	[view setBackgroundData: dataPS length : lengthPS bbox : bboxPS];
	[view setGraphicsRect : &objFrame];
    }
    [focusWindow reenableDisplay];

     /*
      * focus on view with special printing focus, which skips xforms from the
      * View's parents. 
      */
    saveView = privPInfo->currentPrintingView;
    privPInfo->currentPrintingView = view;
    privPInfo->specialPrintFocus = YES;
    viewFrame = privPInfo->currentPrintingRect;
    privPInfo->currentPrintingRect = frameR;    
    if (streamPS)
        useFonts([self zone], streamPS);
    [view lockFocus];
    [view display : &frameR : 1];
    [view unlockFocus];
    privPInfo->currentPrintingView = saveView;
    privPInfo->currentPrintingRect = viewFrame;
    [coverText removeFromSuperview];

    sprintf(strRect,"%f %f %f %f",
	    textOrigin.x,textOrigin.y,graphicsOrigin.x,graphicsOrigin.y);
    NXWriteDefault(NXSystemDomainName,NX_FAXORIGINS,strRect);
    return self;
}

static void printFieldItem( char *format,const char *stringValue, BOOL swap)
{
    char line[256];
    char swapBuffer[256];
    int maxLen;
    int i;
    
    maxLen = 255-strlen(format)+2 /* %s */;
    if (stringValue) {
        line[maxLen] = '\0';
        strncpy(line,stringValue,maxLen);
    } else
	line[0]= '\0';
    maxLen = strlen(line);
    for (i=0; i < maxLen;i++)
        if (line[i] == '\n')
	    line[i] = ' ';
    if (swap)
        swapFirstAndLast(line,swapBuffer);
    DPSPrintf(DPSGetCurrentContext(), format, line);
}

static int printNumberField( id matrix, id cell, int row, void *value)
{
    const char *stringValue;
    char *format;
    
    format = (char *) value;
    stringValue = [cell stringValue];
    if (!stringValue)
        stringValue = "";
    while (*stringValue++)
	/* NULL */;
    printFieldItem(format,stringValue,NO);
    return 0;
}

static int printNameField( id matrix, id cell, int row, void *value)
{
    const char *stringValue;
    char *format;
    
    format = (char *) value;
    stringValue = [cell stringValue];
    if (!stringValue)
        stringValue = "";
    printFieldItem(format,stringValue,YES);
    return 0;
}

-  writeFaxNumberList: (char *) format;
{
    const char *number;
    BOOL needNumber = YES;
    
    number = [faxNumber stringValue];
    if (!(*number)) {
        if (forSelectedCellsDo(numbersMatrix,format,printNumberField) > 0)
	    needNumber = NO;
    }
    if (needNumber)
        printFieldItem(format,number,NO);
    return self;
}

- writeFaxToList: (char *) format;
{
    const char *person;
    BOOL needPerson = YES;
    
    person = [faxName stringValue];
    if (!(*person)) {
        if (forSelectedCellsDo(numbersMatrix,format,printNameField) > 0)
	    needPerson = NO;
    }
    if (needPerson)
        printFieldItem(format, person,YES);
    return self;
}

- (BOOL) notifyIsChecked
{
   return [notifyCheckBox state] ? YES : NO;
}

- (BOOL) hiresIsChecked
{
   return [hiresCheckBox state] ? YES : NO;
}

- keyDown:(NXEvent *)theEvent
{
    NXEvent *tempEv, tempEvSpace, charEvent;
    unsigned short c;
    
    charEvent = *theEvent;
    while (1) {
	c = charEvent.data.key.charCode;
	if (charEvent.data.key.charSet == NX_ASCIISET && c >= ' ') {
	    if (keyPos >= MAXMATCH || charEvent.time > keyTime+KEYINTERVAL)
		keyPos = -1;
	    keyBuffer[++keyPos] = c;
	    keyBuffer[keyPos+1] = '\0';
	    keyTime = theEvent->time;
	    selectByName(self,keyBuffer);
	} else
	    keyPos = -1;
	while (1) {
	    tempEv = [NXApp peekNextEvent:NX_ALLEVENTS into:&tempEvSpace];
	    if (!tempEv || !(NX_EVENTCODEMASK(tempEv->type) &
			     (NX_KEYUPMASK | NX_KEYDOWNMASK)))
		goto breakLoop;
	    if (tempEv->type == NX_KEYUP)
		[NXApp getNextEvent:NX_KEYUPMASK];
	    else {
	        charEvent = tempEvSpace;
		[NXApp getNextEvent:NX_KEYDOWNMASK];
		break;
	    }
	}
    }
breakLoop:
    if (keyPos >= 0)
	selectByName(self,keyBuffer);
    return self;
}

- readPrintInfo
{
    PrintInfo *pInfo = [NXApp printInfo];

    [pInfo _updateFaxInfo];     /* get name, type, and res in sync */
    updateTitlesAndOptions(self);
    [pageMode selectCellAt:0 :0];
    [firstPage setStringValue:NULL];
    [lastPage setStringValue:NULL];
    return self;
}


- writePrintInfo
{
    PrintInfo *pInfo = [NXApp printInfo];
    BOOL                allPages;
    int			fieldVal;

    allPages = [pageMode selectedCol] == 0;
    [pInfo setAllPages:allPages];
    if (!allPages) {
	getFieldValue(firstPage, FirstString, MININT, &fieldVal);
	[pInfo setFirstPage:fieldVal];
	getFieldValue(lastPage, LastString, MAXINT, &fieldVal);
	[pInfo setLastPage:fieldVal];
    }
    [pInfo setCopies:1];
    [pInfo setManualFeed:NO];
    [pInfo setResolution:0];
    [[PrintPanel new] _copyFromFaxPanelPageMode:pageMode 
	              firstPage : firstPage 
		      lastPage: lastPage];

    return self;
}

- (int) runModal {
    const char *s;
    int doCover;
    const char *person,*number;
    NXHandler           exception;
    
    exception.code = 0;

    [NXApp _orderFrontModalWindow:self];
    [[PrintPanel new] orderOut:self];

    faxExit = _NX_FAXCANCEL;
    [self loadNumbers];
    [[[numbersMatrix superview] superview] display];
    
    if (!(s = NXUpdateDefault(NXSystemDomainName,NX_FAXWANTSCOVER)))
        s = NXGetDefaultValue(NXSystemDomainName,NX_FAXWANTSCOVER);
    [coverCheckBox setState : (s && *s == 'Y') ? 1 : 0];
    if (!(s = NXUpdateDefault(NXSystemDomainName, NX_FAXWANTSNOTIFY)))
        s = NXGetDefaultValue(NXSystemDomainName, NX_FAXWANTSNOTIFY);
    [notifyCheckBox setState : (s && *s == 'Y') ? 1 : 0];
    if (!(s = NXUpdateDefault(NXSystemDomainName, NX_FAXWANTSHIRES)))
        s = NXGetDefaultValue(NXSystemDomainName, NX_FAXWANTSHIRES);
    [hiresCheckBox setState : (s && *s == 'Y') ? 1 : 0];
    keyPos = -1;
    [self makeFirstResponder : self];
    person = [faxName stringValue];
    number = [faxNumber stringValue];
    nameIsEmpty = !*person;
    numberIsEmpty = !*number;
    [self disableFlushWindow];
    [self _setControlsEnabled:YES];
    [self reenableFlushWindow];
    [self readPrintInfo];
    [status setStringValueNoCopy:""];
    NXPing();
rerunPanel:
    NX_DURING {
	[NXApp runModalFor:self];
    } NX_HANDLER {
	exception = NXLocalHandler;
	if (exception.code == dps_err_ps)
	    NXReportError(&NXLocalHandler);
	[NXApp stopModal];
	faxExit = _NX_FAXCANCEL;
    } NX_ENDHANDLER
    if (faxExit != _NX_FAXCANCEL) {
	if (![[NXApp printInfo] printerName] && faxExit == _NX_FAXNOCOVER) {
	    if (_NXKitAlert("Printing", "Fax", "You must choose a modem in order to fax.", "OK", "Cancel", NULL) == NX_ALERTDEFAULT) {
		[self makeFirstResponder : self];
		goto rerunPanel;
	    } else
		faxExit = _NX_FAXCANCEL;
	} else
	    [self writePrintInfo];
    }
    if (exception.code && exception.code != dps_err_ps)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    
    [self saveNumbers];
    if (faxExit != _NX_FAXCANCEL) {
    /*  remember if notify is checked */
	NXWriteDefault(NXSystemDomainName,NX_FAXWANTSNOTIFY, [notifyCheckBox state] ? "YES" : "NO");

    /*  remember if hires is checked */
	NXWriteDefault(NXSystemDomainName,NX_FAXWANTSHIRES,  [hiresCheckBox state] ? "YES" : "NO");
		       
    /*  prompt for cover sheet if desired */
    
	doCover = [coverCheckBox state];
	NXWriteDefault(NXSystemDomainName,NX_FAXWANTSCOVER,doCover ? "YES" : "NO");
    
    /*  we unplug coverText when not showing cover panel, because we reuse coverText
	to generate the (unscaled) cover sheet in writeCoverSheet
     */
     
	if ([coverScroller docView] == NULL) {
	    (void) [coverScroller setDocView : coverText];
	    [coverText setDelegate : self];
	}
	if (doCover)
	    [self coverSheet:NULL];
	[coverScroller setDocView : NULL];
	[coverText setDelegate : NULL];
    }
    return faxExit;
}

- _setControlsEnabled:(BOOL)flag
{
    [pageMode setEnabled:flag];
    [firstPage setEnabled:flag];
    [lastPage setEnabled:flag];
    [preview setEnabled:flag];
    [modemButton setEnabled:flag];
    [faxNumber setEnabled:flag];
    [faxName setEnabled:flag];
    [phoneList setEnabled:flag];
    [coverCheckBox setEnabled:flag];
    [notifyCheckBox setEnabled:flag];
    [hiresCheckBox setEnabled:flag];
    if (flag)
	[self enableButtonsIsMultipleSelection:
	    forSelectedCellsDo(numbersMatrix,NULL,NULL) > 1 ? YES:NO];
    else {
	[faxOkButton setEnabled:flag];
	[addButton setEnabled:flag];
	[replaceButton setEnabled:flag];
	[deleteButton setEnabled:flag];
    }
    return self;
}


- _updatePrintStat:(const char *)statusString label:(const char *)label
{
    DPSContext          newCtxt, oldCtxt;
    char		buf[256];
    short		oldStatus;		

    oldCtxt = DPSGetCurrentContext();
    newCtxt = [NXApp context];
    DPSSetContext(newCtxt);	/* talk to the server */
    oldStatus = NXDrawingStatus;
    NXDrawingStatus = NX_DRAWING;
    if (label) {
	sprintf(buf, "%s %4s", statusString, label);
	[status setStringValue:buf];
    } else
	[status setStringValue:statusString];
    NXPing();
    NXDrawingStatus = oldStatus;
    DPSSetContext(oldCtxt);	/* talk to printer */
    return self;
}


/* parses a string from one of the From/To fields.  Returns whether
   a value was successfully extracted.
 */
static BOOL
getFieldValue(Control *ctrl, const char *altString, int altValue,
						int *resultValue)
{
    const char *fieldText;

    fieldText = [ctrl stringValue];
    if (fieldText && !strcmp(altString, fieldText))
	*resultValue = altValue;
    else if (!fieldText || (sscanf(fieldText, "%d", resultValue) != 1))
	return NO;
    return YES;
}

static void
updateTitlesAndOptions(NXFaxPanel *self)
{
    id pInfo = [NXApp printInfo];
    const char *name;

    name = [pInfo printerName];
    if (!name) name = KitString(Printing, "(No modem chosen)", "Message given to user when no fax modem is currently chosen.");
    [self->name setStringValue:name];
}

/*  states of doc fonts FSM */

#define COMMENTSTART 0
#define LINETOSS 1
#define FONTNAME 2
#define WHITESPACE 3
#define EXTENDSTART 4
#define EOLTOSS 5
#define EOLSCAN 6

static void useFonts(NXZone *zone, NXStream *rstream)
{
    int  nFonts;
    int  nSlots;
    char **fontNames,**fn,**fl;
    int  nDocFontComments;
    char c;
    int state;
    char *cmatch;
    char name[256]; /* Conforming PS - lines < 256 chars */
    int  nchars = 0;
    int  i;
        
    nDocFontComments = 0;
    nFonts = 0;
    nSlots = 100;
    fontNames = NXZoneMalloc(zone, nSlots*sizeof(char *));
    state = COMMENTSTART;
    cmatch = "%%DocumentFonts:";
    
    NXSeek(rstream,0,NX_FROMSTART);
    while (1) {
        c = NXGetc(rstream);
	switch (state) {
	case COMMENTSTART:
	    if (c == *cmatch++) {
	        if (!*cmatch) {
		    nDocFontComments++;
		    state = FONTNAME;
		    nchars = 0;
		    break;
		}
	    } else {
	        if (c == EOF)
		    goto breakLoop;
		state = LINETOSS;
	    }
	    break;
	case LINETOSS:
	    if (c == 13 || c == 10) /* eol chars */
	        state = EOLTOSS;
	    break;
	case FONTNAME:
	    if (c == ' ' || c == '\t' || c == 13 || c == 10 || c == EOF) {
	        state = (c == ' ' || c == '\t') ? WHITESPACE : EOLSCAN;
		if (nchars) {
		    name[nchars++] = '\0';
		    if (strcmp(name,"(atend)")) {
		        fn = fontNames;
			fl = fn+nFonts;
			while (fn < fl) {
			    if (!strcmp(*fn,name))
				goto exitFor;
			    fn++;
			}
			i = nFonts++;
			if (nFonts == nSlots) {
			    nSlots += 10;
			    fontNames = NXZoneRealloc(zone,
						 fontNames,nSlots*sizeof(char *));
			}
			fontNames[i] = NXZoneMalloc(zone, nchars);
			strcpy(fontNames[i],name);
		    exitFor:
			/* NULL */;
		    }
		}
		if (c == EOF)
		    goto breakLoop;
	    } else {
		if (nchars < 256) {
		    name[nchars] = c;
	            nchars++;
		}
            }
	    break;
	case WHITESPACE:
	    if (c == EOF)
		goto breakLoop;
	    if (c != ' ' && c != '\t') {
	        NXUngetc(rstream);
	        state = FONTNAME;
		nchars = 0;
	    }
	    break;
	case EXTENDSTART:
	    if (c == *cmatch++) {
	        if (!*cmatch) {
		    state = FONTNAME;
		    nchars = 0;
		    break;
		}
	    } else {
	        if (c == EOF)
		    goto breakLoop;
		if (!*cmatch && c == 'D') {
		    cmatch = "ocumentFonts:";
		    state = COMMENTSTART;
		} else
		    state = LINETOSS;
	    }
	    break;
	case EOLTOSS:
	    if (c != 13 && c != 10) { /* new line starts */
	        state = COMMENTSTART;
		cmatch = "%%DocumentFonts:";
		NXUngetc(rstream);
            }
	    break;
	case EOLSCAN:
	    if (c != 13 && c != 10) { /* new line starts */
	        state = EXTENDSTART;
		cmatch = "%%+";
		NXUngetc(rstream);
            }
	    break;
	}
    }
breakLoop:

    fn = fontNames;
    fl = fn+nFonts;
    while (fn < fl) {
        [Font useFont:*fn];
	free(*fn);
	fn++;
    }
    free(fontNames);
}

static void conformRect(NXRect *aRect,const NXRect *bounds)
{
    aRect->origin.x = floor(aRect->origin.x);
    aRect->origin.y = floor(aRect->origin.y);
    aRect->size.width = ceil(aRect->size.width);
    aRect->size.height = ceil(aRect->size.height);
    if (aRect->origin.x + aRect->size.width > bounds->origin.x+bounds->size.width)
        aRect->origin.x = bounds->size.width+bounds->origin.x - aRect->size.width;
    if (aRect->origin.x < bounds->origin.x)
        aRect->origin.x = bounds->origin.x;
    if (aRect->origin.y + aRect->size.height > bounds->origin.y+bounds->size.height)
        aRect->origin.y = bounds->size.height+bounds->origin.y - aRect->size.height;
    if (aRect->origin.y < bounds->origin.y)
        aRect->origin.y = bounds->origin.y;

}

static const char *expandPath(const char *src,char *dst,const char *name)
{
    register char  *ps2;
    const char *h;
	
    ps2 = &dst[0];
    if (*src == ':')
	src++;
    if (*src == '~') { /* accept home directory */
	src++;
	h = NXHomeDirectory();
	while (*ps2++ = *h++) ;
	ps2--;
    }
    while (*src && (*src != ':'))
	*ps2++ = *src++;
    *ps2++ = '/';
    *ps2 = '\0';
    strcat(dst,name);
    return src;
}

static NXStream *mapByPath(const char *name,
			   int option,
			   char *mPath)
{
    char path[MAXPATHLEN];
    register const char  *ppath;
    NXStream *s;
    
    ppath = NXGetDefaultValue(NXSystemDomainName,NX_LIBRARYPATH);
    
    while (*ppath) {
        ppath = expandPath(ppath,path,name);
	if (s = NXMapFile(path,option)) {
	    if (mPath)
	        strcpy(mPath,path);
	    return s;
	}
    } /* end while */
    
    return NULL;
}

static int createPath(const char *path)
{
/*  This routine takes only absolute pathes */

    char partial[MAXPATHLEN];
    const char *src,*lastComponent;
    int  len;
    
    lastComponent = path+strlen(path);
    while(*--lastComponent != '/')
        /* NULL */;
    if (lastComponent == path) { /* path == "/" */
        partial[0] = '/';
	partial[1] = '\0';
    } else {
        len = lastComponent-path;
        bcopy(path,partial,len);
	partial[len] = '\0';
    }
        
    if (access(partial, R_OK) >= 0)
        return 0;
    if (errno != ENOENT)
        return -1;
    
    src = path+1;
    while (src < lastComponent) {
	while (*src != '/')
	    src++;
	len = src-path;
	bcopy(path,partial,len);
	partial[len] = '\0';
	if (access(partial, F_OK) < 0) {
	    if (errno != ENOENT)
		return -1;
	    if (mkdir(partial,0777) < 0)
	        return -1;
	}
	src++;
    }
    return 0;
}

static NXRect *computeScaledPaperRect(NXRect *scaledPaperRect)
{
    id pInfo;
    const NXRect *paperRect;
    float scalingFactor;
    
    pInfo = [NXApp printInfo];
    paperRect = [pInfo paperRect];
    scalingFactor = [pInfo scalingFactor];
    scaledPaperRect->origin = paperRect->origin;
    scaledPaperRect->size.width = paperRect->size.width / scalingFactor;
    scaledPaperRect->size.height = paperRect->size.height / scalingFactor;
    return scaledPaperRect;
}

static int forSelectedCellsDo(id matrix, void *value,matrixCellProc func)
{
    id cells;
    int selectedRows = 0;
    id cell;
    int row,numRows,numCols;
    int retValue;
    
    cells = [matrix cellList];
    [matrix getNumRows:&numRows numCols:&numCols];
    row = 0;
    while (row < numRows) {
	cell = [cells objectAt:row];
	if ([cell state]) {
	    selectedRows++;
	    if (func != NULL) {
	        retValue = func(matrix,cell,row,value);
		if (retValue > 0) {
		    numRows--;
		    row--;
		} else if (retValue < 0)
		    break;
	    }
	}
	row++;
    }
    return selectedRows;
}

/*
  
79  
--
 		Created by cmf
80
--
 4/??/90 chris	Fixed bug where typing didn't scroll name list after editing
  		name or number then clicking back on name list
 4/??/90 chris	Changed a couple of getdefaultvalues to updatedefault:coversheet
	        check box and graphics and text positioning variables.
81  
--
 4/??/90 chris	Changed it so date is set in cover sheet before putting it up. 
 
82
--
 4/17/90 chris	Made changes is use of defaults, per trey's request.
 
83
--
 4/26/90 chris	Added notify on completion check box to NXFaxPanel.
 4/26/90 chris  Fix newIfFax: to deal with NULL printerType.
 4/27/90 chris  Fix use of NXUpdateDefault()
 
85
--
 5/25/90 chris	Fixed setCoverBackground to recognize when Background eps file has
 		been deleted while we run.
		
88
--
 6/27/90 chris	Modified to support Fax and Printer dichotomy.

89
--
 7/23/90 chris	Fixed ok to commit typing.  
 		Added Hi-res button.
 7/30/90 chris  Set umask before creating fax path or phone list. this is
 		necessary to deal with setuid processes.

92
--
 8/20/90 chris	Fix cancel button not working during generation of PS
 
94
--
 9/17/90 chris	Put in kludge to copy pageMode,firstPage,lastPage to
 		PrintPanel to deal with FrameMaker kludgery of reading
		PrintPanel.
 9/25/90 greg	use byteLength instead of textLength

95
--
 9/27/90 chris	Put in code for multiple selection and passing to name(s)
 		in PS comments.
100
--
 10/24/90 chris	fixed coversheet pages for case where last doc page
 		is not known (numPages = -1)
		
105
---
 11/6/90 glc	use blength not length
 
		
*/
@end

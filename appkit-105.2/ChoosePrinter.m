/*
	ChoosePrinter.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "ChoosePrinter.h"
#import "Panel_Private.h"
#import "PrintInfo_Private.h"
#import "PrintPanel_Private.h"
#import "Application_Private.h"
#import "View.h"
#import "Bitmap.h"
#import "Button.h"
#import "ButtonCell.h"
#import "Application.h"
#import "ScrollView.h"
#import "Matrix.h"
#import "PrintInfo.h"
#import "Text.h"
#import "ColumnCell.h"
#import "Box.h"
#import <objc/List.h>
#import <defaults.h>
#import "nextstd.h"
#import "errors.h"
#import <ctype.h>
#import <dpsclient/dpsclient.h>
#import <printerdb.h>

static void initPrinterList(id self);
static void updateTitles(id self);

/* the factory object will only create one instance of this panel */
static ChoosePrinter *UniqueInstance = NULL;

/* factory used to create the instance */
static ChoosePrinter *UniqueFactory = NULL;

#define CHOOSE_PRINTER_TITLE KitString(Printing, "Choose Printer", "Title of panel used to choose a printer.")
#define CHOOSE_MODEM_TITLE KitString(Printing, "Choose Fax Modem", "Title of panel used to choose a modem.")

@implementation ChoosePrinter:Panel

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
    return [self newContent:NULL style:0 backing:0 buttonMask:0 defer:NO];
}


+ newContent:(const NXRect *)contentRect style:(int)aStyle
	backing:(int)bufferingType buttonMask:(int)mask defer:(BOOL)flag
{
    if (!UniqueInstance) {
	if (UniqueFactory)
	    return _NXCallPanelSuper(UniqueFactory, _cmd, contentRect, aStyle, bufferingType, mask, flag);
	else {
	    UniqueFactory = self;
	    UniqueInstance = _NXLoadNibPanel("ChoosePrinter");
	    UniqueFactory = nil;
	    if (UniqueInstance) {
		[UniqueInstance->ok setTag:NX_OKTAG];
		[UniqueInstance->cancel setTag:NX_CANCELTAG];
		initPrinterList(UniqueInstance);
		{		/* ???this can go when nib lets me assign
				 * this name */
		    [UniqueInstance->appIcon setIcon:"app"];
		}
	    }
	}
    }
    return UniqueInstance;
}


- free
{
    UniqueInstance = NULL;
    [appIcon free];
    [ok free];
    [cancel free];
    [border free];		/* free up list ??? */
    return[super free];
}


- setAccessoryView:aView
{
    return [self _doSetAccessoryView:aView topView:border bottomView:ok
						oldView:&accessoryView];
}


- accessoryView
{
    return accessoryView;
}


- pickedButton:sender
{
    int tag = [sender tag];

    if (tag != NX_OKTAG || [self makeFirstResponder:self]) {
	exitTag = tag;
	[NXApp stopModal];		/* exit modal run */
    }

    return self;
}


- pickedList:sender
{
    updateTitles(self);
    [sender allowEmptySel:NO];
    return self;
}


- _doubleClickedList:sender

{
    updateTitles(self);
    [sender allowEmptySel:NO];
    [ok performClick:sender];
    return self;
}


+ _buildPrinterMatrix:(const NXRect *)aFrame tabs:(const float *)tabs zone:(NXZone *)zone
{
    NXSize cellSize;
    id aCell, matrix;
    char dumbString[2];

    aCell = [[ColumnCell allocFromZone:zone] init];
    [aCell setNumColumns:NUM_COLS];
    [aCell setTabs:tabs];
    [aCell setBordered:YES];
    matrix = [[Matrix allocFromZone:zone] initFrame:aFrame mode:NX_RADIOMODE prototype:aCell numRows:0 numCols:1];
    [matrix allowEmptySel:YES];
    [matrix setAutoscroll:YES];
    dumbString[0] = 'x';
    dumbString[1] = '\0';
    [aCell setStringValue:dumbString];
    [aCell calcCellSize:&cellSize];
    [aCell setStringValue:NULL];
    cellSize.width = aFrame->size.width;
    [matrix setCellSize:&cellSize];

    return matrix;
}


static int cellcmp(void *cell1Arg, void *cell2Arg)
{
    id *cell1 = cell1Arg;
    id *cell2 = cell2Arg;
    const char *host1, *host2;
    const char **data1, **data2;
    BOOL host1islocal, host2islocal;

    data1 = [*cell1 data];
    data2 = [*cell2 data];
    if (!data1 && data2) return 1;
    if (data1 && !data2) return -1;
    if (!data1 && !data2) return 0;
    host1 = data1[HOST];
    host2 = data2[HOST];
    host1islocal = (!host1 || !strcmp(host1, _NXHostName()));
    host2islocal = (!host2 || !strcmp(host2, _NXHostName()));
    if (host1islocal && !host2islocal) return -1;
    if (host2islocal && !host1islocal) return 1;

    return NXOrderStrings((const unsigned char *)data1[NAME], (const unsigned char *)data2[NAME], NO, -1, NULL);
}

static void sortMatrix(id matrix)
{
    int rows;
    id *cells;
    id cellList;

    cellList = [matrix cellList];
    if (cellList) {
	cells = NX_ADDRESS(cellList);
	[matrix getNumRows:&rows numCols:NULL];
	if (cells) qsort(cells, rows, sizeof(id), cellcmp);
    }
}

/* gets a list of available printers and stuffs them into the matrix.
   Does no drawing.
 */
+ (void)_loadPrinterList:matrix isPrinter:(BOOL)printerFlag orderIt:(BOOL)orderIt
{
    id           aCell;
    int          cellCount;
    int          propCount;
    const prdb_ent *entry;
    prdb_property *prop;
    char       **strPtr;
    const char  *cellData[NUM_COLS];
    char         portName[128];
    int          i;
    BOOL	 ignorePrinter;

    [matrix renewRows:0 cols:1];
    prdb_set(NULL);
    cellCount = 0;
    while (entry = prdb_get()) {
	for (strPtr = entry->pe_name; strPtr[1]; strPtr++)
	    ;
	cellData[NAME] = *strPtr;
	cellData[TYPE] = "Unknown";
	cellData[HOST] = _NXHostName();
	cellData[PORT] = NULL;
	cellData[NOTE] = NULL;
	ignorePrinter = NO;
	for (propCount = entry->pe_nprops, prop = entry->pe_prop;
	     propCount--; prop++) {
	    if (!strcmp(prop->pp_key, "ty"))
		cellData[TYPE] = prop->pp_value;
	    else if (!strcmp(prop->pp_key, "rm"))
		cellData[HOST] = prop->pp_value;
	    else if (!strcmp(prop->pp_key, "lp") && prop->pp_value) {
		if (!strcmp(prop->pp_value, "/dev/null"))
		    cellData[PORT] = "Printer";
		else if (!strncmp(prop->pp_value, "/dev/tty", 8)) {
		    cellData[PORT] = portName;
		    sprintf(portName, "Serial %c", toupper(prop->pp_value[8]));
		} else
		    cellData[PORT] = prop->pp_value;
	    } else if (!strcmp(prop->pp_key, "note"))
		cellData[NOTE] = prop->pp_value;
	    else if (!strcmp(prop->pp_key, "_ignore")) {
		/* We're supposed to ignore this printer, try again */
		ignorePrinter = YES;
		break;
	    }
		   
	}
	
	if (!ignorePrinter && (printerFlag != _NXIsFaxType(cellData[TYPE]))) {
	    if (cellCount && orderIt) [NXApp _orderFrontModalWindow:[matrix window]];
	    [matrix addRow];
	    aCell = [matrix cellAt:cellCount :0];
	    for (i = 0; i < NUM_COLS; i++)
		if (!cellData[i])
		    cellData[i] = "";
	    [aCell setData:cellData];
	    cellCount++;
	}
    }
    prdb_end();
    sortMatrix(matrix);
    [matrix sizeToCells];
}


- readPrintInfo:(BOOL) printerFlag
{
    const char	*printerName;
    const char	*printerHost;
    int          rows, cols;
    const char * const *printerData;
    int		 scrollRow;
    const char  *nameDefault;
    const char  *hostDefault;
    id list = [[border contentView] docView];
    
    if (printerFlag) {
        nameDefault = "Printer";
	hostDefault = "PrinterHost";
    } else {
        nameDefault = "Fax";
	hostDefault = "FaxHost";
    }
    if (!(printerName = NXUpdateDefault(NXSystemDomainName, nameDefault)))
	printerName = NXGetDefaultValue(NXSystemDomainName, nameDefault);
    if (!(printerHost = NXUpdateDefault(NXSystemDomainName, hostDefault)))
	printerHost = NXGetDefaultValue(NXSystemDomainName, hostDefault);
  /* remember these so we know whether they need to be written later */
    if (_lastValues) {
	_lastValues[0] = printerName;
	_lastValues[1] = printerHost;
    }

    [list getNumRows:&rows numCols:&cols];
    if (printerHost) {
	if (!*printerHost)
	    printerHost = _NXHostName();   /* set to real local host name */
	while (rows--) {
	    printerData = [[list cellAt:rows :0] data];
	    if (!strcmp(printerName, printerData[NAME]) &&
		!strcmp(printerHost, printerData[HOST]))
		break;
	}
    } else
	rows = -1;
    [self disableDisplay];
    if (rows >= 0)
	scrollRow = rows;
    else {
	[list allowEmptySel:YES];
	scrollRow = 0;
    }
    [list selectCellAt:rows :0];	/* a must for for updateTitles */
    [list scrollCellToVisible:scrollRow :0];
    [self reenableDisplay];
    [[border contentView] display];

    if (rows >= 0) {
	[list allowEmptySel:NO];
	updateTitles(self);
    } else {
	[name setStringValue:NULL];
	[type setStringValue:NULL];
	[note setStringValue:NULL];
    }

    return self;
}

- readPrintInfo
{
    return [self readPrintInfo:YES];
}

+ _writePrintInfo:(BOOL)printerFlag lastValues:(const char **)lastValues cell:cell
{
    id           pInfo = [NXApp printInfo];
    const char * const *data;
    const char  *defHost;	/* host name to write to defaults */
    char	emptyString = '\0';

    if (cell) {
	data = [cell data];
	[pInfo setPrinterName:data[NAME]];
	[pInfo setPrinterType:data[TYPE]];
	[pInfo setPrinterHost:data[HOST]];
	if (!lastValues || strcmp(data[NAME], lastValues[0]))
	    NXWriteDefault(NXSystemDomainName, (printerFlag ? "Printer":"Fax"), data[NAME]);
	if (!strcmp(data[HOST], _NXHostName()))
	    defHost = &emptyString;
	else
	    defHost = data[HOST];
	if (!lastValues || strcmp(defHost, lastValues[1]))
	    NXWriteDefault(NXSystemDomainName, (printerFlag ? "PrinterHost":"FaxHost"), defHost);
    } else {
	[pInfo setPrinterName:NULL];
	[pInfo setPrinterType:NULL];
	[pInfo setPrinterHost:NULL];
    }

    return self;
}

- writePrintInfo:(BOOL)printerFlag
{
    return [[self class] _writePrintInfo:printerFlag lastValues:_lastValues cell:[[[border contentView] docView] selectedCell]];
}

- writePrintInfo
{
    return [self writePrintInfo:YES];
}

- (int)runModalIsPrinter:(BOOL) printerFlag
{
    NXHandler exception;
    const char *lastValuesStorage[2];

    _lastValues = lastValuesStorage;
    exception.code = 0;
    [[contentView findViewWithTag:NX_CPTITLEFIELD] setStringValueNoCopy:(printerFlag ? CHOOSE_PRINTER_TITLE : CHOOSE_MODEM_TITLE)];
    [self disableDisplay];
    [[self class] _loadPrinterList:[[self->border contentView] docView] isPrinter:printerFlag orderIt:NO];
    [self reenableDisplay];
    [self readPrintInfo:printerFlag];
    NXPing();
    NX_DURING {
	[NXApp runModalFor:self];
    } NX_HANDLER {
	exception = NXLocalHandler;
	if (exception.code == dps_err_ps)
	    NXReportError(&NXLocalHandler);
	[NXApp stopModal];
	exitTag = NX_CANCELTAG;
    } NX_ENDHANDLER
    if (exitTag == NX_OKTAG)
	[self writePrintInfo:printerFlag];
    _lastValues = NULL;
    [self orderOut:self];
    if (exception.code && exception.code != dps_err_ps)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return exitTag;
}

- (int)runModal
{
   return [self runModalIsPrinter:YES];
}

- (int)runModalFax
{
   return [self runModalIsPrinter:NO];
}


/* gets a list of available printers and stuffs them into the matrix */
static void
initPrinterList(ChoosePrinter *self)
{
/* we actually set tell the column cell that there are four fields, and
   use a fifth one for the printer's note.
 */
    id           matrix, sView;
    id		 contentView;
    id           labelTextFields[NUM_COLS];
    NXZone      *zone = [self zone];

 /* ??? gross, we use an unseen column to store the note info */
    static float columns[] = {0, 0, 0, 0, 0};	/* ColCell's point to
							 * this */
    NXRect       frame;
    float        fieldOffset = 0.0;
    int          i;

    contentView = [self contentView];
    labelTextFields[0] = [contentView findViewWithTag:NX_CPNAMETITLE];
    labelTextFields[1] = [contentView findViewWithTag:NX_CPTYPETITLE];
    labelTextFields[2] = [contentView findViewWithTag:NX_CPHOSTTITLE];
    labelTextFields[3] = [contentView findViewWithTag:NX_CPPORTTITLE];
    for (i = 0; i < NUM_COLS - 1; i++) {
	[labelTextFields[i] getFrame:&frame];
	if (!i)
	    fieldOffset = frame.origin.x;
	columns[i] = frame.origin.x + 5 - fieldOffset;
    }
    columns[i] = frame.origin.x + frame.size.width + 4 - fieldOffset;
    [[self->border contentView] getFrame:&frame];
    sView = [[ScrollView allocFromZone:zone] initFrame:&frame];
    [sView setVertScrollerRequired:YES];
    [sView setHorizScrollerRequired:NO];
    frame.origin.x = frame.origin.y = 0.0;
    [sView getContentSize:&frame.size];	/* ??? 7/16/89 - wrp - only works for unscaled ClipView. */
    matrix = [[self class] _buildPrinterMatrix:&frame tabs:columns zone:zone];
    [matrix allowEmptySel:NO];
    [matrix setTarget:self];
    [matrix setAction:@selector (pickedList:)];
    [matrix setDoubleAction:@selector (_doubleClickedList:)];
    [[sView setDocView:matrix] free];
    [[self->border setContentView:sView] free];
}

/* updates titles on panel to reflect the selected printer */

static void updateTitles(ChoosePrinter *self)
{
    const char * const *printerData;

    printerData = [[[[self->border contentView] docView] selectedCell] data];
    if (printerData) {
	[self->name setStringValue:printerData[NAME]];
	[self->type setStringValue:printerData[TYPE]];
	[self->note setStringValue:printerData[NOTE]];
    }
}

@end

/*
  
Modifications (starting at 0.8):
  
 2/06/89 trey	creation
 3/14/89 trey	added instance var and methods for customization
		added error handling to take panel down
 3/21/89 wrp	moved default registration to Application.m and removed
		 +initialize

0.92
----
 6/12/89 trey	made all ChoosePrinter panels share the same notion of
		 of the current printer via the defaults database.

0.93
----
 6/12/89 trey	made all ChoosePrinter panels share the same notion of
		 of the current printer via the defaults database
 6/14/89	check appropriateness of fields in the panel when a button
		 is pressed to (potentially) dismiss it
 6/19/89 pah	panel is not dismissed of selection wont give up being
		 first responder
 7/12/89 trey	title views looked up by tag instead of by nib-name

79
--
 3/21/90 king	support for printerdb "_ignore" flag

84
--
 5/13/90 king	fix of support for "_ignore" flag so cellCount wouldnt be
		 incremented if we were ignoring a printer
88
--
 6/27/90 chris	add support for fax/printer dichotomy
*/


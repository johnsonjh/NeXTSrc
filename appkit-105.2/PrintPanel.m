/*
	PrintPanel.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "PrintPanel_Private.h"
#import "Panel_Private.h"
#import "PrintInfo_Private.h"
#import "Application_Private.h"
#import "View_Private.h"
#import "SavePanel.h"
#import "View.h"
#import "Button.h"
#import "ButtonCell.h"
#import "Matrix.h"
#import "ColumnCell.h"
#import "Form.h"
#import "Font.h"
#import "FormCell.h"
#import "Application.h"
#import "Text.h"
#import "TextField.h"
#import "ScrollView.h"
#import "Box.h"
#import "Bitmap.h"
#import "ChoosePrinter.h"
#import "publicWraps.h"
#import <defaults.h>
#import "nextstd.h"
#import "errors.h"
#import <objc/List.h>
#import <dpsclient/wraps.h>
#import <dpsclient/dpsclient.h>
#import <dpsclient/dpsNeXT.h>

/* Ack! Taken from ChoosePrinter.m */

#define NAME		0
#define TYPE		1
#define HOST		2
#define PORT		3
#define NOTE		4
#define NUM_COLS	5

#define printerListBox _reservedPrintPanel2[0]
#define controlBox _reservedPrintPanel2[1]

void _NXMakePopUpList(id button, const char *const * strings, const char *tableName);
static void updateTitlesAndOptions(id self);
static BOOL getFieldValue(Control *ctrl, const char *altString, int altValue,
			  int *resultValue);

/* strings for the feed list */
static const char AutoString[] = "Cassette";
static const char ManualString[] = "Manual";
static const char * const FeedStringsTable[] = {ManualString, AutoString, NULL};

static NXCoord minWidth, minHeight;

/* strings for the resolution list */
static const char * const ResolutionStringsTable[] = {"300 dpi", "400 dpi", NULL};

/* the factory object will only create one instance of this panel */
static PrintPanel *UniqueInstance = NULL;

/* factory used to create the instance */
static PrintPanel *UniqueFactory = NULL;

/* localization macros */

#define NextPrinterTypeString \
    KitString(Printing, "NeXT 400 dpi Laser Printer", "Full name of the NeXT 400 dpi printer.")

static const char *firstString = NULL;
static const char *lastString = NULL;

#define FirstString (firstString ? firstString : (firstString = \
    KitString(Printing, "first", "Word that appears if the user is printing the document starting at the beginning.")))

#define LastString (lastString ? lastString : (lastString = \
    KitString(Printing, "last", "Word that appears if the user is printing the document all the way to the end.")))

#define NO_PRINTER_CHOSEN \
    KitString(Printing, "(No printer chosen)", "Message given to user when no printer is currently chosen.")

/*
    Unfortunately, we have to shadow these here for the
    purpose of documenting the their translation.  Keep
    this up to date with changes in FeedStringsTable[]
    and ResolutionStringsTable[].

    KitString(Printing, "300 dpi", "Printing resolution.")
    KitString(Printing, "400 dpi", "Printing resolution.")
    KitString(Printing, "Cassette", "Word to indicate automatic paper feed into a printer.")
    KitString(Printing, "Manual", "Word to indicate manual paper feed into a printer.")
*/

@implementation PrintPanel:Panel

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
    return [self newContent:(NXRect *)0 style:0 backing:0 buttonMask:0 defer:NO];
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
	    UniqueInstance = _NXLoadNibPanel("PrintPanel");
	    UniqueFactory = nil;
	    if (UniqueInstance) {
		_NXMakePopUpList(UniqueInstance->resolutionList, ResolutionStringsTable, "Printing");
		_NXMakePopUpList(UniqueInstance->feed, FeedStringsTable, "Printing");
		[UniqueInstance->ok setTag:NX_OKTAG];
		[[UniqueInstance->copies cell] setEntryType:NX_POSINTTYPE];
		[UniqueInstance->firstPage setStringValue:NULL];
		[UniqueInstance->lastPage setStringValue:NULL];
		[UniqueInstance->firstPage setOpaque:YES];
		[UniqueInstance->lastPage setOpaque:YES];
		[UniqueInstance->pageMode setOpaque:YES];
		minWidth = UniqueInstance->frame.size.width;
		minHeight = UniqueInstance->frame.size.height;
		{		/* ???this can go when nib lets me assign
				 * this name */
		    [UniqueInstance->appIcon setIcon:"app"];
		}
		[UniqueInstance useOptimizedDrawing:NX_KITPANELSOPTIMIZED];
		[UniqueInstance setOneShot:NX_KITPANELSONESHOT];
	    }
	}
    }
    return UniqueInstance;
}


- free
{
    UniqueInstance = NULL;
    [appIcon free];
    [pageMode free];
    [firstPage free];
    [lastPage free];
    [copies free];
    [name free];
    [status free];
    [[feed target] free];
    [feed free];
    [[resolutionList target] free];
    [resolutionList free];
    return[super free];
}

- windowWillResize:sender toSize:(NXSize *)size
{
    if ([printerListBox superview]) {
	if (size->width < minWidth) size->width = minWidth;
	if (size->height < minHeight) size->height = minHeight;
    } else {
	size->width = frame.size.width;
	size->height = frame.size.height;
    }
    return self;
}

- windowDidResize:sender
{
    NXSize svSize, cellSize;

    if ([printerListBox superview]) {
	[[[printers superview] superview] getContentSize:&svSize];
	[printers getCellSize:&cellSize];
	cellSize.width = svSize.width;
	[printers setCellSize:&cellSize];
	[printers sizeToCells];
    }

    return self;
}


- setAccessoryView:aView
{
    return [self _doSetAccessoryView:aView topView:controlBox bottomView:buttons oldView:&accessoryView];
}


- accessoryView
{
    return accessoryView;
}

- _pickedButton:sender
{
    [sender setTag:[[sender selectedCell] tag]];
    return [self pickedButton:sender];
}

- pickedButton:sender
{
    id                  sp;
    id			oldAccView;
    const char	       *oldReqType;
    int			tag, ret;

    if (![sender selectedCell]) return self;
    tag = [[sender selectedCell] tag];
  /* if we are in the process of printing */
    if (tag == NX_CANCELTAG && ![ok isEnabled]) {
	exitTag = tag;
	NX_RAISE(NX_abortPrinting, NULL, NULL);
    }
    if (tag == NX_CANCELTAG || [self makeFirstResponder:self]) {
	exitTag = tag;
	if (exitTag == NX_SAVETAG) {
	    sp = [SavePanel new];
	    oldAccView = [sp accessoryView];
	    [sp setAccessoryView:nil];
	    oldReqType = [sp requiredFileType];
	    [sp setRequiredFileType:"ps"];
	    ret = [sp runModal];
	    [sp setAccessoryView:oldAccView];
	    [sp setRequiredFileType:oldReqType];
	    if (!ret)
		return self;	/* back to the print panel */
	}
	if (tag == NX_FAXTAG) {
	    [sender setReaction:YES];
	    _NXSetCellParam([sender selectedCell], NX_CELLHIGHLIGHTED, 0);
	}
	[NXApp stopModal];		/* exit modal run */
    }
    return self;
}


- pickedPrinter:sender
{
    const char *const *printerData = [[sender selectedCell] data];
    if (printerData) [note setStringValue:printerData[NOTE]];
    [sender allowEmptySel:NO];
    [ChoosePrinter _writePrintInfo:YES lastValues:NULL cell:[sender selectedCell]];
    updateTitlesAndOptions(self);
    return self;
}


- setPrinterListBox:anObject
{
    printerListBox = anObject;
    return self;
}

- setControlBox:anObject
{
    controlBox = anObject;
    return self;
}

static float columns[] = {0.0, 125.0, 285.0, 355.0, 415.0};

- setPrinterListTitle:anObject
{
    NXRect vframe;
    const char *titles[NUM_COLS];
    id sview, cell = [[ColumnCell allocFromZone:[self zone]] initTextCell:NULL];

    [cell setNumColumns:NUM_COLS];
    [cell setTabs:columns];
    [cell setBezeled:YES];
    [cell setEnabled:NO];
    [cell setAlignment:NX_CENTERED];
    titles[0] = KitString(Printing, "Printer", "In the list of printers in the PrintPanel, the title of the column with the printer name in it.");
    titles[1] = KitString(Printing, "Type", "In the list of printers in the PrintPanel, the title of the column with the printer type in it.");
    titles[2] = KitString(Printing, "Host", "In the list of printers in the PrintPanel, the title of the column with the printer's host in it.");
    titles[3] = KitString(Printing, "Port", "In the list of printers in the PrintPanel, the title of the column with the printer's port in it.");
    titles[4] = KitString(Printing, "Note", "In the list of printers in the PrintPanel, the title of the column with the printer's note in it.");
    [cell setData:titles];
    [cell setFont:[Font newFont:NXBoldSystemFont size:12.0]];
    [anObject getFrame:&vframe];
    sview = [anObject superview];
    [[anObject removeFromSuperview] free];
    anObject = [[Control allocFromZone:[self zone]] initFrame:&vframe];
    [anObject setAutosizing:NX_WIDTHSIZABLE|NX_MINYMARGINSIZABLE];
    [anObject setCell:cell];
    [sview addSubview:anObject];

    return self;
}

- setButtons:anObject
{
    buttons = anObject;
    [buttons setAutosizeCells:YES];
    return self;
}

- setPrinterList:anObject
{
    NXRect rect;

    [anObject setVertScrollerRequired:YES];
    [anObject setHorizScrollerRequired:NO];
    [anObject setBorderType:NX_BEZEL];
    rect.origin.x = rect.origin.y = 0.0;
    [anObject getContentSize:&rect.size];
    printers = [ChoosePrinter _buildPrinterMatrix:&rect tabs:columns zone:[self zone]];
    [printers setTarget:self];
    [printers setAction:@selector(pickedPrinter:)];
    [[anObject setDocView:printers] free];

    return self;
}


- (BOOL)textWillChange:textObject
{
    if ([pageMode selectedCol] == 0)	/* if needs to be changed */
	[pageMode selectCellAt:0 :1];
    if (([firstPage currentEditor] != textObject) &&
	(![firstPage stringValue] || !*[firstPage stringValue]))
	[firstPage setStringValue:FirstString];
    if (([lastPage currentEditor] != textObject) &&
	(![lastPage stringValue] || !*[lastPage stringValue]))
	[lastPage setStringValue:LastString];
    return 0;
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

    if ([firstPage currentEditor]) {
	control = firstPage;
	specialLabel = FirstString;
    } else if ([lastPage currentEditor]) {
	control = lastPage;
	specialLabel = LastString;
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


- pickedAllPages:sender
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


- changePrinter:sender
{
    [[ChoosePrinter new] runModal];
    updateTitlesAndOptions(self);
    return self;
}


- readPrintInfo
{
    int cols, rows = -1;
    const char *printerName, *printerHost = NULL;
    const char *const *printerData = NULL;
    id pInfo = [NXApp printInfo];

    [pInfo _updatePrinterInfo];     /* get name, type, and res in sync */
    [pageMode selectCellAt:0 :0];
    [firstPage setStringValue:NULL];
    [lastPage setStringValue:NULL];
    [copies setIntValue:1];
    printerName = [pInfo printerName];
    if (printerName) {
	printerHost = [pInfo printerHost];
	if (!printerHost || !*printerHost) printerHost = _NXHostName();   /* set to real local host name */
    }
    if (printerName && printerHost && *printerHost) {
	[self->printers getNumRows:&rows numCols:&cols];
	while (rows--) {
	    printerData = [[self->printers cellAt:rows :0] data];
	    if (printerData && !strcmp(printerName, printerData[NAME]) && !strcmp(printerHost, printerData[HOST])) break;
	}
    }
    [printers allowEmptySel:YES];
    [printers selectCellAt:rows :0];
    if (rows >= 0 && printerData) {
	[printers scrollCellToVisible:rows :0];
	[printers allowEmptySel:NO];
	[note setStringValue:printerData[NOTE]];
    } else {
	[note setStringValue:NULL];
    }
    updateTitlesAndOptions(self);

    return self;
}


- writePrintInfo
{
    id                  pInfo = [NXApp printInfo];
    BOOL                allPages;
    int			fieldVal;
    int			newResVal;
    const char		*newResStr;
    const char		*typeVal;

    allPages = [pageMode selectedCol] == 0;
    [pInfo setAllPages:allPages];
    if (!allPages) {
	getFieldValue(firstPage, FirstString, MININT, &fieldVal);
	[pInfo setFirstPage:fieldVal];
	getFieldValue(lastPage, LastString, MAXINT, &fieldVal);
	[pInfo setLastPage:fieldVal];
    }
    [pInfo setCopies:[copies intValue]];
    typeVal = [pInfo printerType];
    if ((!typeVal) || strcmp(typeVal, NextPrinterTypeString)) {
	[pInfo setManualFeed:NO];
	[pInfo setResolution:0];
    } else {
	if (strcmp([feed title], _NXKitString("Printing", ManualString)) == 0) {
	    [pInfo setManualFeed:YES];
	} else {
	    [pInfo setManualFeed:NO];
	}
	newResStr = [resolutionList title];
	newResVal = atoi(newResStr);
	[pInfo setResolution:newResVal];
	if (_lastResolution != newResVal)
	    NXWriteDefault(NXSystemDomainName, "PrinterResolution", newResStr);
    }

    return self;
}

- (BOOL)_loadPrinters
{
    NXRect rect, cFrame, nFrame;

    if (![[printers cellList] count] || ![printers window]) {
	if ([printers window]) {
	    [printerListBox removeFromSuperview];
	    [ChoosePrinter _loadPrinterList:printers isPrinter:YES orderIt:NO];
	    if ([[printers cellList] count] < 2) {
		const char *const *printerData;
		[printerListBox getFrame:&rect];
		[contentView getFrame:&cFrame];
		[self sizeWindow:cFrame.size.width :cFrame.size.height - rect.size.height];
		[self display];
		[printers allowEmptySel:NO];
		if ([[printers cellList] count]) {
		    [printers selectCellAt:0 :0];
		    printerData = [[printers selectedCell] data];
		    if (printerData) {
			[ChoosePrinter _writePrintInfo:YES lastValues:NULL cell:[printers selectedCell]];
		    }
		}
	    } else {
		[contentView addSubview:printerListBox];
		[printerListBox display];
		return NO;
	    }
	} else {
	    [ChoosePrinter _loadPrinterList:printers isPrinter:YES orderIt:NO];
	    if ([[printers cellList] count] > 1) {
		[printerListBox getFrame:&rect];
		[contentView getFrame:&cFrame];
		[self sizeWindow:cFrame.size.width :cFrame.size.height + rect.size.height];
		[note getFrame:&nFrame];
		[controlBox getFrame:&cFrame];
		rect.origin.y = cFrame.origin.y + cFrame.size.height + floor((nFrame.origin.y - cFrame.origin.y - cFrame.size.height - rect.size.height) / 2.0);
		rect.origin.x = floor((frame.size.width - rect.size.width) / 2.0);
		[printerListBox setFrame:&rect];
		[contentView addSubview:printerListBox];
		[self display];
		[printers allowEmptySel:YES];
		[printers selectCellAt:-1 :-1];
	    }
	}
    }

    return YES;
}

- (int)runModal
{
    NXHandler exception;

    exception.code = 0;
    [self disableFlushWindow];
    [self _setControlsEnabled:YES];
    [self reenableFlushWindow];
    [name setStringValue:NULL];
    [note setStringValue:NULL];
    [status setStringValue:NULL];
    [self _loadPrinters];
    DPSFlush();
    [self readPrintInfo];
    [copies selectText:self];
    NXPing();
rerunPanel:
    NX_DURING {
	[NXApp runModalFor:self];
    } NX_HANDLER {
	exception = NXLocalHandler;
	if (exception.code == dps_err_ps)
	    NXReportError(&NXLocalHandler);
	[NXApp stopModal];
	exitTag = NX_CANCELTAG;
    } NX_ENDHANDLER
    if (exitTag != NX_CANCELTAG) {
	if (![[NXApp printInfo] printerName] && exitTag == NX_OKTAG) {
	    if (_NXKitAlert("Printing", "Print", "You must choose a printer in order to print.", "OK", "Cancel", NULL) == NX_ALERTDEFAULT) {
		[copies selectText:self];
		goto rerunPanel;
	    } else
		exitTag = NX_CANCELTAG;
	} else {
	    [self writePrintInfo];
	}
    }
    if (exception.code && exception.code != dps_err_ps)
	NX_RAISE(exception.code, exception.data1, exception.data2);
    return exitTag;
}



- _setControlsEnabled:(BOOL)flag
{
    id cell;
    int rows, cols;

    [copies setEnabled:flag];
    [pageMode setEnabled:flag];
    [firstPage setEnabled:flag];
    [lastPage setEnabled:flag];
    [feed setEnabled:flag];
    [resolutionList setEnabled:flag];
    [printers setEnabled:flag];
    if (buttons) {
	[buttons getNumRows:&rows numCols:&cols];
	while (cols--) {
	    cell = [buttons cellAt:0 :cols];
	    if ([cell tag] != NX_CANCELTAG) [cell setEnabled:flag];
	}
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

- _copyFromFaxPanelPageMode:pageModeFax 
	        firstPage : firstPageFax 
		lastPage: lastPageFax
{
    [pageMode selectCellAt:0 :[pageModeFax selectedCol]];
    [firstPage setStringValue:[firstPageFax stringValue]];
    [lastPage setStringValue:[lastPageFax stringValue]];
    return self;
}

static void
updateTitlesAndOptions(PrintPanel *self)
{
    id pInfo = [NXApp printInfo];
    char const *const *label;
    const char *name, *type = NULL;

    name = [pInfo printerName];
    type = [pInfo printerType];
    if (!name) {
	name = NO_PRINTER_CHOSEN;
    } else {
	[self->status setStringValueNoCopy:""];
    }
    [self->name setStringValue:name];
    if (type && strcmp(type, NextPrinterTypeString) == 0) {
	self->_lastResolution = [pInfo resolution];
	for (label = ResolutionStringsTable; *label; label++)
	    if (self->_lastResolution == atoi(_NXKitString("Printing", *label)))
		break;
	[self->resolutionList setTitleNoCopy:_NXKitString("Printing", *label ? *label : *ResolutionStringsTable)];
	[self->resolutionList setEnabled:YES];
	[self->feed setTitleNoCopy:_NXKitString("Printing", AutoString)];
	[self->feed setEnabled:YES];
    } else {
	[self->resolutionList setTitleNoCopy:KitString(Printing, "Default", "Word which appears when the resolutions (i.e. how many dpi) or paper feeding capability of a printer is not known.")];
	[self->resolutionList setEnabled:NO];
	[self->feed setTitle:_NXKitString("Printing", "Default")];
	[self->feed setEnabled:NO];
    }
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

@end

/*
  
Modifications (starting at 0.8):
  
 1/27/89 trey	major rewrite to load data from nib file
 2/11/89 trey	force initial values instead of getting them from
		 the printInfo object
 3/14/89 trey	added instance var and methods for customization
		added error handling to take panel down
		added private method to enable/disable buttons
 3/21/89 wrp	moved default registration to Application.m and
		 removed +initialize
0.91
----
 4/28/89 trey	made SavePanel have required type .ps
 5/19/89 trey	minimized static data

0.92
----
 6/06/89 trey	set cell entryType for copies field
		allow first and last entries in To/From fields
		error checking on To/From fields
		simplified _updatePrintStat
		override orderWindow:relativeTo: to select the scale
		 field when we come up
		reads defaults database when run to update to the latest
		 printer chosen

0.93
----
 6/14/89 pah	verify validity of fields before allow panel to be dismissed
 6/15/89 trey	changed _setButtonsEnabled to _setControlsEnabled: to support
		 disabling of everything in panel but the cancel button
		 while printing
		firstPage changed from a textfield to a form
		cancel button works during printing
		
 6/16/89 trey	firstPage changed from a textfield to a form
 6/19/89 pah	change NXAlert() to NXRunAlertPanel()
 6/19/89 pah	panel is not dismissed of selection wont give up being
		 first responder
 6/19/89 pah	panel is not dismissed of selection wont give up being
 7/18/89 trey	null printer not checked if user didnt click OK
 8/12/89 trey	exit tag set when cancel hit while printing

86
--
 6/6/90	king	Changed feed to a popup list.
		Added support for disabling feed and resolution when
		printing to a non-NeXT printer.

87
--
 6/26/90 king	Put Manual above Casette in popup list.  It makes it easier
 		to select.

94
--
 9/17/90 chris	put in kludge code to copy pageMode, firstPage & lastPage from
 		NXFaxPanel to work around FrameMaker kludgery of reading
		PrintPanel directly instead of PrintInfo!
 9/25/90 greg	use byteLength instead of textLength

105
---
 11/6/90 glc	use blength not length
 

*/

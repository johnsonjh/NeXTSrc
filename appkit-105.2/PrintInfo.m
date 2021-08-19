/*
	PrintInfo.m
  	Copyright 1988, NeXT, Inc.
	Responsibility: Peter King
*/


#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "appkitPrivate.h"
#import "PrintInfo_Private.h"
#import "Application.h"
#import "PageLayout.h"
#import <defaults.h>
#import "nextstd.h"
#import <objc/hashtable.h>
#import <printerdb.h>

static void updateInstanceVar(NXZone *zonep, char **ivar, const char *newVal);
static void getPrdbTypeHostAndIgnore(const prdb_ent *entry,
				     const char **host, const char **type,
				     const char **ignore);

/* printer type suffix for fax modems */

#define FAXSUFFIX "Fax Modem"

@implementation PrintInfo

+ new
{
    return [[self allocFromZone:NXDefaultMallocZone()] init];
}

- init
{
    const NXSize       *pSize;
    const char         *defaultVal;

    [super init];
    defaultVal = NXGetDefaultValue([NXApp appName], "NXPaperType");
    paperType = NXCopyStringBufferFromZone(defaultVal, [self zone]);
    NX_X(&paperRect) = NX_Y(&paperRect) = 0.0;
    if (pSize = NXFindPaperSize(paperType)) {
	paperRect.size = *pSize;
	pInfoFlags.orientation =
	  (paperRect.size.width <= paperRect.size.height) ?
	  NX_PORTRAIT : NX_LANDSCAPE;
    } else {
	NX_WIDTH(&paperRect) = NX_HEIGHT(&paperRect) = 0.0;
	pInfoFlags.orientation = NX_PORTRAIT;
    }
    defaultVal = NXGetDefaultValue([NXApp appName], "NXMargins");
    sscanf(defaultVal, "%f %f %f %f", &leftPageMargin, &rightPageMargin,
	   &topPageMargin, &bottomPageMargin);
    scalingFactor = 1.0;
    pInfoFlags.horizCentered = pInfoFlags.vertCentered = YES;
    pInfoFlags.horizPagination = NX_CLIPPAGINATION;
    pInfoFlags.vertPagination = NX_AUTOPAGINATION;
    pInfoFlags.orientation = NX_PORTRAIT;
    pageOrder = NX_ASCENDINGORDER;
    pInfoFlags.manualFeed = NO;
    pInfoFlags.allPages = YES;
    firstPage = 1;
    lastPage = MAXINT;
    copies = 1;
    pagesPerSheet = 1;
    resolution = atoi(NXGetDefaultValue(NXSystemDomainName, "PrinterResolution"));
    [self _updatePrinterInfo];
    return self;
}


- free
{
    if (paperType)
	NX_FREE(paperType);
    if (outputFile)
	NX_FREE(outputFile);
    return[super free];
}


- setPaperType:(const char *)type andAdjust:(BOOL)flag
{
    const NXSize       *pSize;

    if (type) {
	NX_ZONEREALLOC([self zone], paperType, char, strlen(type) + 1);
	strcpy(paperType, type);
	if (flag && (pSize = NXFindPaperSize(paperType))) {
	    paperRect.size = *pSize;
	    pInfoFlags.orientation =
	      (paperRect.size.width <= paperRect.size.height) ?
	      NX_PORTRAIT : NX_LANDSCAPE;
	}
    } else
	*paperType = '\0';
    return self;
}


- (const char *)paperType
{
    return paperType;
}


- setPaperRect:(const NXRect *)aRect andAdjust:(BOOL)flag
{
    NX_X(&paperRect) = NX_Y(&paperRect) = 0.0;
    paperRect.size = aRect->size;
    if (flag) {
	pInfoFlags.orientation =
	  (paperRect.size.width <= paperRect.size.height) ?
	  NX_PORTRAIT : NX_LANDSCAPE;
	[self setPaperType:_NXFindPaperName(&aRect->size) andAdjust:NO];
    }
    return self;
}


- (const NXRect *)paperRect
{
    return &paperRect;
}


- setMarginLeft:(NXCoord)leftMargin right:(NXCoord)rightMargin
    top:(NXCoord)topMargin bottom:(NXCoord)bottomMargin
{
    leftPageMargin = leftMargin;
    rightPageMargin = rightMargin;
    topPageMargin = topMargin;
    bottomPageMargin = bottomMargin;
    return self;
}


- getMarginLeft:(NXCoord *)leftMargin right:(NXCoord *)rightMargin
    top:(NXCoord *)topMargin bottom:(NXCoord *)bottomMargin
{
    *leftMargin = leftPageMargin;
    *rightMargin = rightPageMargin;
    *topMargin = topPageMargin;
    *bottomMargin = bottomPageMargin;
    return self;
}


- setScalingFactor:(float)aFloat
{
    scalingFactor = aFloat;
    return self;
}


- (float)scalingFactor
{
    return scalingFactor;
}


- setOrientation:(char)mode andAdjust:(BOOL)flag
{
    float               currMinSize, currMaxSize;

    pInfoFlags.orientation = mode;
    if (flag) {
	currMinSize = MIN(paperRect.size.width, paperRect.size.height);
	currMaxSize = MAX(paperRect.size.width, paperRect.size.height);
	if (mode == NX_PORTRAIT) {
	    paperRect.size.width = currMinSize;
	    paperRect.size.height = currMaxSize;
	} else {
	    paperRect.size.width = currMaxSize;
	    paperRect.size.height = currMinSize;
	}
    }
    return self;
}


- (char)orientation
{
    return pInfoFlags.orientation;
}


- setHorizCentered:(BOOL)flag
{
    pInfoFlags.horizCentered = flag;
    return self;
}


- (BOOL)isHorizCentered
{
    return pInfoFlags.horizCentered;
}


- setVertCentered:(BOOL)flag
{
    pInfoFlags.vertCentered = flag;
    return self;
}


- (BOOL)isVertCentered
{
    return pInfoFlags.vertCentered;
}


- setHorizPagination:(int)mode
{
    pInfoFlags.horizPagination = mode;
    return self;
}


- (int)horizPagination
{
    return pInfoFlags.horizPagination;
}


- setVertPagination:(int)mode
{
    pInfoFlags.vertPagination = mode;
    return self;
}


- (int)vertPagination
{
    return pInfoFlags.vertPagination;
}


- setOutputFile:(const char *)aString
{
    updateInstanceVar([self zone],&outputFile, aString);
    return self;
}


- (const char *)outputFile
{
    return outputFile;
}


- setPageOrder:(char)mode
{
    pageOrder = mode;
    return self;
}


- (char)pageOrder
{
    return pageOrder;
}


- setManualFeed:(BOOL)flag
{
    pInfoFlags.manualFeed = flag;
    return self;
}


- (BOOL)isManualFeed
{
    return pInfoFlags.manualFeed;
}


- setAllPages:(BOOL)flag
{
    pInfoFlags.allPages = flag;
    return self;
}


- (BOOL)isAllPages
{
    return pInfoFlags.allPages;
}


- setFirstPage:(int)anInt
{
    firstPage = anInt;
    return self;
}


- (int)firstPage
{
    return firstPage;
}


- setLastPage:(int)anInt
{
    lastPage = anInt;
    return self;
}


- (int)lastPage
{
    return lastPage;
}


- _setCurrentPage:(int)anInt
{
    currentPage = anInt;
    return self;
}


- (int)currentPage
{
    return currentPage;
}


- setCopies:(int)anInt
{
    copies = anInt;
    return self;
}


- (int)copies
{
    return copies;
}


- setContext:(DPSContext)aContext
{
    context = aContext;
    return self;
}


- (DPSContext)context
{
    return context;
}


- _setPrivateData:(NXPrivatePrintInfo)data
{
    _privateData = data;
    return self;
}


- (NXPrivatePrintInfo)_privateData;
{
    return _privateData;
}


- setPagesPerSheet:(short)aShort
{
    pagesPerSheet = aShort;
    return self;
}


- (short)pagesPerSheet;
{
    return pagesPerSheet;
}


- setPrinterName:(const char *)aString
{
    updateInstanceVar([self zone], &printerName, aString);
    return self;
}


- (const char *)printerName
{
    return printerName;
}


- setPrinterType:(const char *)aString
{
    updateInstanceVar([self zone], &printerType, aString);
    return self;
}


- (const char *)printerType
{
    return printerType;
}


- setPrinterHost:(const char *)aString
{
    if (aString && !*aString)
	aString = _NXHostName();
    updateInstanceVar([self zone], &printerHost, aString);
    return self;
}


- (const char *)printerHost
{
    return printerHost;
}


- setResolution:(int)anInt
{
    resolution = anInt;
    return self;
}


- (int)resolution
{
    return resolution;
}


/* updates a string instance variable */
static void updateInstanceVar(NXZone *zonep, char **ivar, const char *newVal)
{
    if (*ivar)
	free(*ivar);
    if (!newVal)
	*ivar = NULL;
    else
	*ivar = NXCopyStringBufferFromZone(newVal,  zonep);
}

BOOL _NXIsFaxType(const char *printerType)
{
    static const char faxSuffix[] = FAXSUFFIX;
    int typeLen;
    int faxSuffixLen;
    BOOL isFax = NO;
    
    if (printerType) {
	typeLen = strlen(printerType);
	faxSuffixLen = strlen(faxSuffix);
	if (typeLen >= faxSuffixLen &&
	    !strcmp(printerType+typeLen-faxSuffixLen,faxSuffix))
	    isFax = YES;
    }
    return isFax;
}


/* updates printerName, printerHost, and printerType intance vars from
   the current defaults database.  If the printer with the given name
   and host is not found, the instance vars are set to NULL.  A NULL
   hostname means we dont know it, whereas an empty string means local host.
 */
- _updateInfoIsPrinter:(BOOL) printerFlag
{
    const char	       *defName;		/* name from defaults */
    const char        **prdbNamePtr;		/* ptr to name from netinfo */
    const char         *prdbType;		/* type from netinfo */
    const char         *defHost;		/* host from defaults */
    const char         *prdbHost;		/* host from netinfo */
    const char         *defRes;			/* resolution from defaults */
    const char	       *prdbIgnore;		/* ignore flag from netinfo */
    const prdb_ent     *entry;
    BOOL		foundMatch;
    BOOL		prdbOpened = NO;	/* do we need to prdb_end()? */
    const char	       *nameDefault;		/* these change for fax/printer */
    const char	       *hostDefault;
    const char         *resolutionDefault;
    BOOL	        isFax = NO;
    
    if (printerFlag) {
        nameDefault = "Printer";
	hostDefault = "PrinterHost";
	resolutionDefault = "PrinterResolution";
    } else {
        nameDefault = "Fax";
	hostDefault = "FaxHost";
	resolutionDefault = "FaxResolution";
    }
    if (!(defName = NXUpdateDefault(NXSystemDomainName, nameDefault)))
	defName = NXGetDefaultValue(NXSystemDomainName, nameDefault);
    if (!(defHost = NXUpdateDefault(NXSystemDomainName, hostDefault)))
	defHost = NXGetDefaultValue(NXSystemDomainName, hostDefault);
    if (!(defRes = NXUpdateDefault(NXSystemDomainName, resolutionDefault)))
	defRes = NXGetDefaultValue(NXSystemDomainName, resolutionDefault);
    if (defHost && !*defHost)
	defHost = _NXHostName();	/* fill in real local host name */

  /* First try looking up the printer by name, and check the host.  If
     the host doesnt match, we have to enumerate the entries looking for
     one that does.
   */
    foundMatch = NO;
    if (defHost && defName) {
	entry = prdb_getbyname(defName);
	if (entry) {
	    getPrdbTypeHostAndIgnore(entry, &prdbHost, &prdbType, &prdbIgnore);
	    if (defHost && strcmp(defHost, prdbHost)) {
		prdb_set(NULL);
		prdbOpened = YES;
		while (!foundMatch && (entry = prdb_get())) {
		    prdbType = prdbHost = NULL;
		    for (prdbNamePtr = entry->pe_name; prdbNamePtr[1];
							prdbNamePtr++)
			;
		    if (!strcmp(*prdbNamePtr, defName)) {
			getPrdbTypeHostAndIgnore(entry, &prdbHost, &prdbType,
						 &prdbIgnore);
			if (!strcmp(defHost, prdbHost))
			    foundMatch = YES;
		    }
		}
	    } else
		foundMatch = YES;
	}
    }
	
    if (foundMatch && !prdbIgnore)
       isFax = _NXIsFaxType(prdbType);
    if (isFax == printerFlag)
        foundMatch = NO;
    
    if (foundMatch && !prdbIgnore) {
	[self setPrinterName:defName];
	[self setPrinterHost:defHost];
	[self setPrinterType:prdbType];
    } else {
	[self setPrinterName:NULL];
	[self setPrinterHost:NULL];
	[self setPrinterType:NULL];
    }
    [self setResolution:atoi(defRes)];
    if (prdbOpened)
	prdb_end();
    return self;
}

/* updates printerName, printerHost, and printerType intance vars from
   the current defaults database.  If the printer with the given name
   and host is not found, the instance vars are set to NULL.  A NULL
   hostname means we dont know it, whereas an empty string means local host.
 */
- _updatePrinterInfo
{
    return [self _updateInfoIsPrinter:YES];
}

/* updates printerName, printerHost, and printerType intance vars from
   the current defaults database.  If the fax with the given name
   and host is not found, the instance vars are set to NULL.  A NULL
   hostname means we dont know it, whereas an empty string means local host.
 */
- _updateFaxInfo
{
    return [self _updateInfoIsPrinter:NO];
}

/* looks up the type, host and ignore flag of a prdb entry */
static void
getPrdbTypeHostAndIgnore(const prdb_ent *entry, const char **host,
			 const char **type, const char **ignore)
{
    prdb_property      *prop;
    int			propCount;

    *host = *type = *ignore = NULL;
    for (propCount = entry->pe_nprops, prop = entry->pe_prop;
							propCount--; prop++)
	if (!strcmp(prop->pp_key, "ty"))
	    *type = prop->pp_value;
	else if (!strcmp(prop->pp_key, "rm"))
	    *host = prop->pp_value;
        else if (!strcmp(prop->pp_key, "_ignore"))
	    *ignore = prop->pp_value;
    if (!*host)
	*host = _NXHostName();		/* set to real local host name */
}


- write:(NXTypedStream *) stream
{
    [super write:stream];
    NXWriteRect(stream, &paperRect);
    NXWriteTypes(stream, "*fffffcsiii*s***i", &paperType, &leftPageMargin,
		 &rightPageMargin, &topPageMargin, &bottomPageMargin,
		 &scalingFactor, &pageOrder, &pInfoFlags, &firstPage,
		 &lastPage, &copies, &outputFile, &pagesPerSheet,
		 &printerName, &printerType, &printerHost, &resolution);
    return self;
}

- read:(NXTypedStream *) stream
{
    [super read:stream];
    NXReadRect(stream, &paperRect);
    NXReadTypes(stream, "*fffffcsiii*s***i", &paperType, &leftPageMargin,
		&rightPageMargin, &topPageMargin, &bottomPageMargin,
		&scalingFactor, &pageOrder, &pInfoFlags, &firstPage,
		&lastPage, &copies, &outputFile, &pagesPerSheet,
		&printerName, &printerType, &printerHost, &resolution);
    return self;
}


@end

/*
  
Modifications (starting at 0.8):
  
 1/08/89 trey	Removed unimplemented {horiz,vert}Paginated bits, and added
		  {horiz,vert}Pagination to instance vars.
		Removed methods setHorizPaginated:, isHorizPaginated,
		  setVertPaginated:, isVertPaginated
		Added methods setHorizPagination:, horizPagination,
		  setVertPagination:, vertPagination
		Added methods _setPrivateData:, _privateData and instance
		  variable _privateData
		Added methods setPagesPerSheet: and pagesPerSheet and instance
		  variable pagesPerSheet
 1/16/89 trey	Added readSelf and writeSelf methods
 2/20/89 trey	Added read: and write: methods
 3/21/89 wrp	moved default registration to Application.m and removed
		 +initialize
0.91
----
 5/01/89 trey	made methods returning pointers return const pointers
 5/23/89 trey	made default lastPage be MAXINT

0.92
----
 6/12/89 trey	made "" for PrinterHost mean its the local printer
		added _updatePrinterInfo to support panels staying
		 up-to-date with the latest info in defaults database

0.93
----
 6/27/89 trey	added _setCurrentPage: and currentPage

0.94
----
 7/11/89 trey	nuked spooling and setSpooling:

79
--
 3/21/90 king	added support for printerdb "_ignore" flag.

87
--
 6/27/90 chris	added support for Fax vs Printer distinction
 7/13/90 aozer	Renamed isFaxType -> _NXIsFaxType()
 
*/


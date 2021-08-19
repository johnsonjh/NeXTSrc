
/*
	afmParser.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file provides functions for dealing with font metrics
*/

#import "nextstd.h"
#import "afmprivate.h"
#import <objc/error.h>
#import <objc/hashtable.h>
#import <stdio.h>
#import <mach.h>
#import <sys/types.h>
#import <sys/stat.h>
#import <sys/file.h>
#import <ctype.h>
#import <sys/time.h>

#include "afmTables.c"

extern map_fd();
extern BOOL _NXGetHomeDir(char *user);

/* default size of units in afm file (font coord system) */
#define DEFAULT_UNITS_PER_POINT		1000.0

/* NextStep encoding name */
static const char NSEncName[] = "NextStepEncoding";

/* constants describing the max size of the shmems that pbs allocs */
/* must be in sync with Pasteboard object */
int WidthsMaxSize = 1024*8;
int FullDataMaxSize = 1024*64;
int CCMaxSize = 1024*64;
int StringsMaxSize = 1024*64;
int NextStackTag = 1;

NXStack CurrWidthsStack = NULL;
NXStack CurrDataStack = NULL;
NXStack CurrCCStack = NULL;
NXStrTable CurrStringTable = NULL;

/* struct of parsing info */
typedef struct {
    char *bufStart;
    char *bufCurr;
    char *bufEnd;
    int fd;
    int bytesAlloced;
    NXHashTable *metrics;	/* char metrics hashed by name */
    int reencode;		/* do we reencode font with NS encoding? */
    int reqMask;		/* mask of info we're parsing for */
    int doneMask;		/* mask of info we've parsed */
    NXStrTable strTable;	/* string table we're allocing out of */
} LexInfo;

typedef struct {		/* temp struct used when parsing ligs */
    NXCharMetrics *first;
    NXStackOffset second;
    NXStackOffset lig;
} TempLig;

/* hash table of unencoded characters' positions in the new encoding */
static NXHashTable *ISOLatinTable;
/* hash table of info about special tokens */
static NXHashTable *TokenTable;

static void parseFile(LexInfo *lexInfo, AFMInfo *afmInfo);
static void parseCharMetrics(LexInfo *lexInfo, AFMInfo *afmInfo);
#ifndef CACHE_DATA_BUILD
static void parseKerningData(LexInfo *lexInfo, AFMInfo *afmInfo);
static void parseTrackKerningData(LexInfo *lexInfo, AFMInfo *afmInfo);
static void parsePairKerningData(LexInfo *lexInfo, AFMInfo *afmInfo);
static void parseCompositeChars(LexInfo *lexInfo, AFMInfo *afmInfo);
#endif CACHE_DATA_BUILD
static void skipSection(LexInfo *lexInfo, Token endToken, Token firstIgnore, Token lastIgnore);

static void lexOpenFile(const char *filename, LexInfo *newInfo);
static void lexCloseFile(LexInfo *lexInfo);
static void lexToken(LexInfo *lexInfo, const TokenInfo **token);
static void lexString(LexInfo *lexInfo, NXStackOffset *str);
static void lexStringCopy(LexInfo *lexInfo, char **str);
static void lexName(LexInfo *lexInfo, NXStackOffset *str);
static void lexNumber(LexInfo *lexInfo, float *num);
static void lexInteger(LexInfo *lexInfo, int *num);
static void lexBoolean(LexInfo *lexInfo, char *bool);
static void lexNewline(LexInfo *lexInfo);
static void lexSeparator(LexInfo *lexInfo);
static void lexSkipWS(LexInfo *lexInfo);
static void gobbleNewline(LexInfo *lexInfo);
static void gobbleSeparator(LexInfo *lexInfo);

static char *intScan(char *s, int *num);
static char *nameScan(char *s);

static unsigned metricsHash(const void *info, const void *data);
static int metricsCompare(const void *info, const void *data1, const void *data2);
static NXCharMetrics *lookupMetric(NXStackOffset offset, NXHashTable *table);

static void copyLigatures(TempLig *tempLigs, int numTempLigs, NXCharMetrics *charMetrics, NXHashTable *table, AFMInfo *afmInfo);
static void scaleData(AFMInfo *afmInfo, int mask);
static int determineWork(int reqFlags, int existingFlags);
static void reportMissingChar(AFMInfo *afmInfo, NXStackOffset offset, const char *context);

static int encLigCompare(void *data1, void *data2);

/* Entry point into the parsing code.  flags may be any combo of *_INFO.  This routine should not be called if we have already parsed everything being requested.  It may parse more info than requested.
   Errors where we exhaust one of the stacks are counted.  If they happen more than once, we can never parse the file, and we give up.
   For the strings stack you try three times, since the first time might be a stringTable from a cache, in which you can never alloc.
*/
AFMInfo *parseAFMFile(const char *file, AFMInfo *oldInfo, int reqFlags)
{
    AFMInfo newInfo;
    LexInfo lexInfo;
    int parseFlags;			/* the stuff we really need to parse */
    struct timeval start, end, diff;
    int used, max;
    int widthsMark, dataMark, ccMark, stringsMark;
    volatile int done = FALSE;
    volatile dataErrors = 0, ccErrors = 0, stringsErrors = 0;

    if (PrintTimes) {
    	fprintf(stderr, "Reading %s ", file);
	gettimeofday(&start, NULL);
    }
    NX_ASSERT(reqFlags, "parseAFMFile: asked to do nothing");
    do {
	parseFlags = determineWork(reqFlags, oldInfo ? oldInfo->infoParsed :0);
	NX_ASSERT(parseFlags, "parseAFMFile: decided it needs to do nothing");
	if (!parseFlags) {
	    done = TRUE;
	    continue;
	}
	if (oldInfo)
	    newInfo = *oldInfo;
	else
	    bzero(&newInfo, sizeof(AFMInfo));
	if (parseFlags & WIDTH_INFO) {
	    if (CurrWidthsStack)
		NXGetStackInfo(CurrWidthsStack, NULL, &used, &max, NULL, NULL);
	    if (!CurrWidthsStack || max - used < ENCODING_SIZE * sizeof(float))
		CurrWidthsStack = NXCreateStack(NX_stackFixedContig, WidthsMaxSize, (void *)(NextStackTag++));
	    newInfo.widthsStack = CurrWidthsStack;
	}
	if (parseFlags & FULL_INFO) {
	    if (!CurrDataStack)
		CurrDataStack = NXCreateStack(NX_stackFixedContig, FullDataMaxSize, (void *)(NextStackTag++));
	    newInfo.dataStack = CurrDataStack;
	}
	if (parseFlags & CC_INFO) {
	    if (!CurrCCStack)
		CurrCCStack = NXCreateStack(NX_stackFixedContig, CCMaxSize, (void *)(NextStackTag++));
	    newInfo.ccStack = CurrCCStack;
	}
      /* ??? it would be nice to notice here that the string table cant grow because it comes from a .afmcache. */
	if (!newInfo.strTable) {
	    if (!CurrStringTable)
#ifndef CACHE_DATA_BUILD
		CurrStringTable = NXCreateStringTable(NX_stackFixedContig, StringsMaxSize, (void *)(NextStackTag++));
#else
		CurrStringTable = NXCreateStringTable(NX_stackGrowContig, StringsMaxSize, (void *)(NextStackTag++));
#endif CACHE_DATA_BUILD
	    newInfo.strTable = CurrStringTable;
	}
	if (newInfo.widthsStack)
	    NXGetStackInfo(newInfo.widthsStack, NULL, &widthsMark, NULL, NULL, NULL);
	if (newInfo.dataStack)
	    NXGetStackInfo(newInfo.dataStack, NULL, &dataMark, NULL, NULL, NULL);
	if (newInfo.ccStack)
	    NXGetStackInfo(newInfo.ccStack, NULL, &ccMark, NULL, NULL, NULL);
	if (newInfo.strTable)
	    NXGetStringTableInfo(newInfo.strTable, NULL, &stringsMark, NULL, NULL, NULL);
	NX_DURING {
	    lexOpenFile(file, &lexInfo);
	    lexInfo.reqMask = parseFlags;
	    lexInfo.strTable = newInfo.strTable;
	    parseFile(&lexInfo, &newInfo);
	    lexCloseFile(&lexInfo);
	    done = TRUE;
	} NX_HANDLER {
	    if (NXLocalHandler.code != err_stackAllocFailed && NXLocalHandler.code != err_strAllocFailed)
		NXLogError("parse error %d (%#x,%#x) reading %s", NXLocalHandler.code, NXLocalHandler.data1, NXLocalHandler.data2, file);
	    if (newInfo.widthsStack)
		NXResetStack(newInfo.widthsStack, NULL, widthsMark);
	    if (newInfo.dataStack)
		NXResetStack(newInfo.dataStack, NULL, dataMark);
	    if (newInfo.ccStack)
		NXResetStack(newInfo.ccStack, NULL, ccMark);
	    if (newInfo.strTable)
		NXResetStringTable(newInfo.strTable, NULL, stringsMark);
	    lexInfo.bufCurr = lexInfo.bufStart;
	    if (lexInfo.metrics) {
		NXFreeHashTable(lexInfo.metrics);
		lexInfo.metrics = NULL;
	    }
	    lexInfo.reencode = FALSE;
	    lexInfo.reqMask = lexInfo.doneMask = 0;
	    if (NXLocalHandler.code == err_stackAllocFailed && (NXStack)NXLocalHandler.data1 == newInfo.dataStack && !dataErrors) {
		CurrDataStack = NULL;
		dataErrors++;
	    } else if (NXLocalHandler.code == err_stackAllocFailed && (NXStack)NXLocalHandler.data1 == newInfo.ccStack && !ccErrors) {
		CurrCCStack = NULL;
		ccErrors++;
	    } else if (NXLocalHandler.code == err_strAllocFailed && (NXStrTable)NXLocalHandler.data1 == newInfo.strTable && stringsErrors < 2) {
	      /* be sure to parse as much as we're throwing away */
		reqFlags |= oldInfo->infoParsed;
		if (oldInfo)
		    free(oldInfo->globalInfo);
		free(oldInfo);
	      /* ??? Ideally we wouldnt forget existing widths info */
		oldInfo = NULL;		/* must redo, all strings invalid */
		CurrStringTable = NULL;
		stringsErrors++;
	    } else {
		NX_DURING {
		    lexCloseFile(&lexInfo);
		} NX_HANDLER {
		} NX_ENDHANDLER
		if (PrintTimes)
		    fprintf(stderr, "failed!\n");
		return NULL;
	    }
	} NX_ENDHANDLER
    } while (!done);
    if (!oldInfo)
	oldInfo = malloc(sizeof(AFMInfo));
    *oldInfo = newInfo;
    oldInfo->infoParsed |= parseFlags;
    if (PrintTimes) {
	gettimeofday(&end, NULL);
	diff.tv_usec = end.tv_usec - start.tv_usec;
	diff.tv_sec = end.tv_sec - start.tv_sec;
	if (diff.tv_usec < 0) {
	    diff.tv_usec += 1000*1000;
	    diff.tv_sec--;
	}
	fprintf(stderr, "%f msec\n", diff.tv_usec / 1000.0 + diff.tv_sec * 1000.0);
    }
    return oldInfo;
}


/* Top of recursive decent parsing.  Deals with top level directives, and calls sub-parsers for CharMetrics, other sections. */
static void parseFile(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    int i, tempInt;
    const TokenInfo *tok;
    char *tempStr;

    if (lexInfo->reqMask & BASIC_INFO)
	afmInfo->globalInfo = calloc(1, sizeof(NXGlobalFontInfo));
    else
	lexInfo->reencode = !strcmp(NSEncName, NXStringTablePtrFromOffset(afmInfo->strTable, afmInfo->globalInfo->encodingScheme));
    while(!done) {
	lexToken(lexInfo, &tok);
	if (!(lexInfo->reqMask & BASIC_INFO) && tok->tag >= StartFontMetrics_t && tok->tag <= ScreenFontSize_t) {
	    gobbleNewline(lexInfo);
	    continue;
	}
	switch (tok->tag) {
	    case StartFontMetrics_t:
	    case FontName_t:
	    case FullName_t:
	    case FamilyName_t:
	    case Weight_t:
	    case Version_t:
	    case Notice_t:
		lexString(lexInfo, (NXStackOffset *)((char *)(afmInfo->globalInfo) + tok->offset));
		break;
	    case CapHeight_t:
	    case XHeight_t:
	    case Ascender_t:
	    case Descender_t:
	    case UnderlinePosition_t:
	    case UnderlineThickness_t:
	    case ItalicAngle_t:
		lexNumber(lexInfo, (float *)((char *)(afmInfo->globalInfo) + tok->offset));
		lexNewline(lexInfo);
		break;
	    case ScreenFontSize_t:
		lexInteger(lexInfo, &tempInt);
		afmInfo->globalInfo->screenFontSize = tempInt;
		lexNewline(lexInfo);
		break;
	    case IsFixedPitch_t:
	    case IsScreenFont_t:
		lexBoolean(lexInfo, (char *)(afmInfo->globalInfo) + tok->offset);
		lexNewline(lexInfo);
		break;
	    case EndFontMetrics_t:
		lexInfo->doneMask |= (BASIC_INFO | FULL_INFO);
		done = TRUE;
		break;
	    case FontBBox_t:
		for (i = 0; i < 4; i++)
		    lexNumber(lexInfo, afmInfo->globalInfo->fontBBox+i);
		lexNewline(lexInfo);
		break;
	    case EncodingScheme_t:
		lexStringCopy(lexInfo, &tempStr);
		if (!strcmp(tempStr, "AdobeStandardEncoding")) {
		    lexInfo->reencode = TRUE;
		    (void)NXStringTableInsert(afmInfo->strTable, NSEncName, &afmInfo->globalInfo->encodingScheme);
		} else
		    (void)NXStringTableInsert(afmInfo->strTable, tempStr, &afmInfo->globalInfo->encodingScheme);
		free(tempStr);
		break;
	    case StartCharMetrics_t:
	        lexInfo->doneMask |= BASIC_INFO;
		if (!(lexInfo->reqMask & ~lexInfo->doneMask))
		    done = TRUE;
		else if (lexInfo->reqMask & (WIDTH_INFO | FULL_INFO)) {
		    parseCharMetrics(lexInfo, afmInfo);
		    lexInfo->doneMask |= WIDTH_INFO;
		    if (!(lexInfo->reqMask & ~lexInfo->doneMask))
			done = TRUE;
		} else {
		    NX_ASSERT(lexInfo->reqMask & CC_INFO, "Should have stopped parsing");
		    skipSection(lexInfo, EndCharMetrics_t, C_t, L_t);
		}
		break;
	    case StartKernData_t:
#ifndef CACHE_DATA_BUILD
		if (lexInfo->reqMask & FULL_INFO)
		    parseKerningData(lexInfo, afmInfo);
		else {
		    NX_ASSERT(lexInfo->reqMask & CC_INFO, "Should have stopped parsing");
		    skipSection(lexInfo, EndKernData_t, StartTrackKern_t, KPX_t);
		}
#else
		skipSection(lexInfo, EndKernData_t, StartTrackKern_t, KPX_t);
#endif CACHE_DATA_BUILD
		break;
	    case StartComposites_t:
#ifndef CACHE_DATA_BUILD
		if (lexInfo->reqMask & CC_INFO) {
		    parseCompositeChars(lexInfo, afmInfo);
		    lexInfo->doneMask |= CC_INFO;
		} else {
		    NX_ASSERT(lexInfo->reqMask & FULL_INFO, "Should have stopped parsing");
		    skipSection(lexInfo, EndComposites_t, CC_t, PCC_t);
		}
#else
		skipSection(lexInfo, EndComposites_t, CC_t, PCC_t);
#endif CACHE_DATA_BUILD
		break;		
	    case StartKernPairs_t:
#ifndef CACHE_DATA_BUILD
		if (lexInfo->reqMask & FULL_INFO)
		    parsePairKerningData(lexInfo, afmInfo);
		else {
		    NX_ASSERT(lexInfo->reqMask & CC_INFO, "Should have stopped parsing");
		    skipSection(lexInfo, EndKernPairs_t, KP_t, KPX_t);
		}
#else
		skipSection(lexInfo, EndKernPairs_t, KP_t, KPX_t);
#endif CACHE_DATA_BUILD
		break;		
	    case StartTrackKern_t:
#ifndef CACHE_DATA_BUILD
		if (lexInfo->reqMask & FULL_INFO)
		    parseTrackKerningData(lexInfo, afmInfo);
		else {
		    NX_ASSERT(lexInfo->reqMask & CC_INFO, "Should have stopped parsing");
		    skipSection(lexInfo, EndTrackKern_t, TrackKern_t, TrackKern_t);
		}
#else
		skipSection(lexInfo, EndTrackKern_t, TrackKern_t, TrackKern_t);
#endif CACHE_DATA_BUILD
		break;		
	    case newline_t:
		break;
	    case unknown_t:
		gobbleNewline(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }
    if (!afmInfo->globalInfo->isScreenFont)
	scaleData(afmInfo, lexInfo->reqMask & lexInfo->doneMask);
    if (lexInfo->metrics) {
	NXFreeHashTable(lexInfo->metrics);
	lexInfo->metrics = NULL;
    }
}


/* parses character metric information.  File should be positioned at the start of the first line of metric info. */
static void parseCharMetrics(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    const TokenInfo *tok;
    int i;
    int intCharCode;
    int metricsExp;
    float *widths = NULL, *w;		/* initted for clean -Wall */
    float *yWidths = NULL;		/* initted for clean -Wall */
    NXCharMetrics *charMetrics = NULL;	/* initted for clean -Wall */
    NXCharMetrics *cm;			/* current CharMetric being filled */
    NXCharMetrics dummyMetric;		/* dummy cm when not parsing full */
    int metricsRead = 0;		/* num metrics read */
    TempLig *ligs = NULL;		/* initted for clean -Wall */
    int numLigs = 0;			/* # offsets in ligs array */
    int parsedMetric = FALSE;		/* did we parse a metric this line? */
    static const NXHashTablePrototype metricsProto = {metricsHash, metricsCompare, NXNoEffectFree, 0};
    int fixedCharCode = FALSE;		/* did we change the char code of */
					/* this metric for NSEncoding? */
    float currWidth = NAN;		/* width of current char */
    short *encoding, *enc;

    lexInteger(lexInfo, &metricsExp);
    lexNewline(lexInfo);
    if (lexInfo->reqMask & WIDTH_INFO) {
	widths = NXStackAllocPtr(afmInfo->widthsStack, ENCODING_SIZE * sizeof(float), sizeof(float));
	for (w = widths, i = ENCODING_SIZE; i--; *w++ = NAN)
	    ;
    }
    if (lexInfo->reqMask & FULL_INFO) {
	cm = charMetrics = NXStackAllocPtr(afmInfo->dataStack, metricsExp * sizeof(NXCharMetrics), sizeof(int));
	bzero(cm, metricsExp * sizeof(NXCharMetrics));
	ligs = malloc(sizeof(TempLig));
	lexInfo->metrics = NXCreateHashTable(metricsProto, metricsExp, afmInfo->strTable);
    } else
	cm = &dummyMetric;
    while(!done) {
	lexToken(lexInfo, &tok);
	switch (tok->tag) {
	    case C_t:
		lexInteger(lexInfo, &intCharCode);
		if (!fixedCharCode)
		    cm->charCode = intCharCode;
		lexSeparator(lexInfo);
		parsedMetric = TRUE;
		break;
	    case WX_t:
		lexNumber(lexInfo, &currWidth);
		cm->xWidth = currWidth;
		lexSeparator(lexInfo);
		parsedMetric = TRUE;
		break;
	    case W_t:
		lexNumber(lexInfo, &currWidth);
		cm->xWidth = currWidth;
		if (lexInfo->reqMask & FULL_INFO) {
		    float yWidth;			/* yWidth of char */

		    lexNumber(lexInfo, &yWidth);
		    lexSeparator(lexInfo);
		    if (yWidth != 0.0) {
			if (!afmInfo->globalInfo->hasYWidths) {
			    yWidths = NXStackAllocPtr(afmInfo->dataStack, metricsExp * sizeof(float), sizeof(int));
			    for (w = yWidths, i = metricsExp; i--; *w++ = 0.0)
				;
			    afmInfo->globalInfo->hasYWidths = TRUE;
			}
			yWidths[cm - charMetrics] = yWidth;
		    }
		} else {
		    if (!lexInfo->reencode) {
			gobbleNewline(lexInfo);
			goto FINISH_CHAR;
		    } else
			gobbleSeparator(lexInfo);
		}
		parsedMetric = TRUE;
		break;
	    case N_t:
		if (lexInfo->reencode && cm->charCode == -1) {
		    unsigned char *tableEntry;

		  /* if char is one of the ISOLatin1 chars that we encode, change its char number in the metrics.  "+1" is to generate correct prototype for the hash and compare functions (at least one char before the name. */
		    tableEntry = NXHashGet(ISOLatinTable, lexInfo->bufCurr);
		    if (tableEntry) {
			cm->charCode = *tableEntry;
			fixedCharCode = TRUE;
		    }
		}
		if (lexInfo->reqMask & FULL_INFO) {
		    lexName(lexInfo, &cm->name);
		    lexSeparator(lexInfo);
		} else {
		    gobbleNewline(lexInfo);
		    goto FINISH_CHAR;
		}
		parsedMetric = TRUE;
		break;
	    case B_t:
		if (lexInfo->reqMask & FULL_INFO) {
		    for (i = 0; i < 4; i++)
			lexNumber(lexInfo, cm->bbox+i);
		    lexSeparator(lexInfo);
		} else
		    gobbleSeparator(lexInfo);
		parsedMetric = TRUE;
		break;
	    case L_t:
		if (lexInfo->reqMask & FULL_INFO) {
		    ligs = realloc(ligs, (numLigs + 1) * sizeof(TempLig));
		    ligs[numLigs].first = cm;
		    lexName(lexInfo, &ligs[numLigs].second);
		    lexName(lexInfo, &ligs[numLigs].lig);
		    numLigs++;
		    lexSeparator(lexInfo);
		} else
		    gobbleSeparator(lexInfo);
		parsedMetric = TRUE;
		break;
	    case EndCharMetrics_t:
		lexNewline(lexInfo);
		if (metricsRead != metricsExp)
		    NX_RAISE(err_metricsCount, (void *)metricsExp, (void *)metricsRead);
		done = TRUE;
		break;
	    case newline_t:
	    FINISH_CHAR:
		if (parsedMetric) {
		    metricsRead++;
		    if (lexInfo->reqMask & WIDTH_INFO) {
			if (currWidth == currWidth) {	/* if !still NAN */
			    if (cm->charCode >= 0)
				widths[cm->charCode] = currWidth;
			    currWidth = NAN;
			}
		    }
		    if (lexInfo->reqMask & FULL_INFO) {
			if(NXHashInsert(lexInfo->metrics, cm))
			    NX_RAISE(err_nameReused, (void *)cm->name, 0);
			if (metricsRead < metricsExp)
			    cm++;
		    }
		    parsedMetric = FALSE;
		    fixedCharCode = FALSE;
		}
		break;
	    case unknown_t:
		gobbleSeparator(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }

    if (lexInfo->reqMask & FULL_INFO) {
	copyLigatures(ligs, numLigs, charMetrics, lexInfo->metrics, afmInfo);
        free(ligs);
	afmInfo->globalInfo->charMetrics = NXStackOffsetFromPtr(afmInfo->dataStack, charMetrics);
	afmInfo->globalInfo->numCharMetrics = metricsExp;

	encoding = NXStackAllocPtr(afmInfo->dataStack, ENCODING_SIZE * sizeof(short), sizeof(int));
	for (enc = encoding, i = ENCODING_SIZE; i--; *enc++ = -1)
	    ;
	for (cm = charMetrics, i = metricsExp; i--; cm++)
	    if (cm->charCode != -1)
		encoding[cm->charCode] = cm - charMetrics;
	afmInfo->globalInfo->encoding = NXStackOffsetFromPtr(afmInfo->dataStack, encoding);
	if (afmInfo->globalInfo->hasYWidths)
	    afmInfo->globalInfo->yWidths = NXStackOffsetFromPtr(afmInfo->dataStack, yWidths);
	else
	    afmInfo->globalInfo->yWidths = -1;
    }
    
    if (lexInfo->reqMask & WIDTH_INFO) {
      /* sets the widths of all unknown chars to space or 0 */
	currWidth = widths[' '];
      /* space should always be encoded, but just in case... */
	if (currWidth != currWidth)	/* if space is unencoded */
	    currWidth = 0.0;
	for (w = widths, i = ENCODING_SIZE; i--; w++)
	    if (*w != *w)			/* if value still NAN */
		*w = currWidth;
	afmInfo->globalInfo->widths = NXStackOffsetFromPtr(afmInfo->widthsStack, widths);
    }
}


static void copyLigatures(TempLig *tempLigs, int numTempLigs, NXCharMetrics *charMetrics, NXHashTable *table, AFMInfo *afmInfo)
{
    NXLigature *realLigs = NULL, *rl;		/* initted for clean -Wall */
    int numLigs = 0;
    NXEncodedLigature *realEncLigs = NULL, *erl;  /* initted for clean -Wall */
    int numEncLigs = 0;
    TempLig *tl;
    NXCharMetrics *secondCM, *ligCM;
    int i;
    NXStack dataStack = afmInfo->dataStack;
    NXGlobalFontInfo *globalInfo = afmInfo->globalInfo;

    if (numTempLigs) {
	rl = realLigs = NXStackAllocPtr(dataStack, numTempLigs * sizeof(NXLigature), sizeof(int));
	for (tl = tempLigs, i = numTempLigs; i--; tl++) {
	    rl->firstCharIndex = tl->first - charMetrics;
	    secondCM = lookupMetric(tl->second, table);
	    ligCM = lookupMetric(tl->lig, table);
	    if (!secondCM || !ligCM) {	/* lig ref'ed char not in font */
		if (!secondCM)
		    reportMissingChar(afmInfo, tl->second, "ligature");
		if (!ligCM)
		    reportMissingChar(afmInfo, tl->lig, "ligature");
		continue;
	    }
	    rl->secondCharIndex = secondCM - charMetrics;
	    rl->ligatureIndex = ligCM - charMetrics;
	    numLigs++;
	    rl++;
	    if (tl->first->charCode != -1 && secondCM->charCode != -1 && ligCM->charCode != -1) {
		erl = NXStackAllocPtr(dataStack, sizeof(NXEncodedLigature), sizeof(int));
		if (!numEncLigs)
		    realEncLigs = erl;
		erl->firstChar = tl->first->charCode;
		erl->secondChar = secondCM->charCode;
		erl->ligatureChar = ligCM->charCode;
		numEncLigs++;
	    }
	}
    }

    globalInfo->ligatures = numLigs ? NXStackOffsetFromPtr(dataStack, realLigs) : -1;
    globalInfo->numLigatures = numLigs;

    if (numEncLigs) {
	qsort(realEncLigs, numEncLigs, sizeof(NXEncodedLigature), &encLigCompare);
	globalInfo->encLigatures = NXStackOffsetFromPtr(dataStack, realEncLigs);
    } else
	globalInfo->encLigatures = -1;
    globalInfo->numEncLigatures = numEncLigs;
}

#ifndef CACHE_DATA_BUILD

/* parses kerning data.  File should be positioned at the start of the first line of kerning info. */
static void parseKerningData(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    const TokenInfo *tok;

    while(!done) {
	lexToken(lexInfo, &tok);
	switch (tok->tag) {
	    case StartTrackKern_t:
		parseTrackKerningData(lexInfo, afmInfo);
		break;
	    case StartKernPairs_t:
		parsePairKerningData(lexInfo, afmInfo);
		break;
	    case EndKernData_t:
		lexNewline(lexInfo);
		done = TRUE;
		break;
	    case newline_t:
		break;
	    case unknown_t:
		gobbleNewline(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }
}


/* parses track kerning data.  File should be positioned at the start of the first line of track kerning info. */
static void parseTrackKerningData(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    const TokenInfo *tok;
    int tracksExp;
    int tracksRead = 0;
    NXTrackKern *tracks;
    NXTrackKern *trk;

    lexInteger(lexInfo, &tracksExp);
    lexNewline(lexInfo);
    trk = tracks = NXStackAllocPtr(afmInfo->dataStack, tracksExp * sizeof(NXTrackKern), sizeof(int));
    bzero(trk, tracksExp * sizeof(NXTrackKern));
    while(!done) {
	lexToken(lexInfo, &tok);
	switch (tok->tag) {
	    case TrackKern_t:
		lexInteger(lexInfo, &trk->degree);
		lexNumber(lexInfo, &trk->minPointSize);
		lexNumber(lexInfo, &trk->minKernAmount);
		lexNumber(lexInfo, &trk->maxPointSize);
		lexNumber(lexInfo, &trk->maxKernAmount);
		lexNewline(lexInfo);
		tracksRead++;
		if (tracksRead < tracksExp)
		    trk++;
		break;
	    case EndTrackKern_t:
		lexNewline(lexInfo);
		if (tracksRead != tracksExp)
		    NX_RAISE(err_tracksCount, (void *)tracksExp, (void *)tracksRead);
		done = TRUE;
		break;
	    case newline_t:
		break;
	    case unknown_t:
		gobbleNewline(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }
    afmInfo->globalInfo->trackKerns = NXStackOffsetFromPtr(afmInfo->dataStack, tracks);
    afmInfo->globalInfo->numTrackKerns = tracksExp;
}


/* parses pair kerning data.  File should be positioned at the start of the first line of pair kerning info. */
static void parsePairKerningData(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    const TokenInfo *tok;
    int pairsExp;
    int pairsRead = 0;
    int pairsStored = 0;
    int i;
    NXKernPair *kernPairs = NULL;	/* initted for clean -Wall */
    NXKernPair *kp = NULL;		/* initted for clean -Wall */
    NXKernXPair *kernXPairs;
    NXKernXPair *kpx;
    NXStackOffset cName;		/* name of char */
    int secondCharIndex;		/* val for current kern pair */
    float dx, dy;			/* vals for current kern pair */
    NXCharMetrics sentinel;
    NXCharMetrics *c1Metrics = &sentinel;
    NXCharMetrics *c2Metrics;
    NXCharMetrics *metricsArray = NXStackPtrFromOffset(afmInfo->dataStack, afmInfo->globalInfo->charMetrics);

    lexInteger(lexInfo, &pairsExp);
    lexNewline(lexInfo);
    kpx = kernXPairs = NXStackAllocPtr(afmInfo->dataStack, pairsExp * sizeof(NXKernXPair), sizeof(int));
    bzero(kpx, pairsExp * sizeof(NXKernXPair));
    c1Metrics->name = -1;		/* nonsense value to force lookup */
    while(!done) {
	lexToken(lexInfo, &tok);
	switch (tok->tag) {
	    case KPX_t:
	    case KP_t:
		lexName(lexInfo, &cName);
		if (c1Metrics->name != cName)
		    c1Metrics = lookupMetric(cName, lexInfo->metrics);
		if (!c1Metrics)
		    reportMissingChar(afmInfo, cName, "pair kerning");
		lexName(lexInfo, &cName);
		c2Metrics = lookupMetric(cName, lexInfo->metrics);
		if (!c2Metrics)
		    reportMissingChar(afmInfo, cName, "pair kerning");
		lexNumber(lexInfo, &dx);
		if (tok->tag == KP_t)
		    lexNumber(lexInfo, &dy);
		else
		    dy = 0.0;
		if (!c1Metrics || !c2Metrics) {
		    if (!c1Metrics)
			c1Metrics = &sentinel;
		    pairsRead++;
		} else if (dx != 0.0 || dy != 0.0) {
		    secondCharIndex = c2Metrics - metricsArray;
		    if (dy != 0.0 && !afmInfo->globalInfo->hasXYKerns) {
			kernPairs = NXStackAllocPtr(afmInfo->dataStack, pairsExp * (sizeof(NXKernPair) - sizeof(NXKernXPair)), sizeof(int));
			bzero(kernPairs, pairsExp * (sizeof(NXKernPair) - sizeof(NXKernXPair)));
			kernPairs = (NXKernPair *)kernXPairs;
			kp = kernPairs + pairsStored;
			kpx = kernXPairs + pairsStored;
			for (i = pairsStored; i--; ) {
			    kp--;
			    kpx--;
			    kp->dy = 0.0;
			    kp->dx = kpx->dx;
			    kp->secondCharIndex = kpx->secondCharIndex;
			}
			kp = kernPairs + pairsStored;
			afmInfo->globalInfo->hasXYKerns = TRUE;
		    }
		    pairsRead++;
		    pairsStored++;
		    if (afmInfo->globalInfo->hasXYKerns) {
			kp->secondCharIndex = secondCharIndex;
			kp->dx = dx;
			kp->dy = dy;
			if (!c1Metrics->numKernPairs)
			    c1Metrics->kernPairIndex = kp - kernPairs;
			if (pairsRead < pairsExp)
			    kp++;
		    } else {
			kpx->secondCharIndex = secondCharIndex;
			kpx->dx = dx;
			if (!c1Metrics->numKernPairs)
			    c1Metrics->kernPairIndex = kpx - kernXPairs;
			if (pairsRead < pairsExp)
			    kpx++;
		    }
		    c1Metrics->numKernPairs++;
		} else
		    pairsRead++;
		lexNewline(lexInfo);
		break;
	    case EndKernPairs_t:
		lexNewline(lexInfo);
		if (pairsRead != pairsExp)
		    NX_RAISE(err_kernPairsCount, (void *)pairsExp, (void *)pairsRead);
		done = TRUE;
		break;
	    case newline_t:
		break;
	    case unknown_t:
		gobbleNewline(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }
    afmInfo->globalInfo->kerns.kernXPairs = NXStackOffsetFromPtr(afmInfo->dataStack, kernXPairs);
    if (afmInfo->globalInfo->hasXYKerns)
	NXResetStack(afmInfo->dataStack, kernPairs + pairsStored, 0);
    else
	NXResetStack(afmInfo->dataStack, kernXPairs + pairsStored, 0);
    afmInfo->globalInfo->numKernPairs = pairsStored;
}


/* parses composite character data.  File should be positioned at the start of the first line of composite char info.  */
static void parseCompositeChars(LexInfo *lexInfo, AFMInfo *afmInfo)
{
    int done = FALSE;
    const TokenInfo *tok;
    int charsExp;
    int charsRead = 0;
    int charsStored = 0;
    int partsExp;
    int partsRead = -1;			/* initted for clean -Wall */
    int totalPartsRead = 0;		/* initted for clean -Wall */
    NXCompositeChar *compChars;
    NXCompositeChar *cc;
    NXCompositeCharPart *compCharParts;
    NXCompositeCharPart *ccp;
    const void *stackStart;
    int stackUsed;
    NXStackOffset cName;
    NXCharMetrics *cm;
    NXCharMetrics *metricsArray = NXStackPtrFromOffset(afmInfo->dataStack, afmInfo->globalInfo->charMetrics);
    int parsingCompCharLine = FALSE;

    lexInteger(lexInfo, &charsExp);
    lexNewline(lexInfo);
    compChars = cc = NXStackAllocPtr(afmInfo->ccStack, charsExp * sizeof(NXCompositeChar), sizeof(int));
    NXGetStackInfo(afmInfo->ccStack, &stackStart, &stackUsed, NULL, NULL, NULL);
    ccp = compCharParts = (NXCompositeCharPart *)((char *)stackStart + stackUsed);
    while(!done) {
	lexToken(lexInfo, &tok);
	switch (tok->tag) {
	    case CC_t:
		lexName(lexInfo, &cName);
		cm = lookupMetric(cName, lexInfo->metrics);
		if (cm) {
		    lexInteger(lexInfo, &partsExp);
		    ccp = NXStackAllocPtr(afmInfo->ccStack, partsExp * sizeof(NXCompositeCharPart), sizeof(int));
		    bzero(ccp, partsExp * sizeof(NXCompositeCharPart));
		    partsRead = 0;
		    cc->compCharIndex = cm - metricsArray;
		    cc->numParts = partsExp;
		    cc->firstPartIndex = ccp - compCharParts;
		    lexSeparator(lexInfo);
		    parsingCompCharLine = TRUE;
		} else {
		    reportMissingChar(afmInfo, cName, "composite character");
		    gobbleNewline(lexInfo);
		    charsRead++;
		}
		break;
	    case PCC_t:
		if (parsingCompCharLine) {
		    lexName(lexInfo, &cName);
		    cm = lookupMetric(cName, lexInfo->metrics);
		    if (cm) {
			ccp->partIndex = cm - metricsArray;
			lexNumber(lexInfo, &ccp->dx);
			lexNumber(lexInfo, &ccp->dy);
			lexSeparator(lexInfo);
			totalPartsRead++;
			partsRead++;
			if (partsRead < partsExp)
			    ccp++;
		    } else {
			reportMissingChar(afmInfo, cName, "composite character");
			gobbleNewline(lexInfo);
			charsRead++;
		    }
		} else
		    NX_RAISE(err_wrongToken, (void *)PCC_t, NULL);
		break;
	    case EndComposites_t:
		lexNewline(lexInfo);
		if (charsRead != charsExp)
		    NX_RAISE(err_compCharsCount, (void *)charsExp, (void *)charsRead);
		done = TRUE;
		break;
	    case newline_t:
		if (parsingCompCharLine) {
		    if (partsRead != partsExp)
			NX_RAISE(err_compPartsCount, (void *)partsExp, (void *)partsRead);
		    charsRead++;
		    charsStored++;
		    if (charsRead < charsExp)
			cc++;
		    parsingCompCharLine = FALSE;
		}
		break;
	    case unknown_t:
		gobbleNewline(lexInfo);
		break;
	    default:
	    	NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
	}
    }
    afmInfo->globalInfo->compositeChars = NXStackOffsetFromPtr(afmInfo->ccStack, compChars);
    afmInfo->globalInfo->numCompositeChars = charsStored;
    afmInfo->globalInfo->compositeCharParts = NXStackOffsetFromPtr(afmInfo->ccStack, compCharParts);
    afmInfo->globalInfo->numCompositeCharParts = totalPartsRead;
}

#endif CACHE_DATA_BUILD


/* skips a whole section.  We gobble the current line and read until we find the endToken, ignoring everything in the range between firstIgnore and lastIgnore.  */
static void skipSection(LexInfo *lexInfo, Token endToken, Token firstIgnore, Token lastIgnore)
{
    int done = FALSE;
    const TokenInfo *tok;

    gobbleNewline(lexInfo);
    while(!done) {
	lexToken(lexInfo, &tok);
	if (tok->tag >= firstIgnore && tok->tag <= lastIgnore)
	    gobbleNewline(lexInfo);
	else if (tok->tag == unknown_t)
	    gobbleNewline(lexInfo);
	else if (tok->tag == endToken) {
	    lexNewline(lexInfo);
	    done = TRUE;
	} else if (tok->tag != newline_t)
	    NX_RAISE(err_wrongToken, (void *)tok->tag, NULL);
    }
}


/* determines what work really needs to be done */
static int determineWork(int reqFlags, int existingFlags)
{
  /* add in  dependencies implied by requests */
    reqFlags |= BASIC_INFO;		/* always do the basics */
    if (reqFlags & CC_INFO)		/* comp chars needs char metrics */
	reqFlags |= FULL_INFO;
    return reqFlags & ~existingFlags;	/* remaining work to be done */
}


/* scales data from Font coord system to normal PS coord system */
static void scaleData(AFMInfo *afmInfo, int doMask)
{
    NXGlobalFontInfo *gInfo;
    float *f;
    int i, j;
    NXCharMetrics *cm;
    NXKernPair *kp;
    NXKernXPair *kpx;
    NXTrackKern *tk;
    NXCompositeCharPart *ccp;

    gInfo = afmInfo->globalInfo;
    if (doMask & BASIC_INFO) {
	f = gInfo->fontBBox;
	for (i = 4; i--; )
	    *f++ /= DEFAULT_UNITS_PER_POINT;
	gInfo->underlinePosition /= DEFAULT_UNITS_PER_POINT;
	gInfo->underlineThickness /= DEFAULT_UNITS_PER_POINT;
	gInfo->capHeight /= DEFAULT_UNITS_PER_POINT;
	gInfo->xHeight /= DEFAULT_UNITS_PER_POINT;
	gInfo->ascender /= DEFAULT_UNITS_PER_POINT;
	gInfo->descender /= DEFAULT_UNITS_PER_POINT;
    }

    if (doMask & WIDTH_INFO) {
	f = NXStackPtrFromOffset(afmInfo->widthsStack, gInfo->widths);
	for (i = ENCODING_SIZE; i--; )
	    *f++ /= DEFAULT_UNITS_PER_POINT;
    }

    if (doMask & FULL_INFO) {
	cm = NXStackPtrFromOffset(afmInfo->dataStack, gInfo->charMetrics);
	for (i = gInfo->numCharMetrics; i--; ) {
	    cm->xWidth /= DEFAULT_UNITS_PER_POINT;
	    f = cm->bbox;
	    for (j = 4; j--; )
		*f++ /= DEFAULT_UNITS_PER_POINT;
	    cm++;
	}

	if (gInfo->hasYWidths) {
	    f = NXStackPtrFromOffset(afmInfo->dataStack, gInfo->yWidths);
	    for (i = gInfo->numCharMetrics; i--; )
		*f++ /= DEFAULT_UNITS_PER_POINT;
	}

	if (gInfo->hasXYKerns) {
	    kp = NXStackPtrFromOffset(afmInfo->dataStack, gInfo->kerns.kernPairs);
	    for (i = gInfo->numKernPairs; i--; kp++) {
		kp->dx /= DEFAULT_UNITS_PER_POINT;
		kp->dy /= DEFAULT_UNITS_PER_POINT;
	    }
	} else {
	    kpx = NXStackPtrFromOffset(afmInfo->dataStack, gInfo->kerns.kernXPairs);
	    for (i = gInfo->numKernPairs; i--; kpx++)
		kpx->dx /= DEFAULT_UNITS_PER_POINT;
	}

	tk = NXStackPtrFromOffset(afmInfo->dataStack, gInfo->trackKerns);
	for (i = gInfo->numTrackKerns; i--; tk++) {
	    tk->minKernAmount /= DEFAULT_UNITS_PER_POINT;
	    tk->maxKernAmount /= DEFAULT_UNITS_PER_POINT;
	}
    }

    if (doMask & CC_INFO) {
	ccp = NXStackPtrFromOffset(afmInfo->ccStack, gInfo->compositeCharParts);
	for (i = gInfo->numCompositeCharParts; i--; ccp++) {
	    ccp->dx /= DEFAULT_UNITS_PER_POINT;
	    ccp->dy /= DEFAULT_UNITS_PER_POINT;
	}
    }
}


static void reportMissingChar(AFMInfo *afmInfo, NXStackOffset offset, const char *context)
{
    char *fontName;

    if (afmInfo->globalInfo->fullName >= 0)
	fontName = NXStringTablePtrFromOffset(afmInfo->strTable, afmInfo->globalInfo->fullName);
    else
	fontName = "(Unknown)";
    NXLogError("Unknown character \"%s\" referenced in %s data for font %s", NXStringTablePtrFromOffset(afmInfo->strTable, offset), context, fontName);
}


/* Are we at the end of the file in the lexInfo? */
#define LEX_EOF(li)  ((li)->bufCurr >= (li)->bufEnd)

/* All lex routines should check for EOF or call someone who does (lexSkipWS).  The scan routines wont go beyond EOL, so they should only be called when its know there is an EOL for them to hit.  The file will always end in an EOL. */


/* opens a file for parsing, filling in LexInfo structure.  We ensure that the last character of the file is a newline to make parsing robust and easier. */
static void lexOpenFile(const char *filename, LexInfo *newInfo)
{
    struct stat stat;
    kern_return_t kret = KERN_SUCCESS;
    int memLen;

    bzero(newInfo, sizeof(LexInfo));
    newInfo->fd = open(filename, O_RDONLY, 0);
    if (newInfo->fd != -1) {
	fstat(newInfo->fd, &stat);
	if (stat.st_size == 0)
	    goto errorBailout;
	memLen = stat.st_size + 1;
	kret = vm_allocate(task_self(), (vm_address_t *)&newInfo->bufStart, memLen, TRUE);
	if (kret == KERN_SUCCESS) {
	    newInfo->bytesAlloced = memLen;
	    kret = map_fd(newInfo->fd, 0, (vm_offset_t *)&newInfo->bufStart, FALSE, memLen);
	    if (kret == KERN_SUCCESS) {
		newInfo->bufCurr = newInfo->bufStart;
		newInfo->bufEnd = newInfo->bufStart + stat.st_size;
		if (newInfo->bufEnd[-1] != '\n')
		    *newInfo->bufEnd++ = '\n';
	    }
	}
    } else
	NX_RAISE(err_cantOpen, (void *)errno, 0);
 errorBailout:
    if (kret != KERN_SUCCESS) {
	if (newInfo->fd >= 0)
	    (void)close(newInfo->fd);
	if (newInfo->bufStart)
	    (void)vm_deallocate(task_self(), (vm_address_t)newInfo->bufStart, newInfo->bytesAlloced);
	NX_RAISE(err_cantOpen, (void *)kret, 0);
    }
}


/* closes a file opened with lexOpenFile */
static void lexCloseFile(LexInfo *lexInfo)
{
    int closeRet;
    kern_return_t kret = KERN_SUCCESS;

    closeRet = close(lexInfo->fd);    
    kret = vm_deallocate(task_self(), (vm_address_t)lexInfo->bufStart, lexInfo->bytesAlloced);
    if (closeRet == -1)
	NX_RAISE(err_cantClose, (void *)errno, 0);
    if (kret != KERN_SUCCESS)
	NX_RAISE(err_cantClose, (void *)kret, 0);
}


/* scans the file for the next token */
static void lexToken(LexInfo *lexInfo, const TokenInfo **token)
{
    TokenInfo temp;
    static const TokenInfo unknownToken = {"", unknown_t, 0};
    static const TokenInfo newLineToken = {"\n", newline_t, 0};

    lexSkipWS(lexInfo);
    if (isalpha(*lexInfo->bufCurr)) {
	temp.name = lexInfo->bufCurr;
	*token = NXHashGet(TokenTable, &temp);
	if (!*token)
	    *token = &unknownToken;
	lexInfo->bufCurr = nameScan(lexInfo->bufCurr);
    } else if (*lexInfo->bufCurr == '\n') {
	*token = &newLineToken;
	lexInfo->bufCurr++;
    } else
	NX_RAISE(err_invalidToken, lexInfo->bufCurr, 0); 
}


/* scans file for next newline terminated string, skipping initial whitespace.  Adds string to the string table.  Leaves file at next line. */
static void lexString(LexInfo *lexInfo, NXStackOffset *str)
{
    char *start;

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	(void)NXStringTableInsert(lexInfo->strTable, "", str);
    else {
	start = lexInfo->bufCurr;
	while (*lexInfo->bufCurr != '\n')
	    lexInfo->bufCurr++;
	(void)NXStringTableInsertWithLength(lexInfo->strTable, start, lexInfo->bufCurr - start, str);
    }
    lexInfo->bufCurr++;
}


/* Scans file for next newline terminated string, skipping initial whitespace.  Returns a copy of the string made with malloc (caller must use free()).  Leaves file at start of next line. */
static void lexStringCopy(LexInfo *lexInfo, char **str)
{
    char *start, *end;
    char *new;
    int len;
    char emptyStr = '\0';

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	start = end = &emptyStr;
    else {
	start = lexInfo->bufCurr;
	while (*lexInfo->bufCurr != '\n')
	    lexInfo->bufCurr++;
	end = lexInfo->bufCurr;
    }
    len = end - start;
    new = malloc(len + 1);
    bcopy(start, new, len);
    new[len] = '\0';
    *str = new;
    lexInfo->bufCurr++;
}


static void lexName(LexInfo *lexInfo, NXStackOffset *str)
{
    char *end;

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	NX_RAISE(err_invalidName, 0, 0);
    end = nameScan(lexInfo->bufCurr);
    (void)NXStringTableInsertWithLength(lexInfo->strTable, lexInfo->bufCurr, end - lexInfo->bufCurr, str);
    lexInfo->bufCurr = end;
}


/* scans file for a number, real or int.  Returns number in num.  Leaves the file at the next whitespace, semi-colon or EOF. */
static void lexNumber(LexInfo *lexInfo, float *num)
{
    char *save;
    char *s;
    int intVal;

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	NX_RAISE(err_invalidNumber, 0, 0);
    save = s = lexInfo->bufCurr;
    s = intScan(s, &intVal);
    if (((*s == 'e') || (*s == 'E') || (*s == '.')))
	*num = atof(save);
    else
	*num = intVal;
    lexInfo->bufCurr = nameScan(lexInfo->bufCurr);
}


/* scans the next integer in the file, returning the value in num.  If doNewLine is TRUE, we consume the rest of the line, leaving the file at the start of the next line or at EOF.  If doNewLine is FALSE, we leave the file at the next whitespace or
 semi-colon. */
static void lexInteger(LexInfo *lexInfo, int *num)
{
    char *s;

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	NX_RAISE(err_invalidInteger, 0, 0);
    s = intScan(lexInfo->bufCurr, num);
    lexInfo->bufCurr = nameScan(lexInfo->bufCurr);
}


static void lexBoolean(LexInfo *lexInfo, char *bool)
{
    char *end;
    int len;

    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	NX_RAISE(err_invalidBoolean, 0, 0);
    end = nameScan(lexInfo->bufCurr);
    len = end - lexInfo->bufCurr; 
    if (len == 4 && !strncmp("true", lexInfo->bufCurr, 4))
	*bool = TRUE;
    else if (len == 5 && !strncmp("false", lexInfo->bufCurr, 5))
	*bool = FALSE;
    else
	NX_RAISE(err_invalidBoolean, 0, 0);
    lexInfo->bufCurr = nameScan(lexInfo->bufCurr);
}


/* scans for a newline */
static void lexNewline(LexInfo *lexInfo)
{
    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == '\n')
	lexInfo->bufCurr++;
    else
	NX_RAISE(err_badEOL, 0, 0);
}


/* gobbles chars through the next newline */
static void gobbleNewline(LexInfo *lexInfo)
{
    if (LEX_EOF(lexInfo))
	NX_RAISE(err_EOF, 0, 0);
    while (*lexInfo->bufCurr != '\n')
	lexInfo->bufCurr++;
    lexInfo->bufCurr++;
}


/* scans the file through the next separator (;) or up to the next newline */
static void lexSeparator(LexInfo *lexInfo)
{
    lexSkipWS(lexInfo);
    if (*lexInfo->bufCurr == ';')
	lexInfo->bufCurr++;
    else if (*lexInfo->bufCurr == '\n')
	return;
    else
	NX_RAISE(err_badSep, 0, 0);
}


/* gobbles chars through the next separator (;) or up to the next newline */
static void gobbleSeparator(LexInfo *lexInfo)
{
    if (LEX_EOF(lexInfo))
	NX_RAISE(err_EOF, 0, 0);
    while (*lexInfo->bufCurr != '\n' && *lexInfo->bufCurr != ';')
	lexInfo->bufCurr++;
    if (*lexInfo->bufCurr == ';')
	lexInfo->bufCurr++;
}


/* skips white space in file up to newline.  On exit the next char is either some non-WS or newline.  Raises exception on EOF. */
static void lexSkipWS(LexInfo *lexInfo)
{
    if (LEX_EOF(lexInfo))
	NX_RAISE(err_EOF, 0, 0);
    while (IS_WHITE(*lexInfo->bufCurr))
	lexInfo->bufCurr++;
}


/* scans a signed integer from the string, returning the updated position */
static char *intScan(char *s, int *num)
{
    int intVal;
    int negate;

    negate = FALSE;
    intVal = 0;
    if (*s == '-') {
	s++;
	negate = TRUE;
    } else if (*s == '+')
	s++;
    while(isdigit(*s))
	intVal = intVal * 10 + *s++ - '0';
    *num = negate ? -intVal : intVal;
    return s;
}


/* scans a name of non-WS, non-semicolon chars, returning updated position */
static char *nameScan(char *s)
{
    while (!IS_WHITE_NL_OR_SEMI(*s))
	s++;
    return s;
}


/* hash function for ISOLatin1 char names */
static unsigned isoNameHash(const void *info, const void *data)
{
    register unsigned		hash = 0;
    register unsigned char	*s;	/* unsigned to avoid a sign-extend */
    register int		shift = 0;
    
    s = (unsigned char *)data+1;
    while (IS_WHITE(*s))
	s++;
    while (!IS_WHITE_NL_OR_SEMI(*s)) { 
	hash ^= *s++ << shift;
	shift = (shift + 8) % 32;
    }
    return hash;
}


static int isoNameStrcmp(const void *info, const void *data1, const void *data2)
{
    const char *tableString, *fileString;
    int offset;

    offset = ISOLatinTableData - (char *)data1;
    if(offset >= 0 && offset < sizeof(ISOLatinTableData)) {
	tableString = data1+1;
	fileString = data2+1;
    } else {
	fileString = data1+1;
	tableString = data2+1;
    }
    while (IS_WHITE(*fileString))
	fileString++;
    while (*tableString != ';') {
	if (*tableString++ != *fileString++)
	    return FALSE;
    }
    return IS_WHITE_NL_OR_SEMI(*fileString);
}


/* hash function for hashtable of Tokens */
static unsigned tokenHash(const void *info, const void *data)
{
    register unsigned		hash = 0;
    register unsigned char	*s;	/* unsigned to avoid a sign-extend */
    register int		shift = 0;
    
    s = (unsigned char *)((TokenInfo *)data)->name;
    while (isalnum(*s)) { 
	hash ^= *s++ << shift;
	shift = (shift + 8) % 32;
    }
    return hash;
}


/* compare function for hashtable of Tokens */
static int tokenCompare(const void *info, const void *data1, const void *data2)
{
    const char *tableString, *fileString;
    const TokenInfo *tok1 = (TokenInfo *)data1;
    const TokenInfo *tok2 = (TokenInfo *)data2;
    int offset;

    offset = (char *)data1 - (char *)TokenTableData;
    if(offset >= 0 && offset < sizeof(TokenTableData)) {
	tableString = tok1->name;
	fileString = tok2->name;
    } else {
	tableString = tok2->name;
	fileString = tok1->name;
    }
    while (*tableString) {
	if (*tableString++ != *fileString++)
	    return FALSE;
    }
    return !isalnum(*fileString);
}


/* hash function for hashtable of metrics */
static unsigned metricsHash(const void *info, const void *data)
{
    char *str;

    str = NXStringTablePtrFromOffset((NXStrTable)info, ((NXCharMetrics *)data)->name);
    return NXStrHash(NULL, str);
}


/* compare function for hashtable of metrics */
static int metricsCompare(const void *info, const void *data1, const void *data2)
{
    char *str1, *str2;

    str1 = NXStringTablePtrFromOffset((NXStrTable)info, ((NXCharMetrics *)data1)->name);
    str2 = NXStringTablePtrFromOffset((NXStrTable)info, ((NXCharMetrics *)data2)->name);
    return NXStrIsEqual(NULL, str1 , str2);
}


/* looks up a metric by name (represented as an offset) */
static NXCharMetrics *lookupMetric(NXStackOffset offset, NXHashTable *table)
{
    NXCharMetrics temp;

    temp.name = offset;
    return NXHashGet(table, &temp);
}


void initAFMParser(void)
{
    static const NXHashTablePrototype isoProto = {isoNameHash, isoNameStrcmp, NXNoEffectFree, 0};
    const char *start;
    static const NXHashTablePrototype tokenProto = {tokenHash, tokenCompare, NXNoEffectFree, 0};
    const TokenInfo *tok;

    ISOLatinTable = NXCreateHashTable(isoProto, 71, NULL);
    for (start = ISOLatinTableData; *start; start = index(start, ';') + 1)
	NXHashInsert(ISOLatinTable, start);
    TokenTable = NXCreateHashTable(tokenProto, sizeof(TokenTableData)/sizeof(TokenInfo) - 1, NULL);
    for (tok = TokenTableData; tok->name; tok++)
	NXHashInsert(TokenTable, tok);
}


/* qsort compare functions */

static int encLigCompare(void *data1, void *data2)
{
    NXEncodedLigature *l1 = data1, *l2 = data2;

    if (l1->firstChar != l2->firstChar)
	return l1->firstChar - l2->firstChar;
    else
	return l1->secondChar - l2->secondChar;
}

/*

Modifications (starting at 0.8):

80
--
 4/09/90 trey	rewrite to support getting kerns and other AFM data

86
--
 6/12/90 trey	finished comp chars
		fixed bogus screenFontSize parsing
 7/29/90 trey	slight mods for use by cacheAFMData

*/

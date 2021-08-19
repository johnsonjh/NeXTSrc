/*
	afmUtils.c
  	Copyright 1989, NeXT, Inc.
	Responsibility: Trey Matteson
  
	Contains code for byte-swapping the afm data structures.
  
	Shared routines between afm readers.
*/

#import "pbtypes.h"
#import "afmprivate.h"
#import "nextstd.h"
#import "mach.h"
#import "libc.h"

static void swapGlobalInfo(NXGlobalFontInfo *info);
static void copySwapCharMetrics(void *srcArg, void *destArg, int num);
static void copySwapLongs(void *srcArg, void *destArg, int num);
#define copySwapShorts(srcArg, destArg, num)	\
		swab(((char *)(srcArg)), ((char *)(destArg)), ((num)*2))

void _NXGetSwappedAFMInfo(AFMInfo *info,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt,
		data_t *fullData, unsigned int *fullDataCnt,
		data_t *ccData, unsigned int *ccDataCnt)
{
    int widthsSize, encodingSize, yWidthsSize, cmSize, ligsSize, encLigsSize, kernsSize, tKernsSize, ccSize, ccpSize;
    int totalSize;
    char *stuff, *currStuff;
    NXGlobalFontInfo *gi = info->globalInfo;
    NXGlobalFontInfo *newGI;

    totalSize = sizeof(NXGlobalFontInfo);
    totalSize += widthsSize = (info->infoParsed & WIDTH_INFO) ? ENCODING_SIZE * sizeof(float) : 0;
    if (info->infoParsed & FULL_INFO) {
	totalSize += encodingSize = ENCODING_SIZE * sizeof(short);
	totalSize += yWidthsSize = gi->hasYWidths ? gi->numCharMetrics * sizeof(float) : 0;
	totalSize += cmSize = gi->numCharMetrics * sizeof(NXCharMetrics);
	totalSize += ligsSize = gi->numLigatures * sizeof(NXLigature);
	totalSize += encLigsSize = gi->numEncLigatures * sizeof(NXEncodedLigature);
	totalSize += kernsSize = gi->numKernPairs * (gi->hasXYKerns ? sizeof(NXKernPair) : sizeof(NXKernXPair));
	totalSize += tKernsSize = gi->numTrackKerns * sizeof(NXTrackKern);
    } else
	encodingSize = yWidthsSize = cmSize = ligsSize = encLigsSize = kernsSize = tKernsSize = 0;
    if (info->infoParsed & CC_INFO) {
	totalSize += ccSize = gi->numCompositeChars * sizeof(NXCompositeChar);
	totalSize += ccpSize = gi->numCompositeCharParts * sizeof(NXCompositeCharPart);
    } else
	ccSize = ccpSize = 0;
    currStuff = stuff = calloc(1, totalSize);

    newGI = (NXGlobalFontInfo *)*globalInfo = stuff;
    *newGI = *gi;
    *globalInfoCnt = sizeof(NXGlobalFontInfo);
    currStuff += sizeof(NXGlobalFontInfo);

    if (info->infoParsed & WIDTH_INFO) {
	*widths = currStuff;
	*widthsCnt = widthsSize;
	newGI->widths = 0;
	copySwapLongs(NXStackPtrFromOffset(info->widthsStack, gi->widths), currStuff, ENCODING_SIZE);
	currStuff += widthsSize;
    }

    if (info->infoParsed & FULL_INFO) {
	*fullData = currStuff;
	if (encodingSize) {
	    newGI->encoding = currStuff - *fullData;
	    copySwapShorts(NXStackPtrFromOffset(info->dataStack, gi->encoding), currStuff, ENCODING_SIZE);
	    currStuff += encodingSize;
	}
	if (yWidthsSize) {
	    newGI->yWidths = currStuff - *fullData;
	    copySwapLongs(NXStackPtrFromOffset(info->dataStack, gi->yWidths), currStuff, yWidthsSize / sizeof(float));
	    currStuff += yWidthsSize;
	}
	if (cmSize) {
	    newGI->charMetrics = currStuff - *fullData;
	    copySwapCharMetrics(NXStackPtrFromOffset(info->dataStack, gi->charMetrics), currStuff, cmSize / sizeof(NXCharMetrics));
	    currStuff += cmSize;
	}
	if (ligsSize) {
	    newGI->ligatures = currStuff - *fullData;
	    copySwapLongs(NXStackPtrFromOffset(info->dataStack, gi->ligatures), currStuff, ligsSize / sizeof(int));
	    currStuff += ligsSize;
	}
	if (encLigsSize) {
	    newGI->encLigatures = currStuff - *fullData;
	    bcopy(NXStackPtrFromOffset(info->dataStack, gi->encLigatures), currStuff, encLigsSize);
	    currStuff += encLigsSize;
	}
	if (kernsSize) {
	    newGI->kerns.kernPairs = currStuff - *fullData;
	    copySwapLongs(NXStackPtrFromOffset(info->dataStack, gi->hasXYKerns ? gi->kerns.kernPairs : gi->kerns.kernXPairs), currStuff, kernsSize / sizeof(int));
	    currStuff += kernsSize;
	}
	if (tKernsSize) {
	    newGI->trackKerns = currStuff - *fullData;
	    copySwapLongs(NXStackPtrFromOffset(info->dataStack, gi->trackKerns), currStuff, tKernsSize / sizeof(int));
	    currStuff += tKernsSize;
	}
	*fullDataCnt = currStuff - *fullData;
    }

    if (info->infoParsed & CC_INFO) {
	*ccData = currStuff;
	if (ccSize) {
	    newGI->compositeChars = currStuff - *ccData;
	    copySwapLongs(NXStackPtrFromOffset(info->ccStack, gi->compositeChars), currStuff, ccSize / sizeof(int));
	    currStuff += ccSize;
	}
	if (ccpSize) {
	    newGI->compositeCharParts = currStuff - *ccData;
	    copySwapLongs(NXStackPtrFromOffset(info->ccStack, gi->compositeCharParts), currStuff, ccpSize / sizeof(int));
	    currStuff += ccpSize;
	}
	*ccDataCnt = currStuff - *ccData;
    }
    NX_ASSERT(totalSize == currStuff - stuff, "Funny sizes in byte swapper");

  /* swap this last since we filled in the offsets above */
    swapGlobalInfo(newGI);
}


static void swapGlobalInfo(NXGlobalFontInfo *info)
{
  /* swaps: formatVersion name fullName familyName weight italicAngle */
    swapLongs(&info->formatVersion, 6);
    
    swapShorts(&info->screenFontSize, 1);
    
  /* swaps: underlinePosition underlineThickness version notice
		encodingScheme capHeight xHeight ascender descender */
    swapLongs(&info->fontBBox, 13);
    
    swapShorts(&info->hasYWidths, 1);
    swapLongs(&info->widths, 1);

  /* swaps: encoding yWidths charMetrics numCharMetrics ligatures numLigatures
		encLigatures numEncLigatures kerns numKernPairs
		trackKerns numTrackKerns compositeChars numCompositeChars
		compositeCharParts numCompositeCharParts */
    swapLongs(&info->encoding, 16);
}


static void copySwapCharMetrics(void *srcArg, void *destArg, int num)
{
    NXCharMetrics *src;
    NXCharMetrics *dest;

    for (src = srcArg, dest = destArg;  num--; src++, dest++) {
	copySwapShorts(&src->charCode, &dest->charCode, 1);
	src->numKernPairs = dest->numKernPairs;
	copySwapLongs(&src->xWidth, &dest->xWidth, 7);
    }
}


static void copySwapLongs(void *srcArg, void *destArg, int num)
{
    char *src = srcArg;
    char *dest = destArg;

    while (num--) {
	*dest++ = src[3];
	*dest++ = src[2];
	*dest++ = src[1];
	*dest++ = src[0];
	src += 4;
    }
}


void swapLongs(void *data, int num)
{
    char *bytePtr;
    char swapper;

    for (bytePtr = data; num--; bytePtr += 4) {
	swapper = bytePtr[0];
	bytePtr[0] = bytePtr[3];
	bytePtr[3] = swapper;
	swapper = bytePtr[1];
	bytePtr[1] = bytePtr[2];
	bytePtr[2] = swapper;
    }
}


void swapShorts(void *data, int num)
{
    char *bytePtr;
    char swapper;

    for (bytePtr = (char *)data; num--; bytePtr += 2) {
	swapper = bytePtr[0];
	bytePtr[0] = bytePtr[1];
	bytePtr[1] = swapper;
    }
}


int strStructCompare(void *data1, void *data2)
{
    return strcmp(*(char **)data1, *(char **)data2);
}


/* Parses size and face name from the AFM file name.  faceName is assumed to have enough space in it for the faceName.  A size of zero means that its not a screen font.  If faceName is returned as an empty string, the name was unparsable.  */
void parseFileName(const char *nameArg, char *faceName, int *size)
{
    char nameSpace[MAXPATHLEN];
    char *name = nameSpace;
    char *dot;
    int len = strlen(nameArg);

    NX_ASSERT(*nameArg != '/', "Absolute path passed to parseFileName");
    bcopy(nameArg, name, len + 1);
    if (len > 4 && !strcmp(name + len - 4, ".afm")) {
	name[len - 4] = '\0';
	len -= 4;
    }
    *faceName = '\0';
    if (!strncmp("Screen-", name, 7)) {
	name += 7;
	dot = rindex(name, '.');
	if (dot) {
	    *dot = '\0';
	    *size = atoi(dot + 1);
	    if (*size)
		strcpy(faceName, name);
	}
    } else {
	*size = 0;
	bcopy(name, faceName, len + 1);
    }
}

 
/* looks for s2 as a suffix of s1 */
int rstrcmp(const char *s1, const char *s2)
{
    int len1 = strlen(s1);
    int len2 = strlen(s2);

    if (len1 >= len2) {
	return strcmp(s1 + len1 - len2, s2);
    } else
	return 1;
}


void pathStrcat(const char *head, const char *tail, char *result)
{
    if (head != result)
	strcpy(result, head);
    strcat(result, "/");
    strcat(result, tail);
}

/* copies data mapped from a file over NFS so it doesnt disappear on us later when someone unlinks the file */
void fixNFSData(const char *path, char **start, int *len, int *maxLen)
{
    char *newData;
    kern_return_t kret;
    struct stat statBuf;

    if (stat(path, &statBuf) == 0) {
	if (major(statBuf.st_dev) == 255) {		/* if over NFS */
	    kret = vm_allocate(task_self(), (vm_address_t *)&newData, *len, TRUE);
	    if (kret == KERN_SUCCESS) {
		bcopy(*start, newData, *len);
		(void)vm_deallocate(task_self(), (vm_address_t)*start, *maxLen);
		*start = newData;
		*maxLen = *len;
	    }
	}
    } else
	NX_ASSERT(FALSE, "Couldn't stat file we just opened");
}

/*

Modifications (starting at 0.8):

10/29/89 trey	created
*/



/*
	afmFinder.c
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	This file provides functions for dealing with font metrics files.
	The macros BIG_ENDIAN_NATIVE and LITTLE_ENDIAN_NATIVE should be
	configured for the machine's architecture (in afmprivate.h).
*/

#import <sys/types.h>
#import <sys/stat.h>
#import <sys/time.h>
#import <sys/dir.h>
#import <sys/param.h>
#import <mach.h>
#import <objc/hashtable.h>
#import <streams/streams.h>
#import "pbtypes.h"
#import "perfTimer.h"
#import "afmprivate.h"
#import "nextstd.h"

extern BOOL _NXSetUser(const char *user);
extern BOOL _NXGetHomeDir(char *user);
extern void _NXUnsetUser(void);

typedef struct {	/* a cache file containing AFM info */
    char version;
    char *start;
    int maxLen;
    AFMCDirEntry *sections;
    NXStrTable strings;
    int numFaces;
    AFMCFace *faces;
    AFMCFile *files;
    char *data;
} AFMCache;

typedef struct {	/* a dir containing fonts */
    char *name;			/* must have name first for hashing */
    time_t lastCheck;		/* last time we updated this dir's info */
    time_t cacheTime;		/* time stamp of cache we're using */
    short disinterests;		/* increments for every request that doesnt */
				/* mention this dir.  Used for disposal. */
    char exists;		/* does this dir even exist? */
    char cacheOutOfDate;	/* has cache been updated? */
    AFMCache *cache;		/* cache file of afm data */
    NXHashTable *files;		/* hash table of AFMFiles in this dir */
    char mark;
    char hasCache;		/* does dir contain a .afmcache file? */
} AFMDir;

typedef struct {	/* a file containing AFM info */
    char *name;			/* must have name first for hashing */
    AFMInfo *info;		/* data parsed from the file */
    char isOldStyle;		/* is the file in a .font dir or "afm" dir? */
} AFMFile;

/* a func called by findAFMData */
typedef int AFMFileProc(AFMDir *dir, AFMFile *file, AFMCFace *cface, AFMCFile *cfile, void *data);

typedef struct {	/* set of params passed through findAFMData */
    int attributes;
    int *totalAttributes;
    AFMInfo *results;
} ParsingParams;

typedef struct {	/* set of params passed through findAFMData */
    void *data;
    unsigned int length;
} ContentsParams;

/* max number of times a directory can be not in the requested path before 
   being flushed. */
#define MAX_DISINTERESTS	15

/* AFMDir's that we have cached info on */
static NXHashTable *CachedDirs;

static int commonGetFullAFMInfo(int useNativeEncoding, int *attributes,
		data_t paramStrings, unsigned int paramStringsCnt,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt, int *widthsTag,
		data_t *fullData, unsigned int *fullDataCnt, int *fullDataTag,
		data_t *ccData, unsigned int *ccDataCnt, int *ccDataTag,
		data_t *strings, unsigned int *stringsCnt, int *stringsTag);
static int findAFMData(AFMFileProc *func, void *data, data_t paramStrings, unsigned int paramStringsCnt);
static AFMDir *getCorrectAFMDir(const char *dirName, const char *user);
static AFMFileProc getCorrectInfo;
static AFMFileProc mapAFMFile;
#ifdef OLD
static void readAFMDir(AFMDir *dir);
static void loadAFMFiles(AFMDir *dir, const char *fontDir);
#endif
static AFMDir *allocAFMDir(const char *dirName);
static void freeAFMDir(const void *info, void *data);
static AFMFile *allocAFMFile(const char *fileName, AFMDir *dir);
static void freeAFMFile(const void *info, void *data);
static void preparePath(char *path, int *numElements);
static char *resolveHomeDir(const char *inPath, char *outPath, const char *user);
static const char *getHomeDir(const char *user);
static void clearDirMarks(void);
static void checkDirMarks(void);
static char findAFMFileInDir(AFMDir *dir, char *fontName);
static void getFullFilename(AFMDir *dir, AFMFile *file, char *fullName);
static void doUntoAllTables(void (*stackFunc)(NXStack aStack), void (*stringFunc)(NXStrTable aTable), BOOL doSharedTables);
static void printStringTable(NXStrTable aTable);
static void printStack(NXStack aStack);

static AFMCache *openCache(const char *path);
static void freeCache(AFMCache *cache);
static AFMCFile *findFileInFace(AFMCache *cache, AFMCFace *face, unsigned short size);
static AFMInfo *parseAFMCache(AFMCache *cache, AFMCFace *cface, AFMCFile *cfile);
static int parseWidths(NXStack widthsStack, char *data);


/* gets full AFM info in big endian format */
int _NXGetFullAFMInfoBigE(port_t server, int *attributes,
		data_t paramStrings, unsigned int paramStringsCnt,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt, int *widthsTag,
		data_t *fullData, unsigned int *fullDataCnt, int *fullDataTag,
		data_t *ccData, unsigned int *ccDataCnt, int *ccDataTag,
		data_t *strings, unsigned int *stringsCnt, int *stringsTag)
{
    return commonGetFullAFMInfo(BIG_ENDIAN_NATIVE, attributes,
		paramStrings, paramStringsCnt, globalInfo, globalInfoCnt,
		widths, widthsCnt, widthsTag,
		fullData, fullDataCnt, fullDataTag,
		ccData, ccDataCnt, ccDataTag,
		strings, stringsCnt, stringsTag);
}


/* gets full AFM info in big endian format */
int _NXGetFullAFMInfoLittleE(port_t server, int *attributes,
		data_t paramStrings, unsigned int paramStringsCnt,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt, int *widthsTag,
		data_t *fullData, unsigned int *fullDataCnt, int *fullDataTag,
		data_t *ccData, unsigned int *ccDataCnt, int *ccDataTag,
		data_t *strings, unsigned int *stringsCnt, int *stringsTag)
{
    return commonGetFullAFMInfo(LITTLE_ENDIAN_NATIVE, attributes,
		paramStrings, paramStringsCnt, globalInfo, globalInfoCnt,
		widths, widthsCnt, widthsTag,
		fullData, fullDataCnt, fullDataTag,
		ccData, ccDataCnt, ccDataTag,
		strings, stringsCnt, stringsTag);
}


static void doUntoAllTables(void (*stackFunc)(NXStack aStack), void (*stringFunc)(NXStrTable aTable), BOOL doSharedTables)
{
    AFMDir *dir;
    AFMFile *file;
    NXHashTable *stacks, *strings;
    NXHashState	enumState1, enumState2;
    NXStack aStack;
    NXStrTable aStringTable;

    stacks = NXCreateHashTable(NXPtrPrototype, 0, NULL);
    strings = NXCreateHashTable(NXPtrPrototype, 0, NULL);
    enumState1 = NXInitHashState(CachedDirs);
    while (NXNextHashState(CachedDirs, &enumState1, &(void *)dir)) {
	enumState2 = NXInitHashState(dir->files);
	while (NXNextHashState(dir->files, &enumState2, &(void *)file)) {
	    if (file->info) {
		NXHashInsert(stacks, file->info->widthsStack);
		NXHashInsert(stacks, file->info->dataStack);
		NXHashInsert(stacks, file->info->ccStack);
		if (doSharedTables || !file->info->sharedStrTable)
		    NXHashInsert(strings, file->info->strTable);
		if (file->info->globalInfo)
		    free(file->info->globalInfo);
		free(file->info);
		file->info = NULL;
	    }
	}
	NXResetHashTable(dir->files);
    }
    NXHashInsert(stacks, CurrWidthsStack);
    NXHashInsert(stacks, CurrDataStack);
    NXHashInsert(stacks, CurrCCStack);
    NXHashInsert(strings, CurrStringTable);

    enumState1 = NXInitHashState(stacks);
    while (NXNextHashState(stacks, &enumState1, &(void *)aStack))
	if (aStack)
	    (*stackFunc)(aStack);
    NXFreeHashTable(stacks);

    enumState1 = NXInitHashState(strings);
    while (NXNextHashState(strings, &enumState1, &(void *)aStringTable))
	if (aStringTable)
	    (*stringFunc)(aStringTable);
    NXFreeHashTable(strings);
}


void _NXFreeFontData(port_t server)
{
    doUntoAllTables(&NXDestroyStack, &NXDestroyStringTable, NO);
    CurrWidthsStack = CurrDataStack = CurrCCStack = NULL;
    CurrStringTable = NULL;
}


static void printStack(NXStack aStack)
{
    const void *start;
    int used;
    int maxSize;
    void *userData;
    NXStackType type;

    NXGetStackInfo(aStack, &start, &used, &maxSize, &userData, &type);
    fprintf(stderr, "stack at %#x, used=%d, max=%d, data=%d, type=%d\n", start, used, maxSize, userData, type);
}


static void printStringTable(NXStrTable aTable)
{
    const char *start;
    int used;
    int maxSize;
    void *userData;
    NXStackType type;

    NXGetStringTableInfo(aTable, &start, &used, &maxSize, &userData, &type);
    fprintf(stderr, "stack at %#x, used=%d, max=%d, data=%d, type=%d\n", start, used, maxSize, userData, type);
}


void _NXPrintFontData(port_t server)
{
  /* This wont work because doUnto does some free's */
    doUntoAllTables(&printStack, &printStringTable, YES);
}


/* returns the contents of an AFM file */
int _NXGetAFMFileContents(port_t server,
		data_t paramStrings, unsigned int paramStringsCnt,
		deallocData_t *fileContents, unsigned int *fileContentsCnt)
{
    ContentsParams findDataParams;
    int success;

    success = findAFMData(&mapAFMFile, &findDataParams, paramStrings, paramStringsCnt);
    (void)vm_deallocate(task_self(), (vm_address_t)paramStrings, paramStringsCnt);
    if (success) {
	*fileContents = findDataParams.data;
	*fileContentsCnt = findDataParams.length;
	return AFM_OK;
    } else {
	*fileContents = NULL;
	fileContentsCnt = 0;
	return AFM_NOT_FOUND;
    }
}


static int commonGetFullAFMInfo(int useNativeEncoding, int *attributes,
		data_t paramStrings, unsigned int paramStringsCnt,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt, int *widthsTag,
		data_t *fullData, unsigned int *fullDataCnt, int *fullDataTag,
		data_t *ccData, unsigned int *ccDataCnt, int *ccDataTag,
		data_t *strings, unsigned int *stringsCnt, int *stringsTag)
{
    void *tag;
    ParsingParams findDataParams;
    AFMInfo *info;
    int success;

    *globalInfo = *widths = *fullData = *ccData = *strings = NULL;
    *globalInfoCnt = *widthsCnt = *fullDataCnt = *ccDataCnt = *stringsCnt = 0;
    findDataParams.attributes = *attributes;
    findDataParams.totalAttributes = attributes;
    success = findAFMData(&getCorrectInfo, &findDataParams, paramStrings, paramStringsCnt);
    (void)vm_deallocate(task_self(), (vm_address_t)paramStrings, paramStringsCnt);
    if (success) {
	info = findDataParams.results;
	if (info->strTable) {
	    NXGetStringTableInfo(info->strTable, strings, (int *)stringsCnt, NULL, &tag, NULL);
	    *stringsTag = (int)tag;
	}
	if (useNativeEncoding) {
	    *globalInfo = (char *)info->globalInfo;
	    *globalInfoCnt = sizeof(NXGlobalFontInfo);
	    if (info->widthsStack) {
		NXGetStackInfo(info->widthsStack, (void **)widths, (int *)widthsCnt, NULL, &tag, NULL);
		*widthsTag = (int)tag;
	    }
	    if (info->dataStack) {
		NXGetStackInfo(info->dataStack, (void **)fullData, (int *)fullDataCnt, NULL, &tag, NULL);
		*fullDataTag = (int)tag;
	    }
	    if (info->ccStack) {
		NXGetStackInfo(info->ccStack, (void **)ccData, (int *)ccDataCnt, NULL, &tag, NULL);
		*ccDataTag = (int)tag;
	    }
	} else
	    _NXGetSwappedAFMInfo(info, globalInfo, globalInfoCnt, widths, widthsCnt, fullData, fullDataCnt, ccData, ccDataCnt);
	return AFM_OK;
    } else
	return AFM_NOT_FOUND;
}


/* calls func with each AFM file found along the path until func returns true.  findAFMData returns whether func ever returned true.  */
int findAFMData(AFMFileProc *func, void *info, data_t paramStrings, unsigned int paramStringsCnt)
{
    char *path = paramStrings;
    char *user = path + strlen(path) + 1;
    char *fontName = user + strlen(user) + 1;
    int numPathElements;
    AFMDir *dir;
    AFMFile *file;
    AFMCFace *cface;
    AFMCFile *cfile;
    char pathBuffer[MAXPATHLEN];
    char tempPathSpace[MAXPATHLEN];
    char *tempPath = tempPathSpace;
    char *dirName;
    int foundOne = FALSE;
    int size;

    preparePath(path, &numPathElements);
    clearDirMarks();
    while (numPathElements--) {
	MARKTIME(DoMarks, "finding dir", 1);
	if (*path == '~')
	    dirName = resolveHomeDir(path, pathBuffer, user);
	else
	    dirName = path;
	if (dirName)
	    if (!foundOne) {
		if (dir = getCorrectAFMDir(dirName, user)) {
		    dir->mark = TRUE;
		    if (dir->exists) {
			file = NXHashGet(dir->files, &fontName);
			cface = NULL;
			cfile = NULL;
			if (!file) {
			    int allocFile = FALSE;
			    char isOldStyle;
	    
			    isOldStyle = -1;
			    if (dir->hasCache) {
				parseFileName(fontName, tempPath, &size);
				cface = bsearch(&tempPath, dir->cache->faces, dir->cache->numFaces, sizeof(AFMCFace), &strStructCompare);
				if (cface) {
				    cfile = findFileInFace(dir->cache, cface, size);
				    if (cfile)
					allocFile = TRUE;
				}
			    }
			    if (!dir->hasCache || (dir->cacheOutOfDate && !allocFile)) {
				isOldStyle = findAFMFileInDir(dir, fontName);
				if (isOldStyle != -1)
				    allocFile = TRUE;
			    }
			    if (allocFile) {
				file = allocAFMFile(fontName, dir);
				file->isOldStyle = isOldStyle;
				NXHashInsert(dir->files, file);
			    }
			}
			if (file) {
			    BOOL setUserOK;

			    MARKTIME(DoMarks, "Processing file", 1);
			    setUserOK = _NXSetUser(user);
			    if ((*func)(dir, file, cface, cfile, info))
				foundOne = TRUE;
			    if (setUserOK)
				_NXUnsetUser();
			}
		    }
		}
	    } else {
		if (dir = NXHashGet(CachedDirs, &dirName))
		    dir->mark = TRUE;
	    }
	path += strlen(path) + 1;	/* skip to next element */
    }
    MARKTIME(DoMarks, "findAFMData afterglue", 1);
    checkDirMarks();
    return foundOne;
}


void _NXInitAFMModule()
{
    NXHashTablePrototype AFMDirProto;

    if (sizeof(NXGlobalFontInfo) != sizeof(NXFontMetrics)) {
	NXLogError("_NXInitAFMModule: sizes of structures not correct");
	exit(-1);
    }
    AFMDirProto = NXStrStructKeyPrototype;
    AFMDirProto.free = freeAFMDir;
    CachedDirs = NXCreateHashTable(AFMDirProto, 3, NULL);
    initAFMParser();
}


/* gets current info for a directory */
static AFMDir *getCorrectAFMDir(const char *dirName, const char *user)
{
    const char *dirNamePtr = dirName;
    AFMDir *dir;
    struct timeval now;
    char cacheExists;
    BOOL setUserOK;

    dir = NXHashGet(CachedDirs, &dirNamePtr);
    if (!dir)
	if (dir = allocAFMDir(dirName))
	    NXHashInsert(CachedDirs, dir);

    if (dir) {
	gettimeofday(&now, NULL);
	if (dir->lastCheck + 30 < now.tv_sec) {	/* if info too stale */
	    struct stat statBuf;
	    char path[MAXPATHLEN];

	    setUserOK = _NXSetUser(user);
	    if (stat(dir->name, &statBuf) == 0) {
		dir->exists = TRUE;
		dir->lastCheck = now.tv_sec;

		if (!dir->cacheOutOfDate) {
		    pathStrcat(dir->name, AFMCACHE_NAME, path);
		    cacheExists = (stat(path, &statBuf) == 0);
    
		    if (dir->hasCache)
			dir->cacheOutOfDate = cacheExists ? statBuf.st_mtime > dir->cacheTime : TRUE;
		    else if (cacheExists && (dir->cache = openCache(path))) {
			dir->hasCache = TRUE;
			dir->cacheTime = statBuf.st_mtime;
		    }
		}
	    } else {
		dir->exists = FALSE;
		dir->hasCache = FALSE;
	    }
	    if (setUserOK)
		_NXUnsetUser();
	}
    }
    return dir;
}


static AFMDir *allocAFMDir(const char *dirName)
{
    AFMDir *new;
    NXHashTablePrototype AFMFileProto;

    new = (AFMDir *)calloc(1, sizeof(AFMDir));
    new->name = NXCopyStringBuffer(dirName);
    AFMFileProto = NXStrStructKeyPrototype;
    AFMFileProto.free = freeAFMFile;
    new->files = NXCreateHashTable(AFMFileProto, 0, NULL);
    return new;
}


static void freeAFMDir(const void *info, void *data)
{
    AFMDir *dir = (AFMDir *)data;

    free(dir->name);
    NXFreeHashTable(dir->files);
    if (dir->hasCache)
	freeCache(dir->cache);
    free(data);
}


#ifdef OLDOLDOLDOLDOLDOLD
/* loads filenames from a dir.  allocs hash table for them */
static void readAFMDir(AFMDir *dir)
{
    NXHashTablePrototype AFMFileProto;
    DIR *wholeDir;
    struct direct *dirEntry;
    struct timeval now;

    if (dir->files)
	NXFreeHashTable(dir->files);
    AFMFileProto.hash = indirStringHash;
    AFMFileProto.isEqual = indirStringEqu;
    AFMFileProto.free = freeAFMFile;
    AFMFileProto.style = 0;
    dir->files = NXCreateHashTable(AFMFileProto, 0, NULL);
    dir->hasNewFonts = dir->hasOldFonts = FALSE;
    if (wholeDir = opendir(dir->name)) {
	while (dirEntry = readdir(wholeDir))
	    if (!rstrcmp(dirEntry->d_name, ".font")) {
		loadAFMFiles(dir, dirEntry->d_name);
		dir->hasNewFonts = TRUE;
	    } else if (!strcmp(dirEntry->d_name, "afm"))
		dir->hasOldFonts = TRUE;
	closedir(wholeDir);
    }
    if (dir->hasOldFonts)
      /* must be done second so symlink emulations of old scheme dont end up in the hash table instead of the real files. */
	loadAFMFiles(dir, NULL);
    gettimeofday(&now, NULL);
    dir->lastRead = now.tv_sec;
}


/* NULL font dir means look in the afm dir in the old style */
static void loadAFMFiles(AFMDir *dir, const char *fontDir)
{
    DIR *wholeDir;
    struct direct *dirEntry;
    char path[MAXPATHLEN];
    AFMFile *newFile;

    pathStrcat(dir->name, fontDir ? fontDir : "afm", path);
    if (wholeDir = opendir(path)) {
	while (dirEntry = readdir(wholeDir))
	    if (!rstrcmp(dirEntry->d_name, ".afm")) {
		newFile = allocAFMFile(dirEntry->d_name, dir);
		newFile->isOldStyle = (fontDir == NULL);
		if (NXHashInsertIfAbsent(dir->files, newFile) != newFile)
		    freeAFMFile(NULL, newFile);
	    }
	closedir(wholeDir);
    }
}
#endif OLDOLDOLDOLDOLDOLD


static AFMFile *allocAFMFile(const char *fileName, AFMDir *dir)
{
    AFMFile *new;

    new = calloc(1, sizeof(AFMFile));
    new->name = NXCopyStringBuffer(fileName);
    new->isOldStyle = -1;	/* nonsense value to find assumption bugs */
    return new;
}


static void freeAFMFile(const void *info, void *data)
{
    AFMFile *file = (AFMFile *)data;

    free(file->name);
    if (file->info) {
	if (file->info->globalInfo)
	    free(file->info->globalInfo);
#ifdef NOT_YET
	if (file->info->widths)
	    free(file->info->widths);
#endif
	free(file->info);
    }
    free(file);
}


static int mapAFMFile(AFMDir *dir, AFMFile *file, AFMCFace *cface, AFMCFile *cfile, void *info)
{
    NXStream *st;
    int maxlen;
    char fullName[MAXPATHLEN];
    ContentsParams *results = info;

    results->data = NULL;
    results->length = 0;
    if (file->isOldStyle == -1)
	file->isOldStyle = findAFMFileInDir(dir, file->name);
    if (file->isOldStyle == -1)
	return FALSE;
    getFullFilename(dir, file, fullName);
    st = NXMapFile(fullName, NX_READONLY);
    if (st) {
	NXGetMemoryBuffer(st, (char **)&results->data, (int *)&results->length, &maxlen);
	NXCloseMemory(st, NX_TRUNCATEBUFFER);
	return TRUE;
    } else
	return FALSE;
}


static int getCorrectInfo(AFMDir *dir, AFMFile *file, AFMCFace *cface, AFMCFile *cfile, void *info)
{
    char fullName[MAXPATHLEN];
    int infoRequested = 0;
    int existingInfo;
    int attributes = ((ParsingParams *)info)->attributes;
    int *totalAttributes = ((ParsingParams *)info)->totalAttributes;

    if (attributes & (NX_FONTHEADER | NX_FONTMETRICS))
	infoRequested |= BASIC_INFO;
    if (attributes & NX_FONTWIDTHS)
	infoRequested |= WIDTH_INFO;
    if (attributes & (NX_FONTCHARDATA | NX_FONTKERNING))
	infoRequested |= FULL_INFO;
    if (attributes & NX_FONTCOMPOSITES)
	infoRequested |= CC_INFO;
    existingInfo = file->info ? file->info->infoParsed : 0;

    if (infoRequested & ~existingInfo) {
	if (cfile && !file->info && !(infoRequested & (FULL_INFO | CC_INFO))) {
	    file->info = parseAFMCache(dir->cache, cface, cfile);
	} else {
	    if (file->isOldStyle == -1)
		file->isOldStyle = findAFMFileInDir(dir, file->name);
	    if (file->isOldStyle != -1) {
		getFullFilename(dir, file, fullName);
		file->info = parseAFMFile(fullName, file->info, infoRequested);
	    }
	}
    }
    if (file->info) {
	if (file->info->infoParsed & BASIC_INFO)
	    *totalAttributes = NX_FONTHEADER | NX_FONTMETRICS;
	if (file->info->infoParsed & WIDTH_INFO)
	    *totalAttributes |= NX_FONTWIDTHS;
	if (file->info->infoParsed & FULL_INFO)
	    *totalAttributes |= (NX_FONTCHARDATA | NX_FONTKERNING);
	if (file->info->infoParsed & CC_INFO)
	    *totalAttributes |= NX_FONTCOMPOSITES;
    }
    ((ParsingParams *)info)->results = file->info;
    return file->info != NULL;
}


static AFMCache *openCache(const char *path)
{
    AFMCache *cache = NULL;
    NXStream *st;
    char *start;
    int len, maxLen;
    AFMCFileHeader *header;
    char *kludgeData;
    AFMCDirEntry *sect;
    int i;
    AFMCFace *face;
    struct stat statBuf;

    st = NXMapFile(path, NX_READONLY);
    if (st) {
	NXGetMemoryBuffer(st, &start, &len, &maxLen);
	fixNFSData(path, &start, &len, &maxLen);
	header = (AFMCFileHeader *)start;
	if (header->isLittleEndian == LITTLE_ENDIAN_NATIVE) {
	    cache = calloc(1, sizeof(AFMCache));
	    cache->version = header->version;
	    cache->start = start;
	    cache->maxLen = maxLen;

	  /* restore start of string table */
	    kludgeData = start + *(int *)(start + 4);
	    cache->sections = (AFMCDirEntry *)(kludgeData + 12);
	    bcopy(kludgeData, start, 8);

	    sect = cache->sections + STRINGS_SECTION;
	    cache->strings = NXCreateStringTableFromMemory(start, sect->length, sect->length, (void *)(NextStackTag++), NX_stackFixedContig, FALSE);
	    sect = cache->sections + FACES_SECTION;
	    cache->numFaces = sect->length / sizeof(AFMCFace);
	    cache->faces = (AFMCFace *)(start + sect->offset);
	    sect = cache->sections + FILES_SECTION;
	    cache->files = (AFMCFile *)(start + sect->offset);
	    sect = cache->sections + BASIC_DATA_SECTION;
	    cache->data = start + sect->offset;

	    for (i = cache->numFaces, face = cache->faces; i--; face++)
		face->name = (int)NXStringTablePtrFromOffset(cache->strings, face->name);
	    NXCloseMemory(st, NX_SAVEBUFFER);
	} else
	    NXCloseMemory(st, NX_FREEBUFFER);
    }
    return cache;
}


static void freeCache(AFMCache *cache)
{
    if (cache) {
	(void)vm_deallocate(task_self(), (vm_address_t)cache->data, cache->maxLen);
	NXDestroyStringTable(cache->strings);
    }
    free(cache);
}


static AFMCFile *findFileInFace(AFMCache *cache, AFMCFace *face, unsigned short size)
{
    int i;
    AFMCFile *file;

    for (i = face->numFiles, file = cache->files + face->files; i--; file++)
	if (file->size == size)
	    return file;
    return NULL;
}


static void clearDirMarks(void)
{
    AFMDir *dir;
    NXHashState	enumState = NXInitHashState(CachedDirs);

    while (NXNextHashState(CachedDirs, &enumState, &(void *)dir))
	dir->mark = FALSE;
}


static void checkDirMarks(void)
{
    AFMDir *dir;
    NXHashState	enumState = NXInitHashState(CachedDirs);
#define LIST_SIZE  100
    AFMDir *freeList[LIST_SIZE];
    int numToFree = 0;

    while (NXNextHashState(CachedDirs, &enumState, &(void *)dir)) {
	if (dir->mark)
	    dir->disinterests = 0;
	else
	    dir->disinterests++;
	if (dir->disinterests > MAX_DISINTERESTS)
	    if (numToFree < LIST_SIZE)
		freeList[numToFree++] = dir;
    }
    while (numToFree--) {
	NXHashRemove(CachedDirs, freeList[numToFree]);
	freeAFMDir(NULL, freeList[numToFree]);
    }
}


/* for a dir that starts with ~, resolves it to an absolule path */
static char *resolveHomeDir(const char *inPath, char *outPath, const char *user)
{
    const char *who;		/* person who's home dir we're looking for */
    char whoBuffer[MAXPATHLEN];
    char *s;
    const char *homeDir;

    if (*++inPath == '/' || !*inPath)	/* "~/..." or just "~" */
	who = user;
    else {				/* "~joeUser/..." or "~joeUser" */
	s = whoBuffer;
	while (*inPath != '/' && *inPath)
	    *s++ = *inPath++;
	*s = '\0';
	who = whoBuffer;
    }
    if (homeDir = getHomeDir(who)) {
	strcpy(outPath, homeDir);
	strcat(outPath, inPath);
	return outPath;
    } else
	return NULL;
}


/* nulls out colons to separate elements and counts elements */
static void preparePath(char *path, int *numElements)
{
    *numElements = 0;
    if (*path) {
	do {
	    if (*path == ':') {
		*path = '\0';
		(*numElements)++;
	    }
	} while (*++path);
	(*numElements)++;
    }
}


static const char *getHomeDir(const char *user)
{
    static char *lastUser = NULL;
    static char *lastHomeDir = NULL;
    char buffer[MAXPATHLEN];

    if (lastUser && !strcmp(lastUser, user))
	return lastHomeDir;
    else {
	strcpy(buffer, user);
	if (_NXGetHomeDir(buffer)) {
	    if (lastUser)
		free(lastUser);
	    if (lastHomeDir)
		free(lastHomeDir);
	    lastUser = NXCopyStringBuffer(user);
	    lastHomeDir = NXCopyStringBuffer(buffer);
	    return lastHomeDir;
	} else
	    return NULL;
    }
}


/* returns 1 if file is found in the afm directory, 0 if found in a .font package, and -1 if not found at all. */
static char findAFMFileInDir(AFMDir *dir, char *fontName)
{
    AFMFile testFile;
    char tempPath[MAXPATHLEN];

    testFile.name = fontName;
    testFile.info = NULL;
    testFile.isOldStyle = FALSE;
    getFullFilename(dir, &testFile, tempPath);
    if (!access(tempPath, R_OK))
	return 0;
    else {
	testFile.isOldStyle = TRUE;
	getFullFilename(dir, &testFile, tempPath);
	if (!access(tempPath, R_OK))
	    return 1;
    }
    return -1;
}

static void getFullFilename(AFMDir *dir, AFMFile *file, char *fullName)
{
    char *dot;

    NX_ASSERT(file->isOldStyle != -1, "getFullFilename called with uncertain info.");
    if (file->isOldStyle) {
	pathStrcat(dir->name, "afm", fullName);
	pathStrcat(fullName, file->name, fullName);
    } else {
	if (strncmp(file->name, "Screen-", 7)) {
	    pathStrcat(dir->name, file->name, fullName);
	} else {
	    pathStrcat(dir->name, file->name + 7, fullName);
	    dot = rindex(fullName, '.');
	    if (dot)
		*dot = '\0';
	}
	strcat(fullName, ".font");
	pathStrcat(fullName, file->name, fullName);
    }
    strcat(fullName, ".afm");
}


static AFMInfo *parseAFMCache(AFMCache *cache, AFMCFace *cface, AFMCFile *cfile)
{
    struct timeval start, end, diff;
    AFMInfo *newInfo;
    NXGlobalFontInfo *gInfo;
    int used, max;
    char *dataStart;
    int dataSize;

    if (PrintTimes) {
    	fprintf(stderr, "Reading %s:%d  ", cface->name, cfile->size);
	gettimeofday(&start, NULL);
    }

    newInfo = calloc(1, sizeof(AFMInfo));
    if (CurrWidthsStack)
	NXGetStackInfo(CurrWidthsStack, NULL, &used, &max, NULL, NULL);
    if (!CurrWidthsStack || max - used < ENCODING_SIZE * sizeof(float))
	CurrWidthsStack = NXCreateStack(NX_stackFixedContig, WidthsMaxSize, (void *)(NextStackTag++));
    newInfo->widthsStack = CurrWidthsStack;
    newInfo->strTable = cache->strings;
    newInfo->sharedStrTable = TRUE;
    gInfo = newInfo->globalInfo = calloc(1, sizeof(NXGlobalFontInfo));
    dataStart = cache->data + cfile->data;
    dataSize = (char *)&gInfo->widths - (char *)gInfo;
    bcopy(dataStart, gInfo, dataSize);
    gInfo->widths = parseWidths(newInfo->widthsStack, dataStart + dataSize);
    newInfo->infoParsed = (BASIC_INFO | WIDTH_INFO);

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
    return newInfo;
}


/* returns offset into the stack where the widths are located */
static int parseWidths(NXStack widthsStack, char *data)
{
    float *widths, *w;
    float aFloat;
    int anInt;
    unsigned short aUShort;
    unsigned char aUChar;
    int num;
    char type;

    widths = NXStackAllocPtr(widthsStack, ENCODING_SIZE * sizeof(float), sizeof(float));
    type = *data;
    data += 2;
    switch (type) {
	case FLOAT_WIDTHS:
	    num = 0;
	    w = widths;
	    while (w < widths + ENCODING_SIZE) {
		aFloat = *(float *)data;
		data += 4;
		if (aFloat == 0.0) {
		    anInt = *(int *)data;
		    data += 4;
		    if (anInt == 1)
			*w++ = 0.0;
		    else {
			aFloat = *(float *)data;
			data += 4;
			while (anInt--)
			    *w++ = aFloat;
		    }
		} else
		    *w++ = aFloat;
	    }
	    break;
	case USHORT_WIDTHS:
	    num = 0;
	    w = widths;
	    while (w < widths + ENCODING_SIZE) {
		aUShort = *(unsigned short *)data;
		data += 2;
		if (aUShort == 0) {
		    anInt = *(unsigned short *)data;
		    data += 2;
		    if (anInt == 1)
			*w++ = 0.0;
		    else {
			aUShort = *(unsigned short *)data;
			data += 2;
			while (anInt--)
			    *w++ = (float)aUShort / 1000.0;
		    }
		} else
		    *w++ = (float)aUShort / 1000.0;
	    }
	    break;
	case UCHAR_WIDTHS:
	    num = 0;
	    w = widths;
	    while (w < widths + ENCODING_SIZE) {
		aUChar = *(unsigned char *)data;
		data += 1;
		if (aUChar == 0) {
		    anInt = *(unsigned char *)data;
		    if (anInt == 0)
			anInt = 256;
		    data += 1;
		    if (anInt == 1)
			*w++ = 0.0;
		    else {
			aUChar = *(unsigned char *)data;
			data += 1;
			while (anInt--)
			    *w++ = aUChar;
		    }
		} else
		    *w++ = aUChar;
	    }
	    break;
    }
    return NXStackOffsetFromPtr(widthsStack, widths);
}


/*

Modifications:

10/29/89 trey	support for little endian machines getting AFM data

80
--
 4/09/90 trey	support for getting kerns and other AFM data

85
--
 5/21/90 trey	added support for new .font files

86
--
 6/10/90 trey	finished comp char support, added _NXFreeFontData
		 added _NXGetAFMFileContents

87
--
 7/4/90 trey	nulls out global stack and string tables when freeing font data

89
--
 7/29/90 trey	nuked cache of which files are in what dirs
		added parsing of .afmcache files

93
--
 9/4/90 trey	fixed to work with old Font file organization

95
--
 10/1/90 trey	made use of _NXSetUser

*/

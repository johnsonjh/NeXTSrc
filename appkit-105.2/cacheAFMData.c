/*
	cacheAFMData.c
	Copyright 1990, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	A program to read an afm dir and write out a binary cache.
*/

#import "afmprivate.h"
#import "nextstd.h"
#import <stdlib.h>
#import <sys/dir.h>
#import <objc/hashtable.h>
#import <streams/streams.h>
#import <sys/stat.h>
#import <sys/time.h>

#define DO_TRACE	0
#if DO_TRACE
#define TRACE(msg)		fprintf(stderr, (msg))
#define TRACE1(msg, a1)		fprintf(stderr, (msg), (a1))
#define TRACE2(msg, a1, a2)	fprintf(stderr, (msg), (a1), (a2))
static int traceInt;	/* var used during size tracing */
#else
#define TRACE(msg)
#define TRACE1(msg, a1)
#define TRACE2(msg, a1, a2)
#endif

#include "logErrorInc.c"
int PrintTimes = FALSE;		/* global needed by afmParser.c */

typedef struct {	/* a file containing AFM info */
    AFMInfo *info;		/* data parsed from the file */
    unsigned short size;	/* font size, 0 for non-screen fonts */
    char isOldStyle;		/* is the file in a .font dir or "afm" dir? */
} AFMFile;

typedef struct {	/* a bunch of AFM files for a a typeface */
    union {
	char *s;
	int i;
    } name;			/* must have name first for hashing */
    int numFiles;		/* size of files array */
    AFMFile *files;		/* data parsed from the file */
} AFMFace;

typedef struct {	/* a dir containing fonts */
    const char *name;
    int numFaces;
    AFMFace *faces;		/* sorted array of faces */
    NXHashTable *faceTable;	/* hash table of AFMFiles in this dir */
    NXStrTable strings;	/* string table of AFMFiles in this dir */
    char hasOldFonts;		/* does dir have an old format afm dir? */
    char hasNewFonts;		/* does dir contain new format .font dirs? */
} AFMDir;

    static AFMDir *parseAFMDir(const char *name);
    static int loadAFMFiles(AFMDir *dir, const char *fontDir);
    static AFMDir *allocAFMDir(const char *name);
    static AFMFace *allocAFMFace(const char *name);
    static AFMFile *findFileInFace(AFMFace *face, int size);
    
    static int writeAFMDir(AFMDir *dir);
    static void writeStringsSection(NXStream *st, NXStrTable strings, int *len);
    static void writeDirectorySection(NXStream *st);
    static void writeDirEntry(NXStream *st, int dirSectionOffset, int index, int sectStart, int sectLength);
    static void writeFacesSection(NXStream *st, AFMDir *dir, int *len);
    static void writeFilesSection(NXStream *st, AFMDir *dir, int *len, NXStream **dataSt);
    static void writeAFMData(NXStream *st, AFMInfo *info);
    static void sortFacesAndFiles(AFMDir *dir);
    static void roundStream(NXStream *st);
    static int afmFileCompare(void *data1, void *data2);
    
    static void floatWriteNum(NXStream *st, float f);
    static void ushortWriteNum(NXStream *st, float f);
    static void ucharWriteNum(NXStream *st, float f);
    static void floatWriteInt(NXStream *st, int i);
    static void ushortWriteInt(NXStream *st, int i);
    static void ucharWriteInt(NXStream *st, int i);
    static int lockDir(const char *dirName);
    static void unlockDir(const char *dirName);


static AFMDir *parseAFMDir(const char *name)
{
    AFMDir *dir = NULL;
    DIR *wholeDir;
    struct direct *dirEntry;
    char mightHaveOldFonts = FALSE;

    if (wholeDir = opendir(name)) {
	dir = allocAFMDir(name);
	while (dirEntry = readdir(wholeDir))
	    if (!rstrcmp(dirEntry->d_name, ".font")) {
		if (loadAFMFiles(dir, dirEntry->d_name) > 0)
		    dir->hasNewFonts = TRUE;
	    } else if (!strcmp(dirEntry->d_name, "afm"))
		mightHaveOldFonts = TRUE;
	closedir(wholeDir);
    }
    if (mightHaveOldFonts)
      /* must be done second so symlink emulations of old scheme dont end up in the hash table instead of the real files. */
	dir->hasOldFonts = loadAFMFiles(dir, NULL) != 0;
    return dir;
}


/* NULL font dir means look in the afm dir in the old style */
static int loadAFMFiles(AFMDir *dir, const char *fontDir)
{
    DIR *wholeDir;
    struct direct *dirEntry;
    char path[MAXPATHLEN];
    char *faceName = path;
    int size;
    AFMFace *face;
    AFMFile newFile;
    int filesRead = 0;

    pathStrcat(dir->name, fontDir ? fontDir : "afm", path);
    if (chdir(path) == -1) {
	perror("cacheAFMData");
	return -1;
    }
    if (wholeDir = opendir(".")) {
	while (dirEntry = readdir(wholeDir))
	    if (!rstrcmp(dirEntry->d_name, ".afm")) {
		parseFileName(dirEntry->d_name, faceName, &size);
		if (!*faceName)
		    continue;		/* couldnt grock name */
		face = NXHashGet(dir->faceTable, &faceName);
		if (!findFileInFace(face, size)) {
		    newFile.info = NULL;
		    newFile.size = size;
		    newFile.isOldStyle = (fontDir == NULL);
		    newFile.info = parseAFMFile(dirEntry->d_name, newFile.info, BASIC_INFO | WIDTH_INFO);
		    if (newFile.info && (newFile.info->infoParsed & (BASIC_INFO | WIDTH_INFO)) == (BASIC_INFO | WIDTH_INFO)) {
			if (!face) {
			    face = allocAFMFace(faceName);
			    (void)NXHashInsert(dir->faceTable, face);
			    dir->numFaces++;
			}
			face->files = realloc(face->files, sizeof(AFMFile) * (face->numFiles+1));
			face->files[face->numFiles] = newFile;
			face->numFiles++;
			filesRead++;
			dir->strings = newFile.info->strTable;
		    }
		}
	    }
	closedir(wholeDir);
    } else
	return -1;
    return filesRead;
}


static AFMDir *allocAFMDir(const char *name)
{
    AFMDir *dir;

    dir = calloc(1, sizeof(AFMDir));
    dir->name = name;
    dir->faceTable = NXCreateHashTable(NXStrStructKeyPrototype, 0, NULL);
    return dir;
}


static AFMFace *allocAFMFace(const char *name)
{
    AFMFace *face;

    face = calloc(1, sizeof(AFMFace));
    face->name.s = NXCopyStringBuffer(name);
    return face;
}


static AFMFile *findFileInFace(AFMFace *face, int size)
{
    int i;
    AFMFile *file;

    if (face)
	for (i = face->numFiles, file = face->files; i--; file++)
	    if (file->size == size)
		return file;
    return NULL;
}


static int writeAFMDir(AFMDir *dir)
{
    int dirSectionOffset;
    int sectStart;
    int sectLength;
    NXStream *st;		/* stream of file we write */
    NXStream *dataSt;		/* accessory stream to collect detailed data */
    char *start;
    int len, maxLen;
    char path[MAXPATHLEN];
    int ret = 0;

    st = NXOpenMemory(NULL, 0, NX_WRITEONLY);

    sortFacesAndFiles(dir);	/* must be before writing strings */
    writeStringsSection(st, dir->strings, &sectLength);

    dirSectionOffset = NXTell(st);
    writeDirectorySection(st);
    writeDirEntry(st, dirSectionOffset, STRINGS_SECTION, 0, sectLength);

    sectStart = NXTell(st);
    writeFacesSection(st, dir, &sectLength);
    writeDirEntry(st, dirSectionOffset, FACES_SECTION, sectStart, sectLength);

    sectStart = NXTell(st);
    writeFilesSection(st, dir, &sectLength, &dataSt);
    writeDirEntry(st, dirSectionOffset, FILES_SECTION, sectStart, sectLength);

    sectStart = NXTell(st);
    NXGetMemoryBuffer(dataSt, &start, &len, &maxLen);
    NXWrite(st, start, len);
    NXCloseMemory(dataSt, NX_FREEBUFFER);
    writeDirEntry(st, dirSectionOffset, BASIC_DATA_SECTION, sectStart, len);

    pathStrcat(dir->name, AFMCACHE_NAME, path);
    (void)unlink(path);			/* in case it already exists */
    if (NXSaveToFile(st, path) != 0)
	ret = -1;
    NXCloseMemory(st, NX_FREEBUFFER);
    return ret;
}


static void writeStringsSection(NXStream *st, NXStrTable strings, int *len)
{
    char *start;
    int dataLen;

    NXGetStringTableInfo(strings, &start, &dataLen, NULL, NULL, NULL);
    NXWrite(st, start, dataLen);
    *len = dataLen;
    roundStream(st);
}


static void writeDirectorySection(NXStream *st)
{
    char *start;
    int len, maxLen;
    char kludgeBuffer[8];
    AFMCFileHeader *header;
    int numSections = NUM_SECTIONS;
    AFMCDirEntry dummyEntry;

    NXGetMemoryBuffer(st, &start, &len, &maxLen);
    bcopy(start, kludgeBuffer, 8);
    header = (AFMCFileHeader *)start;
    header->version = 1;
    header->isLittleEndian = LITTLE_ENDIAN_NATIVE;
    header->dirOffset = NXTell(st);
    NXWrite(st, kludgeBuffer, 8);
    NXWrite(st, &numSections, sizeof(int));
    while (numSections--) 
	NXWrite(st, &dummyEntry, sizeof(dummyEntry));
}


static void writeFacesSection(NXStream *st, AFMDir *dir, int *len)
{
    AFMFace *face;
    int i;
    int fileCount = 0;

    for (face = dir->faces, i = dir->numFaces; i--; face++) {
	NXWrite(st, &face->name.i, sizeof(int));
	NXWrite(st, &face->numFiles, sizeof(int));
	NXWrite(st, &fileCount, sizeof(int));
	fileCount += face->numFiles;
    }
    *len = sizeof(AFMCFace) * dir->numFaces;
}

#define FUDGE 0.0001

static void writeFilesSection(NXStream *st, AFMDir *dir, int *len, NXStream **dataSt)
{
    AFMFace *face;
    AFMFile *file;
    int i, j;
    int fileCount = 0;
    int dataOffset = 0;

    *dataSt = NXOpenMemory(NULL, 0, NX_WRITEONLY);
    for (face = dir->faces, i = dir->numFaces; i--; face++) {
	for (file = face->files, j = face->numFiles; j--; file++) {
	    NXWrite(st, &file->size, sizeof(unsigned short));
	    NXWrite(st, &file->isOldStyle, sizeof(char));
	    NXPutc(st, ' ');		/* unused bytes */
	    dataOffset = NXTell(*dataSt);
	    NXWrite(st, &dataOffset, sizeof(int));
	    writeAFMData(*dataSt, file->info);
	}
	fileCount += face->numFiles;
    }
    *len = sizeof(AFMCFile) * fileCount;
}


static void writeAFMData(NXStream *st, AFMInfo *info)
{
    float *widths, *w;
    int iWidth;
    int i;
    float lastWidth = NAN;
    char numType;
    int sameWidthCount = 0;
    static void (*writeNum[3])(NXStream *st, float num) =
		{&floatWriteNum, &ushortWriteNum, &ucharWriteNum};
    static void (*writeInt[3])(NXStream *st, int num) =
		{&floatWriteInt, &ushortWriteInt, &ucharWriteInt};

    NXWrite(st, info->globalInfo, (char *)&info->globalInfo->widths - (char *)info->globalInfo);
    widths = NXStackPtrFromOffset(info->widthsStack, info->globalInfo->widths);

  /* check if chars can be encoded as chars */
    numType = UCHAR_WIDTHS;
    for (i = ENCODING_SIZE, w = widths; i--; w++) {
	iWidth = *w + FUDGE;
	if (ABS(iWidth - *w) > FUDGE || iWidth > 255) {
	    numType = USHORT_WIDTHS;
	    break;
	}
    }

  /* check if chars can be encoded as shorts */
    if (numType == USHORT_WIDTHS)
	for (i = ENCODING_SIZE, w = widths; i--; w++) {
	    iWidth = *w * 1000 + FUDGE;
	    if (ABS(iWidth - *w * 1000) > FUDGE || iWidth > 256 * 256 - 1) {
		numType = FLOAT_WIDTHS;
		break;
	    }
	}

    TRACE1("encoding %d  ", (traceInt = NXTell(st)) ? numType : 666);
    NXPutc(st, numType);
    NXPutc(st, ' ');		/* unused byte */
    for (i = ENCODING_SIZE + 1, w = widths; i--; w++) {
	if (i && *w == lastWidth)
	    sameWidthCount++;
	else {
	    if (sameWidthCount > 2) {
		(*writeNum[numType])(st, 0.0);
		(*writeInt[numType])(st, sameWidthCount);
		(*writeNum[numType])(st, lastWidth);
	    } else if (sameWidthCount > 0)
		for ( ; sameWidthCount; sameWidthCount--)
		    (*writeNum[numType])(st, lastWidth);
	    if (i) {			/* dont do this the last time */
		if (*w != 0.0) {
		    lastWidth = *w;
		    sameWidthCount = 1;
		} else {
		    (*writeNum[numType])(st, 0.0);
		    (*writeInt[numType])(st, 1);
		    lastWidth = NAN;	/* dont encode 0 strings */
		    sameWidthCount = 0;
		}
	    }
	}
    }
    roundStream(st);
    TRACE1("widths size =%4d ", NXTell(st) - traceInt);
    TRACE1(" %s\n", NXStringTablePtrFromOffset(info->strTable, info->globalInfo->fullName));
}


static void writeDirEntry(NXStream *st, int dirSectionOffset, int index, int sectStart, int sectLength)
{
    char *start;
    int len, maxLen;
    AFMCDirEntry *entries;

   NXGetMemoryBuffer(st, &start, &len, &maxLen);
   entries = (AFMCDirEntry *)(start + dirSectionOffset + 8 + sizeof(int));
   entries[index].offset = sectStart;
   entries[index].length = sectLength;
}


static void sortFacesAndFiles(AFMDir *dir)
{
    NXHashState	state = NXInitHashState(dir->faceTable);
    AFMFace *face, *f;
    int i;
    char *oldStr;

    NX_ASSERT(dir->numFaces, "No fonts to sort in sortFacesAndFiles");
    dir->faces = malloc(dir->numFaces * sizeof(AFMFace));
    f = dir->faces;
    while (NXNextHashState(dir->faceTable, &state, &(void *)face))
	*f++ = *face;
    qsort(dir->faces, dir->numFaces, sizeof(AFMFace), &strStructCompare);
    NXFreeHashTable(dir->faceTable);
    for (f = dir->faces, i = dir->numFaces; i--; f++) {
	oldStr = f->name.s;
	(void)NXStringTableInsert(dir->strings, oldStr, &f->name.i);
	free(oldStr);
	qsort(f->files, f->numFiles, sizeof(AFMFile), &afmFileCompare);
    }
}


static void roundStream(NXStream *st)
{
    int len = NXTell(st);

    while (len % 4) {		/* round out data to a multiple of 4 */
	NXPutc(st, ' ');
	len++;
    }
}


static void floatWriteNum(NXStream *st, float f)
{
    NXWrite(st, &f, sizeof(float));
}

static void ushortWriteNum(NXStream *st, float f)
{
    unsigned short val = f * 1000 + FUDGE;
    
    NXPutc(st, ((char *)&val)[0]);
    NXPutc(st, ((char *)&val)[1]);
}

static void ucharWriteNum(NXStream *st, float f)
{
    unsigned char val = f + FUDGE;
    
    NXPutc(st, val);
}

static void floatWriteInt(NXStream *st, int i)
{
    NXWrite(st, &i, sizeof(int));
}

static void ushortWriteInt(NXStream *st, int i)
{
#if BIG_ENDIAN_NATIVE
    NXPutc(st, ((char *)&i)[2]);
    NXPutc(st, ((char *)&i)[3]);
#else BIG_ENDIAN_NATIVE
    NXPutc(st, ((char *)&i)[0]);
    NXPutc(st, ((char *)&i)[1]);
#endif
}

static void ucharWriteInt(NXStream *st, int i)
{
#if BIG_ENDIAN_NATIVE
    NXPutc(st, ((char *)&i)[3]);
#else BIG_ENDIAN_NATIVE
    NXPutc(st, ((char *)&i)[0]);
#endif
}


static int afmFileCompare(void *data1, void *data2)
{
    AFMFile *f1 = (AFMFile *)data1;
    AFMFile *f2 = (AFMFile *)data2;

    return f1->size - f2->size;
}

#define LOCK_TIMEOUT	(3*60)		/* max age of valid locks (secs) */
#define LOCK_NAME	".cacheAFMDataLock"

/* grabs lock that we hold while compiling this dir.  If we find the dir
   already locked, we check the time on the lock.  If its three minutes old,
   we assume that the lock is stale and remove it.
 */
static int lockDir(const char *dirName)
{
    char path[MAXPATHLEN];
    int fd;
    struct stat cacheInfo;
    struct timeval now;

    pathStrcat(dirName, LOCK_NAME, path);
    if (stat(path, &cacheInfo) == 0) {
	gettimeofday(&now, NULL);
	if (now.tv_sec > cacheInfo.st_mtime + LOCK_TIMEOUT)
	    if (unlink(path) != 0)
		NXLogError("Couldn't remove lock file: %s (error %d)", path, errno);
	else
	    return FALSE;
    }
    fd = open(path, O_RDONLY | O_CREAT | O_EXCL, 0444);
    if (fd >= 0) {
	close(fd);
	return TRUE;
    } else
	return FALSE;
}


/* removes the lock that we held while compiling this dir */
static void unlockDir(const char *dirName)
{
    char path[MAXPATHLEN];

    pathStrcat(dirName, LOCK_NAME, path);
    if (unlink(path) != 0)
	NXLogError("Couldn't remove lock file: %s (error %d)", path, errno);
}


void main(int argc, char *argv[])
{
    AFMDir *dir;
    char *dirName;
    char path[MAXPATHLEN];

    if (argc != 2) {
	fprintf(stderr, "Usage: %s <afm dir>\n", argv[0]);
	exit(-1);
    }

  /* make sure path to dir is absolute so we dont lose the lock file after chdir'ing around to read the afm files. */
    if (argv[1][0] == '/')
	dirName = argv[1];
    else {
	dirName = path;
	getwd(path);
	pathStrcat(path, argv[1], path);
    }

    if (DO_TRACE || lockDir(dirName)) {
	initAFMParser();
	dir = parseAFMDir(dirName);
	if (dir) {
	    if (!dir->numFaces || !writeAFMDir(dir)) {
		unlockDir(dirName);
		exit(0);
	    } else
		fprintf(stderr, "%s: Could not write %s file in directory %s\n", argv[0], AFMCACHE_NAME, dirName);
	} else
	    fprintf(stderr, "%s: Could not parse files in directory %s\n", argv[0], dirName);
	unlockDir(dirName);
	exit(-1);
    } else
	exit(0);
}

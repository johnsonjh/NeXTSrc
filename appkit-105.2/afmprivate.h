/*
	afmprivate.h
	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  	
	kit private info about afms.
*/

#import "afm.h"
#import "pbtypes.h"
#import <objc/hashtable.h>

/* # chars in an encoding vector */
#define ENCODING_SIZE		256

/* Configure these for your architecture.  One should be true, one false */
#define BIG_ENDIAN_NATIVE	1
#define LITTLE_ENDIAN_NATIVE	0

/* Configure these for your architecture.  One should be true, one false */
#define AFMCACHE_NAME		".afmcache"

/* masks for pieces of the AFM info */
#define	BASIC_INFO		1	/* top level info */
#define WIDTH_INFO		2	/* char x widths */
#define FULL_INFO		4	/* all but comp char info */
#define CC_INFO			8	/* comp char info */

typedef struct _NXStack *NXStack;

typedef struct _NXStringTable *NXStrTable;


typedef int NXStackOffset;
    /* an offset into the string table for a font.  All strings in the font info are returned from the appkit server in this way.  The offsets can be resolved to real strings with the NX_AFMSTRING macro. */

#define NX_AFMSTRING(metric, offset)  ((metric)->strings + (offset))

/* global font info for the font as a whole */
/* NOTE: this MUST match the first part of the NXFontMetrics structure */
typedef struct _NXGlobalFontInfo {
    NXStackOffset formatVersion;	/* version of afm file format */
    NXStackOffset name;			/* name of font for findfont */
    NXStackOffset fullName;		/* full name of font */
    NXStackOffset familyName;		/* "font family" name */
    NXStackOffset weight;		/* weight of font */
    float italicAngle;			/* degrees ccw from vertical */
    char isFixedPitch;			/* is the font mono-spaced? */
    char isScreenFont;			/* is the font a screen font? */
    short screenFontSize;		/* If it is, how big is it? */
    float fontBBox[4];			/* bounding box (llx lly urx ury) */
    float underlinePosition;		/* dist from basline for underlines */
    float underlineThickness;		/* thickness of underline stroke */
    NXStackOffset version;		/* version identifier */
    NXStackOffset notice;		/* trademark or copyright */
    NXStackOffset encodingScheme;	/* default encoding vector */
    float capHeight;			/* top of 'H' */
    float xHeight;			/* top of 'x' */
    float ascender;			/* top of 'd' */
    float descender;			/* bottom of 'p' */
    short hasYWidths;			/* do any chars have non-0 y width? */
    NXStackOffset widths;		/* character widths in x */
    unsigned int appOnly1;		/* "widthsLength" in app */
    unsigned int appOnly2;		/* "strings" in app */
    unsigned int appOnly3;		/* "stringsLength" in app */
    char hasXYKerns;		/* Do any of the kern pairs have nonzero dy? */
    char reserved;
    NXStackOffset encoding;		/* 256 offsets into charMetrics */
    NXStackOffset yWidths;		/* character widths in y.  NOT in */
	/* encoding order, but a parallel array to the charMetrics array */
    NXStackOffset charMetrics;		/* array of NXCharMetrics */
    int numCharMetrics;			/* num elements */
    NXStackOffset ligatures;		/* array of NXLigatures */
    int numLigatures;			/* num elements */
    NXStackOffset encLigatures;		/* array of NXEncodedLigatures */
    int numEncLigatures;		/* num elements */
    union {
	NXStackOffset kernPairs;	/* array of NXKernPairs */
	NXStackOffset kernXPairs;	/* array of NXKernXPairs */
    } kerns;
    int numKernPairs;			/* num elements */
    NXStackOffset trackKerns;		/* array of NXTrackKerns */
    int numTrackKerns;			/* num elements */
    NXStackOffset compositeChars;	/* array of NXCompositeChar */
    int numCompositeChars;		/* num elements */
    NXStackOffset compositeCharParts;	/* array of NXCompositeCharPart */
    int numCompositeCharParts;		/* num elements */
} NXGlobalFontInfo;


typedef struct {		/* AFM info from a file */
    NXGlobalFontInfo *globalInfo;	/* global font info */
    NXStack widthsStack;		/* stacks holding the data for font */
    NXStack dataStack;
    NXStack ccStack;			/* composite char data */
    NXStrTable strTable;
    char sharedStrTable;
    unsigned short infoParsed;		/* mask of info parsed for this font */
} AFMInfo;
    /* All the info about this font face. */


/* FILE FORMAT FOR .afmcache files */

/* The file is a binary file with many sections.  The first 8 bytes are a AFMCFileHeader.  The first byte in the file is guaranteed to be the format version, starting with 1.  The next byte will be one or zero if the file's data is little endian or
 big endian.  The next two bytes are unused.  The next four bytes will comprise a long int that give the offset to the Directory section of the file.

Directory - This section begins with the 8 bytes that belong at the start of this file instead of the header that is found there.  These bytes should be copied to the start of the file's memory image.  Following those 8 bytes is a long int telling 
how many entries are in this directory section.  After this int is an array of AFMCDirEntry's, giving the offsets to the other sections.  The indices in the array are #defined below.

Faces - This section holds an array of AFMCFace's, one for every typeface in the directory.  These are sorted alphabetically by name.

Files - This section holds an array of AFMCFile's.  They are ordered by font size, with the non-screen font at the first position.

Basic data - This section holds all the parsed metrics and widths data.  For each file there will be the first part of a NXGlobalFontInfo, and then some encoded widths.

*/

typedef struct {
    unsigned char version;
    char isLittleEndian;
    short unused;
    int dirOffset;
} AFMCFileHeader;

/* indices into Directory section's array */
#define	STRINGS_SECTION		0
#define	FACES_SECTION		1
#define	FILES_SECTION		2
#define	BASIC_DATA_SECTION	3
#define NUM_SECTIONS		4

typedef struct {
    int offset;		/* offset to start of section */
    int length;		/* length of section */
} AFMCDirEntry;

typedef struct {
    int name;		/* offset into string section of face name */
    int numFiles;	/* number of files within this face */
    int files;		/* start of array of files in Files section */
} AFMCFace;

typedef struct {
    unsigned short size;	/* font size, 0 for non-screen fonts */
    char isOldStyle;	/* is the file in a .font dir or "afm" dir? */
    char unused;
    int data;		/* offset into basic data section of file data */
} AFMCFile;

/* codes for width compression */
#define	FLOAT_WIDTHS		0
#define	USHORT_WIDTHS		1
#define	UCHAR_WIDTHS		2


typedef enum {
    NX_stackGrowContig,
    NX_stackGrowDiscontig,
    NX_stackFixedContig
} NXStackType;
    /* Mode that describes what happens when the memory for a NXStack fills up.  NX_stackGrowContig means that the stack will grow and its memory will remain contiguous.  This implies that its memory may move when it grows.  NX_stackGrowDiscontig m
eans that the stack will grow, but the additional memory may not be adjoining existing stack memory.  Allocated objects will not straddle such memory areas.  NX_stackFixedContig means the stack will not grow beyond the initially specified size.  In
 this case, an allocation that exceeds available space will fail.  In all modes, an exception is raised when allocations fail. */

extern NXStack NXCreateStack(NXStackType type, int sizeHint, void *userData);
    /* creates a new stack */

extern NXStack NXCreateStackFromMemory(void *start, int used, int maxSize, void *userData, NXStackType type, int freeBuffer);
    /* creates a new stack pointing to malloced memory */

extern void *NXStackAllocPtr(NXStack stack, int size, int alignment);
    /* allocates new space at the end of the stack.  Alignment is the byte allignment of the allocated data.  A pointer to the new data is returned.  Ths is not valid for NX_stackGrowContig stacks. */

extern int NXStackAllocOffset(NXStack stack, int size, int alignment);
    /* allocates new space at the end of the stack.  Alignment is the byte allignment of the allocated data.  The offset from the start of the stack of the new data is returned.  This is not valid for NX_stackGrowDiscontig stacks. */

extern void NXResetStack(NXStack stack, const void *oldData, int oldOffset);
    /* resets the stack to the state right before an object was allocated.  Objects allocated since that object are also reclaimed.  Either a previously returned pointer or offset is passed in. */

extern void NXGetStackInfo(NXStack stack, const void **start, int *used, int *maxSize, void **userData, NXStackType *type);
    /* returns various pieces of info about the stack by value.  If any of the given data pointers is NULL, then that info is not returned. */

extern int NXStackOffsetFromPtr(NXStack stack, const void *ptr);
extern void *NXStackPtrFromOffset(NXStack stack, int offset);
    /* converts from pointers to offset and vice-versa. */

extern void NXDestroyStack(NXStack stack);
    /* deallocates the stack and its contents */


extern NXStrTable NXCreateStringTable(NXStackType type, int sizeHint, void *userData);

extern NXStrTable NXCreateStringTableFromMemory(char *start, int used, int maxSize, void *userData, NXStackType type, int freeBuffer);

extern const char *NXStringTableInsert(NXStrTable sTable, const char *str, int *offset);

extern const char *NXStringTableInsertWithLength(NXStrTable sTable, const char *str, int length, int *offset);

extern void NXResetStringTable(NXStrTable sTable, const char *oldData, int oldOffset);

extern void NXGetStringTableInfo(NXStrTable sTable, const char **start, int *used, int *maxSize, void **userData, NXStackType *type);

extern int NXStringTableOffsetFromPtr(NXStrTable sTable, const void *ptr);
extern void *NXStringTablePtrFromOffset(NXStrTable sTable, int offset);
    /* converts from pointers to offset and vice-versa. */

extern void NXDestroyStringTable(NXStrTable sTable);


extern AFMInfo *parseAFMFile(const char *file, AFMInfo *info, int flags);
extern void fixNFSData(const char *path, char **start, int *len, int *maxLen);
extern void initAFMParser(void);
extern void _NXInitAFMModule(void);
extern void _NXGetSwappedAFMInfo(AFMInfo *info,
		data_t *globalInfo, unsigned int *globalInfoCnt,
		data_t *widths, unsigned int *widthsCnt,
		data_t *fullData, unsigned int *fullDataCnt,
		data_t *ccData, unsigned int *ccDataCnt);
extern void swapLongs(void *data, int num);
extern void swapShorts(void *data, int num);
extern int strStructCompare(void *data1, void *data2);
extern void parseFileName(const char *name, char *faceName, int *size);
extern int rstrcmp(const char *s1, const char *s2);
extern void pathStrcat(const char *head, const char *tail, char *result);

extern int PrintTimes;
extern int DoMarks;

extern int WidthsMaxSize;
extern int FullDataMaxSize;
extern int CCDataMaxSize;
extern int StringsMaxSize;
extern int NextStackTag;


extern NXStack CurrWidthsStack;
extern NXStack CurrDataStack;
extern NXStack CurrCCStack;
extern NXStrTable CurrStringTable;

/* internal error types and data passed with error */
typedef enum {
    err_cantOpen = 10000000,		/* OS error code, 0 */
    err_cantClose,			/* OS error code, 0 */
    err_invalidToken,			/* start of item, 0 */
    err_invalidNumber,			/* 0, 0 */
    err_invalidInteger,			/* 0, 0 */
    err_invalidBoolean,			/* 0, 0 */
    err_invalidName,			/* 0, 0 */
    err_wrongToken,			/* badToken, 0 */
    err_EOF,				/* 0, 0 */
    err_badEOL,				/* 0, 0 */
    err_badSep,				/* 0, 0 */
    err_metricsCount,			/* numExpected, numFound */
    err_tracksCount,			/* numExpected, numFound */
    err_kernPairsCount,			/* numExpected, numFound */
    err_compCharsCount,			/* numExpected, numFound */
    err_compPartsCount,			/* numExpected, numFound */
    err_nameReused,			/* offset of name, 0 */
    err_stackAllocFailed,		/* stack, amount */	
    err_strAllocFailed			/* string table, amount */	
} AFMErrors;


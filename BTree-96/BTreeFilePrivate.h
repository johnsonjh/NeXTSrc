
#ifndef	_BTREEFILEPRIVATE_H_
#define	_BTREEFILEPRIVATE_H_

#import	"BTreeCursor.h"
#import	"BTreeStoreFile.h"

#import	<libc.h>
#import	<mach.h>
#import	<mach_error.h>
#import	<sys/time.h>

#import	<sys/mman.h>
#import	<sys/types.h>
#import	<sys/file.h>

extern char *NXBTreeFileTemporary;
extern char *NXBTreeFileExtension;
extern char *NXBTreeFileDirectory;

#define	BTreeFilePagesPerPageTable	128
#define	BTreeFilePageTablesPerPageBlock	128
#define	BTreeFilePagesPerPageBlock	\
	(BTreeFilePagesPerPageTable * BTreeFilePageTablesPerPageBlock)
#define	BTreeFilePageBlocksPerBTreeFile	32

#define	strsave(string)	strcpy(malloc(1 + strlen(string)), (string))

/*
disk resident structures - file format dependency
*/

typedef struct BTreeFileBTreeRecord	{
	unsigned long		RESERVED[2];
	unsigned long		tableIndex;
	vm_offset_t		rootOffset;
} BTreeFileBTreeRecord;

typedef struct BTreeFilePageHeader	{
	unsigned long		RESERVED[2];
	vm_offset_t		verification;
	vm_offset_t		backupOffset;
	struct timeval		timeStamp;
	unsigned long		pageLevel;
	vm_offset_t		rootOffset;
	char			contents[0];
} BTreeFilePageHeader;

typedef struct BTreeFileCountEntry	{
	unsigned long		recordCount;
} BTreeFileCountEntry;

typedef struct BTreeFileCountTable	{
	unsigned long		RESERVED[2];
	unsigned short		tableCount;
	unsigned short		tableLimit;
	unsigned short		startEntry;
	BTreeFileCountEntry	countTable[0];
} BTreeFileCountTable;

typedef struct BTreeFileOffsetEntry	{
	vm_offset_t		entryOffset;
	unsigned long		entryLength;
} BTreeFileOffsetEntry;

typedef struct BTreeFileOffsetTable	{
	unsigned long		RESERVED[2];
	unsigned long		offsetCount;
	vm_offset_t		offsetQueue;
	BTreeFileOffsetEntry	entryArray[0];
} BTreeFileOffsetTable;

typedef struct BTreeFileOffsetCache	{
	unsigned long		RESERVED[2];
	unsigned long		offsetTotal;
	unsigned long		offsetLimit;
	vm_offset_t		offsetQueue;
	vm_offset_t		offsetTable;
} BTreeFileOffsetCache;

typedef struct BTreeFileStatus	{
	unsigned		creating:1;
	unsigned		recoverable:1;
	unsigned		instantly:1;
	unsigned		transactions:1;
	unsigned		writeable:1;
	unsigned		modified:1;
	unsigned		fulcrumDirty:1;
	unsigned		headerDirty:1;
	unsigned		RESERVED:8;
} BTreeFileStatus;

typedef struct BTreeFileHeader	{
	char			magicString[6];
	unsigned char		RESERVED[122];
	BTreeFileStatus		fileStatus;
	vm_offset_t		headerBackup;
	vm_offset_t		currentFulcrum;
	struct timeval		timeStamp;
	struct timeval		versionStarted;
	unsigned long		codeVersion[4];
	unsigned long		pageSize;
	unsigned long		fileSize;
	BTreeFileOffsetCache	freeOffsetCache;
	BTreeFileOffsetCache	tempOffsetCache;
	BTreeFileBTreeRecord	defaultRecord;
	BTreeFileBTreeRecord	directoryRecord;
	BTreeFileCountTable	countTable;
} BTreeFileHeader;

/*
memory only structures - no file format dependency
*/

typedef struct BTreeFileTemplate	{
	@defs(BTreeFile)
} BTreeFileTemplate;

typedef struct BTreeFileBTreeHandle	{
	char			*btreeName;
	BTreeStoreFile		*btreeStore;
	BTree			*btree;
} BTreeFileBTreeHandle;

typedef struct BTreePageEntry		{
	struct BTreePageEntry		*nextEntry;
	struct BTreePageEntry		*prevEntry;
	struct BTreePageEntryChain	*entryChain;
	BTreePage			*btreePage;
	unsigned long			pageCount;
	vm_address_t			copyBuffer;
} BTreePageEntry;

typedef struct BTreePageEntryChain	{
	unsigned long		entryCount;
	unsigned long		entryLimit;
	BTreePageEntry		*nextEntry;
	BTreePageEntry		*prevEntry;
} BTreePageEntryChain;

typedef struct BTreeFilePageTable	{
	BTreePage		*btreePage;
	unsigned short		minimumPage;
	unsigned short		maximumPage;
} BTreeFilePageTable;

typedef struct BTreeFileCache	{
	BTreeFileHeader		*fileHeader;
	BTreeFilePageTable	*pageArray[BTreeFilePageBlocksPerBTreeFile];
	vm_size_t		fileSize;
	BTreeFileStatus		fileStatus;
	int			fileDescriptor;
	BTreePageEntryChain	spareChain;
	BTreeFileHeader		*copyHeader;
} BTreeFileCache;

extern unsigned long 
_btreeFileMergeOffsetCache(BTreeFileCache *fileCache);

extern void
_btreeFileFreeOffsetEntry(BTreeFileCache *fileCache, 
	BTreeFileOffsetEntry offsetEntry);

extern BTreeFileOffsetEntry 
_btreeFileNewOffsetEntry(BTreeFileCache *fileCache, unsigned long entryLength);

extern BTreePage 
*_btreeFilePageGroupFromOffset(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, unsigned long *pageCount, boolean_t modifying);

#if	defined(NOTDEF)

inline static void 
_btreeProtectBuffer(vm_address_t pageBuffer, 
	vm_size_t pageSize, boolean_t writeable)
{
	kern_return_t		error;

	error = vm_protect(task_self(), pageBuffer, pageSize, NO, 
		(writeable) ? VM_PROT_READ | VM_PROT_WRITE : VM_PROT_READ);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeProtectBuffer", mach_error_string(error));
}

#else

#define	_btreeProtectBuffer(pageBuffer, pageSize, writeable)	

#endif

/*
routines for managing the page array
*/

inline static BTreePage 
*_btreeFilePageFromOffset(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, boolean_t modifying)
{
	unsigned long	pageCount = 1;

	return _btreeFilePageGroupFromOffset(fileCache, 
		pageOffset, &pageCount, modifying);
}

inline static vm_address_t 
_btreeFilePageBufferFromOffset(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, boolean_t modifying)
{
	BTreePage	*btreePage;

	btreePage = _btreeFilePageFromOffset(fileCache, pageOffset, modifying);
	return btreePage->pageBuffer;
}

#define	_btreeFileReadFromOffset(fileCache, pageOffset) \
	_btreeFilePageFromOffset(fileCache, pageOffset, NO)

#define	_btreeFileReadBufferFromOffset(fileCache, pageOffset) \
	_btreeFilePageBufferFromOffset(fileCache, pageOffset, NO)

/*
routines for allocating and freeing page offsets
*/

inline static vm_offset_t 
_btreeFileNewOffsetGroup(BTreeFileCache *fileCache, unsigned long *pageCount)
{
	vm_size_t		pageSize;
	BTreeFileOffsetEntry	offsetEntry;

	pageSize = fileCache->fileHeader->pageSize;
	offsetEntry = _btreeFileNewOffsetEntry(fileCache, 
		*pageCount * pageSize);
	*pageCount = offsetEntry.entryLength / pageSize;
	return offsetEntry.entryOffset;
}

inline static vm_offset_t 
_btreeFileNewOffset(BTreeFileCache *fileCache)
{
	vm_size_t		pageSize;
	BTreeFileOffsetEntry	offsetEntry;

	pageSize = fileCache->fileHeader->pageSize;
	offsetEntry = _btreeFileNewOffsetEntry(fileCache, pageSize);
	return offsetEntry.entryOffset;
}

inline static void 
_btreeFileFreeOffsetGroup(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, unsigned long pageCount)
{
	BTreeFileOffsetEntry	offsetEntry;

	offsetEntry.entryOffset = pageOffset;
	offsetEntry.entryLength = pageCount * fileCache->fileHeader->pageSize;
	_btreeFileFreeOffsetEntry(fileCache, offsetEntry);
}

#define	_btreeFileFreeOffset(fileCache, pageOffset) \
	_btreeFileFreeOffsetGroup(fileCache, pageOffset, 1)

/*
routines for reading and writing the file
*/

inline static void
_btreeFileRead(int fileDescriptor, vm_offset_t readOffset, 
	vm_address_t *readAddress, vm_size_t readLength, boolean_t writeable)
{
	kern_return_t		error;

	if (! *readAddress)
	{
		error = vm_allocate(task_self(), readAddress, readLength, YES);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"_btreeFileRead", mach_error_string(error));
	}

	if (mmap(*readAddress, readLength, 
		(writeable) ? PROT_READ | PROT_WRITE : PROT_READ, 
			MAP_SHARED, fileDescriptor, readOffset) < 0)
		_NXRaiseError(NX_BTreeUnixError + errno, 
			"_btreeFileRead", strerror(errno));
}

inline static void 
_btreeFileWrite(vm_address_t writeAddress, vm_size_t writeLength)
{
	kern_return_t	error;

	error = vm_synchronize(task_self(), writeAddress, writeLength);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeFileWrite", mach_error_string(error));
}

inline static void 
_btreeFileAdjustFileSize(BTreeFileCache *fileCache)
{
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	if (fileCache->fileSize != fileHeader->fileSize)
	{
		fileCache->fileSize = fileHeader->fileSize;
		if (ftruncate(fileCache->fileDescriptor, fileCache->fileSize))
			_NXRaiseError(NX_BTreeUnixError + errno, 
				"_btreeFileAdjustFileSize", strerror(errno));
	}
}

inline static void 
_btreeFileWriteHeader(BTreeFileCache *fileCache)
{
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	fileHeader->fileStatus = fileCache->fileStatus;
	fileHeader->fileStatus.writeable = NO;
	fileHeader->fileStatus.creating = NO;
	gettimeofday(&fileHeader->timeStamp, 0);
	_btreeFileWrite((vm_address_t) fileHeader, 
		(vm_size_t) fileHeader->pageSize);
}

#endif _BTREEFILEPRIVATE_H_


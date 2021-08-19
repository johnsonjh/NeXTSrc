
#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreeFilePrivate.h"

/*
per instance run time structures
*/

typedef struct BTreeFileEntryLevel	{
	BTreePageEntryChain	dirtyChain;
} BTreeFileEntryLevel;

typedef struct BTreeFileBindRecord	{
	BTreePageEntryChain	dirtyChain;
	BTreePageEntryChain	allocChain;
	BTreePageEntryChain	trashChain;
} BTreeFileBindRecord;

typedef struct BTreeStoreCache	{
	BTreeFileCache		*fileCache;
	BTreeFileBindRecord	bindRecord;
	unsigned long		levelCount;
	unsigned long		levelLimit;
	BTreeFileEntryLevel	*levelArray;
	BTreePageEntryChain	allocChain;
	BTreePageEntryChain	trashChain;
	BTreeFile		*btreeFile;
	BTreeFileCountEntry	*countEntry;
	unsigned char		storeValid;
	unsigned char		saveEnabled;
} BTreeStoreCache;

/*
routines for managing buffers
*/

inline static void 
_btreeFileFreeBuffer(vm_address_t pageBuffer, vm_size_t freeLength)
{
	kern_return_t		error;

	error = vm_deallocate(task_self(), pageBuffer, freeLength);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeFileFreeBuffer", mach_error_string(error));
}

inline static void 
_btreeFileCopyBuffer(vm_address_t fromAddress, 
	vm_size_t copyLength, vm_address_t toAddress)
{
	kern_return_t		error;

	error = vm_copy(task_self(), fromAddress, copyLength, toAddress);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeFileCopyBuffer", mach_error_string(error));
}

inline static void 
_btreeFileMakeCopyBuffer(BTreeFileHeader *fileHeader, 
	BTreePageEntry *pageEntry)
{
	kern_return_t		error;

	error = vm_allocate(task_self(), &pageEntry->copyBuffer, 
		pageEntry->pageCount * fileHeader->pageSize, YES);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeFileMakeCopyBuffer", mach_error_string(error));
}

inline static void 
_btreeFileFreeCopyBuffer(BTreeFileHeader *fileHeader, 
	BTreePageEntry *pageEntry)
{
	if (pageEntry->copyBuffer)
	{
		_btreeFileFreeBuffer(pageEntry->copyBuffer, 
			pageEntry->pageCount * fileHeader->pageSize);
		pageEntry->copyBuffer = (vm_address_t) 0;
	}
}

/*
routines for managing page entry chains
*/

inline static BTreePageEntry 
*_btreeFileInsertPageEntry(BTreePageEntryChain *entryChain, 
	BTreePageEntry *pageEntry)
{
#if	defined(DEBUG)
	BTreePage	*btreePage;

	if (pageEntry->entryChain)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileInsertPageEntry", 0);

	btreePage = pageEntry->btreePage;
	if (btreePage && (BTreePageEntry *) btreePage->RESERVED != pageEntry)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileInsertPageEntry", 0);
#endif
	++entryChain->entryCount;
	pageEntry->entryChain = entryChain;
	pageEntry->nextEntry = entryChain->nextEntry;
	if (pageEntry->nextEntry)
		pageEntry->nextEntry->prevEntry = pageEntry;
	else
		entryChain->prevEntry = pageEntry;

	entryChain->nextEntry = pageEntry;
	pageEntry->prevEntry = (BTreePageEntry *) 0;
	return pageEntry;
}

inline static BTreePageEntry 
*_btreeFileAppendPageEntry(BTreePageEntryChain *entryChain, 
	BTreePageEntry *pageEntry)
{
#if	defined(DEBUG)
	BTreePage	*btreePage;

	if (pageEntry->entryChain)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileAppendPageEntry", 0);

	btreePage = pageEntry->btreePage;
	if (btreePage && (BTreePageEntry *) btreePage->RESERVED != pageEntry)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileAppendPageEntry", 0);
#endif
	++entryChain->entryCount;
	pageEntry->entryChain = entryChain;
	pageEntry->prevEntry = entryChain->prevEntry;
	if (pageEntry->prevEntry)
		pageEntry->prevEntry->nextEntry = pageEntry;
	else
		entryChain->nextEntry = pageEntry;

	entryChain->prevEntry = pageEntry;
	pageEntry->nextEntry = (BTreePageEntry *) 0;
	return pageEntry;
}

inline static void 
_btreeFileAppendPageEntryChain(BTreePageEntryChain *sourceChain, 
	BTreePageEntryChain *targetChain)
{
	BTreePageEntry		*pageEntry;

	if (sourceChain->entryCount < 1)
		return;

	pageEntry = sourceChain->nextEntry;
	for (; pageEntry; pageEntry = pageEntry->nextEntry)
		pageEntry->entryChain = targetChain;

	targetChain->entryCount += sourceChain->entryCount;
	sourceChain->entryCount = 0;
	pageEntry = targetChain->prevEntry;
	if (pageEntry)
	{
		sourceChain->nextEntry->prevEntry = pageEntry;
		pageEntry->nextEntry = sourceChain->nextEntry;
	} else
		targetChain->nextEntry = sourceChain->nextEntry;

	targetChain->prevEntry = sourceChain->prevEntry;
	sourceChain->nextEntry = (BTreePageEntry *) 0;
	sourceChain->prevEntry = (BTreePageEntry *) 0;
}

inline static BTreePageEntryChain 
*_btreeFileRemovePageEntry(BTreePageEntry *pageEntry)
{
	BTreePageEntryChain	*entryChain;

#if	defined(DEBUG)
	BTreePage	*btreePage;

	btreePage = pageEntry->btreePage;
	if (btreePage && (BTreePageEntry *) btreePage->RESERVED != pageEntry)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileRemovePageEntry", 0);
#endif
	entryChain = pageEntry->entryChain;
	if (entryChain)
	{
		--entryChain->entryCount;
		pageEntry->entryChain = (BTreePageEntryChain *) 0;
		if (pageEntry->prevEntry)
			pageEntry->prevEntry->nextEntry = pageEntry->nextEntry;
		else
			entryChain->nextEntry = pageEntry->nextEntry;
	
		if (pageEntry->nextEntry)
			pageEntry->nextEntry->prevEntry = pageEntry->prevEntry;
		else
			entryChain->prevEntry = pageEntry->prevEntry;
	}

	return entryChain;
}

inline static BTreePageEntry 
*_btreeFileMakePageEntry(BTreeFileCache *fileCache, 
	BTreePage *btreePage, unsigned long pageCount)
{
	BTreePageEntry		*pageEntry;
	BTreePageEntryChain	*spareChain;

#if	defined(DEBUG)
	if (btreePage->RESERVED)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileMakePageEntry", 0);
#endif
	spareChain = &fileCache->spareChain;
	if (pageEntry = spareChain->nextEntry)
		_btreeFileRemovePageEntry(pageEntry);
	else
	{
		pageEntry = (void *) malloc(sizeof(BTreePageEntry));
		if (! pageEntry)
			_NXRaiseError(NX_BTreeNoMemory, 
				"_btreeFileMakePageEntry", 0);
	}

	btreePage->RESERVED = pageEntry;
	bzero((char *) pageEntry, sizeof(BTreePageEntry));
	pageEntry->btreePage = btreePage;
	pageEntry->pageCount = pageCount;
	return pageEntry;
}

inline static void 
_btreeFileFreePageEntry(BTreeFileCache *fileCache, BTreePage *btreePage)
{
	BTreePageEntry		*pageEntry;

	pageEntry = btreePage->RESERVED;
	if (pageEntry)
	{
#if	defined(DEBUG)
		if (pageEntry->btreePage != btreePage)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeFileFreePageEntry", 0);
#endif
		pageEntry->btreePage = (BTreePage *) 0;
		if (pageEntry->entryChain)
			_btreeFileRemovePageEntry(pageEntry);

		_btreeFileFreeCopyBuffer(fileCache->fileHeader, pageEntry);
		btreePage->RESERVED = (void *) 0;
		_btreeFileInsertPageEntry(&fileCache->spareChain, pageEntry);
	}
}

inline static BTreePageEntry 
*_btreeFileLoadPageEntry(BTreeFileCache *fileCache, 
	BTreePage *btreePage, unsigned long pageCount)
{
	BTreePageEntry		*pageEntry;
	BTreeFileHeader		*fileHeader;

	pageEntry = (BTreePageEntry *) btreePage->RESERVED;
	if (! pageEntry)
		pageEntry = _btreeFileMakePageEntry(fileCache, 
			btreePage, pageCount);

	fileHeader = fileCache->fileHeader;
	_btreeFileMakeCopyBuffer(fileHeader, pageEntry);
	_btreeFileCopyBuffer(btreePage->pageBuffer, 
		pageCount * fileHeader->pageSize, pageEntry->copyBuffer);
	return pageEntry;
}

#define	growthSize (sizeof(BTreeStoreCache) + sizeof(BTreeStoreFile))

inline static void 
_btreeFileBuildLevelArray(BTreeStoreCache *storeCache, 
	unsigned short levelCount)
{
	unsigned long		formerSize;
	unsigned long		actualSize;
	BTreeFileEntryLevel	*tempArray;

	if (++levelCount > storeCache->levelLimit)
	{
		actualSize = levelCount * sizeof(BTreeFileEntryLevel);
		actualSize += growthSize - 1;
		actualSize -= actualSize % growthSize;
		if (storeCache->levelArray)
			tempArray = (BTreeFileEntryLevel *) 
				realloc(storeCache->levelArray, actualSize);
		else
			tempArray = (BTreeFileEntryLevel *) malloc(actualSize);

		if (! tempArray)
			_NXRaiseError(NX_BTreeNoMemory, 
				"_btreeFileBuildLevelArray", 0);

		storeCache->levelArray = tempArray;
		formerSize = storeCache->levelLimit * 
			sizeof(BTreeFileEntryLevel);
		bzero((char *) tempArray + formerSize, 
			actualSize - formerSize);
		storeCache->levelLimit = actualSize / 
			sizeof(BTreeFileEntryLevel);
		actualSize = 0;
		for (; actualSize < storeCache->levelCount; ++actualSize)
		{
			BTreePageEntryChain	*entryChain;
			BTreePageEntry		*pageEntry;

			entryChain = &tempArray[actualSize].dirtyChain;
			pageEntry = entryChain->nextEntry;
			for (; pageEntry; pageEntry = pageEntry->nextEntry)
				pageEntry->entryChain = entryChain;
		}
	}

	storeCache->levelCount = levelCount;
}

/*
routines for managing the page array
*/

static BTreeFilePageTable 
*_btreeFileFillPageBlock(BTreeFileCache *fileCache, unsigned short groupEntry)
{
	BTreeFilePageTable	*pageTable;

	pageTable = (BTreeFilePageTable *) 
		calloc(BTreeFilePageTablesPerPageBlock, 
			sizeof(BTreeFilePageTable));
	if (! pageTable)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileFillPageBlock", 0);

	fileCache->pageArray[groupEntry] = pageTable;
	return pageTable;
}

static BTreePage 
*_btreeFileFillPageTable(BTreeFileCache *fileCache, 
	BTreeFilePageTable *pageTable, vm_offset_t readOffset)
{
	vm_address_t		pageBuffer;
	unsigned short		i;
	unsigned long		readLength;
	BTreePage		*btreePage;
	BTreeFileHeader		*fileHeader;

	btreePage = (BTreePage *) 
		calloc(BTreeFilePagesPerPageTable, sizeof(BTreePage));
	if (! btreePage)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileFillPageTable", 0);

	pageTable->btreePage = btreePage;
	pageTable->minimumPage = BTreeFilePagesPerPageTable;
	fileHeader = fileCache->fileHeader;
	readLength = fileHeader->pageSize * BTreeFilePagesPerPageTable;
	readOffset = readOffset - (readOffset % readLength);
	pageBuffer = (vm_address_t) 0;
	_btreeFileRead(fileCache->fileDescriptor, readOffset, 
		&pageBuffer, readLength, fileCache->fileStatus.writeable);
	for (i = 0; i < BTreeFilePagesPerPageTable; ++i, ++btreePage)
	{
		btreePage->pageBuffer = pageBuffer;
		pageBuffer += fileHeader->pageSize;
	}

	return pageTable->btreePage;
}

BTreePage 
*_btreeFilePageGroupFromOffset(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, unsigned long *pageCount, boolean_t modifying)
{
	unsigned long		pageNumber;
	BTreePage		*btreePage;
	unsigned short		countEntry;
	unsigned short		groupEntry;
	BTreeFilePageTable	*pageTable;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
#if	defined(DEBUG)
	if (pageOffset >= fileHeader->fileSize)
		_NXRaiseError(NX_BTreeInvalidArguments, 
			"_btreeFilePageGroupFromOffset", 0);
#endif
	pageNumber = pageOffset / fileHeader->pageSize;
	groupEntry = pageNumber / BTreeFilePagesPerPageBlock;
	pageNumber %= BTreeFilePagesPerPageBlock;
	countEntry = pageNumber / BTreeFilePagesPerPageTable;
	pageNumber %= BTreeFilePagesPerPageTable;
	pageTable = fileCache->pageArray[groupEntry];
	if (! pageTable)
		pageTable = _btreeFileFillPageBlock(fileCache, groupEntry);

	pageTable += countEntry;
	btreePage = pageTable->btreePage;
	if (! btreePage)
		btreePage = _btreeFileFillPageTable(fileCache, 
			pageTable, pageOffset);

	countEntry = pageNumber + *pageCount;
	if (countEntry > BTreeFilePagesPerPageTable)
	{
		countEntry = BTreeFilePagesPerPageTable;
		*pageCount = countEntry - pageNumber;
	}

	if (modifying)
	{
		if (pageNumber < pageTable->minimumPage)
			pageTable->minimumPage = pageNumber;

		if (countEntry > pageTable->maximumPage)
			pageTable->maximumPage = countEntry;
	}

	return btreePage + pageNumber;
}

static BTreePage 
*_btreeFileOpenPageGroup(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, unsigned long *pageCount, boolean_t modifying, 
		unsigned long *pageLevel)
{
	unsigned long		openCount;
	unsigned long		openLevel;
	BTreeFileHeader		*fileHeader;
	BTreeFilePageHeader	*pageHeader;
	BTreePage		*btreePage;
	BTreePage		*extraPage;

	fileHeader = fileCache->fileHeader;
	btreePage = _btreeFilePageGroupFromOffset(fileCache, 
		pageOffset, pageCount, modifying);
	if (! btreePage->pageOffset)
	{
		if (! pageLevel)
		{
			pageHeader = (BTreeFilePageHeader *) 
				btreePage->pageBuffer;
			openLevel = pageHeader->pageLevel;
#if			defined(DEBUG)
			if (fileHeader->codeVersion[0] > 1 && 
					pageHeader->verification != pageOffset)
				_NXRaiseError(NX_BTreeInvalidArguments, 
					"_btreeFileOpenPageGroup", 0);
#endif
		} else
			openLevel = *pageLevel;

		extraPage = btreePage;
		for (openCount = *pageCount; openCount > 0; --openCount)
		{
			extraPage->pageLevel = openLevel;
			extraPage->pageOffset = pageOffset;
			pageOffset += fileHeader->pageSize;
			++extraPage;
		}
	}

	return btreePage;
}

inline static BTreePage 
*_btreeFileOpenPage(BTreeFileCache *fileCache, 
	vm_offset_t pageOffset, boolean_t modifying)
{
	unsigned long		pageCount = 1;

	return _btreeFileOpenPageGroup(fileCache, 
		pageOffset, &pageCount, modifying, (unsigned long *) 0);
}

@implementation BTreeStoreFile

+ newFile: (BTreeFile *) sender 
	entry: (unsigned short) countEntry offset: (vm_offset_t) pageOffset
{
	NXHandler		jumpHandler;
	BTreeFileTemplate	*btreeFile;
	BTreePage		*btreePage;
	BTreeFileCountTable	*countTable;
	BTreeFileCache		*fileCache;
	BTreeFileHeader		*fileHeader;
	BTreeStoreCache		*storeCache;

	self = [super new];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[self free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	rootOffset = pageOffset;
	headerSize = sizeof(BTreeFilePageHeader);
	_storeCache = (void *) 
		calloc(sizeof(BTreeStoreCache), sizeof(char));
	if (! _storeCache)
		_NXRaiseError(NX_BTreeNoMemory, "newFile:entry:offset:", 0);

	storeCache = (BTreeStoreCache *) _storeCache;
	storeCache->btreeFile = sender;
	btreeFile = (BTreeFileTemplate *) storeCache->btreeFile;
	fileCache = (BTreeFileCache *) btreeFile->_backingStore;
	storeCache->fileCache = fileCache;
	storeStatus.writeable = fileCache->fileStatus.writeable;
	if (storeStatus.writeable)
	{
		btreePage = _btreeFileOpenPage(fileCache, pageOffset, NO);
		_btreeFileBuildLevelArray(storeCache, btreePage->pageLevel);
	}

	fileHeader = fileCache->fileHeader;
	pageSize = fileHeader->pageSize;
	codeVersion = fileHeader->codeVersion[1];
	countTable = &fileHeader->countTable;
	if (countEntry < countTable->tableLimit)
	{
		storeCache->countEntry = countTable->countTable + countEntry;
		recordCount = storeCache->countEntry->recordCount;
	} else
		recordCount = (unsigned long) -1L;

	_NXRemoveHandler(&jumpHandler);
	storeCache->storeValid = YES;
	return self;
}

- free
{
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	if (storeCache)
	{
		if (storeCache->storeValid)
		{
			[storeCache->btreeFile save];
			[storeCache->btreeFile _btreeClosed: self];
		}
	
		if (storeCache->levelArray)
			free((void *) storeCache->levelArray);

		free((void *) storeCache);
	}

	return [super free];
}

- (unsigned) count
{
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	return (recordCount < (unsigned long) -1L) ? recordCount : 
		[[storeCache->btreeFile _btreeByStore: self] _count];
}

static BTreePage 
*_btreeFileCreatePageGroup(BTreeStoreCache *storeCache, 
	unsigned long *pageCount, unsigned long pageLevel)
{
	NXHandler		jumpHandler;
	vm_offset_t		pageOffset;
	unsigned long		saveCount;
	BTreePage		*btreePage;
	BTreeFileHeader		*fileHeader;
	BTreeFileCache		*fileCache;

	fileCache = storeCache->fileCache;
	pageOffset = _btreeFileNewOffsetGroup(fileCache, pageCount);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		_btreeFileFreeOffsetGroup(fileCache, pageOffset, *pageCount);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	saveCount = *pageCount;
	btreePage = _btreeFileOpenPageGroup(fileCache, 
		pageOffset, pageCount, YES, &pageLevel);
	if (*pageCount < saveCount)
	{
		fileHeader = fileCache->fileHeader;
		_btreeFileFreeOffsetGroup(fileCache, 
			pageOffset + (*pageCount * fileHeader->pageSize), 
				saveCount - *pageCount);
	}

	_btreeFileAppendPageEntry(&storeCache->bindRecord.allocChain, 
		_btreeFileMakePageEntry(fileCache, btreePage, *pageCount));
	_NXRemoveHandler(&jumpHandler);
	return btreePage;
}

inline static BTreePage 
*_btreeFileCreatePage(BTreeStoreCache *storeCache)
{
	unsigned long	pageCount = 1;

	return _btreeFileCreatePageGroup(storeCache, 
		&pageCount, (unsigned long) 0);
}

- (BTreePage *) createPage
{
	BTreePage	*btreePage;

	btreePage = _btreeFileCreatePage((BTreeStoreCache *) _storeCache);
	return btreePage;
}

- (BTreePage *) openPageAt: (vm_offset_t) pageOffset
{
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	return _btreeFileOpenPage(storeCache->fileCache, pageOffset, NO);
}

static BTreePage 
*_btreeFileTouchPageGroup(BTreeStoreCache *storeCache, 
	vm_offset_t pageOffset, unsigned long *pageCount)
{
	BTreePageEntry		*pageEntry;
	BTreeFileCache		*fileCache;
	BTreePage		*btreePage;
	BTreeFileHeader		*fileHeader;

	fileCache = storeCache->fileCache;
	btreePage = _btreeFileOpenPageGroup(fileCache, 
		pageOffset, pageCount, YES, (unsigned long *) 0);
	pageEntry = (BTreePageEntry *) btreePage->RESERVED;
	if (! pageEntry)
	{
		if (storeCache->saveEnabled)
			pageEntry = _btreeFileLoadPageEntry(fileCache, 
				btreePage, *pageCount);
		else
			pageEntry = _btreeFileMakePageEntry(fileCache, 
				btreePage, *pageCount);
	}

	if (! pageEntry->entryChain)
	{
		_btreeFileAppendPageEntry(&storeCache->bindRecord.dirtyChain, 
			pageEntry);
		fileCache->fileStatus.modified = YES;
		fileHeader = fileCache->fileHeader;
		if (! fileHeader->fileStatus.modified)
			_btreeFileWriteHeader(fileCache);
	}

	return btreePage;
}

inline static void 
_btreeFileTouchPage(BTreeStoreCache *storeCache, vm_offset_t pageOffset)
{
	unsigned long	pageCount = 1;

	_btreeFileTouchPageGroup(storeCache, pageOffset, &pageCount);
}

- (void) touchPage: (BTreePage *) btreePage
{
	_btreeFileTouchPage((BTreeStoreCache *) 
		_storeCache, btreePage->pageOffset);
}

inline static void 
_btreeFileDestroyPageEntry(BTreeFileCache *fileCache, 
	BTreePageEntry *pageEntry)
{
	unsigned long	pageCount;
	BTreePage	*btreePage;

	btreePage = pageEntry->btreePage;
	pageCount = pageEntry->pageCount;
	_btreeFileFreeOffsetGroup(fileCache, btreePage->pageOffset, pageCount);
	_btreeFileFreePageEntry(fileCache, btreePage);
	if (btreePage->pageOffset)
		for (; pageCount > 0; --pageCount, ++btreePage)
		{
			btreePage->pageLevel = 0;
			btreePage->pageOffset = (vm_offset_t) 0;
		}
}

static void 
_btreeFileTrashPageGroup(BTreeStoreCache *storeCache, 
	vm_offset_t pageOffset, unsigned long *pageCount)
{
	BTreePage		*btreePage;
	BTreePageEntry		*pageEntry;
	BTreePageEntryChain	*entryChain;
	BTreeFileCache		*fileCache;

	fileCache = storeCache->fileCache;
	btreePage = _btreeFileOpenPageGroup(fileCache, 
		pageOffset, pageCount, NO, (unsigned long *) 0);
	pageEntry = (BTreePageEntry *) btreePage->RESERVED;
	if (pageEntry)
	{
		entryChain = _btreeFileRemovePageEntry(pageEntry);
		if (entryChain == &storeCache->allocChain || 
			entryChain == &storeCache->bindRecord.allocChain)
		{
			_btreeFileDestroyPageEntry(fileCache, pageEntry);
			return;
		}
	} else
		pageEntry = _btreeFileMakePageEntry(fileCache, 
			btreePage, *pageCount);

	_btreeFileAppendPageEntry(&storeCache->bindRecord.trashChain, 
		pageEntry);
}

inline static void 
_btreeFileTrashPage(BTreeStoreCache *storeCache, vm_offset_t pageOffset)
{
	unsigned long	pageCount = 1;

	_btreeFileTrashPageGroup(storeCache, pageOffset, &pageCount);
}

- (void) destroyPage: (BTreePage *) btreePage
{
	_btreeFileTrashPage((BTreeStoreCache *) 
		_storeCache, btreePage->pageOffset);
}

- (void) destroyPageAt: (vm_offset_t) pageOffset
{
	_btreeFileTrashPage((BTreeStoreCache *) _storeCache, pageOffset);
}

- (void) createPageGroup: (vm_offset_t *) pageOffset 
	count: (unsigned long *) pageCount
{
	BTreePage		*btreePage;
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	btreePage = _btreeFileCreatePageGroup(storeCache, pageCount, 
		(unsigned long) -1L);
	*pageOffset = btreePage->pageOffset;
}

- (void) readPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount into: (vm_address_t) toAddress
{
	unsigned long		copyLength;
	unsigned long		copyCount;
	BTreePage		*btreePage;
	BTreeStoreCache		*storeCache;
	BTreeFileCache		*fileCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	fileCache = storeCache->fileCache;
	for (; pageCount; pageCount -= copyCount, 
		toAddress += copyLength, pageOffset += copyLength)
	{
		copyCount = pageCount;
		btreePage = _btreeFileOpenPageGroup(fileCache, 
			pageOffset, &copyCount, NO, (unsigned long *) 0);
		_btreeFileCopyBuffer(btreePage->pageBuffer, 
			copyLength = copyCount * pageSize, toAddress);
	}
}

- (void) writePageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount from: (vm_address_t) fromAddress
{
	unsigned long		copyLength;
	unsigned long		copyCount;
	BTreePage		*btreePage;

	for (; pageCount; pageCount -= copyCount, 
		fromAddress += copyLength, pageOffset += copyLength)
	{
		copyCount = pageCount;
		btreePage = _btreeFileTouchPageGroup((BTreeStoreCache *) 
			_storeCache, pageOffset, &copyCount);
		bcopy((char *) fromAddress, (char *) btreePage->pageBuffer, 
			copyLength = copyCount * pageSize);
	}
}

- (void) destroyPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount
{
	unsigned long		copyCount;

	for (; pageCount; pageCount -= copyCount, 
		pageOffset += copyCount * pageSize)
	{
		copyCount = pageCount;
		(void) _btreeFileTrashPageGroup((BTreeStoreCache *) 
			_storeCache, pageOffset, &copyCount);
	}
}

inline static void 
_btreeFileRecoverChain(BTreeFileCache *fileCache, 
	BTreePageEntryChain *entryChain)
{
	vm_offset_t		pageOffset;
	unsigned long		pageCount;
	BTreePage		*btreePage;
	BTreeFileHeader		*fileHeader;
	BTreePageEntry		*pageEntry;

	fileHeader = fileCache->fileHeader;
	while (pageEntry = entryChain->nextEntry)
	{
		btreePage = pageEntry->btreePage;
		pageCount = pageEntry->pageCount;
		bcopy((char *) pageEntry->copyBuffer, (char *) 
			btreePage->pageBuffer, 
				pageCount * fileHeader->pageSize);
		_btreeFileFreePageEntry(fileCache, btreePage);
		pageOffset = btreePage->pageOffset;
		btreePage->pageOffset = (vm_offset_t) 0;
		_btreeFileOpenPageGroup(fileCache, 
			pageOffset, &pageCount, YES, (unsigned long *) 0);
	}
}

inline static void 
_btreeFileDestroyChain(BTreeFileCache *fileCache, 
	BTreePageEntryChain *entryChain)
{
	BTreePageEntry	*pageEntry;

	while (pageEntry = entryChain->nextEntry)
		_btreeFileDestroyPageEntry(fileCache, pageEntry);
}

inline static void 
_btreeFileDiscardChain(BTreeFileCache *fileCache, 
	BTreePageEntryChain *entryChain)
{
	BTreePageEntry	*pageEntry;

	while (pageEntry = entryChain->nextEntry)
		_btreeFileFreePageEntry(fileCache, pageEntry->btreePage);
}

- (void) _save
{
	unsigned long		levelNumber;
	BTreeFileCache		*fileCache;
	BTreeFileHeader		*fileHeader;
	BTreeFileEntryLevel	*entryLevel;
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	fileCache = storeCache->fileCache;
	fileHeader = fileCache->fileHeader;
	if (codeVersion > fileHeader->codeVersion[1])
		fileHeader->codeVersion[1] = codeVersion;

	if (storeCache->countEntry)
		storeCache->countEntry->recordCount = recordCount;

	if (storeCache->saveEnabled)
	{
		_btreeFileDestroyChain(fileCache, &storeCache->trashChain);
		_btreeFileDiscardChain(fileCache, &storeCache->allocChain);
		levelNumber = 0;
		for (; levelNumber < storeCache->levelCount; ++levelNumber)
		{
			entryLevel = storeCache->levelArray + levelNumber;
			_btreeFileDiscardChain(fileCache, 
					&entryLevel->dirtyChain);
		}
	}
}

- (void) save
{
	BTreeFileCache		*fileCache;
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	if (storeCache->saveEnabled)
	{
		fileCache = storeCache->fileCache;
		if (fileCache->fileStatus.modified)
			[self _save];
	} else
		storeCache->saveEnabled = YES;
}

- (void) _undo
{
	unsigned long		levelNumber;
	BTreeFileCache		*fileCache;
	BTreeFileEntryLevel	*entryLevel;
	BTreeStoreCache		*storeCache;
	BTreePage		*btreePage;
	BTreeFilePageHeader	*pageHeader;

	[self bind];
	storeCache = (BTreeStoreCache *) _storeCache;
	if (storeCache->saveEnabled)
	{
		fileCache = storeCache->fileCache;
		if (storeCache->countEntry)
			recordCount = storeCache->countEntry->recordCount;
		else
			recordCount = (unsigned long) -1L;

		_btreeFileDestroyChain(fileCache, &storeCache->allocChain);
		_btreeFileDiscardChain(fileCache, &storeCache->trashChain);
		levelNumber = 0;
		for (; levelNumber < storeCache->levelCount; ++levelNumber)
		{
			entryLevel = storeCache->levelArray + levelNumber;
			_btreeFileRecoverChain(fileCache, 
					&entryLevel->dirtyChain);
		}

		btreePage = _btreeFileOpenPage(fileCache, rootOffset, NO);
		pageHeader = (BTreeFilePageHeader *) btreePage->pageBuffer;
		btreePage->pageLevel = pageHeader->pageLevel;
	}
}

- (void) undo
{
	BTreeFileCache		*fileCache;
	BTreeStoreCache		*storeCache;

	storeCache = (BTreeStoreCache *) _storeCache;
	fileCache = storeCache->fileCache;
	if (fileCache->fileStatus.modified)
		[self _undo];
}

static void 
_btreeFileRecycleChain(BTreeFileCache *fileCache, 
	BTreePageEntryChain *entryChain, vm_offset_t rootOffset)
{
	BTreeFilePageHeader	*pageHeader;
	BTreePage		*btreePage;
	BTreePageEntry		*pageEntry;

	pageEntry = entryChain->nextEntry;
	for (; pageEntry; pageEntry = pageEntry->nextEntry)
	{
		btreePage = pageEntry->btreePage;
		if (btreePage->pageLevel < (unsigned long) -1L)
		{
			pageHeader = (BTreeFilePageHeader *) 
				btreePage->pageBuffer;
			gettimeofday(&pageHeader->timeStamp, 0);
			if (rootOffset)
			{
				pageHeader->rootOffset = rootOffset;
				pageHeader->pageLevel = btreePage->pageLevel;
				pageHeader->verification = 
					btreePage->pageOffset;
			}
		}
	}
}

static void 
_btreeFileExplodeChain(BTreeStoreCache *storeCache, 
	BTreePageEntryChain *entryChain)
{
	unsigned long		pageLevel;
	BTreePage		*btreePage;
	BTreePageEntryChain	*dirtyChain;
	BTreePageEntry		*pageEntry;

	while (pageEntry = entryChain->nextEntry)
	{
		btreePage = pageEntry->btreePage;
		pageLevel = btreePage->pageLevel + 1;
		if (pageLevel >= storeCache->levelCount)
			_btreeFileBuildLevelArray(storeCache, pageLevel);
	
		dirtyChain = &storeCache->levelArray[pageLevel].dirtyChain;
		_btreeFileRemovePageEntry(pageEntry);
		_btreeFileAppendPageEntry(dirtyChain, pageEntry);
	}
}

- (void) bind
{
	BTreeFileCache		*fileCache;
	BTreePage		*btreePage;
	BTreeFilePageHeader	*pageHeader;
	BTreeFileBindRecord	*bindRecord;
	BTreeStoreCache		*storeCache;
	BTreePageEntry		*pageEntry;
	BTreeFileEntryLevel	*entryLevel;

	storeCache = (BTreeStoreCache *) _storeCache;
	fileCache = storeCache->fileCache;
	bindRecord = &storeCache->bindRecord;
	btreePage = _btreeFileOpenPage(fileCache, rootOffset, NO);
	pageHeader = (BTreeFilePageHeader *) btreePage->pageBuffer;
	pageEntry = btreePage->RESERVED;
	if (pageEntry && pageHeader->pageLevel != btreePage->pageLevel)
	{
		entryLevel = storeCache->levelArray + 
			1 + pageHeader->pageLevel;
		pageHeader->pageLevel = btreePage->pageLevel;
		if (pageEntry->entryChain == &entryLevel->dirtyChain)
		{
			_btreeFileRemovePageEntry(pageEntry);
			_btreeFileAppendPageEntry(&bindRecord->dirtyChain, 
				pageEntry);
		}
	}

	_btreeFileRecycleChain(fileCache, 
		&bindRecord->allocChain, rootOffset);
	_btreeFileRecycleChain(fileCache, 
		&bindRecord->dirtyChain, (vm_offset_t) 0);
	if (storeCache->saveEnabled)
	{
		_btreeFileAppendPageEntryChain(&bindRecord->allocChain, 
			&storeCache->allocChain);
		_btreeFileAppendPageEntryChain(&bindRecord->trashChain, 
			&storeCache->trashChain);
		_btreeFileExplodeChain(storeCache, &bindRecord->dirtyChain);
	} else
	{
		_btreeFileDiscardChain(fileCache, &bindRecord->allocChain);
		_btreeFileDiscardChain(fileCache, &bindRecord->dirtyChain);
		_btreeFileDestroyChain(fileCache, &bindRecord->trashChain);
	}
}


@end


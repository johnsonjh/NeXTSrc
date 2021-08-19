
#ifdef	SHLIB
#import	"shlib.h"
#endif


#import	"BTreeFilePrivate.h"

static void 
_btreeFileFlushOffsetCache(BTreeFileCache *fileCache)
{
	vm_address_t		copyBuffer;
	vm_offset_t		pageOffset;
	unsigned long		offsetCount;
	BTreeFileOffsetCache	*tempOffsetCache;
	BTreeFileOffsetTable	*tempOffsetTable;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	tempOffsetCache = (fileCache->fileStatus.recoverable) ? 
		&fileHeader->tempOffsetCache : &fileHeader->freeOffsetCache;
	tempOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + tempOffsetCache->offsetTable);
	if (tempOffsetTable->offsetCount < 3)
		return;

	pageOffset = _btreeFileNewOffset(fileCache);
	copyBuffer = _btreeFilePageBufferFromOffset(fileCache, 
		pageOffset, YES);
	if (! fileCache->fileStatus.recoverable)
	{
		offsetCount = tempOffsetTable->offsetCount; 
		tempOffsetTable->offsetCount = offsetCount >> 1;
	}

	bcopy((char *) tempOffsetTable, (char *) copyBuffer, 
		sizeof(BTreeFileOffsetTable) + 
		(tempOffsetTable->offsetCount * sizeof(BTreeFileOffsetEntry)));
	if (! fileCache->fileStatus.recoverable)
	{
		offsetCount -= tempOffsetTable->offsetCount;
		bcopy((char *) (tempOffsetTable->entryArray + 
					tempOffsetTable->offsetCount), 
			(char *) tempOffsetTable->entryArray, 
				offsetCount * sizeof(BTreeFileOffsetEntry));
		tempOffsetTable->offsetCount = offsetCount;
	} else
		tempOffsetTable->offsetCount = 0;

	tempOffsetCache->offsetTotal++;
	tempOffsetTable->offsetQueue = pageOffset;
	if (! tempOffsetCache->offsetQueue)
		tempOffsetCache->offsetQueue = pageOffset;
}

static int 
_btreeFileCompareOffsets(const void *data1, const void *data2)
{
	BTreeFileOffsetEntry	*offsetEntry1;
	BTreeFileOffsetEntry	*offsetEntry2;

	offsetEntry1 = (BTreeFileOffsetEntry *) data1;
	offsetEntry2 = (BTreeFileOffsetEntry *) data2;
	return offsetEntry1->entryOffset - offsetEntry2->entryOffset;
}

unsigned long 
_btreeFileMergeOffsetCache(BTreeFileCache *fileCache)
{
	BTreeFileHeader		*fileHeader;
	BTreeFileOffsetCache	*freeOffsetCache;
	BTreeFileOffsetCache	*tempOffsetCache;
	BTreeFileOffsetTable	*freeOffsetTable;
	BTreeFileOffsetTable	*tempOffsetTable;
	BTreeFileOffsetTable	*copyOffsetTable;

	fileHeader = fileCache->fileHeader;
	tempOffsetCache = &fileHeader->tempOffsetCache;
	if (tempOffsetCache->offsetTotal < 1)
		return 0;

	tempOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + tempOffsetCache->offsetTable);
	freeOffsetCache = &fileHeader->freeOffsetCache;
	freeOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + freeOffsetCache->offsetTable);
	fileHeader->fileStatus.modified = YES;
	fileCache->fileStatus.modified = YES;
	if (freeOffsetTable->offsetCount + tempOffsetTable->offsetCount > 
			freeOffsetCache->offsetLimit)
		_btreeFileFlushOffsetCache(fileCache);

	if (tempOffsetCache->offsetQueue)
	{
		if (freeOffsetCache->offsetQueue)
		{
			copyOffsetTable = (BTreeFileOffsetTable *)
				_btreeFilePageBufferFromOffset(fileCache, 
					freeOffsetCache->offsetQueue, YES);
			copyOffsetTable->offsetQueue = 
				tempOffsetTable->offsetQueue;
		} else
			freeOffsetTable->offsetQueue = 
				tempOffsetTable->offsetQueue;

		freeOffsetCache->offsetQueue = tempOffsetCache->offsetQueue;
		tempOffsetCache->offsetQueue = (vm_offset_t) 0;
		tempOffsetTable->offsetQueue = (vm_offset_t) 0;
	}

	bcopy((char *) tempOffsetTable->entryArray, (char *) 
		(freeOffsetTable->entryArray + freeOffsetTable->offsetCount), 
			tempOffsetTable->offsetCount * 
				sizeof(BTreeFileOffsetEntry));
	freeOffsetTable->offsetCount += tempOffsetTable->offsetCount;
	if (freeOffsetTable->offsetCount > tempOffsetTable->offsetCount)
		qsort(freeOffsetTable->entryArray, 
		freeOffsetTable->offsetCount, sizeof(BTreeFileOffsetEntry), 
			_btreeFileCompareOffsets);

	tempOffsetTable->offsetCount = 0;
	freeOffsetCache->offsetTotal += tempOffsetCache->offsetTotal;
	tempOffsetCache->offsetTotal = 0;
	return freeOffsetCache->offsetTotal;
}

static vm_offset_t 
_btreeFileLoadOffsetTable(BTreeFileCache *fileCache, 
	BTreeFileOffsetTable *thisOffsetTable, 
	BTreeFileOffsetCache *thisOffsetCache)
{
	vm_offset_t	pageOffset;
	vm_address_t	pageBuffer;

	pageOffset = thisOffsetTable->offsetQueue;
	if (pageOffset)
	{
		pageBuffer = _btreeFileReadBufferFromOffset(fileCache, 
			pageOffset);
		bcopy((char *) pageBuffer, (char *) thisOffsetTable, 
			sizeof(BTreeFileOffsetTable) + 
				(thisOffsetCache->offsetLimit * 
					sizeof(BTreeFileOffsetEntry)));
		if (! thisOffsetTable->offsetQueue)
			thisOffsetCache->offsetQueue = (vm_offset_t) 0;

		--thisOffsetCache->offsetTotal;
		_btreeFileFreeOffset(fileCache, pageOffset);
	}

	return pageOffset;
}

static void 
_btreeFileTrimLength(BTreeFileCache *fileCache, vm_offset_t entryOffset)
{
	unsigned long		offsetCount;
	unsigned long		offsetIndex;
	BTreeFileOffsetEntry	*arrayEntry;
	BTreeFileOffsetEntry	*entryArray;
	BTreeFileOffsetCache	*tempOffsetCache;
	BTreeFileOffsetTable	*tempOffsetTable;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	tempOffsetCache = (fileCache->fileStatus.recoverable) ? 
		&fileHeader->tempOffsetCache : &fileHeader->freeOffsetCache;
	tempOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + tempOffsetCache->offsetTable);
	fileHeader->fileStatus.modified = YES;
	fileCache->fileStatus.modified = YES;
	entryArray = tempOffsetTable->entryArray;
	fileHeader->fileSize = entryOffset;
	for (;;)
	{
		offsetCount = tempOffsetTable->offsetCount;
		offsetIndex = 0;
		for (; offsetIndex < offsetCount; offsetIndex++)
		{
			arrayEntry = entryArray + offsetIndex;
			entryOffset -= arrayEntry->entryLength;
			if (arrayEntry->entryOffset < entryOffset)
			{
				entryOffset += arrayEntry->entryLength;
				break;
			}
		}
	
		if (! offsetIndex)
			break;
	
		offsetCount = fileHeader->fileSize - entryOffset;
		tempOffsetCache->offsetTotal -= 
			offsetCount / fileHeader->pageSize;
		fileHeader->fileSize = entryOffset;
		tempOffsetTable->offsetCount -= offsetIndex;
		if (offsetCount = tempOffsetTable->offsetCount)
		{
			bcopy((char *) (entryArray + offsetIndex), 
				(char *) entryArray, offsetCount * 
					sizeof(BTreeFileOffsetEntry));
			break;
		}
	
		if (! _btreeFileLoadOffsetTable(fileCache, 
			tempOffsetTable, tempOffsetCache))
			break;
	}
}

inline static unsigned long
_btreeFileFindOffset(vm_offset_t searchOffset, 
	BTreeFileOffsetTable *offsetTable)
{
	vm_offset_t		entryOffset;
	unsigned long		upperBound;
	unsigned long		lowerBound;
	unsigned long		searchEntry;
	BTreeFileOffsetEntry	*entryArray;

	entryArray = offsetTable->entryArray;
	upperBound = offsetTable->offsetCount;
	lowerBound = 0;
	while (lowerBound < upperBound)
	{
		searchEntry = (lowerBound + upperBound) >> 1;
		entryOffset = entryArray[searchEntry].entryOffset;
		if (entryOffset < searchOffset)
			upperBound = searchEntry;
		else
		if (entryOffset > searchOffset)
			lowerBound = searchEntry + 1;
		else
			return searchEntry;
	}

	return lowerBound;
}

inline static void
_btreeFileFreeOffsetBlock(BTreeFileCache *fileCache, 
	vm_offset_t entryOffset, unsigned long entryLength)
{
	unsigned long		offsetCount;
	unsigned long		offsetIndex;
	unsigned long		offsetLimit;
	BTreeFileOffsetEntry	*arrayEntry;
	BTreeFileOffsetEntry	*entryArray;
	BTreeFileOffsetCache	*tempOffsetCache;
	BTreeFileOffsetTable	*tempOffsetTable;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	tempOffsetCache = (fileCache->fileStatus.recoverable) ? 
		&fileHeader->tempOffsetCache : &fileHeader->freeOffsetCache;
	tempOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + tempOffsetCache->offsetTable);
	fileHeader->fileStatus.modified = YES;
	fileCache->fileStatus.modified = YES;
	if (tempOffsetTable->offsetCount >= tempOffsetCache->offsetLimit)
		_btreeFileFlushOffsetCache(fileCache);

	offsetCount = entryLength / fileHeader->pageSize;
	entryArray = tempOffsetTable->entryArray;
	if (offsetIndex = tempOffsetTable->offsetCount)
	{
		arrayEntry = entryArray + offsetIndex - 1;
		if (entryOffset >= arrayEntry->entryOffset)
		{
			offsetIndex = _btreeFileFindOffset(entryOffset, 
				tempOffsetTable);
			arrayEntry = entryArray + offsetIndex;
			offsetLimit = arrayEntry->entryOffset + 
				arrayEntry->entryLength;
			if (entryOffset < offsetLimit)
				_NXRaiseError(NX_BTreeInternalError, 
					"_btreeFileFreeOffsetBlock", 0);
	
			if (entryOffset == offsetLimit)
			{
				arrayEntry->entryLength += entryLength;
				tempOffsetCache->offsetTotal += offsetCount;
				return;
			}

			offsetLimit = tempOffsetTable->offsetCount - 
				offsetIndex;
			offsetLimit *= sizeof(BTreeFileOffsetEntry);
			bcopy((char *) arrayEntry, (char *) (arrayEntry + 1), 
				offsetLimit);
		} else
		if (entryOffset + entryLength == arrayEntry->entryOffset)
		{
			arrayEntry->entryLength += entryLength;
			arrayEntry->entryOffset = entryOffset;
			tempOffsetCache->offsetTotal += offsetCount;
			return;
		} else
		if (entryOffset + entryLength > arrayEntry->entryOffset)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeFileFreeOffsetBlock", 0);
	}

	tempOffsetTable->offsetCount++;
	tempOffsetCache->offsetTotal += offsetCount;
	entryArray[offsetIndex].entryOffset = entryOffset;
	entryArray[offsetIndex].entryLength = entryLength;
}

void
_btreeFileFreeOffsetEntry(BTreeFileCache *fileCache, 
	BTreeFileOffsetEntry offsetEntry)
{
	vm_offset_t		offsetLimit;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	offsetLimit = offsetEntry.entryOffset + offsetEntry.entryLength;
	if (offsetLimit < fileHeader->fileSize)
		_btreeFileFreeOffsetBlock(fileCache, 
			offsetEntry.entryOffset, offsetEntry.entryLength);
	else
	if (offsetLimit > fileHeader->fileSize)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeFileFreeOffset", 0);
	else
		_btreeFileTrimLength(fileCache, offsetEntry.entryOffset);
}

BTreeFileOffsetEntry 
_btreeFileNewOffsetEntry(BTreeFileCache *fileCache, unsigned long entryLength)
{
	vm_offset_t		entryOffset;
	unsigned long		offsetCount;
	unsigned long		totalLength;
	BTreeFileOffsetEntry	offsetEntry;
	BTreeFileOffsetEntry	*arrayEntry;
	BTreeFileOffsetEntry	*entryArray;
	BTreeFileOffsetTable	*freeOffsetTable;
	BTreeFileOffsetCache	*freeOffsetCache;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	freeOffsetCache = &fileHeader->freeOffsetCache;
	freeOffsetTable = (BTreeFileOffsetTable *) 
		((char *) fileHeader + freeOffsetCache->offsetTable);
	fileHeader->fileStatus.modified = YES;
	fileCache->fileStatus.modified = YES;
	if (! freeOffsetTable->offsetCount && 
		! (entryOffset = _btreeFileLoadOffsetTable(fileCache, 
			freeOffsetTable, freeOffsetCache)))
	{
		entryOffset = fileHeader->fileSize;
		if (entryOffset + entryLength < entryLength)
			_NXRaiseError(NX_BTreeStoreFull, 
				"_btreeFileNewOffsetEntry", 0);

		fileHeader->fileSize += entryLength;
		if (entryOffset < fileCache->fileSize)
			bzero((char *) 
				_btreeFileReadBufferFromOffset(fileCache, 
					entryOffset), fileHeader->pageSize);

		if (fileHeader->fileSize > fileCache->fileSize)
		{
			totalLength = fileHeader->pageSize << 5;
			fileCache->fileSize = fileHeader->fileSize;
			if (fileCache->fileSize < 
				fileCache->fileSize + totalLength)
			{
				fileCache->fileSize += totalLength;
				fileCache->fileSize -= fileCache->fileSize % 
					totalLength;
			}

			if (ftruncate(fileCache->fileDescriptor, 
				fileCache->fileSize))
			_NXRaiseError(NX_BTreeUnixError + errno, 
				"_btreeFileSizeIncrease", strerror(errno));
		}

		offsetEntry.entryOffset = entryOffset;
		offsetEntry.entryLength = entryLength;
		return offsetEntry;
	}

	entryArray = freeOffsetTable->entryArray;
	entryOffset = (vm_offset_t) 0;
	totalLength = 0;
	offsetCount = freeOffsetTable->offsetCount;
	for (; offsetCount && entryLength; --offsetCount)
	{
		arrayEntry = entryArray + offsetCount - 1;
		if (! entryOffset)
			entryOffset = arrayEntry->entryOffset;
		else
		if (entryOffset + totalLength < arrayEntry->entryOffset)
			break;

		if (arrayEntry->entryLength > entryLength)
		{
			totalLength += entryLength;
			arrayEntry->entryLength -= entryLength;
			arrayEntry->entryOffset += entryLength;
			break;
		}

		totalLength += arrayEntry->entryLength;
		entryLength -= arrayEntry->entryLength;
	}

	freeOffsetTable->offsetCount = offsetCount;
	freeOffsetCache->offsetTotal -= totalLength / fileHeader->pageSize;
	offsetEntry.entryOffset = entryOffset;
	offsetEntry.entryLength = totalLength;
	bzero((char *) _btreeFileReadBufferFromOffset(fileCache, 
		entryOffset), fileHeader->pageSize);
	return offsetEntry;
}


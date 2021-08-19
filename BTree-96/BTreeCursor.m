
#ifdef	SHLIB
#import	"shlib.h"
#endif

/* cursor management routines */

#import	"BTreePrivate.h"

inline static boolean_t 
_btreeTraceSynchronized(__BTreeCursor *btreeCursor)
{
	__BTree		*__btree;

	__btree = (__BTree *) btreeCursor->btree;
	return (btreeCursor->_syncVersion == __btree->_syncVersion && 
		! btreeCursor->_cursorStatus.modified && 
		! btreeCursor->_cursorStatus.traceNeeded);
}

inline static boolean_t 
_btreeIsMatch(__BTreeCursor *btreeCursor)
{
	BTreeTraceRecord	*traceRecord;
	boolean_t		returnValue;

	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	returnValue = traceRecord->traceStatus.exactMatch;
	if (! returnValue && traceRecord->traceStatus.lastPosition)
		_btreeForwardExternal(btreeCursor);

	return returnValue;
}

inline static void 
_btreeSetKeyLength(__BTreeCursor *btreeCursor, unsigned short keyLength)
{
	if (btreeCursor->bufferLength < keyLength)
	{
		if (btreeCursor->keyBuffer)
			free(btreeCursor->keyBuffer);

		btreeCursor->keyBuffer = malloc(keyLength);
		if (! btreeCursor->keyBuffer)
			_NXRaiseError(NX_BTreeNoMemory, 
				"_btreeSetKeyLength", 0);

		btreeCursor->bufferLength = 
			malloc_size(btreeCursor->keyBuffer);
	}

	btreeCursor->keyLength = keyLength;
}

inline static void 
_btreeReadKey(__BTreeCursor *btreeCursor)
{
	__BTree			*__btree;
	BTreeExternalRecord	*externalRecord;
	BTreeTraceRecord	*traceRecord;

	__btree = (__BTree *) btreeCursor->btree;
	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	if (btreeCursor->_cursorStatus.lastPosition)
		traceRecord->traceStatus.exactMatch = NO;
	else
	{
		if (traceRecord->traceStatus.lastPosition)
			_btreeForwardExternal(btreeCursor);

		_btreeReadPage((BTreeBackingStore *) 
			__btree->backingStore, traceRecord);
		externalRecord = _btreeExternalRecord(traceRecord);
		_btreeSetKeyLength(btreeCursor, 
			externalRecord->recordHeader.keyLength);
		bcopy(externalRecord->recordContent, btreeCursor->keyBuffer, 
			externalRecord->recordHeader.keyLength);
		traceRecord->traceStatus.exactMatch = YES;
	}

	btreeCursor->_cursorStatus.modified = NO;
	btreeCursor->_syncVersion = __btree->_syncVersion;
	btreeCursor->positionHint = 0L;
}

inline static void 
_btreeSetKey(__BTreeCursor *btreeCursor, 
	void *key, unsigned short keyLength, unsigned long positionHint)
{
	if (keyLength > btreeCursor->maxKeyLength)
		_NXRaiseError(NX_BTreeRecordTooLarge, "_btreeSetKey", 0);

	btreeCursor->_cursorStatus.modified = YES;
	_btreeSetKeyLength(btreeCursor, keyLength);
	bcopy((char *) key, (char *) btreeCursor->keyBuffer, keyLength);
	btreeCursor->positionHint = positionHint;
}

@implementation BTreeCursor

+ _newWith: (void *) private
{
	NXHandler		jumpHandler;
	BTreeStore		*btreeStore;
	__BTree			*__btree;
	BTreeBackingStore	*backingStore;

	self = [super new];
	__btree = (__BTree *) private;
	btree = (void *) __btree;
	backingStore = (BTreeBackingStore *) __btree->backingStore;
	btreeStore = backingStore->btreeStore;
	maxKeyLength = _btreeNodeSize(btreeStore) - BTreeExternalNodeLength;
	maxKeyLength /= 2;
	maxKeyLength -= sizeof(BTreeMappingRecord) + 
		sizeof(BTreeExternalRecord) + sizeof(unsigned short);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[self free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		_btreeIncreaseTraceDepth((__BTreeCursor *) self, 
			__btree->btreeDepth);
		_NXRemoveHandler(&jumpHandler);
	}

	_cursorStatus.leftRotation = YES;
	if ([btreeStore isMemberOf: [BTreeStoreMemory class]])
		_cursorStatus.rightRotation = YES;

	return self;
}

+ new
{
	return [self _newWith: [BTree new]];
}

- (void) _setSpanning: (BOOL) spanning
{
	_cursorStatus.spanning = YES;
}

- (void) _setRightRotation: (BOOL) rightRotation
{
	_cursorStatus.rightRotation = YES;
}

- (void) _setLeftRotation: (BOOL) leftRotation
{
	_cursorStatus.leftRotation = YES;
}

- free
{
	_btreeDecreaseTraceDepth((__BTreeCursor *) self, 0);
	if (keyBuffer)
		free(keyBuffer);

	return [super free];
}

- (void) setAutoSave: (BOOL) autoSave
{
	_cursorStatus.performSave = autoSave;
}

- (BOOL) autoSave
{
	return _cursorStatus.performSave;
}

- btree
{
	return btree;
}

- (void) setKey: (void *) data length: (unsigned short) dataLength 
	hint: (unsigned long) aHint
{
	_btreeSetKey((__BTreeCursor *) self, data, dataLength, aHint);
}

- (void) setKey: (void *) data length: (unsigned short) dataLength
{
	_btreeSetKey((__BTreeCursor *) self, data, dataLength, 0L);
}

- (unsigned short) maxKeyLength
{
	return maxKeyLength;
}

- (BOOL) getKey: (void **) data 
	length: (unsigned short *) dataLength valid: (BOOL *) validFlag;
{
	boolean_t	matchFlag;
	boolean_t	traceValid;

	traceValid = _btreeTraceSynchronized((__BTreeCursor *) self);
	if (! traceValid && (! positionHint || 
		! _btreeCheckHint((__BTreeCursor *) self)))
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	matchFlag = _btreeIsMatch((__BTreeCursor *) self);
	if (! traceValid || ! matchFlag)
		_btreeReadKey((__BTreeCursor *) self);

	*dataLength = keyLength;
	*data = keyBuffer;
	*validFlag = ! _cursorStatus.lastPosition;
	return matchFlag;
}

- (BOOL) setFirst;
{
	positionHint = 0L;
	if (! _btreeTraceSynchronized((__BTreeCursor *) self) || 
		! _cursorStatus.firstPosition)
	{
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceTop);
		_btreeReadKey((__BTreeCursor *) self);
	}

	return ! _cursorStatus.lastPosition;
}

- (void) setEnd;
{
	positionHint = 0L;
	if (! _btreeTraceSynchronized((__BTreeCursor *) self) || 
		! _cursorStatus.lastPosition)
	{
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceEnd);
		_btreeReadKey((__BTreeCursor *) self);
	}
}

- (BOOL) setPrevious
{
	if (! _btreeTraceSynchronized((__BTreeCursor *) self))
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	_btreeReverseExternal((__BTreeCursor *) self);
	_btreeReadKey((__BTreeCursor *) self);
	return ! _cursorStatus.firstPosition;
}

- (BOOL) setNext
{
	BTreeTraceRecord	*traceRecord;

	if (! _btreeTraceSynchronized((__BTreeCursor *) self))
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	if (traceRecord->traceStatus.exactMatch || 
			traceRecord->traceStatus.lastPosition)
		_btreeForwardExternal((__BTreeCursor *) self);

	_btreeReadKey((__BTreeCursor *) self);
	return ! _cursorStatus.lastPosition;
}

static void 
_btreeReadMapping(BTreeBackingStore *backingStore, BTreeMappingRecord 
	*mappingRecord, unsigned long mappingLength, 
	unsigned short mappingLevel, void **data, unsigned long *dataLength)
{
	kern_return_t			error;
	NXHandler			jumpHandler;
	vm_address_t			alignBuffer;
	unsigned long			readingLength;
	unsigned long			i;
	unsigned long			bufferLength;
	unsigned long			recordLength;
	unsigned long			mappingCount;
	unsigned long			regionLength;
	char				*alignTarget;
	char				*mappingTarget;
	void				*mappingData;
	BTreeStore			*btreeStore;

	volatile vm_address_t		recordBuffer;
	volatile unsigned long		alignLength;

	if (mappingLevel)
	{
		mappingData = (void *) 0;
		_btreeReadMapping(backingStore, mappingRecord, mappingLength, 
			mappingLevel - 1, &mappingData, &mappingLength);
		mappingRecord = (BTreeMappingRecord *) mappingData;
	}

	alignBuffer = (vm_address_t) 0;
	recordBuffer = (vm_address_t) 0;
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		if (mappingLevel)
			free((void *) mappingRecord);

		if (alignBuffer)
			vm_deallocate(task_self(), alignBuffer, 
				(unsigned long) alignLength);

		if (recordBuffer && recordBuffer != (vm_address_t) *data)
			free((void *) recordBuffer);

		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	btreeStore = backingStore->btreeStore;
	regionLength = sizeof(BTreeMappingRecord);
	regionLength = (mappingLength + regionLength - 1) / regionLength;
	recordLength = 0;
	for (mappingCount = 0; mappingCount < regionLength; ++mappingCount)
	{
		if (mappingRecord[mappingCount].regionOffset < 
			btreeStore->pageSize)
		{
			recordLength += 
				mappingRecord[mappingCount].regionOffset;
			break;
		}

		recordLength += mappingRecord[mappingCount].regionLength;
	}

	if (data)
	{
		bufferLength = recordLength;
		if (! *data)
		{
			recordBuffer = (vm_address_t) valloc(bufferLength);
			if (! recordBuffer)
				_NXRaiseError(NX_BTreeNoMemory, 
					"_btreeReadMapping", 0);
		} else
		{
			recordBuffer = (vm_address_t) *data;
			if (bufferLength > *dataLength)
				bufferLength = *dataLength;
		}

		if (recordBuffer % vm_page_size)
		{
			alignTarget = (char *) recordBuffer;
			alignLength = bufferLength + btreeStore->pageSize;
			error = vm_allocate(task_self(), 
				&alignBuffer, alignLength, YES);
			mappingTarget = (char *) alignBuffer;
		} else
		{
			alignTarget = (char *) 0;
			alignLength = btreeStore->pageSize;
			error = vm_allocate(task_self(), 
				&alignBuffer, alignLength, YES);
			mappingTarget = (char *) recordBuffer;
		}

		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"_btreeReadMapping", mach_error_string(error));

		mappingLength = bufferLength;
		for (i = 0; i < mappingCount; ++i)
		{
			regionLength = mappingRecord[i].regionLength;
			if (regionLength > mappingLength)
				regionLength = mappingLength;

			readingLength = regionLength / btreeStore->pageSize;
			backingStore->readPageGroup(btreeStore, 0, 
				mappingRecord[i].regionOffset, 
				readingLength, (vm_address_t) mappingTarget);
			readingLength *= btreeStore->pageSize;
			mappingTarget += readingLength;
			mappingLength -= readingLength;
			if (regionLength -= readingLength)
			{
				if (! alignTarget)
				{
					bufferLength = mappingLength;
					alignTarget = mappingTarget;
					mappingTarget = (char *) alignBuffer;
				}

				backingStore->readPageGroup(btreeStore, 0, 
					mappingRecord[i].regionOffset + 
					readingLength, 1, 
						(vm_address_t) mappingTarget);
				mappingTarget += regionLength;
				mappingLength -= regionLength;
				if (mappingLength && i < mappingCount - 1)
					_NXRaiseError(NX_BTreeInternalError, 
						"_btreeReadMapping", 0);

				break;
			}
		}

		if (alignTarget)
		{
			bufferLength -= mappingLength;
			bcopy((char *) alignBuffer, alignTarget, bufferLength);
			mappingTarget = alignTarget + bufferLength;
		}
		
		if (mappingLength)
		{
			if (mappingLength > mappingRecord[i].regionOffset)
				mappingLength = mappingRecord[i].regionOffset;

			bcopy((char *) &mappingRecord[i].regionLength, 
				mappingTarget, mappingLength);
		}

		error = vm_deallocate(task_self(), alignBuffer, alignLength);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"_btreeReadMapping", mach_error_string(error));

		*data = (void *) recordBuffer;
	}

	_NXRemoveHandler(&jumpHandler);
	*dataLength = recordLength;
	if (mappingLevel)
		free((void *) mappingRecord);
}

inline static BTreeTraceRecord 
*_btreeRightCousin(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord)
{
	BTreeTraceRecord	*rightRecord;

	if (_btreeRightSibling(btreeCursor, traceRecord))
		return traceRecord->siblingRecord;

	_btreeForwardExternal(btreeCursor);
	if (btreeCursor->_cursorStatus.lastPosition)
		_NXRaiseError(NX_BTreeInternalError, "_btreeRightCousin", 0);

	rightRecord = _btreeNewTraceRecord();
	bcopy((char *) traceRecord, (char *) rightRecord, 
		sizeof(BTreeTraceRecord));
	_btreeReverseExternal(btreeCursor);
	if (btreeCursor->_cursorStatus.firstPosition)
		_NXRaiseError(NX_BTreeInternalError, "_btreeRightCousin", 0);

	rightRecord->siblingRecord = traceRecord->siblingRecord;
	return traceRecord->siblingRecord = rightRecord;
}

- (BOOL) read: (void **) data 
	length: (unsigned long *) dataLength valid: (BOOL *) validFlag;
{
	boolean_t		traceValid;
	boolean_t		matchFlag;
	unsigned long		copyLength;
	char			*recordBuffer;
	__BTree			*__btree;
	BTreeTraceRecord	*traceRecord;
	BTreeTraceRecord	*rightRecord;
	BTreeExternalRecord	*externalRecord;
	BTreeExternalNode	*externalNode;

	traceValid = _btreeTraceSynchronized((__BTreeCursor *) self);
	if (! traceValid && (! positionHint || 
		! _btreeCheckHint((__BTreeCursor *) self)))
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	matchFlag = _btreeIsMatch((__BTreeCursor *) self);
	if (! traceValid || ! matchFlag)
		_btreeReadKey((__BTreeCursor *) self);

	if (_cursorStatus.lastPosition)
	{
		*validFlag = NO;
		return matchFlag;
	}

	*validFlag = YES;
	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	__btree = (__BTree *) btree;
	_btreeReadPage((BTreeBackingStore *) 
		__btree->backingStore, traceRecord);
	externalNode = traceRecord->traceNode.externalNode;
	copyLength = externalNode->entryArray[traceRecord->tracePosition];
	externalRecord = (BTreeExternalRecord *) 
		((char *) externalNode + copyLength);
	if (externalNode->nodeStatus.spanned && 
		traceRecord->tracePosition >= externalNode->entryCount - 1)
	{
		_btreeRightCousin((__BTreeCursor *) self, traceRecord);
		rightRecord = traceRecord->siblingRecord;
		externalNode = rightRecord->traceNode.externalNode;
		copyLength = sizeof(BTreeExternalRecord) + 
			externalRecord->recordHeader.keyLength + 
			externalRecord->recordLength;
		recordBuffer = alloca(copyLength + externalNode->spanLength);
		bcopy((char *) externalRecord, recordBuffer, copyLength);
		bcopy((char *) externalNode + externalNode->spanOffset, 
			recordBuffer + copyLength, externalNode->spanLength);
		externalRecord = (BTreeExternalRecord *) recordBuffer;
		externalRecord->recordLength += externalNode->spanLength;
		_btreeFreeTraceRecord(rightRecord);
		traceRecord->siblingRecord = traceRecord;
	}

	if (externalRecord->recordHeader.indirection)
	{
		recordBuffer = externalRecord->recordContent + 
			externalRecord->recordHeader.keyLength;
		_btreeReadMapping((BTreeBackingStore *) __btree->backingStore, 
			(BTreeMappingRecord *) recordBuffer, 
			externalRecord->recordLength, 
			externalRecord->recordHeader.indirection - 1, 
				data, dataLength);
		return matchFlag;
	}

	if (data)
	{
		copyLength = externalRecord->recordLength;
		if (*data)
		{
			if (copyLength > *dataLength)
				copyLength = *dataLength;
		} else
		if (! (*data = malloc(copyLength)))
			_NXRaiseError(NX_BTreeNoMemory, "_btreeReadRecord", 0);

		bcopy(externalRecord->recordContent + 
			externalRecord->recordHeader.keyLength, 
			(char *) *data, copyLength);
	}

	*dataLength = externalRecord->recordLength;
	return matchFlag;
}

- (unsigned long) hint
{
	boolean_t		exactMatch;
	boolean_t		traceValid;
	BTreeTraceRecord	*traceRecord;

	positionHint = 0L;
	traceValid = _btreeTraceSynchronized((__BTreeCursor *) self);
	if (! traceValid)
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	exactMatch = _btreeIsMatch((__BTreeCursor *) self);
	if (! traceValid || ! exactMatch)
	{
		_btreeReadKey((__BTreeCursor *) self);
		if (! exactMatch)
			return 0L;
	}

	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	return traceRecord->btreePage->pageOffset + traceRecord->tracePosition;
}

inline static void 
_btreeClearTrace(__BTreeCursor *btreeCursor)
{
	BTreeTraceRecord	*traceRecord;
	BTreeTraceRecord	*extraRecord;
	BTreeTraceRecord	*closeRecord;

	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	do
	{
		closeRecord = traceRecord->siblingRecord;
		while (closeRecord != traceRecord)
		{
			extraRecord = closeRecord->siblingRecord;
			_btreeFreeTraceRecord(closeRecord);
			closeRecord = extraRecord;
		}

		traceRecord->traceStatus.shadowed = NO;
		traceRecord->siblingRecord = traceRecord;
		traceRecord = traceRecord->childRecord;

	} while (traceRecord != 
		(BTreeTraceRecord *) btreeCursor->_tracePrivate);
}

static void 
_btreeInsertMapping(__BTreeCursor *btreeCursor, void *data, 
	unsigned long dataLength, unsigned long mappingLevel)
{
	NXHandler		jumpHandler;
	vm_offset_t		createOffset;
	unsigned long		createCount;
	unsigned long		mappingLength;
	unsigned short		mappingCount;
	unsigned short		recordLength;
	unsigned short		remainLength;
	unsigned short		excessLength;
	unsigned short		lengthLimit;
	__BTree			*__btree;
	BTreeBackingStore	*backingStore;
	BTreeStore		*btreeStore;
	BTreeMappingRecord	*mappingRecord;

	char			*volatile mappingBuffer;

	mappingBuffer = (char *) 0;
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		if (mappingBuffer)
			free((void *) mappingBuffer);

		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	__btree = (__BTree *) btreeCursor->btree;
	backingStore = (BTreeBackingStore *) __btree->backingStore;
	btreeStore = backingStore->btreeStore;
	lengthLimit = _btreeNodeSize(btreeStore) - 
		BTreeExternalNodeLength - BTreeExternalRecordOverhead;
	lengthLimit -= btreeCursor->keyLength;
	remainLength = dataLength % btreeStore->pageSize;
	mappingCount = dataLength / btreeStore->pageSize;
	recordLength = mappingCount * sizeof(BTreeMappingRecord);
	excessLength = recordLength;
	recordLength += sizeof(BTreeMappingRecord);
	if (remainLength)
		excessLength += remainLength + sizeof(vm_offset_t);

	if (! mappingCount || 
		(excessLength > lengthLimit && recordLength <= lengthLimit))
	{
		++mappingCount;
		excessLength = btreeStore->pageSize - remainLength;
		remainLength = 0;
	} else
	{
		recordLength = excessLength;
		excessLength = 0;
	}

	mappingBuffer = (char *) calloc(recordLength, sizeof(char));
	if (! mappingBuffer)
		_NXRaiseError(NX_BTreeNoMemory, "insert:length:", 0);

	mappingLength = 0;
	mappingRecord = (BTreeMappingRecord *) mappingBuffer;
	for (; mappingCount; ++mappingRecord, mappingCount -= createCount)
	{
		createCount = mappingCount;
		backingStore->createPageGroup(btreeStore, 
			0, &createOffset, &createCount);
		backingStore->writePageGroup(btreeStore, 0, createOffset, 
			createCount, (vm_address_t) data);
		mappingRecord->regionOffset = createOffset;
		mappingRecord->regionLength = 
			createCount * btreeStore->pageSize;
		data += mappingRecord->regionLength;
		mappingLength += sizeof(BTreeMappingRecord);
	}

	if (remainLength)
	{
		mappingLength += sizeof(vm_offset_t) + remainLength;
		mappingRecord->regionOffset = remainLength;
		bcopy((char *) data, (char *) 
			mappingRecord + sizeof(vm_offset_t), remainLength);
	} else
		mappingRecord[-1].regionLength -= excessLength;

	if (mappingLength > lengthLimit)
		_btreeInsertMapping(btreeCursor, mappingBuffer, 
			mappingLength, mappingLevel + 1);
	else
		_btreeInsertExternal(btreeCursor, mappingBuffer, 
			mappingLength, mappingLevel);

	_NXRemoveHandler(&jumpHandler);
	free((void *) mappingBuffer);
}

- (BOOL) insert: (void *) data length: (unsigned long) dataLength
{
	NXHandler		jumpHandler;
	BTreeStore		*btreeStore;
	__BTree			*__btree;
	BTreeBackingStore	*backingStore;
	BTreeTraceRecord	*traceRecord;

	positionHint = 0L;
	_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);
	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	if (traceRecord->traceStatus.exactMatch)
		return NO;

	__btree = (__BTree *) btree;
	backingStore = (BTreeBackingStore *) __btree->backingStore;
	btreeStore = backingStore->btreeStore;
	_btreeTouchPage(backingStore, traceRecord);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		_cursorStatus.traceNeeded = YES;
		backingStore->undo(btreeStore, 0);
		_btreeClearTrace((__BTreeCursor *) self);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	if (dataLength + keyLength > _btreeNodeSize(btreeStore) - 
			BTreeExternalNodeLength - BTreeExternalRecordOverhead)
		_btreeInsertMapping((__BTreeCursor *) self, 
			data, dataLength, 1);
	else
		_btreeInsertExternal((__BTreeCursor *) self, 
			data, dataLength, 0);

	++btreeStore->recordCount;
	if (backingStore->bind)
		backingStore->bind(btreeStore, 0);

	if (_cursorStatus.performSave || btreeStore->storeStatus.performSave)
		backingStore->save(btreeStore, 0);

	_cursorStatus.modified = NO;
	_btreeClearTrace((__BTreeCursor *) self);
	_NXRemoveHandler(&jumpHandler);
	return YES;
}

void 
_btreeRemoveMapping(BTreeBackingStore *backingStore, 
	BTreeMappingRecord *mappingRecord, unsigned long mappingLength, 
	unsigned short mappingLevel)
{
	NXHandler		jumpHandler;
	unsigned long		destroyCount;
	unsigned short		mappingCount;
	unsigned short		mappingIndex;
	void			*mappingData;
	BTreeStore		*btreeStore;

	mappingCount = mappingLength / sizeof(BTreeMappingRecord);
	if (mappingLevel)
	{
		mappingData = (void *) 0;
		_btreeReadMapping(backingStore, mappingRecord, 
			mappingLength, 0, &mappingData, &mappingLength);
		_NXAddHandler(&jumpHandler);
		if (_setjmp(jumpHandler.jumpState))
		{
			free(mappingData);
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}

		_btreeRemoveMapping(backingStore, (BTreeMappingRecord *) 
			mappingData, mappingLength, mappingLevel - 1);
		free(mappingData);
		_NXRemoveHandler(&jumpHandler);
	}

	btreeStore = backingStore->btreeStore;
	for (mappingIndex = 0; mappingIndex < mappingCount; ++mappingIndex)
	{
		if (mappingRecord[mappingIndex].regionOffset < 
				btreeStore->pageSize)
			break;
	}

	while (mappingIndex > 0)
	{
		--mappingIndex;
		destroyCount = mappingRecord[mappingIndex].regionLength + 
			btreeStore->pageSize - 1;
		backingStore->destroyPageGroup(btreeStore, 0, 
			mappingRecord[mappingIndex].regionOffset, 
			destroyCount / btreeStore->pageSize);
	}
}

- (void) replace: (void *) data length: (unsigned long) dataLength
{
	NXHandler		jumpHandler;
	boolean_t		traceValid;
	BTreeStore		*btreeStore;
	__BTree			*__btree;
	BTreeBackingStore	*backingStore;
	BTreeExternalRecord	*externalRecord;
	BTreeTraceRecord	*traceRecord;

	positionHint = 0L;
	traceValid = _btreeTraceSynchronized((__BTreeCursor *) self);
	if (! traceValid)
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	if (! traceRecord->traceStatus.exactMatch)
		_NXRaiseError(NX_BTreeKeyNotFound, "replace:length:", 0);

	__btree = (__BTree *) btree;
	backingStore = (BTreeBackingStore *) __btree->backingStore;
	btreeStore = backingStore->btreeStore;
	_btreeReadPage(backingStore, traceRecord);
	_btreeTouchPage(backingStore, traceRecord);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		_cursorStatus.traceNeeded = YES;
		backingStore->undo(btreeStore, 0);
		_btreeClearTrace((__BTreeCursor *) self);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	externalRecord = _btreeExternalRecord(traceRecord);
	if (externalRecord->recordHeader.indirection)
		_btreeRemoveMapping(backingStore, (BTreeMappingRecord *) 
			(externalRecord->recordContent + 
				externalRecord->recordHeader.keyLength), 
			externalRecord->recordLength, 
			externalRecord->recordHeader.indirection - 1);

	if (dataLength + keyLength > _btreeNodeSize(btreeStore) - 
			BTreeExternalNodeLength - BTreeExternalRecordOverhead)
	{
		_btreeRemoveExternal((__BTreeCursor *) self);
		_btreeInsertMapping((__BTreeCursor *) 
			self, data, dataLength, 1);
	} else
		_btreeUpdateExternal((__BTreeCursor *) 
			self, data, dataLength, 0);

	if (backingStore->bind)
		backingStore->bind(btreeStore, 0);

	if (_cursorStatus.performSave || btreeStore->storeStatus.performSave)
		backingStore->save(btreeStore, 0);

	_btreeClearTrace((__BTreeCursor *) self);
	_NXRemoveHandler(&jumpHandler);
}

- (void) remove
{
	NXHandler		jumpHandler;
	__BTree			*__btree;
	BTreeStore		*btreeStore;
	BTreeTraceRecord	*traceRecord;
	BTreeExternalRecord	*externalRecord;
	boolean_t		traceValid;

	 BTreeBackingStore	*volatile backingStore;

	positionHint = 0L;
	traceValid = _btreeTraceSynchronized((__BTreeCursor *) self);
	if (! traceValid)
		_btreeTraceCursor((__BTreeCursor *) self, btreeTraceKey);

	traceRecord = (BTreeTraceRecord *) _tracePrivate;
	traceRecord = traceRecord->superRecord;
	if (! traceRecord->traceStatus.exactMatch)
		_NXRaiseError(NX_BTreeKeyNotFound, "remove", 0);

	__btree = (__BTree *) btree;
	backingStore = (BTreeBackingStore *) __btree->backingStore;
	btreeStore = backingStore->btreeStore;
	_btreeReadPage(backingStore, traceRecord);
	_btreeTouchPage(backingStore, traceRecord);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		_cursorStatus.traceNeeded = YES;
		backingStore->undo(btreeStore, 0);
		_btreeClearTrace((__BTreeCursor *) self);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	externalRecord = _btreeExternalRecord(traceRecord);
	if (externalRecord->recordHeader.indirection)
		_btreeRemoveMapping(backingStore, 
			(BTreeMappingRecord *) 
			(externalRecord->recordContent + 
			externalRecord->recordHeader.keyLength), 
			externalRecord->recordLength, 
			externalRecord->recordHeader.indirection - 1);

	_btreeRemoveExternal((__BTreeCursor *) self);
	--btreeStore->recordCount;
	if (backingStore->bind)
		backingStore->bind(btreeStore, 0);

	if (_cursorStatus.performSave || btreeStore->storeStatus.performSave)
		backingStore->save(btreeStore, 0);

	_btreeClearTrace((__BTreeCursor *) self);
	_NXRemoveHandler(&jumpHandler);
}
 
- (void) append: (void *) data length: (unsigned long) dataLength
{
	[self notImplemented: _cmd];
}

- (void) insert: (void *) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset
{
	[self notImplemented: _cmd];
}

- (void) read: (void **) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset
{
	[self notImplemented: _cmd];
}

- (void) remove: (unsigned long) length at: (unsigned long) byteOffset
{
	[self notImplemented: _cmd];
}

- (void) replace: (void *) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset
{
	[self notImplemented: _cmd];
}


@end


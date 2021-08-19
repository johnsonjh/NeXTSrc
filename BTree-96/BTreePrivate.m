
#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreePrivate.h"

inline static BTreeTraceRecord 
*_btreeSplitExternal(__BTreeCursor *btreeCursor);

inline static BTreeTraceRecord 
*_btreeSplitInternal(__BTreeCursor *btreeCursor);

static BTreeTraceRecord *
_btreeRotateInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *sourceRecord, BTreeTraceRecord *targetRecord, 
	unsigned short insertLength, BTreeRotation rotation);

static void 
_btreeRemoveInternalRecord(__BTree *btree, 
	BTreeInternalNode *internalNode, short position);

static unsigned short 
_btreePackInternal(__BTree *btree, 
	BTreeInternalNode *internalNode, boolean_t monotonic);

static void 
_btreeInsertInternalEntry(__BTree *btree, BTreeInternalNode *internalNode, 
	short position, unsigned short recordLength);

static boolean_t 
_btreeInsertInternalRecord(__BTree *btree, 
	BTreeInternalNode *internalNode, BTreeKeyPromotion *keyPromotion);

static void 
_btreeInsertInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord, BTreeKeyPromotion *keyPromotion);

static void 
_btreeUpdateInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord, BTreeKeyPromotion *keyPromotion);

static void 
_btreeUpdateOffset(__BTree *btree, 
	BTreeTraceRecord *traceRecord, short position);

static BTreeKeyPromotion 
*_btreePromoteKey(__BTree *btree, BTreeKeyPromotion *keyPromotion, 
	BTreeTraceRecord *sourceRecord, BTreeTraceRecord *targetRecord);

static BTreeTraceRecord 
*_btreeLeftSibling(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord);

static BTreeTraceRecord 
*_btreeRotateExternal(__BTree *btree, BTreeTraceRecord *sourceRecord, 
	BTreeTraceRecord *targetRecord, BTreeExternalRecord *insertRecord, 
	BTreeRotation rotation);

inline static unsigned short 
_btreePackExternal(BTreeExternalNode *externalNode, boolean_t monotonic);

inline static void 
_btreeInsertExternalEntry(BTreeExternalNode *externalNode, 
	short position, unsigned short recordLength);

inline static boolean_t 
_btreeInsertExternalRecord(BTreeExternalNode *externalNode, short position, 
	BTreeExternalRecord *externalRecord, char *key, char *record);

inline static void 
_btreeRemoveExternalRecord(BTreeExternalNode *externalNode, short position);

static vm_offset_t 
_btreeForwardInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord);

static vm_offset_t 
_btreeReverseInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord);

#if	defined(VERIFY)

static void 
verifyExternalOrdering(__BTree *btree, BTreeExternalNode *externalNode)
{
	int			result;
	unsigned short		i;
	unsigned short		entryCount;
	unsigned short		*entryArray;
	BTreeExternalRecord	*thisRecord;
	BTreeExternalRecord	*lastRecord;

	entryCount = externalNode->entryCount;
	entryArray = externalNode->entryArray;
	for (i = 1; i < entryCount; ++i)
	{
		lastRecord = (BTreeExternalRecord *) 
			((char *) externalNode + entryArray[i - 1]);
		thisRecord = (BTreeExternalRecord *) 
			((char *) externalNode + entryArray[i]);
		result = btree->comparator(lastRecord->recordContent, 
			lastRecord->recordHeader.keyLength, 
			thisRecord->recordContent, 
			thisRecord->recordHeader.keyLength);
		if (result >= 0)
			_NXRaiseError(NX_BTreeInternalError, 
				"verifyExternalOrdering", 0);
	}
}

static void 
verifyInternalOrdering(__BTree *btree, BTreeInternalNode *internalNode)
{
	int			result;
	vm_size_t		pageSize;
	unsigned short		i;
	unsigned short		entryCount;
	vm_offset_t		*entryArray;
	BTreeInternalRecord	*thisRecord;
	BTreeInternalRecord	*lastRecord;

	entryCount = internalNode->entryCount;
	entryArray = internalNode->entryArray;
	pageSize = btree->pageSize;
	for (i = 1; i < entryCount; ++i)
	{
		lastRecord = (BTreeInternalRecord *) 
			((char *) internalNode + entryArray[i - 1] % pageSize);
		thisRecord = (BTreeInternalRecord *) 
			((char *) internalNode + entryArray[i] % pageSize);
		result = btree->comparator(lastRecord->recordContent, 
			lastRecord->keyLength, thisRecord->recordContent, 
			thisRecord->keyLength);
		if (result >= 0)
			_NXRaiseError(NX_BTreeInternalError, 
				"verifyInternalOrdering", 0);
	}
}

#else

#define	verifyExternalOrdering(btree, externalNode)
#define	verifyInternalOrdering(btree, internalNode)

#endif

void 
_btreeDecreaseTraceDepth(__BTreeCursor *btreeCursor, 
	unsigned short _traceDepth)
{
	BTreeTraceRecord	*rootRecord;
	BTreeTraceRecord	*traceRecord;

	rootRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	while (_traceDepth < btreeCursor->_traceDepth)
	{
#if	defined(DEBUG)
		if (! rootRecord)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeDecreaseTraceDepth", 0);
#endif
		if (--btreeCursor->_traceDepth)
		{
			traceRecord = rootRecord->childRecord;
			rootRecord->childRecord = traceRecord->childRecord;
			rootRecord->childRecord->superRecord = rootRecord;
		} else
		{
			traceRecord = rootRecord;
			btreeCursor->_tracePrivate = (void *) 0;
			rootRecord = (BTreeTraceRecord *) 0;
		}

		_btreeFreeTraceRecord(traceRecord);
	}
}

void 
_btreeIncreaseTraceDepth(__BTreeCursor *btreeCursor, 
	unsigned short _traceDepth)
{
	BTreeTraceRecord	*rootRecord;
	BTreeTraceRecord	*traceRecord;

	rootRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	while (_traceDepth > btreeCursor->_traceDepth)
	{
		traceRecord = _btreeNewTraceRecord();
		if (btreeCursor->_traceDepth++)
		{
#if	defined(DEBUG)
			if (! rootRecord)
				_NXRaiseError(NX_BTreeInternalError, 
					"_btreeIncreaseTraceDepth", 0);
#endif
			traceRecord->superRecord = rootRecord;
			traceRecord->childRecord = rootRecord->childRecord;
			rootRecord->childRecord = traceRecord;
			traceRecord->childRecord->superRecord = traceRecord;
		} else
		{
#if	defined(DEBUG)
			if (rootRecord)
				_NXRaiseError(NX_BTreeInternalError, 
					"_btreeIncreaseTraceDepth", 0);
#endif
			rootRecord = traceRecord;
			rootRecord->childRecord = rootRecord;
			rootRecord->superRecord = rootRecord;
			btreeCursor->_tracePrivate = (void *) rootRecord;
		}
	}
}

inline static BTreeTraceRecord 
*_btreeSplitExternal(__BTreeCursor *btreeCursor)
{
	__BTree			*btree;
	BTreeTraceRecord	*superRecord;
	BTreeTraceRecord	*childRecord;
	BTreeExternalNode	*childNode;
	BTreeExternalNode	*externalNode;
	BTreeInternalNode	*internalNode;

	btree = (__BTree *) btreeCursor->btree;
	_btreeIncreaseTraceDepth(btreeCursor, ++btree->btreeDepth);
	superRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	externalNode = superRecord->traceNode.externalNode;
	internalNode = superRecord->traceNode.internalNode;
	childRecord = superRecord->childRecord;
	_btreeCreatePage((BTreeBackingStore *) 
		btree->backingStore, childRecord);
	childNode = childRecord->traceNode.externalNode;
	bcopy((char *) externalNode, 
		(char *) childNode, externalNode->nodeSize);
	superRecord->btreePage->pageLevel = 1;
#if	defined	(DEBUG)
	bzero((char *) externalNode, externalNode->nodeSize);
#else
	bzero((char *) externalNode, sizeof(BTreeInternalNode));
#endif
	internalNode->recordLimit = childNode->nodeSize;
	internalNode->nodeSize = childNode->nodeSize;
	internalNode->nodeLength = 
		sizeof(vm_offset_t) + sizeof(BTreeInternalNode);
	internalNode->childOffset = childRecord->btreePage->pageOffset;
	_btreeCloseInternal(superRecord, -1);
	return childRecord;
}

inline static BTreeTraceRecord 
*_btreeSplitInternal(__BTreeCursor *btreeCursor)
{
	__BTree			*btree;
	BTreeTraceRecord	*superRecord;
	BTreeTraceRecord	*childRecord;
	BTreeInternalNode	*superNode;
	BTreeInternalNode	*childNode;

	btree = (__BTree *) btreeCursor->btree;
	_btreeIncreaseTraceDepth(btreeCursor, ++btree->btreeDepth);
	superRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	superNode = superRecord->traceNode.internalNode;
	childRecord = superRecord->childRecord;
	_btreeCreatePage((BTreeBackingStore *) 
		btree->backingStore, childRecord);
	childNode = childRecord->traceNode.internalNode;
	bcopy((char *) superNode, (char *) childNode, superNode->nodeSize);
#if	defined	(DEBUG)
	bzero((char *) superNode, superNode->nodeSize);
#else
	bzero((char *) superNode, sizeof(BTreeInternalNode));
#endif
	childRecord->btreePage->pageLevel = superRecord->btreePage->pageLevel;
	superRecord->btreePage->pageLevel++;
	superNode->recordLimit = childNode->nodeSize;
	superNode->nodeSize = childNode->nodeSize;
	superNode->nodeLength = 
		sizeof(vm_offset_t) + sizeof(BTreeInternalNode);
	superNode->childOffset = childRecord->btreePage->pageOffset;
	_btreeCloseInternal(superRecord, -1);
	return childRecord;
}

static BTreeTraceRecord 
*_btreeRotateInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *sourceRecord, BTreeTraceRecord *targetRecord, 
	unsigned short insertLength, BTreeRotation rotation)
{
	const char		*functionName = "_btreeRotateInternal";
	boolean_t		previousValid;
	enum			{
				leftDirection,
				rightDirection
	}			direction;
	enum			{
				shiftStrategy, 
				mergeStrategy
	}			strategy;
	vm_size_t		pageSize;
	vm_offset_t		recordEntry;
	unsigned short		sourceCount;
	unsigned short		sourceLimit;
	unsigned short		targetLimit;
	unsigned short		entryLength;
	unsigned short		entriesMoved;
	unsigned short		sourceLength;
	unsigned short		targetLength;
	unsigned short		parentLength;
	unsigned short		validEntriesMoved;
	unsigned short		validSourceLength;
	unsigned short		validTargetLength;
	unsigned short		pivotLength;
	short			pivotPosition;
	short			tracePosition;
	short			entryPosition;
	__BTree			*btree;
	BTreeTraceRecord	*parentRecord;
	BTreeTraceRecord	*resultRecord;
	BTreeInternalRecord	*pivotRecord;
	BTreeInternalRecord	*internalRecord;
	BTreeInternalNode	*sourceNode;
	BTreeInternalNode	*targetNode;
	BTreeInternalNode	*parentNode;
	BTreeKeyPromotion	keyPromotion;

	btree = (__BTree *) btreeCursor->btree;
	parentRecord = sourceRecord->superRecord;
	parentNode = parentRecord->traceNode.internalNode;
	sourceNode = sourceRecord->traceNode.internalNode;
	targetNode = targetRecord->traceNode.internalNode;
	strategy = shiftStrategy;
	if (insertLength)
	{
		insertLength += 
			sizeof(vm_offset_t) + sizeof(BTreeInternalRecord);
		tracePosition = sourceRecord->tracePosition;
		_btreeInsertInternalEntry(btree, sourceNode, tracePosition, 0);
		sourceNode->nodeStatus.polytonal = NO;
	} else
	{
		tracePosition = 0;
		if (rotation == mergeLeft || rotation == mergeRight)
		{
			sourceLimit = 0;
			targetLimit = targetNode->nodeSize;
			strategy = mergeStrategy;
		} else
		{
			sourceLimit = sourceNode->nodeSize >> 1;
			targetLimit = targetNode->nodeSize >> 1;
		}
	}

	sourceLength = sourceNode->nodeLength + insertLength;
	targetLength = targetNode->nodeLength;
	pivotPosition = parentRecord->tracePosition;
	if (rotation != shiftLeft && rotation != mergeLeft)
	{
		++pivotPosition;
		direction = rightDirection;
	} else
		direction = leftDirection;

	pageSize = btree->pageSize;
	if (rotation == splitRight)
		pivotLength = 0;
	else
	{
		recordEntry = parentNode->entryArray[pivotPosition] % pageSize;
		pivotRecord = (BTreeInternalRecord *) 
			((char *) parentNode + recordEntry);
		pivotLength = sizeof(BTreeInternalRecord) + 
			sizeof(vm_offset_t) + pivotRecord->keyLength;
	}

	parentLength = parentNode->nodeLength - pivotLength;
	previousValid = YES;
	sourceCount = sourceNode->entryCount;
	for (entriesMoved = 0; entriesMoved < sourceCount; ++entriesMoved)
	{
		if (strategy != mergeStrategy && 
			(targetLength > sourceLength && previousValid && 
			(insertLength ? (sourceLength <= sourceNode->nodeSize) 
				: (targetLength >= targetLimit))))
			break;

		if (direction == leftDirection)
			entryPosition = entriesMoved;
		else
			entryPosition = sourceCount - entriesMoved - 1;

		recordEntry = sourceNode->entryArray[entryPosition] % pageSize;
		if (recordEntry)
		{
			internalRecord = (BTreeInternalRecord *) 
				((char *) sourceNode + recordEntry);
			entryLength = sizeof(vm_offset_t) + 
				sizeof(BTreeInternalRecord);
			entryLength += internalRecord->keyLength;
		} else
			entryLength = insertLength;

		if (targetLength + pivotLength > targetNode->nodeSize)
			break;

		if (! insertLength && sourceLength - entryLength < sourceLimit)
			break;

		if (rotation == splitRight)
			previousValid = YES;
		else
		if (parentLength + entryLength <= parentNode->nodeSize)
			previousValid = YES;
		else
		if (previousValid)
		{
			previousValid = NO;
			validEntriesMoved = entriesMoved;
			validSourceLength = sourceLength;
			validTargetLength = targetLength;
		}

		targetLength += pivotLength;
		sourceLength -= (pivotLength = entryLength);
	}

	if (! previousValid)
	{
		entriesMoved = validEntriesMoved;
		sourceLength = validSourceLength;
		targetLength = validTargetLength;
	}

	if (! entriesMoved || ((insertLength) ? 
		(sourceLength > sourceNode->nodeSize) : 
		(strategy != mergeStrategy && targetLength < targetLimit)))
	{
		if (insertLength)
			_btreeRemoveInternalRecord(btree, 
				sourceNode, tracePosition);

		return (BTreeTraceRecord *) 0;
	}

	_btreeTouchPage((BTreeBackingStore *) 
		btree->backingStore, targetRecord);
	_btreeTouchPage((BTreeBackingStore *) 
		btree->backingStore, sourceRecord);
	_btreePackInternal(btree, targetNode, YES);
	_btreePackInternal(btree, sourceNode, YES);
	resultRecord = sourceRecord;
	if (direction == leftDirection)
	{
		keyPromotion.keyLength = pivotRecord->keyLength;
		keyPromotion.keyBuffer = pivotRecord->recordContent;
		keyPromotion.position = targetNode->entryCount;
		keyPromotion.pageOffset = sourceNode->childOffset;
		if (! _btreeInsertInternalRecord(btree, 
				targetNode, &keyPromotion))
			_NXRaiseError(NX_BTreeInternalError, functionName, 0);

		--entriesMoved;
		_btreeRemoveInternalRecord(btree, parentNode, pivotPosition);
		if (! insertLength)
			goto movePivotLeft;

		_btreeRemoveInternalRecord(btree, sourceNode, tracePosition);
		if (tracePosition == entriesMoved)
		{
			resultRecord = parentRecord;
			if (! entriesMoved)
				return resultRecord;

			recordEntry = sourceNode->entryArray
				[entriesMoved - 1] % pageSize;
		} else
		{
			if (tracePosition > entriesMoved)
				_btreeCloseInternal(sourceRecord, 
					tracePosition - entriesMoved - 1);
			else
			if (tracePosition < entriesMoved)
			{
				--entriesMoved;
				resultRecord = targetRecord;
				_btreeCloseInternal(targetRecord, 
					tracePosition + 
						targetNode->entryCount);
			}

			movePivotLeft: 
			recordEntry = sourceNode->entryArray
				[entriesMoved] % pageSize;
			internalRecord = (BTreeInternalRecord *) 
				((char *) sourceNode + recordEntry);
			keyPromotion.position = pivotPosition;
			keyPromotion.keyLength = internalRecord->keyLength;
			keyPromotion.keyBuffer = internalRecord->recordContent;
			keyPromotion.pageOffset = 
				sourceRecord->btreePage->pageOffset;
			if (! _btreeInsertInternalRecord(btree, 
					parentNode, &keyPromotion))
				_NXRaiseError(NX_BTreeInternalError, 
					functionName, 0);
	
			sourceNode->childOffset = 
				sourceNode->entryArray[entriesMoved];
			sourceNode->childOffset -= recordEntry;
			_btreeRemoveInternalRecord(btree, 
				sourceNode, entriesMoved);
		}

		if (entriesMoved)
		{
			targetLength = sourceNode->nodeSize - 
				targetNode->recordLimit;
			entryLength = sourceNode->entryArray
				[entriesMoved - 1] % pageSize;
			entryPosition = 0;
			for (; entryPosition < entriesMoved; ++entryPosition)
				sourceNode->entryArray[entryPosition] -= 
					targetLength;

			sourceLength = sourceNode->nodeSize - recordEntry;
			sourceCount = sourceNode->entryCount;
			for (; entryPosition < sourceCount; ++entryPosition)
				sourceNode->entryArray[entryPosition] += 
					sourceLength;

			insertLength = entriesMoved * sizeof(vm_offset_t);
			bcopy((char *) sourceNode->entryArray, 
				(char *) (targetNode->entryArray + 
					targetNode->entryCount), insertLength);
			targetNode->entryCount += entriesMoved;
			sourceNode->entryCount -= entriesMoved;
			bcopy((char *) (sourceNode->entryArray + entriesMoved), 
				(char *) sourceNode->entryArray, 
				sizeof(vm_offset_t) * sourceNode->entryCount);
			targetLength = sourceNode->nodeSize - entryLength;
			targetNode->recordLimit -= targetLength;
			bcopy((char *) sourceNode + entryLength, 
				(char *) targetNode + targetNode->recordLimit, 
					targetLength);
			bcopy((char *) sourceNode + sourceNode->recordLimit, 
				(char *) sourceNode + sourceNode->recordLimit + 
					sourceLength, 
					recordEntry - sourceNode->recordLimit);
			targetNode->nodeLength += targetLength + insertLength;
			sourceNode->nodeLength -= targetLength + insertLength;
			if (sourceNode->entryCount)
				sourceNode->recordLimit += sourceLength;
			else
				sourceNode->recordLimit = sourceNode->nodeSize;
		}
	} else
	{
		if (rotation != splitRight)
		{
			keyPromotion.keyLength = pivotRecord->keyLength;
			keyPromotion.keyBuffer = pivotRecord->recordContent;
			keyPromotion.position = 0;
			keyPromotion.pageOffset = targetNode->childOffset;
			if (! _btreeInsertInternalRecord(btree, 
					targetNode, &keyPromotion))
				_NXRaiseError(NX_BTreeInternalError, 
					functionName, 0);

			_btreeRemoveInternalRecord(btree, 
				parentNode, pivotPosition);
			targetNode->nodeStatus.polytonal = YES;
		}

		entryPosition = sourceCount - entriesMoved;
		if (! insertLength)
			goto movePivotRight;

		_btreeRemoveInternalRecord(btree, sourceNode, tracePosition);
		if (tracePosition == entryPosition)
		{
			--entriesMoved;
			resultRecord = parentRecord;
			_btreeCloseInternal(parentRecord, pivotPosition);
			entryLength = entryPosition ? 
				sourceNode->entryArray[entryPosition - 1] % 
					pageSize : sourceNode->nodeSize;
		} else
		{
			if (tracePosition < entryPosition)
				--entryPosition;
			else
			if (tracePosition > entryPosition)
			{
				--entriesMoved;
				resultRecord = targetRecord;
				_btreeCloseInternal(resultRecord, 
					tracePosition - entryPosition - 1);
			}

			movePivotRight: 
			entryLength = sourceNode->entryArray
				[entryPosition] % pageSize;
			internalRecord = (BTreeInternalRecord *) 
				((char *) sourceNode + entryLength);
			keyPromotion.position = pivotPosition;
			keyPromotion.keyLength = internalRecord->keyLength;
			keyPromotion.keyBuffer = internalRecord->recordContent;
			keyPromotion.pageOffset = 
				targetRecord->btreePage->pageOffset;
			if (rotation == splitRight)
				_btreeInsertInternal(btreeCursor, 
					parentRecord, &keyPromotion);
			else
			if (! _btreeInsertInternalRecord(btree, 
					parentNode, &keyPromotion))
				_NXRaiseError(NX_BTreeInternalError, 
					functionName, 0);
	
			--entriesMoved;
			targetNode->childOffset = 
				sourceNode->entryArray[entryPosition];
			targetNode->childOffset -= entryLength;
			_btreeRemoveInternalRecord(btree, 
				sourceNode, entryPosition);
		}

		if (entriesMoved)
		{
			recordEntry = entryPosition ? 
				sourceNode->entryArray[entryPosition - 1] % 
		   			pageSize : sourceNode->nodeSize;
			sourceLength = recordEntry - sourceNode->recordLimit;
			sourceCount = sourceNode->entryCount;
			tracePosition = entryLength - targetNode->recordLimit;
			for (; entryPosition < sourceCount; ++entryPosition)
				sourceNode->entryArray[entryPosition] -= 
					tracePosition;

			insertLength = entriesMoved * sizeof(vm_offset_t);
			bcopy((char *) targetNode->entryArray, (char *) 
				(targetNode->entryArray + entriesMoved), 
				sizeof(vm_offset_t) * targetNode->entryCount);
			bcopy((char *) (sourceNode->entryArray + 
				sourceCount - entriesMoved), (char *) 
				targetNode->entryArray, insertLength);
			targetNode->entryCount += entriesMoved;
			sourceNode->entryCount -= entriesMoved;
			targetLength = entryLength - sourceNode->recordLimit;
			targetNode->recordLimit -= targetLength;
			bcopy((char *) sourceNode + sourceNode->recordLimit, 
				(char *) targetNode + targetNode->recordLimit, 
					targetLength);
			sourceNode->recordLimit += sourceLength;
			targetNode->nodeLength += targetLength + insertLength;
			sourceNode->nodeLength -= targetLength + insertLength;
		}
	}

	return resultRecord;
}

static void 
_btreeRemoveInternalRecord(__BTree *btree, 
	BTreeInternalNode *internalNode, short position)
{
	vm_offset_t		recordEntry;
	unsigned short		recordLength;
	char			*copyTarget;
	BTreeInternalRecord	*internalRecord;

#if	defined(DEBUG)
	if (position < 0 || position >= internalNode->entryCount)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeRemoveInternalRecord", 0);
#endif

	recordEntry = internalNode->entryArray[position] % btree->pageSize;
	if (recordEntry)
	{
		internalRecord = (BTreeInternalRecord *) 
			((char *) internalNode + recordEntry);
		recordLength = internalRecord->keyLength + 
			sizeof(BTreeInternalRecord);
		internalNode->nodeLength -= recordLength + sizeof(vm_offset_t);
		if (recordEntry != internalNode->recordLimit)
			internalNode->nodeStatus.packable = YES;
		else
			internalNode->recordLimit += recordLength;
	}

	--internalNode->entryCount;
	copyTarget = (char *) (internalNode->entryArray + position);
	bcopy(copyTarget + sizeof(vm_offset_t), copyTarget, 
		sizeof(vm_offset_t) * (internalNode->entryCount - position));
}

static unsigned short 
_btreePackInternal(__BTree *btree, BTreeInternalNode *internalNode, 
	boolean_t monotonic)
{
	vm_size_t		pageSize;
	vm_offset_t		*entryArray;
	vm_offset_t		*entryLimit;
	unsigned short		recordLimit;
	unsigned short		recordLength;
	vm_offset_t		recordEntry;
	char			*copyNode;
	BTreeInternalRecord	*internalRecord;

	if (! internalNode->nodeStatus.packable && 
		(! monotonic || ! internalNode->nodeStatus.polytonal))
		return 0;

	copyNode = alloca((int) internalNode->nodeSize);
	internalNode->nodeStatus.packable = NO;
	internalNode->nodeStatus.polytonal = NO;
	pageSize = btree->pageSize;
	recordLimit = internalNode->nodeSize;
	entryArray = internalNode->entryArray;
	entryLimit = internalNode->entryCount + entryArray;
	for (; entryArray < entryLimit; ++entryArray)
	{
		recordEntry = *entryArray % pageSize;
		internalRecord = (BTreeInternalRecord *) 
			((char *) internalNode + recordEntry);
		recordLength = internalRecord->keyLength + 
			sizeof(BTreeInternalRecord);
		recordLimit -= recordLength;
		*entryArray += recordLimit - recordEntry;
		bcopy((char *) internalRecord, 
			copyNode + recordLimit, recordLength);
	}

#if	defined(DEBUG)
	bzero((char *) internalNode + internalNode->recordLimit, 
		recordLimit - internalNode->recordLimit);
#endif

	bcopy(copyNode + recordLimit, (char *) internalNode + recordLimit, 
		internalNode->nodeSize - recordLimit);
	recordEntry = internalNode->recordLimit;
	internalNode->recordLimit = recordLimit;
	return recordLimit - recordEntry;
}

static void 
_btreeInsertInternalEntry(__BTree *btree, BTreeInternalNode *internalNode, 
	short position, unsigned short recordLength)
{
	char			*copyTarget;
	unsigned short		spaceNeeded;

	spaceNeeded = sizeof(BTreeInternalNode) + sizeof(vm_offset_t);
	spaceNeeded += internalNode->entryCount * sizeof(vm_offset_t);
	if (spaceNeeded > internalNode->recordLimit - recordLength)
	{
		_btreePackInternal(btree, internalNode, NO);

#if	defined(DEBUG)
		if (spaceNeeded > internalNode->recordLimit - recordLength)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeInsertInternalEntry", 0);
#endif

	}

	if (position < internalNode->entryCount)
	{
		internalNode->nodeStatus.polytonal = YES;
		copyTarget = (char *) (internalNode->entryArray + position);
		bcopy(copyTarget, copyTarget + sizeof(vm_offset_t), 
			sizeof(vm_offset_t) * 
				(internalNode->entryCount - position));
	}

	++internalNode->entryCount;
	internalNode->entryArray[position] = (vm_offset_t) 0;
}

static boolean_t 
_btreeInsertInternalRecord(__BTree *btree, BTreeInternalNode *internalNode, 
	BTreeKeyPromotion *keyPromotion)
{
	char			*copyTarget;
	unsigned short		recordLength;
	unsigned short		spaceNeeded;
	BTreeInternalRecord	internalRecord;

#if	defined(DEBUG)
	if (keyPromotion->position < 0 || 
			keyPromotion->position > internalNode->entryCount)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeInsertInternalRecord", 0);
#endif

	recordLength = sizeof(BTreeInternalRecord) + keyPromotion->keyLength;
	spaceNeeded = internalNode->nodeLength;
	spaceNeeded += recordLength + sizeof(vm_offset_t);
	if (spaceNeeded > internalNode->nodeSize)
		return NO;

	_btreeInsertInternalEntry(btree, internalNode, 
		keyPromotion->position, recordLength);
	internalNode->recordLimit -= recordLength;
	internalNode->entryArray[keyPromotion->position] = 
		keyPromotion->pageOffset + internalNode->recordLimit;
	copyTarget = (char *) internalNode + internalNode->recordLimit;
	internalRecord.keyLength = keyPromotion->keyLength;
	bcopy((char *) &internalRecord, 
		copyTarget, sizeof(BTreeInternalRecord));
	bcopy(keyPromotion->keyBuffer, copyTarget + 
		sizeof(BTreeInternalRecord), internalRecord.keyLength);
	internalNode->nodeLength += recordLength + sizeof(vm_offset_t);
	return YES;
}

static void 
_btreeInsertInternal(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord, 
	BTreeKeyPromotion *keyPromotion)
{
	short			tracePosition;
	__BTree			*btree;
	BTreeInternalNode	*traceNode;
	BTreeInternalNode	*splitNode;
	BTreeTraceRecord	*superRecord;
	BTreeTraceRecord	*leftRecord;
	BTreeTraceRecord	*rightRecord;
	BTreeTraceRecord	*extraRecord;
	BTreeTraceRecord	*splitRecord;

	traceNode = traceRecord->traceNode.internalNode;
	btree = (__BTree *) btreeCursor->btree;
	if (_btreeInsertInternalRecord(btree, traceNode, keyPromotion))
		goto finished;

	_btreePackInternal(btree, traceNode, YES);
	if (traceRecord == (BTreeTraceRecord *) btreeCursor->_tracePrivate)
	{
		superRecord = traceRecord;
		traceRecord = _btreeSplitInternal(btreeCursor);
	} else
	{
		superRecord = traceRecord->superRecord;
		_btreeReadPage((BTreeBackingStore *) 
			btree->backingStore, superRecord);
		_btreeTouchPage((BTreeBackingStore *) 
			btree->backingStore, superRecord);
	}

	_btreeCloseInternal(traceRecord, keyPromotion->position);
	tracePosition = superRecord->tracePosition;
	rightRecord = (BTreeTraceRecord *) 0;
	splitRecord = (BTreeTraceRecord *) 0;
	if (traceRecord->traceStatus.shadowed)
		_btreeUpdateOffset(btree, traceRecord, tracePosition);

	if ((leftRecord = _btreeLeftSibling(btreeCursor, traceRecord)) && 
		(extraRecord = _btreeRotateInternal(btreeCursor, traceRecord, 
		leftRecord, keyPromotion->keyLength, shiftLeft)))
	{
		splitRecord = traceRecord;
		traceRecord = leftRecord;
		if (traceRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, 
				traceRecord, tracePosition - 1);
	} else
	if ((rightRecord = _btreeRightSibling(btreeCursor, traceRecord)) && 
		(extraRecord = _btreeRotateInternal(btreeCursor, traceRecord, 
		rightRecord, keyPromotion->keyLength, shiftRight)))
	{
		splitRecord = rightRecord;
		if (splitRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, 
				splitRecord, tracePosition + 1);
	} else
	{
		splitRecord = _btreeNewTraceRecord();
		_btreeCreatePage((BTreeBackingStore *) btree->backingStore, 
			splitRecord);
		splitRecord->superRecord = traceRecord->superRecord;
		splitRecord->childRecord = traceRecord->childRecord;
		splitRecord->siblingRecord = traceRecord->siblingRecord;
		traceRecord->siblingRecord = splitRecord;
		splitNode = splitRecord->traceNode.internalNode;
		traceNode = traceRecord->traceNode.internalNode;
		splitNode->nodeSize = traceNode->nodeSize;
		splitNode->recordLimit = traceNode->nodeSize;
		splitNode->nodeLength = 
			sizeof(vm_offset_t) + sizeof(BTreeInternalNode);
		splitRecord->btreePage->pageLevel = 
			traceRecord->btreePage->pageLevel;
		extraRecord = _btreeRotateInternal(btreeCursor, traceRecord, 
			splitRecord, keyPromotion->keyLength, splitRight);
#if	defined(DEBUG)
		if (! extraRecord)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeInsertInternal", 0);
#endif
	}

	if (extraRecord == superRecord)
	{
		splitNode = splitRecord->traceNode.internalNode;
		splitNode->childOffset = keyPromotion->pageOffset;
		keyPromotion->pageOffset = splitRecord->btreePage->pageOffset;
	}

	keyPromotion->position = extraRecord->tracePosition;
	_btreeInsertInternal(btreeCursor, extraRecord, keyPromotion);
	if (! btreeCursor->_cursorStatus.traceNeeded && 
		! btreeCursor->_cursorStatus.lastPosition)
	{
		if (leftRecord && leftRecord != traceRecord)
		{
			_btreeCloseInternal(superRecord, tracePosition - 1);
			_btreeRotateInternal(btreeCursor, 
				leftRecord, traceRecord, 0, shiftRight);
			if (leftRecord->traceStatus.shadowed)
				_btreeUpdateOffset(btree, 
					traceRecord, tracePosition - 1);
		}
	
		if (rightRecord && rightRecord != splitRecord)
		{
			_btreeCloseInternal(superRecord, tracePosition + 2);
			_btreeRotateInternal(btreeCursor, 
				rightRecord, splitRecord, 0, shiftLeft);
			if (rightRecord->traceStatus.shadowed)
				_btreeUpdateOffset(btree, 
					traceRecord, tracePosition + 2);
		}
	}

	btreeCursor->_cursorStatus.traceNeeded = YES;
	verifyInternalOrdering(btree, splitRecord->traceNode.internalNode);
	verifyInternalOrdering(btree, superRecord->traceNode.internalNode);
	finished:;
	verifyInternalOrdering(btree, traceRecord->traceNode.internalNode);
}

/*
** this needs to be more sophisticated about building the new internal
** record when offsets and prefix compression are introduced. At the moment, 
** it just slams the new key and key length into the record.
*/
static void 
_btreeUpdateInternal(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord, 
	BTreeKeyPromotion *keyPromotion)
{
	__BTree			*btree;
	BTreeInternalNode	*internalNode;
	BTreeInternalRecord	*internalRecord;

	internalNode = traceRecord->traceNode.internalNode;
	btree = (__BTree *) btreeCursor->btree;
	internalRecord = (BTreeInternalRecord *) 
		((char *) internalNode + 
		(internalNode->entryArray[keyPromotion->position] % 
			btree->pageSize));
	if (keyPromotion->keyLength > internalRecord->keyLength)
	{
		_btreeRemoveInternalRecord(btree, 
			internalNode, keyPromotion->position);
		_btreeInsertInternal(btreeCursor, traceRecord, keyPromotion);
	} else
	{
		if (keyPromotion->keyLength < internalRecord->keyLength)
		{
			internalNode->nodeStatus.packable = YES;
			internalNode->nodeLength -= internalRecord->keyLength - 
				keyPromotion->keyLength;
			internalRecord->keyLength = keyPromotion->keyLength;
		}

		bcopy(keyPromotion->keyBuffer, internalRecord->recordContent, 
			internalRecord->keyLength);
		verifyInternalOrdering(btree, internalNode);
	}
}

static void 
_btreeUpdateOffset(__BTree *btree, 
	BTreeTraceRecord *traceRecord, short position)
{
	BTreeInternalNode	*internalNode;
	BTreeTraceRecord	*superRecord;

	superRecord = traceRecord->superRecord;
	if (position < 0)
		superRecord->traceNode.internalNode->childOffset = 
			traceRecord->btreePage->pageOffset;
	else
	{
		internalNode = superRecord->traceNode.internalNode;
		internalNode->entryArray[position] %= btree->pageSize;
		internalNode->entryArray[position] += 
			traceRecord->btreePage->pageOffset;
	}
}

inline static unsigned short 
_btreePromoteString(BTreeExternalRecord *sourceRecord, 
	BTreeExternalRecord *targetRecord)
{
	unsigned short		i;
	unsigned short		compareLength;

	compareLength = targetRecord->recordHeader.keyLength;
	if (compareLength > sourceRecord->recordHeader.keyLength)
		compareLength = sourceRecord->recordHeader.keyLength;

	for (i = 0; i < compareLength; ++i)
		if (targetRecord->recordContent[i] != 
				sourceRecord->recordContent[i])
			break;

	return i + 1;
}

inline static unsigned short 
_btreePromoteMonocaseString(BTreeExternalRecord *sourceRecord, 
	BTreeExternalRecord *targetRecord)
{
	unsigned short		i;
	unsigned short		compareLength;

	compareLength = targetRecord->recordHeader.keyLength;
	if (compareLength > sourceRecord->recordHeader.keyLength)
		compareLength = sourceRecord->recordHeader.keyLength;

	for (i = 0; i < compareLength; ++i)
		if (monocase(targetRecord->recordContent[i]) != 
				monocase(sourceRecord->recordContent[i]))
			break;

	return i + 1;
}

static BTreeKeyPromotion 
*_btreePromoteKey(__BTree *btree, BTreeKeyPromotion *keyPromotion, 
	BTreeTraceRecord *leftRecord, BTreeTraceRecord *rightRecord)
{

#if	defined(DEBUG)
	int			comparison;
#endif

	BTreeExternalNode	*sourceNode;
	BTreeExternalNode	*targetNode;
	BTreeExternalRecord	*sourceRecord;
	BTreeExternalRecord	*targetRecord;

	sourceNode = leftRecord->traceNode.externalNode;
	targetNode = rightRecord->traceNode.externalNode;
	sourceRecord = (BTreeExternalRecord *) ((char *) sourceNode + 
		sourceNode->entryArray[sourceNode->entryCount - 1]);
	if (targetNode->entryCount)
	{
		targetRecord = (BTreeExternalRecord *) 
			((char *) targetNode + targetNode->entryArray[0]);
#if		defined(DEBUG)
		comparison = btree->comparator(sourceRecord->recordContent, 
			sourceRecord->recordHeader.keyLength, 
			targetRecord->recordContent, 
			targetRecord->recordHeader.keyLength);
		if (comparison >= 0)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreePromoteKey", 0);
#endif
		keyPromotion->keyBuffer = targetRecord->recordContent;
		if (btree->comparator == NXBTreeCompareStrings)
			keyPromotion->keyLength = 
			_btreePromoteString(sourceRecord, targetRecord);
		else
		if (btree->comparator == NXBTreeCompareMonocaseStrings)
			keyPromotion->keyLength = 
				_btreePromoteMonocaseString(sourceRecord, 
				targetRecord);
		else
			keyPromotion->keyLength = 
				targetRecord->recordHeader.keyLength;
	} else
	{
		keyPromotion->keyLength = 1 + 
			sourceRecord->recordHeader.keyLength;
		keyPromotion->keyBuffer = sourceRecord->recordContent;
	}

	keyPromotion->pageOffset = rightRecord->btreePage->pageOffset;
	return keyPromotion;
}

static BTreeTraceRecord 
*_btreeLeftSibling(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord)
{
	vm_offset_t		pageOffset;
	short			tracePosition;
	__BTree			*btree;
	BTreeTraceRecord	*superRecord;
	BTreeTraceRecord	*leftRecord;

	if (traceRecord == (BTreeTraceRecord *) btreeCursor->_tracePrivate)
		return (BTreeTraceRecord *) 0;

	superRecord = traceRecord->superRecord;
	if (superRecord->tracePosition < 0)
		return (BTreeTraceRecord *) 0;

	tracePosition = superRecord->tracePosition - 1;
	btree = (__BTree *) btreeCursor->btree;
	if (tracePosition < 0)
		pageOffset = superRecord->traceNode.internalNode->childOffset;
	else
	{
		pageOffset = superRecord->traceNode.
			internalNode->entryArray[tracePosition];
		pageOffset -= pageOffset % btree->pageSize;
	}

	leftRecord = _btreeNewTraceRecord();
	_btreeOpenPage((BTreeBackingStore *) 
		btree->backingStore, leftRecord, pageOffset);
	leftRecord->superRecord = traceRecord->superRecord;
	leftRecord->childRecord = traceRecord->childRecord;
	leftRecord->siblingRecord = traceRecord->siblingRecord;
	traceRecord->siblingRecord = leftRecord;
	return leftRecord;
}

BTreeTraceRecord 
*_btreeRightSibling(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord)
{
	vm_offset_t		pageOffset;
	short			tracePosition;
	__BTree			*btree;
	BTreeTraceRecord	*superRecord;
	BTreeTraceRecord	*rightRecord;

	if (traceRecord == (BTreeTraceRecord *) btreeCursor->_tracePrivate)
		return (BTreeTraceRecord *) 0;

	superRecord = traceRecord->superRecord;
	if (superRecord->traceStatus.lastPosition)
		return (BTreeTraceRecord *) 0;

	tracePosition = superRecord->tracePosition + 1;

#if	defined(DEBUG)
	if (tracePosition >= superRecord->traceNode.internalNode->entryCount)
		_NXRaiseError(NX_BTreeInternalError, "_btreeRightSibling", 0);
#endif

	pageOffset = superRecord->traceNode.
		internalNode->entryArray[tracePosition];
	btree = (__BTree *) btreeCursor->btree;
	pageOffset -= pageOffset % btree->pageSize;
	rightRecord = _btreeNewTraceRecord();
	_btreeOpenPage((BTreeBackingStore *) 
		btree->backingStore, rightRecord, pageOffset);
	rightRecord->superRecord = traceRecord->superRecord;
	rightRecord->childRecord = traceRecord->childRecord;
	rightRecord->siblingRecord = traceRecord->siblingRecord;
	traceRecord->siblingRecord = rightRecord;
	return rightRecord;
}

static BTreeTraceRecord 
*_btreeRotateExternal(__BTree *btree, BTreeTraceRecord *sourceRecord, 
	BTreeTraceRecord *targetRecord, BTreeExternalRecord *insertRecord, 
	BTreeRotation rotation)
{
	enum				{
					leftDirection,
					rightDirection
	}				direction;
	enum				{
					shiftStrategy, 
					mergeStrategy
	}				strategy;
	unsigned short			entryLength;
	unsigned short			entriesMoved;
	unsigned short			entryCount;
	unsigned short			sourceLength;
	unsigned short			targetLength;
	unsigned short			sourceLimit;
	unsigned short			targetLimit;
	unsigned short			insertLength;
	short				recordEntry;
	short				tracePosition;
	short				entryPosition;
	BTreeExternalNode		*sourceNode;
	BTreeExternalNode		*targetNode;
	BTreeExternalRecord		*externalRecord;
	BTreeTraceRecord		*resultRecord;

	sourceNode = sourceRecord->traceNode.externalNode;
	targetNode = targetRecord->traceNode.externalNode;
	if (targetNode->nodeStatus.spanned)
		return (BTreeTraceRecord *) 0;

	strategy = shiftStrategy;
	if (insertRecord)
	{
		insertLength = insertRecord->recordHeader.keyLength;
		if (insertLength)
		{
			insertLength += insertRecord->recordLength;
			insertLength += sizeof(BTreeExternalRecord) + 
				sizeof(unsigned short);
		}

		tracePosition = sourceRecord->tracePosition;
		_btreeInsertExternalEntry(sourceNode, tracePosition, 0);
		sourceNode->nodeStatus.polytonal = NO;
	} else
	{
		insertLength = 0;
		if (rotation == mergeLeft || rotation == mergeRight)
		{
			sourceLimit = 0;
			targetLimit = targetNode->nodeSize;
			strategy = mergeStrategy;
		} else
		{
			sourceLimit = sourceNode->nodeSize >> 1;
			targetLimit = targetNode->nodeSize >> 1;
		}
	}

	if (rotation == mergeLeft || rotation == shiftLeft)
		direction = leftDirection;
	else
		direction = rightDirection;

	entryCount = sourceNode->entryCount;
	sourceLength = sourceNode->nodeLength + insertLength;
	targetLength = targetNode->nodeLength;
	for (entriesMoved = 0; entriesMoved < entryCount; ++entriesMoved)
	{
		entryPosition = entryCount - entriesMoved - 1;
		if (strategy != mergeStrategy && 
			((! entryPosition && insertRecord && ! insertLength) || 
			(targetLength > sourceLength && 
			(insertRecord ? (sourceLength <= sourceNode->nodeSize) 
				: (targetLength >= targetLimit)))))
			break;

		if (direction == leftDirection)
			entryPosition = entriesMoved;

		recordEntry = sourceNode->entryArray[entryPosition];
		if (recordEntry)
		{
			externalRecord = (BTreeExternalRecord *) 
				((char *) sourceNode + recordEntry);
			entryLength = sizeof(unsigned short) + 
				sizeof(BTreeExternalRecord);
			entryLength += externalRecord->recordHeader.keyLength + 
				externalRecord->recordLength;
		} else
			entryLength = insertLength;

		if (targetLength + entryLength > targetNode->nodeSize)
			break;

		if (! insertRecord && sourceLength - entryLength < sourceLimit)
			break;

		targetLength += entryLength, sourceLength -= entryLength;
	}

	if (! entriesMoved || ((insertRecord) ? 
		(sourceLength > sourceNode->nodeSize) : 
		(strategy != mergeStrategy && targetLength < targetLimit)))
	{
		if (insertRecord)
			_btreeRemoveExternalRecord(sourceNode, tracePosition);

		return (BTreeTraceRecord *) 0;
	}

	resultRecord = sourceRecord;
	_btreeTouchPage((BTreeBackingStore *) 
		btree->backingStore, targetRecord);
	_btreeTouchPage((BTreeBackingStore *) 
		btree->backingStore, sourceRecord);
	_btreePackExternal(targetNode, YES);
	_btreePackExternal(sourceNode, YES);
	if (direction == leftDirection)
	{
		if (insertRecord)
		{
			_btreeRemoveExternalRecord(sourceNode, tracePosition);
			if (tracePosition < entriesMoved)
			{
				--entriesMoved;
				resultRecord = targetRecord;
				_btreeCloseExternal(resultRecord, 
					tracePosition + 
					targetNode->entryCount);
			} else
				_btreeCloseExternal(resultRecord, 
					tracePosition - entriesMoved);
		}

		if (entriesMoved)
		{
			recordEntry = sourceNode->entryArray[entriesMoved - 1];
			sourceLength = sourceNode->nodeSize - 
				targetNode->recordLimit;
			entryPosition = 0;
			for (; entryPosition < entriesMoved; ++entryPosition)
				sourceNode->entryArray[entryPosition] -= 
					sourceLength;

			entryCount = sourceNode->entryCount;
			sourceLength = sourceNode->nodeSize - recordEntry;
			for (; entryPosition < entryCount; ++entryPosition)
				sourceNode->entryArray[entryPosition] += 
					sourceLength;

			insertLength = entriesMoved * sizeof(unsigned short);
			bcopy((char *) sourceNode->entryArray, 
				(char *) (targetNode->entryArray + 
					targetNode->entryCount), insertLength);
			targetNode->entryCount += entriesMoved;
			sourceNode->entryCount -= entriesMoved;
			bcopy((char *) (sourceNode->entryArray + entriesMoved), 
				(char *) sourceNode->entryArray, 
				sizeof(unsigned short) * 
					sourceNode->entryCount);
			targetNode->recordLimit -= sourceLength;
			bcopy((char *) sourceNode + recordEntry, 
				(char *) targetNode + targetNode->recordLimit, 
					sourceLength);
			targetNode->nodeLength += sourceLength + insertLength;
			entryCount = sourceNode->recordLimit + sourceLength;
			bcopy((char *) sourceNode + sourceNode->recordLimit, 
				(char *) sourceNode + entryCount, 
					recordEntry - sourceNode->recordLimit);
			sourceNode->recordLimit += sourceLength;
			sourceNode->nodeLength -= sourceLength + insertLength;
		}
	} else
	{
		targetNode->nodeStatus.polytonal = YES;
		entryPosition = entryCount - entriesMoved;
		if (insertRecord)
		{
			_btreeRemoveExternalRecord(sourceNode, tracePosition);
			if (tracePosition < entryPosition)
				--entryPosition;
			else
			{
				--entriesMoved;
				resultRecord = targetRecord;
				_btreeCloseExternal(resultRecord, 
					tracePosition - entryPosition);
			}
		}

		if (entriesMoved)
		{
			recordEntry = (entryPosition < 1) ? 
				sourceNode->nodeSize : 
				sourceNode->entryArray[entryPosition - 1];
			sourceLength = recordEntry - sourceNode->recordLimit;
			targetLength = recordEntry - targetNode->recordLimit;
			entryCount = sourceNode->entryCount;
			for (; entryPosition < entryCount; ++entryPosition)
				sourceNode->entryArray[entryPosition] -= 
					targetLength;

			insertLength = entriesMoved * sizeof(unsigned short);
			bcopy((char *) targetNode->entryArray, (char *) 
				(targetNode->entryArray + entriesMoved), 
				sizeof(unsigned short) * 
				targetNode->entryCount);
			bcopy((char *) (sourceNode->entryArray + 
				entryCount - entriesMoved), (char *) 
				targetNode->entryArray, insertLength);
			targetNode->entryCount += entriesMoved;
			sourceNode->entryCount -= entriesMoved;
			targetNode->recordLimit -= sourceLength;
			bcopy((char *) sourceNode + sourceNode->recordLimit, 
				(char *) targetNode + targetNode->recordLimit, 
					sourceLength);
			sourceNode->recordLimit += sourceLength;
			targetNode->nodeLength += sourceLength + insertLength;
			sourceNode->nodeLength -= sourceLength + insertLength;
		}
	}

	return resultRecord;
}

inline static unsigned short 
_btreePackExternal(BTreeExternalNode *externalNode, boolean_t monotonic)
{
	unsigned short		recordLimit;
	unsigned short		recordLength;
	short			recordEntry;
	unsigned short		*entryArray;
	unsigned short		*entryLimit;
	char			*copyNode;
	BTreeExternalRecord	*externalRecord;

	if (! externalNode->nodeStatus.packable && 
		(! monotonic || ! externalNode->nodeStatus.polytonal))
		return 0;

	copyNode = alloca((int) externalNode->nodeSize);
	recordLimit = externalNode->nodeSize;
	externalNode->nodeStatus.polytonal = NO;
	externalNode->nodeStatus.packable = NO;
	entryArray = externalNode->entryArray;
	entryLimit = externalNode->entryCount + entryArray;
	for (; entryArray < entryLimit; ++entryArray)
	{
		recordEntry = *entryArray;
		externalRecord = (BTreeExternalRecord *) 
			((char *) externalNode + recordEntry);
		recordLength = externalRecord->recordHeader.keyLength + 
			externalRecord->recordLength + 
				sizeof(BTreeExternalRecord);
		recordLimit -= recordLength;
		*entryArray = recordLimit;
		bcopy((char *) externalRecord, 
			copyNode + recordLimit, recordLength);
	}

#if	defined(DEBUG)
	bzero((char *) externalNode + externalNode->recordLimit, 
		recordLimit - externalNode->recordLimit);
#endif

	bcopy(copyNode + recordLimit, (char *) externalNode + recordLimit, 
		externalNode->nodeSize - recordLimit);
	recordEntry = externalNode->recordLimit;
	externalNode->recordLimit = recordLimit;
	return recordLimit - recordEntry;
}

inline static void 
_btreeInsertExternalEntry(BTreeExternalNode *externalNode, 
	short position, unsigned short recordLength)
{
	char			*copyTarget;
	unsigned short		spaceNeeded;

	spaceNeeded = sizeof(BTreeExternalNode) + sizeof(unsigned short);
	spaceNeeded += externalNode->entryCount * sizeof(unsigned short);
	if (spaceNeeded > externalNode->recordLimit - recordLength)
	{
		_btreePackExternal(externalNode, NO);

#if	defined(DEBUG)
		if (spaceNeeded > externalNode->recordLimit - recordLength)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeInsertExternalEntry", 0);
#endif

	}

	if (position < externalNode->entryCount)
	{
		externalNode->nodeStatus.polytonal = YES;
		copyTarget = (char *) (externalNode->entryArray + position);
		bcopy(copyTarget, copyTarget + sizeof(unsigned short), 
			sizeof(unsigned short) * 
				(externalNode->entryCount - position));
	}

	++externalNode->entryCount;
	externalNode->entryArray[position] = 0;
}

inline static boolean_t 
_btreeInsertExternalRecord(BTreeExternalNode *externalNode, short position, 
	BTreeExternalRecord *externalRecord, char *key, char *record)
{
	char			*copyTarget;
	unsigned short		recordLength;
	unsigned short		spaceNeeded;

#if	defined(DEBUG)
	if (position < 0 || position > externalNode->entryCount)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeInsertExternalRecord", 0);
#endif

	recordLength = sizeof(BTreeExternalRecord) + 
		externalRecord->recordHeader.keyLength + 
		externalRecord->recordLength;
	spaceNeeded = externalNode->nodeLength + 
		recordLength + sizeof(unsigned short);
	if (spaceNeeded > externalNode->nodeSize)
		return NO;

	_btreeInsertExternalEntry(externalNode, position, recordLength);
	externalNode->recordLimit -= recordLength;
	externalNode->entryArray[position] = externalNode->recordLimit;
	copyTarget = (char *) externalNode + externalNode->recordLimit;
	bcopy((char *) externalRecord, 
		copyTarget, sizeof(BTreeExternalRecord));
	copyTarget += sizeof(BTreeExternalRecord);
	bcopy(key, copyTarget, externalRecord->recordHeader.keyLength);
	bcopy(record, copyTarget + externalRecord->recordHeader.keyLength, 
		externalRecord->recordLength);
	externalNode->nodeLength += recordLength + sizeof(unsigned short);
	return YES;
}

__BTreeCursor 
*_btreeInsertExternal(__BTreeCursor *btreeCursor, 
	void *record, unsigned long recordLength, unsigned short indirection)
{
	static BTreeExternalRecord	externalRecord;
	BTreeKeyPromotion		promotion;
	short				tracePosition;
	short				extraPosition;
	unsigned short			extraLength;
	__BTree				*btree;
	BTreeExternalNode		*traceNode;
	BTreeExternalNode		*splitNode;
	BTreeExternalNode		*extraNode;
	BTreeTraceRecord		*superRecord;
	BTreeTraceRecord		*traceRecord;
	BTreeTraceRecord		*leftRecord;
	BTreeTraceRecord		*rightRecord;
	BTreeTraceRecord		*splitRecord;
	BTreeTraceRecord		*extraRecord;

	btree = (__BTree *) btreeCursor->btree;
	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	tracePosition = traceRecord->tracePosition;
	externalRecord.recordHeader.indirection = indirection;
	externalRecord.recordHeader.keyLength = btreeCursor->keyLength;
	externalRecord.recordLength = recordLength;
	traceNode = traceRecord->traceNode.externalNode;
	if (_btreeInsertExternalRecord(traceNode, tracePosition, 
		&externalRecord, btreeCursor->keyBuffer, record))
	{
		traceRecord->traceStatus.exactMatch = YES;
		traceRecord->traceStatus.lastPosition = NO;
		goto finished;
	}

	_btreePackExternal(traceNode, YES);
	if (traceRecord == (BTreeTraceRecord *) btreeCursor->_tracePrivate)
	{
		superRecord = traceRecord;
		traceRecord = _btreeSplitExternal(btreeCursor);
		_btreeCloseExternal(traceRecord, tracePosition);
	} else
	{
		superRecord = traceRecord->superRecord;
		_btreeReadPage((BTreeBackingStore *) 
			btree->backingStore, superRecord);
		_btreeTouchPage((BTreeBackingStore *) 
			btree->backingStore, superRecord);
	}

	tracePosition = superRecord->tracePosition;
	if (traceRecord->traceStatus.shadowed)
		_btreeUpdateOffset(btree, traceRecord, tracePosition);

	leftRecord = (BTreeTraceRecord *) 0;
	rightRecord = (BTreeTraceRecord *) 0;
	splitRecord = (BTreeTraceRecord *) 0;
	if (btreeCursor->_cursorStatus.leftRotation && 
		(leftRecord = _btreeLeftSibling(btreeCursor, traceRecord)) && 
		(extraRecord = _btreeRotateExternal(btree, traceRecord, 
			leftRecord, &externalRecord, shiftLeft)))
	{
		--tracePosition;
		splitRecord = traceRecord;
		traceRecord = leftRecord;
		if (traceRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, traceRecord, tracePosition);

		goto finishRotation;
	} else
	if (btreeCursor->_cursorStatus.rightRotation && 
		(rightRecord = _btreeRightSibling(btreeCursor, traceRecord)) && 
		(extraRecord = _btreeRotateExternal(btree, traceRecord, 
			rightRecord, &externalRecord, shiftRight)))
	{
		splitRecord = rightRecord;
		if (rightRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, 
				rightRecord, tracePosition + 1);

		finishRotation: 
		traceNode = extraRecord->traceNode.externalNode;
		extraPosition = extraRecord->tracePosition;
		_btreeInsertExternalRecord(traceNode, extraPosition, 
			&externalRecord, btreeCursor->keyBuffer, record);
		promotion.position = tracePosition + 1;
		_btreePromoteKey(btree, &promotion, traceRecord, splitRecord);
		_btreeUpdateInternal(btreeCursor, superRecord, &promotion);
	} else
	{
		splitRecord = _btreeNewTraceRecord();
		_btreeCreatePage((BTreeBackingStore *) 
			btree->backingStore, splitRecord);
		splitRecord->superRecord = traceRecord->superRecord;
		splitRecord->childRecord = traceRecord->childRecord;
		splitRecord->siblingRecord = traceRecord->siblingRecord;
		traceRecord->siblingRecord = splitRecord;
		splitNode = splitRecord->traceNode.externalNode;
		traceNode = traceRecord->traceNode.externalNode;
		splitNode->nodeSize = traceNode->nodeSize;
		splitNode->recordLimit = traceNode->nodeSize;
		splitNode->nodeLength = 
			sizeof(unsigned short) + sizeof(BTreeExternalNode);
		if (! btreeCursor->_cursorStatus.lastPosition)
		{
			extraRecord = _btreeRotateExternal(btree, 
				traceRecord, splitRecord, &externalRecord, 
				splitRight);
			splitNode->nodeStatus.polytonal = NO;
		} else
		if (! btreeCursor->_cursorStatus.spanning || 
			((extraLength = sizeof(BTreeExternalRecord) + 
				sizeof(unsigned short) + 
				externalRecord.recordHeader.keyLength + 
				traceNode->nodeLength) > traceNode->nodeSize))
		{
			extraRecord = splitRecord;
			_btreeCloseExternal(extraRecord, 0);
		} else
		{
			traceNode->nodeStatus.spanned = YES;
			extraLength = traceNode->nodeSize - extraLength;
			recordLength -= extraLength;
			splitNode->nodeLength += recordLength;
			splitNode->recordLimit -= recordLength;
			splitNode->spanLength = recordLength;
			splitNode->spanOffset = splitNode->recordLimit;
			bcopy((char *) record + extraLength, 
				(char *) splitNode + splitNode->spanOffset, 
					recordLength);
			externalRecord.recordLength = extraLength;
			extraRecord = traceRecord;
		}

		if (extraRecord)
		{
			extraNode = extraRecord->traceNode.externalNode;
			_btreeInsertExternalRecord(extraNode, 
				extraRecord->tracePosition, &externalRecord, 
				btreeCursor->keyBuffer, record);
		} else
		{
			extraNode = (BTreeExternalNode *) 0;
			externalRecord.recordHeader.keyLength = 0;
			extraRecord = _btreeRotateExternal(btree, traceRecord, 
				splitRecord, &externalRecord, shiftRight);
			splitNode->nodeStatus.polytonal = NO;
		}

		promotion.position = tracePosition + 1;
		_btreePromoteKey(btree, &promotion, traceRecord, splitRecord);
		_btreeInsertInternal(btreeCursor, superRecord, &promotion);
		if (! extraNode)
		{
			if (extraRecord == splitRecord && 
				! btreeCursor->_cursorStatus.traceNeeded && 
				leftRecord && _btreeRotateExternal(btree, 
				leftRecord, traceRecord, 0, shiftRight))
			{
				if (leftRecord->traceStatus.shadowed)
					_btreeUpdateOffset(btree, traceRecord, 
						tracePosition - 1);
		
				promotion.position = tracePosition;
				_btreePromoteKey(btree, 
					&promotion, leftRecord, traceRecord);
				_btreeUpdateInternal(btreeCursor, 
					superRecord, &promotion);
			}
	
			if (extraRecord == traceRecord && 
				! btreeCursor->_cursorStatus.traceNeeded && 
				rightRecord && _btreeRotateExternal(btree, 
				rightRecord, splitRecord, 0, shiftLeft))
			{
				tracePosition += 2;
				if (rightRecord->traceStatus.shadowed)
					_btreeUpdateOffset(btree, 
						traceRecord, tracePosition);
		
				promotion.position = tracePosition;
				_btreePromoteKey(btree, 
					&promotion, splitRecord, rightRecord);
				_btreeUpdateInternal(btreeCursor, 
					superRecord, &promotion);
			}
	
			_btreeTraceCursor(btreeCursor, btreeTraceKey);
			superRecord = (BTreeTraceRecord *) 
				btreeCursor->_tracePrivate;
			_btreeTouchPage((BTreeBackingStore *) 
				btree->backingStore, superRecord->superRecord);
			return _btreeInsertExternal(btreeCursor, 
				record, recordLength, indirection);
		}
	}

	if (leftRecord && leftRecord != traceRecord && 
		! btreeCursor->_cursorStatus.lastPosition && 
		! btreeCursor->_cursorStatus.traceNeeded && 
		_btreeRotateExternal(btree, 
			leftRecord, traceRecord, 0, shiftRight))
	{
		if (leftRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, 
				traceRecord, tracePosition - 1);

		promotion.position = tracePosition;
		_btreePromoteKey(btree, &promotion, leftRecord, traceRecord);
		_btreeUpdateInternal(btreeCursor, superRecord, &promotion);
	}

	if (rightRecord && rightRecord != splitRecord && 
		! btreeCursor->_cursorStatus.traceNeeded && 
		_btreeRotateExternal(btree, 
			rightRecord, splitRecord, 0, shiftLeft))
	{
		if (rightRecord->traceStatus.shadowed)
			_btreeUpdateOffset(btree, 
				traceRecord, tracePosition + 2);

		promotion.position = tracePosition + 2;
		_btreePromoteKey(btree, &promotion, splitRecord, rightRecord);	
		_btreeUpdateInternal(btreeCursor, superRecord, &promotion);
	}

	if (! btreeCursor->_cursorStatus.traceNeeded)
	{
		if (leftRecord != traceRecord)
			++tracePosition;
		else
			traceRecord = splitRecord;
	
		traceRecord->traceStatus.exactMatch = YES;
		traceRecord->traceStatus.lastPosition = NO;
		if (extraRecord != traceRecord)
		{
			_btreeCloseInternal(superRecord, tracePosition);
			extraRecord->superRecord = traceRecord->superRecord;
			extraRecord->childRecord = traceRecord->childRecord;
			extraRecord->superRecord->childRecord = extraRecord;
			extraRecord->childRecord->superRecord = extraRecord;
		}
	}

	verifyExternalOrdering(btree, splitRecord->traceNode.externalNode);
	verifyInternalOrdering(btree, superRecord->traceNode.internalNode);
	finished:
	btreeCursor->_cursorStatus.lastPosition = NO;
	verifyExternalOrdering(btree, traceRecord->traceNode.externalNode);
	return btreeCursor;
}

__BTreeCursor 
*_btreeUpdateExternal(__BTreeCursor *btreeCursor, 
	char *record, unsigned long recordLength, unsigned short indirection)
{
	unsigned short		insertLength;
	unsigned short		currentLength;
	short			tracePosition;
	BTreeExternalNode	*externalNode;
	BTreeExternalRecord	*externalRecord;
	BTreeTraceRecord	*traceRecord;

	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	externalNode = traceRecord->traceNode.externalNode;
	tracePosition = traceRecord->tracePosition;
	externalRecord = (BTreeExternalRecord *) ((char *) 
		externalNode + externalNode->entryArray[tracePosition]);
	currentLength = externalRecord->recordHeader.keyLength + 
		externalRecord->recordLength;
	insertLength = btreeCursor->keyLength + recordLength;
	if (insertLength > currentLength)
	{
		_btreeRemoveExternalRecord(externalNode, tracePosition);
		_btreeInsertExternal(btreeCursor, 
			record, recordLength, indirection);
	} else
	{
		if (insertLength < currentLength)
		{
			externalNode->nodeStatus.packable = YES;
			externalNode->nodeLength -= 
				currentLength - insertLength;
		}

		externalRecord->recordLength = recordLength;
		externalRecord->recordHeader.keyLength = 
			btreeCursor->keyLength;
		externalRecord->recordHeader.indirection = indirection;
		bcopy(btreeCursor->keyBuffer, 
			externalRecord->recordContent, btreeCursor->keyLength);
		bcopy(record, externalRecord->recordContent + 
			btreeCursor->keyLength, recordLength);
		verifyExternalOrdering((__BTree *) 
			btreeCursor->btree, externalNode);
	}

	return btreeCursor;
}

inline static void 
_btreeRemoveExternalRecord(BTreeExternalNode *externalNode, short position)
{
	unsigned short		recordLength;
	short			recordEntry;
	char			*copyTarget;
	BTreeExternalRecord	*externalRecord;

#if	defined(DEBUG)
	if (position < 0 || position >= externalNode->entryCount)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeRemoveExternalRecord", 0);
#endif

	recordEntry = externalNode->entryArray[position];
	if (recordEntry)
	{
		externalRecord = (BTreeExternalRecord *) 
			((char *) externalNode + recordEntry);
		recordLength = externalRecord->recordHeader.keyLength +  
			externalRecord->recordLength + 
				sizeof(BTreeExternalRecord);
		externalNode->nodeLength -= 
			recordLength + sizeof(unsigned short);
		if (recordEntry != externalNode->recordLimit)
			externalNode->nodeStatus.packable = YES;
		else
			externalNode->recordLimit += recordLength;
	}

	--externalNode->entryCount;
	copyTarget = (char *) (externalNode->entryArray + position);
	bcopy(copyTarget + sizeof(unsigned short), copyTarget, 
		sizeof(unsigned short) * 
			(externalNode->entryCount - position));
}

__BTreeCursor 
*_btreeRemoveExternal(__BTreeCursor *btreeCursor)
{
	BTreeTraceRecord	*traceRecord;

	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	_btreeRemoveExternalRecord(traceRecord->traceNode.externalNode, 
		traceRecord->tracePosition);
	verifyExternalOrdering((__BTree *) 
		btreeCursor->btree, traceRecord->traceNode.externalNode);
#if	defined(NOTDEF)
	if (traceRecord->traceNode.externalNode->entryCount)
		traceRecord->traceStatus.exactMatch = NO;
	else
	if (traceRecord != (BTreeTraceRecord *) btreeCursor->_tracePrivate)
	{
		_btreeDestroyPage(btreeCursor->btree, traceRecord);
		btreeCursor->_cursorStatus.traceNeeded = YES;
	}
#else
	traceRecord->traceStatus.exactMatch = NO;
	if (traceRecord->tracePosition >= 
		traceRecord->traceNode.externalNode->entryCount)
		traceRecord->traceStatus.lastPosition = YES;
#endif
	return btreeCursor;
}

void 
_btreeTraceCursor(__BTreeCursor *btreeCursor, BTreeTraceType traceType)
{
	vm_offset_t		*entryArray;
	vm_offset_t		pageOffset;
	unsigned short		currentLevel;
	boolean_t		lastPosition;
	__BTree			*btree;
	BTreePage		*btreePage;
	BTreeBackingStore	*backingStore;
	BTreeInternalNode	*internalNode;
	BTreeTraceRecord	*traceRecord;

	lastPosition = YES;
	btreeCursor->_cursorStatus.traceNeeded = YES;
	btreeCursor->_cursorStatus.firstPosition = NO;
	btreeCursor->_cursorStatus.lastPosition = NO;
	btree = (__BTree *) btreeCursor->btree;
	currentLevel = btree->btreeDepth;
	if (btreeCursor->_traceDepth > currentLevel)
		_btreeDecreaseTraceDepth(btreeCursor, currentLevel);
	else
	if (btreeCursor->_traceDepth < currentLevel)
		_btreeIncreaseTraceDepth(btreeCursor, currentLevel);

	backingStore = (BTreeBackingStore *) btree->backingStore;
	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	btreePage = traceRecord->btreePage;
	if (! btreePage || ! btreePage->pageBuffer)
		_btreeOpenPage(backingStore, traceRecord, 
			backingStore->btreeStore->rootOffset);

	for (--currentLevel; currentLevel; --currentLevel)
	{
		_btreeSearchInternal(btree, traceRecord, 
			btreeCursor->keyBuffer, 
			btreeCursor->keyLength, traceType);
		internalNode = traceRecord->traceNode.internalNode;
		if (traceRecord->tracePosition < 0)
			pageOffset = internalNode->childOffset;
		else
		{
			entryArray = internalNode->entryArray;
			pageOffset = entryArray[traceRecord->tracePosition];
			pageOffset -= pageOffset % btree->pageSize;
		}

		lastPosition = lastPosition && 
			traceRecord->traceStatus.lastPosition;
		traceRecord = traceRecord->childRecord;
		btreePage = traceRecord->btreePage;
		if (! btreePage || btreePage->pageOffset != pageOffset || 
				! btreePage->pageBuffer)
			_btreeOpenPage(backingStore, traceRecord, pageOffset);
	}

	_btreeSearchExternal(btree, traceRecord, 
		btreeCursor->keyBuffer, btreeCursor->keyLength, traceType);
	btreeCursor->_cursorStatus.lastPosition = 
		lastPosition && traceRecord->traceStatus.lastPosition;
	btreeCursor->_cursorStatus.traceNeeded = NO;
}

static vm_offset_t 
_btreeForwardInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord)
{
	vm_offset_t		pageOffset;
	short			tracePosition;
	__BTree			*btree;
	BTreeInternalNode	*internalNode;

	btree = (__BTree *) btreeCursor->btree;
	if (! traceRecord->traceStatus.lastPosition)
	{
		_btreeReadPage((BTreeBackingStore *) 
			btree->backingStore, traceRecord);
		internalNode = traceRecord->traceNode.internalNode;
		tracePosition = traceRecord->tracePosition + 1;
		if (tracePosition < internalNode->entryCount)
		{
			_btreeCloseInternal(traceRecord, tracePosition);
			pageOffset = internalNode->entryArray[tracePosition];
			return pageOffset - pageOffset % btree->pageSize;
		}
	
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeForwardInternal", 0);
	}

	if (traceRecord == (BTreeTraceRecord *) btreeCursor->_tracePrivate)
		return (vm_offset_t) 0;

	if (! (pageOffset = 
		_btreeForwardInternal(btreeCursor, traceRecord->superRecord)))
		return (vm_offset_t) 0;

	_btreeOpenPage((BTreeBackingStore *) btree->backingStore, 
		traceRecord, pageOffset);
	_btreeCloseInternal(traceRecord, -1);
	return traceRecord->traceNode.internalNode->childOffset;
}

void 
_btreeForwardExternal(__BTreeCursor *btreeCursor)
{
	vm_offset_t		pageOffset;
	short			tracePosition;
	__BTree			*btree;
	BTreeTraceRecord	*traceRecord;
	BTreeTraceRecord	*rootRecord;

	if (btreeCursor->_cursorStatus.lastPosition)
		return;

	rootRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = rootRecord->superRecord;
	traceRecord->traceStatus.exactMatch = YES;
	btree = (__BTree *) btreeCursor->btree;
	_btreeReadPage((BTreeBackingStore *) 
		btree->backingStore, traceRecord);
	tracePosition = traceRecord->tracePosition + 1;
	while (tracePosition >= 
		traceRecord->traceNode.externalNode->entryCount)
	{
		if (traceRecord != rootRecord && 
			(pageOffset = _btreeForwardInternal(btreeCursor, 
				traceRecord->superRecord)))
		{
			tracePosition = 0;
			_btreeOpenPage((BTreeBackingStore *) 
				btree->backingStore, traceRecord, pageOffset);
		} else
		{
			traceRecord->traceStatus.exactMatch = NO;
			btreeCursor->_cursorStatus.lastPosition = YES;
			break;
		}
	}

	_btreeCloseExternal(traceRecord, tracePosition);
	btreeCursor->_cursorStatus.firstPosition = NO;
}

static vm_offset_t 
_btreeReverseInternal(__BTreeCursor *btreeCursor, 
	BTreeTraceRecord *traceRecord)
{
	vm_offset_t		pageOffset;
	__BTree			*btree;
	short			tracePosition;
	BTreeInternalNode	*internalNode;

	btree = (__BTree *) btreeCursor->btree;
	if (traceRecord->tracePosition < 0)
	{
		if (traceRecord == (BTreeTraceRecord *) 
				btreeCursor->_tracePrivate)
			return (vm_offset_t) 0;

		pageOffset = _btreeReverseInternal(btreeCursor, 
				traceRecord->superRecord);
		if (! pageOffset)
			return (vm_offset_t) 0;

		_btreeOpenPage((BTreeBackingStore *) 
			btree->backingStore, traceRecord, pageOffset);
		internalNode = traceRecord->traceNode.internalNode;
		tracePosition = internalNode->entryCount - 1;
	} else
	{
		_btreeReadPage((BTreeBackingStore *) 
			btree->backingStore, traceRecord);
		internalNode = traceRecord->traceNode.internalNode;
		tracePosition = traceRecord->tracePosition - 1;
	}

	if (tracePosition < 0)
	{
		_btreeCloseInternal(traceRecord, -1);
		return internalNode->childOffset;
	}

	_btreeCloseInternal(traceRecord, tracePosition);
	pageOffset = internalNode->entryArray[tracePosition];
	return pageOffset - pageOffset % btree->pageSize;
}

void 
_btreeReverseExternal(__BTreeCursor *btreeCursor)
{
	vm_offset_t		pageOffset;
	short			tracePosition;
	__BTree			*btree;
	BTreeTraceRecord	*traceRecord;
	BTreeTraceRecord	*rootRecord;
	BTreeExternalNode	*externalNode;

	if (btreeCursor->_cursorStatus.firstPosition)
		return;

	btree = (__BTree *) btreeCursor->btree;
	rootRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = rootRecord->superRecord;
	traceRecord->traceStatus.exactMatch = YES;
	tracePosition = traceRecord->tracePosition - 1;
	while (tracePosition < 0)
	{
		if (traceRecord != rootRecord && 
			(pageOffset = _btreeReverseInternal(btreeCursor, 
				traceRecord->superRecord)))
		{
			_btreeOpenPage((BTreeBackingStore *) 
				btree->backingStore, traceRecord, pageOffset);
			externalNode = traceRecord->traceNode.externalNode;
			tracePosition = externalNode->entryCount - 1;
		} else
		{
			tracePosition = 0;
			btreeCursor->_cursorStatus.firstPosition = YES;
			break;
		}
	}

	_btreeCloseExternal(traceRecord, tracePosition);
	btreeCursor->_cursorStatus.lastPosition = NO;
}


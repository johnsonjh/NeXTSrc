

#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreePrivate.h"

boolean_t 
_btreeCheckHint(__BTreeCursor *btreeCursor)
{
	short			lowerBound;
	short			upperBound;
	short			midpoint;
	int			comparison;
	vm_offset_t		pageOffset;
	BTreePage		*btreePage;
	__BTree			*btree;
	BTreeTraceRecord	*traceRecord;
	BTreeExternalRecord	*externalRecord;
	BTreeExternalNode	*externalNode;
	BTreeBackingStore	*backingStore;

	btree = (__BTree *) btreeCursor->btree;
	if (btreeCursor->_traceDepth != btree->btreeDepth)
		return NO;

	btreeCursor->_cursorStatus.lastPosition = NO;
	btreeCursor->_cursorStatus.firstPosition = NO;
	traceRecord = (BTreeTraceRecord *) btreeCursor->_tracePrivate;
	traceRecord = traceRecord->superRecord;
	btreePage = traceRecord->btreePage;
	midpoint = btreeCursor->positionHint % btree->pageSize;
	pageOffset = btreeCursor->positionHint - midpoint;
	if (! btreePage || btreePage->pageOffset != pageOffset || 
		! btreePage->pageBuffer)
	{
		btreeCursor->_cursorStatus.traceNeeded = YES;
		backingStore = (BTreeBackingStore *) btree->backingStore;
		_btreeOpenPage(backingStore, traceRecord, pageOffset);
	}

	externalNode = traceRecord->traceNode.externalNode;
	upperBound = externalNode->entryCount;
	if (midpoint >= 0 && midpoint < upperBound)
		for (lowerBound = 0; lowerBound < upperBound; 
			midpoint = (lowerBound + upperBound) >> 1)
		{
			externalRecord = (BTreeExternalRecord *) ((char *) 
				externalNode + 
				externalNode->entryArray[midpoint]);
			comparison = 
			btree->comparator(externalRecord->recordContent, 
				externalRecord->recordHeader.keyLength, 
				btreeCursor->keyBuffer, 
				btreeCursor->keyLength);		
			if (comparison > 0)
				upperBound = midpoint;
			else
			if (comparison < 0)
				lowerBound = midpoint + 1;
			else
			{
				traceRecord->traceStatus.exactMatch = YES;
				_btreeCloseExternal(traceRecord, midpoint);
				return YES;
			}
		}

	return NO;
}

void 
_btreeSearchInternal(__BTree *btree, BTreeTraceRecord *traceRecord, 
	void *key, unsigned short keyLength, BTreeTraceType traceType)
{
	vm_size_t		pageSize;
	short			lowerBound;
	short			upperBound;
	short			midpoint;
	int			comparison;
	vm_offset_t		*entryArray;
	NXBTreeComparator	*comparator;
	BTreeInternalRecord	*internalRecord;
	BTreeInternalNode	*internalNode;

	lowerBound = -1;
	if (traceType == btreeTraceTop)
	{
		_btreeCloseInternal(traceRecord, lowerBound);
		return;
	}

	internalNode = traceRecord->traceNode.internalNode;
	upperBound = internalNode->entryCount - 1;
	if (traceType == btreeTraceEnd)
	{
		_btreeCloseInternal(traceRecord, upperBound);
		return;
	}

	pageSize = btree->pageSize;
	comparator = btree->comparator;
	entryArray = internalNode->entryArray;
	while (lowerBound < upperBound)
	{
		midpoint = (1 + lowerBound + upperBound) >> 1;
		internalRecord = (BTreeInternalRecord *) ((char *) 
			internalNode + entryArray[midpoint] % pageSize);
		comparison = comparator(internalRecord->recordContent, 
			internalRecord->keyLength, key, keyLength);		
		if (comparison < 0)
			lowerBound = midpoint;
		else
		if (comparison > 0)
			upperBound = midpoint - 1;
		else
		{
			lowerBound = midpoint;
			break;
		}
	}

	_btreeCloseInternal(traceRecord, lowerBound);
}

void
_btreeSearchExternal(__BTree *btree, BTreeTraceRecord *traceRecord, 
	void *key, unsigned short keyLength, BTreeTraceType traceType)
{
	short			lowerBound;
	short			upperBound;
	short			midpoint;
	int			comparison;
	unsigned short		*entryArray;
	NXBTreeComparator	*comparator;
	BTreeExternalRecord	*externalRecord;
	BTreeExternalNode	*externalNode;

	lowerBound = 0;
	traceRecord->traceStatus.exactMatch = NO;
	if (traceType == btreeTraceTop)
	{
		_btreeCloseExternal(traceRecord, lowerBound);
		return;
	}

	externalNode = traceRecord->traceNode.externalNode;
	upperBound = externalNode->entryCount;
	if (traceType == btreeTraceEnd)
	{
		_btreeCloseExternal(traceRecord, upperBound);
		return;
	}

	entryArray = externalNode->entryArray;
	comparator = btree->comparator;
	while (lowerBound < upperBound)
	{
		midpoint = (lowerBound + upperBound) >> 1;
		externalRecord = (BTreeExternalRecord *) 
			((char *) externalNode + entryArray[midpoint]);
		comparison = 
			comparator(externalRecord->recordContent, 
			externalRecord->recordHeader.keyLength, key, 
			keyLength);		
		if (comparison > 0)
			upperBound = midpoint;
		else
		if (comparison < 0)
			lowerBound = midpoint + 1;
		else
		{
			lowerBound = midpoint;
			traceRecord->traceStatus.exactMatch = YES;
			break;
		}
	}
	
	_btreeCloseExternal(traceRecord, lowerBound);
}


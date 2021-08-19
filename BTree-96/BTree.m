
#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreePrivate.h"

static int _NXBTreeVersion = 1;

typedef struct _btreeWalkRecord	{
	__BTree			*btree;
	BTreeTraceRecord	*traceRecord;
	BTreeInternalRecord	*parentRecord;
	void			*userPointer;
} _btreeWalkRecord;

typedef boolean_t 
_btreeWalkHandler(_btreeWalkRecord *walkRecord);

void 
_btreePostOrderWalk(__BTree *btree, BTreeTraceRecord *traceRecord, 
	_btreeWalkHandler *walkHandler, BTreeInternalRecord *parentRecord, 
		void *userPointer)
{
	_btreeWalkRecord	walkRecord;
	vm_offset_t		pageOffset;
	unsigned short		pageLevel;
	unsigned short		recordEntry;
	unsigned short		position;
	BTreeTraceRecord	childRecord;
	BTreeInternalRecord	*internalRecord;
	BTreeBackingStore	*backingStore;
	BTreeInternalNode	*internalNode;

	backingStore = (BTreeBackingStore *) btree->backingStore;
	if ((pageLevel = traceRecord->btreePage->pageLevel) < 1)
		_NXRaiseError(NX_BTreeInternalError, "_btreePostOrderWalk", 0);

	internalNode = traceRecord->traceNode.internalNode;
	internalRecord = parentRecord;
	pageOffset = internalNode->childOffset;
	bzero((char *) &childRecord, sizeof(BTreeTraceRecord));
	walkRecord.btree = btree;
	walkRecord.userPointer = userPointer;
	for (position = 0;; ++position)
	{
		_btreeOpenPage(backingStore, &childRecord, pageOffset);
		if (pageLevel > 1)
			_btreePostOrderWalk(btree, &childRecord, 
				walkHandler, internalRecord, userPointer);

		walkRecord.traceRecord = &childRecord;
		walkRecord.parentRecord = internalRecord;
		if (! walkHandler(&walkRecord))
			break;

		if (position >= internalNode->entryCount)
			break;

		pageOffset = internalNode->entryArray[position];
		recordEntry = pageOffset % btree->pageSize;
		pageOffset -= recordEntry;
		internalRecord = (BTreeInternalRecord *) 
			((char *) internalNode + recordEntry);
	}
}

@implementation BTree

+ initialize
{
	[self setVersion: _NXBTreeVersion];
	return self;
}

+ _newWith: (void *) private;
{
	NXHandler		errorHandler;
	BTreePageHeader		*pageHeader;
	BTreePage		*btreePage;
	BTreeExternalNode	*externalNode;
	BTreeBackingStore	*_backingStore;
	BTreeStore		*btreeStore;

	self = [super new];
	_NXAddHandler(&errorHandler);
	if (_setjmp(errorHandler.jumpState))
	{
		[self free];
		_NXRaiseError(errorHandler.code, 
			errorHandler.data1, errorHandler.data2);
	}

	btreeStore = (BTreeStore *) private;
	pageSize = btreeStore->pageSize;
	comparator = NXBTreeCompareStrings;
	_backingStore = (BTreeBackingStore *) 
		malloc(sizeof(BTreeBackingStore));
	if (! _backingStore)
		_NXRaiseError(NX_BTreeNoMemory, "_newWith:", 0);

	backingStore = _backingStore;
	btreePage = [btreeStore openPageAt: btreeStore->rootOffset];
	btreeDepth = btreePage->pageLevel + 1;
	_codeVersion = [[self class] version];
	if (btreeStore->codeVersion > _codeVersion)
		_NXRaiseError(NX_BTreeInvalidVersion, "_newWith:", 0);

	btreeStore->codeVersion = _codeVersion;
	_backingStore->openPageAt = (BTreePage *(*)()) 
		[btreeStore methodFor: @selector(openPageAt:)];
	_backingStore->createPage = (BTreePage *(*)()) 
		[btreeStore methodFor: @selector(createPage)];
	_backingStore->readPage = (void (*)()) 
		[btreeStore methodFor: @selector(readPage:)];
	_backingStore->destroyPage = (void (*)()) 
		[btreeStore methodFor: @selector(destroyPage:)];
	_backingStore->destroyPageAt = (void (*)()) 
		[btreeStore methodFor: @selector(destroyPageAt:)];
	_backingStore->touchPage = (void (*)()) 
		[btreeStore methodFor: @selector(touchPage:)];

	_backingStore->createPageGroup = (void (*)()) 
		[btreeStore methodFor: @selector(createPageGroup:count:)];
	_backingStore->readPageGroup = (void (*)()) 
		[btreeStore methodFor: @selector(readPageGroup:count:into:)];
	_backingStore->writePageGroup = (void (*)()) 
		[btreeStore methodFor: @selector(writePageGroup:count:from:)];
	_backingStore->destroyPageGroup = (void (*)()) 
		[btreeStore methodFor: @selector(destroyPageGroup:count:)];

	_backingStore->save = (void (*)()) 
		[btreeStore methodFor: @selector(save)];
	_backingStore->undo = (void (*)()) 
		[btreeStore methodFor: @selector(undo)];
	_backingStore->bind = (void (*)()) 
		[btreeStore methodFor: @selector(bind)];
	if (_backingStore->bind == (void (*)()) 
			[BTreeStore instanceMethodFor: @selector(bind)])
		_backingStore->bind = (void (*)()) 0;

	if (! btreeStore->recordCount && btreeStore->storeStatus.writeable)
	{
		_backingStore->touchPage(btreeStore, (SEL) 0, btreePage);
		pageHeader = (BTreePageHeader *) 
			(btreePage->pageBuffer + btreeStore->headerSize);
		externalNode = (BTreeExternalNode *) pageHeader->contents;
		externalNode->nodeLength = 
			sizeof(unsigned short) + sizeof(BTreeExternalNode);
		externalNode->nodeSize = btreeStore->pageSize - 
			btreeStore->headerSize - sizeof(BTreePageHeader);
		externalNode->recordLimit = externalNode->nodeSize;
		if (_backingStore->bind)
			_backingStore->bind(btreeStore, (SEL) 0);
	}

	_NXRemoveHandler(&errorHandler);
	_backingStore->btreeStore = btreeStore;
	return self;
}

+ new
{
	return [self _newWith: [BTreeStoreMemory new]];
}

- free
{
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	[_backingStore->btreeStore free];
	free(backingStore);
	return [super free];
}

- cursor
{
	return [BTreeCursor _newWith: self];
}

- (void) setComparator: (NXBTreeComparator *) aComparator
{
	comparator = aComparator;
}

- (NXBTreeComparator *) comparator
{
	return comparator;
}

static boolean_t 
_btreeDestroyPageWalkHandler(_btreeWalkRecord *walkRecord)
{
	unsigned short		position;
	BTreeBackingStore	*backingStore;
	BTreeExternalRecord	*externalRecord;
	BTreeTraceRecord	*traceRecord;
	BTreeExternalNode	*externalNode;

	traceRecord = walkRecord->traceRecord;
	externalNode = traceRecord->traceNode.externalNode;
	backingStore = (BTreeBackingStore *) walkRecord->btree->backingStore;
	for (position = 0; position < externalNode->entryCount; ++position)
	{
		externalRecord = (BTreeExternalRecord *) 
			((char *) externalNode + 
				externalNode->entryArray[position]);
		if (externalRecord->recordHeader.indirection)
			_btreeRemoveMapping(backingStore, 
				(BTreeMappingRecord *) 
				(externalRecord->recordContent + 	
				externalRecord->recordHeader.keyLength), 
				externalRecord->recordLength, 
				externalRecord->recordHeader.indirection - 1);
	}

	backingStore->destroyPage(backingStore->btreeStore, 
				0, traceRecord->btreePage);
	return YES;
}

inline static void 
_btreeEmpty(__BTree *btree)
{
	unsigned short		nodeSize;
	BTreeInternalNode	*internalNode;
	BTreeExternalNode	*externalNode;
	BTreeTraceRecord	traceRecord;
	BTreeStore		*btreeStore;
	BTreeBackingStore	*backingStore;

	btree->_syncVersion++;
	bzero((char *) &traceRecord, sizeof(BTreeTraceRecord));
	backingStore = (BTreeBackingStore *) btree->backingStore;
	btreeStore = backingStore->btreeStore;
	_btreeOpenPage(backingStore, &traceRecord, btreeStore->rootOffset);
	externalNode = traceRecord.traceNode.externalNode;
	if (traceRecord.btreePage->pageLevel)
	{
		_btreePostOrderWalk(btree, 
			&traceRecord, _btreeDestroyPageWalkHandler, 
				(BTreeInternalRecord *) 0, (void *) 0);
		btree->btreeDepth = 1;
		traceRecord.btreePage->pageLevel = 0;
		internalNode = traceRecord.traceNode.internalNode;
		nodeSize = internalNode->nodeSize;
	} else
		nodeSize = externalNode->nodeSize;

	_btreeTouchPage(backingStore, &traceRecord);
	btreeStore->recordCount = 0;
	bzero((char *) traceRecord.btreePage->pageBuffer + 
		btreeStore->headerSize, nodeSize + sizeof(BTreePageHeader));
	externalNode->nodeSize = nodeSize;
	externalNode->recordLimit = nodeSize;
	externalNode->nodeLength = 
		sizeof(unsigned short) + sizeof(BTreeExternalNode);
}

- (void) empty
{
	NXHandler		errorHandler;
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	_NXAddHandler(&errorHandler);
	if (! _setjmp(errorHandler.jumpState))
	{
		_btreeEmpty((__BTree *) self);
		_NXRemoveHandler(&errorHandler);
	} else
	{
		_backingStore->undo(_backingStore->btreeStore, 0);
		_NXRaiseError(errorHandler.code, 
			errorHandler.data1, errorHandler.data2);
	}

	if (_backingStore->bind)
		_backingStore->bind(_backingStore->btreeStore, 0);
}

static boolean_t 
_btreeCountWalkHandler(_btreeWalkRecord *walkRecord)
{
	BTreeTraceRecord	*traceRecord;

	traceRecord = walkRecord->traceRecord;
	if (traceRecord->btreePage->pageLevel < 1)
		*((unsigned long *) walkRecord->userPointer) += 
			traceRecord->traceNode.externalNode->entryCount;

	return YES;
}

inline static unsigned 
_btreeCount(__BTree *btree)
{
	unsigned long		recordCount;
	BTreeExternalNode	*externalNode;
	BTreeTraceRecord	traceRecord;
	BTreeStore		*btreeStore;
	BTreeBackingStore	*backingStore;

	bzero((char *) &traceRecord, sizeof(BTreeTraceRecord));
	backingStore = (BTreeBackingStore *) btree->backingStore;
	btreeStore = backingStore->btreeStore;
	_btreeOpenPage(backingStore, &traceRecord, btreeStore->rootOffset);
	if (traceRecord.btreePage->pageLevel < 1)
	{
		externalNode = traceRecord.traceNode.externalNode;
		recordCount = externalNode->entryCount;
	} else
	{
		recordCount = 0;
		_btreePostOrderWalk(btree, 
			&traceRecord, _btreeCountWalkHandler, 
				(BTreeInternalRecord *) 0, &recordCount);
	}

	btreeStore->recordCount = recordCount;
	return (unsigned) recordCount;
}

- (unsigned) _count
{
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	return _btreeCount((__BTree *) self);
}

- (unsigned) count
{
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	return [_backingStore->btreeStore count];
}

- (void) save
{
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	[_backingStore->btreeStore save];
}

- (void) undo
{
	BTreeBackingStore	*_backingStore;

	_backingStore = (BTreeBackingStore *) backingStore;
	[_backingStore->btreeStore undo];
}

- read: (NXTypedStream *) stream
{
	return [self notImplemented: _cmd];
}

- write: (NXTypedStream *) stream
{
	return [self notImplemented: _cmd];
}


@end


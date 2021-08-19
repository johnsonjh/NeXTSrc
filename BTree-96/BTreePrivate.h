
#ifndef	_BTREEPRIVATE_H_
#define	_BTREEPRIVATE_H_

#define	NOT_YET

#import	"BTree.h"
#import "BTreeStoreMemory.h"

#import	<ctype.h>
#import	<libc.h>
#import	<mach_error.h>
#import <sys/types.h>

#if defined(_BTREESTATISTICS) || defined(_BTREETIMINGSTATISTICS)
#import	"BTreeStatistics.h"
#endif

#define	monocase(c)	(isupper(c) ? tolower(c) : (c))

extern int _NXBTreeClassVersion;

typedef struct __BTree	{
	@defs(BTree)
} __BTree;

typedef struct __BTreeCursor	{
	@defs(BTreeCursor)
} __BTreeCursor;

typedef enum BTreeRotation {
	shiftLeft, shiftRight, splitRight, spanLeft, mergeLeft, mergeRight
} BTreeRotation;

/* disk resident data structures */

typedef struct BTreePageHeader	{
	unsigned long		RESERVED[2];
	struct			{
	unsigned		backFill:4;
	unsigned		RESERVED:4;
	}			pageStatus;
	char			contents[0];
} BTreePageHeader;

typedef struct BTreeInternalRecord	{
	unsigned short		keyLength;
	char			recordContent[0];
} BTreeInternalRecord;

typedef struct BTreeInternalNode	{
	unsigned long		RESERVED[2];
	struct			{
	unsigned		packable:1;
	unsigned		polytonal:1;
	unsigned		RESERVED:6;
	}			nodeStatus;
	unsigned short		recordLimit;
	unsigned short		nodeSize;
	unsigned short		nodeLength;
	vm_offset_t		childOffset;
	unsigned short		entryCount;
	vm_offset_t		entryArray[0];
} BTreeInternalNode;

typedef struct BTreeMappingRecord	{
	vm_offset_t		regionOffset;
	vm_size_t		regionLength;
} BTreeMappingRecord;

typedef struct BTreeExternalRecord	{
	struct			{
	unsigned		indirection:2;
	unsigned		keyLength:14;
	}			recordHeader;
	unsigned short		recordLength;
	char			recordContent[0];
} BTreeExternalRecord;

typedef struct BTreeExternalNode	{
	unsigned long		RESERVED[2];
	struct			{
	unsigned		packable:1;
	unsigned		polytonal:1;
	unsigned		spanned:1;
	unsigned		RESERVED:5;
	}			nodeStatus;
	unsigned short		recordLimit;
	unsigned short		nodeSize;
	unsigned short		nodeLength;
	unsigned short		spanOffset;
	unsigned short		spanLength;
	unsigned short		entryCount;
	unsigned short		entryArray[0];
} BTreeExternalNode;

/* memory-only data structures */

typedef struct BTreeTraceRecord		{
	struct				{
	unsigned			exactMatch:1;
	unsigned			shadowed:1;
	unsigned			lastPosition:1;
	unsigned			RESERVED:5;
	}				traceStatus;
	short				tracePosition;
	union				{
	BTreeInternalNode		*internalNode;
	BTreeExternalNode		*externalNode;
	}				traceNode;
	BTreePage			*btreePage;
	struct BTreeTraceRecord		*childRecord;
	struct BTreeTraceRecord		*superRecord;
	struct BTreeTraceRecord		*siblingRecord;
} BTreeTraceRecord;

typedef struct BTreeKeyPromotion	{
	unsigned short		keyLength;
	short			position;
	void			*keyBuffer;
	vm_offset_t		pageOffset;
} BTreeKeyPromotion;

typedef enum BTreeTraceType		{
	btreeTraceTop, btreeTraceEnd, btreeTraceKey
} BTreeTraceType;

typedef struct BTreeBackingStore	{
	BTreePage	*(*openPageAt)(BTreeStore *store, 
				SEL selector, vm_offset_t offset);
	BTreePage	*(*createPage)(BTreeStore *store, SEL selector);
	void		(*touchPage)(BTreeStore *store, 
				SEL selector, BTreePage *page);
	void		(*readPage)(BTreeStore *store, 
				SEL selector, BTreePage *page);
	void		(*destroyPage)(BTreeStore *store, 
				SEL selector, BTreePage *page);
	void		(*destroyPageAt)(BTreeStore *store, 
				SEL selector, vm_offset_t offset);
	void		(*bind)(BTreeStore *store, SEL selector);
	void		(*save)(BTreeStore *store, SEL selector);
	void		(*undo)(BTreeStore *store, SEL selector);
	void		(*createPageGroup)(BTreeStore *store, SEL selector, 
				vm_offset_t *pageOffset, 
				unsigned long *pageCount);
	void		(*readPageGroup)(BTreeStore *store, SEL selector, 
				vm_offset_t pageOffset, 
				unsigned long pageCount, 
				vm_address_t toAddress);
	void		(*writePageGroup)(BTreeStore *store, SEL selector, 
				vm_offset_t pageOffset, 
				unsigned long pageCount, 
				vm_address_t fromAddress);
	void		(*destroyPageGroup)(BTreeStore *store, SEL selector, 
				vm_offset_t pageOffset, 
				unsigned long pageCount);

	BTreeStore	*btreeStore;
} BTreeBackingStore;

/*
private external function declarations
*/

extern void 
_btreeDecreaseTraceDepth(__BTreeCursor *btreeCursor, 
	unsigned short _traceDepth);

extern void 
_btreeIncreaseTraceDepth(__BTreeCursor *btreeCursor, 
	unsigned short _traceDepth);

extern void 
_btreeRemoveMapping(BTreeBackingStore *backingStore, 
	BTreeMappingRecord *mappingRecord, unsigned long mappingLength, 
	unsigned short mappingLevel);

extern BTreeTraceRecord 
*_btreeRightSibling(__BTreeCursor *btreeCursor, BTreeTraceRecord *traceRecord);

extern __BTreeCursor 
*_btreeInsertExternal(__BTreeCursor *btreeCursor, void *record, 
	unsigned long recordLength, unsigned short indirection);

extern __BTreeCursor 
*_btreeUpdateExternal(__BTreeCursor *btreeCursor, char *record, 
	unsigned long recordLength, unsigned short indirection);

extern __BTreeCursor 
*_btreeRemoveExternal(__BTreeCursor *btreeCursor);

extern void 
_btreeTraceCursor(__BTreeCursor *btreeCursor, BTreeTraceType traceType);

extern boolean_t 
_btreeCheckHint(__BTreeCursor *btreeCursor);

extern void 
_btreeSearchInternal(__BTree *btree, BTreeTraceRecord *traceRecord, 
	void *key, unsigned short keyLength, BTreeTraceType traceType);

extern void 
_btreeSearchExternal(__BTree *btree, BTreeTraceRecord *traceRecord, 
	void *key, unsigned short keyLength, BTreeTraceType traceType);

extern void 
_btreeForwardExternal(__BTreeCursor *btreeCursor);

extern void 
_btreeReverseExternal(__BTreeCursor *btreeCursor);

/*
inline routine definitions
*/

inline static unsigned short 
_btreeNodeSize(BTreeStore *btreeStore)
{
	return btreeStore->pageSize - 
		btreeStore->headerSize - sizeof(BTreePageHeader);
}

#define	BTreeExternalNodeLength			\
	(sizeof(unsigned short) + sizeof(BTreeExternalNode))
#define BTreeExternalRecordOverhead		\
	(sizeof(unsigned short) + sizeof(BTreeExternalRecord))

inline static void 
_btreeCloseInternal(BTreeTraceRecord *traceRecord, short position)
{
	traceRecord->tracePosition = position;
	traceRecord->traceStatus.lastPosition = (position >= 
		traceRecord->traceNode.internalNode->entryCount - 1);
}

inline static void 
_btreeCloseExternal(BTreeTraceRecord *traceRecord, short position)
{
	traceRecord->tracePosition = position;
	traceRecord->traceStatus.lastPosition = 
		(position >= traceRecord->traceNode.externalNode->entryCount);
}

inline static BTreeTraceRecord 
*_btreeNewTraceRecord()
{
	BTreeTraceRecord	*traceRecord;

	traceRecord = (BTreeTraceRecord *) calloc(sizeof(BTreeTraceRecord), 1);
	if (! traceRecord)
		_NXRaiseError(NX_BTreeNoMemory, "_btreeNewTraceRecord", 0);

	traceRecord->siblingRecord = traceRecord;
	return traceRecord;
}

#define	_btreeFreeTraceRecord(traceRecord)	free(traceRecord)

inline static BTreeExternalRecord 
*_btreeExternalRecord(BTreeTraceRecord *traceRecord)
{
	BTreeExternalNode	*externalNode;

	externalNode = traceRecord->traceNode.externalNode;
	return (BTreeExternalRecord *) ((char *) externalNode 
		+ externalNode->entryArray[traceRecord->tracePosition]);
}

inline static void 
_btreeInitTraceRecord(BTreeStore *btreeStore, BTreeTraceRecord *traceRecord)
{
	traceRecord->traceNode.internalNode = (BTreeInternalNode *) 
		(traceRecord->btreePage->pageBuffer + 
			btreeStore->headerSize + sizeof(BTreePageHeader));
}

inline static void 
_btreeCreatePage(BTreeBackingStore *backingStore, 
	BTreeTraceRecord *traceRecord)
{
	traceRecord->btreePage = 
		backingStore->createPage(backingStore->btreeStore, 0);
	_btreeInitTraceRecord(backingStore->btreeStore, traceRecord);
}

inline static void 
_btreeOpenPage(BTreeBackingStore *backingStore, 
	BTreeTraceRecord *traceRecord, vm_offset_t pageOffset)
{
	traceRecord->btreePage = 
		backingStore->openPageAt(backingStore->btreeStore, 
			0, pageOffset);
	_btreeInitTraceRecord(backingStore->btreeStore, traceRecord);
}

inline static void 
_btreeReadPage(BTreeBackingStore *backingStore, 
	BTreeTraceRecord *traceRecord)
{
#if	defined(DEBUG)
	if (! traceRecord->btreePage)
		_NXRaiseError(NX_BTreeInternalError, "_btreeReadPage", 0);
#endif
	if (traceRecord->btreePage->pageBuffer)
		return;

	backingStore->readPage(backingStore->btreeStore, 
		0, traceRecord->btreePage);
	_btreeInitTraceRecord(backingStore->btreeStore, traceRecord);
}

inline static void 
_btreeTouchPage(BTreeBackingStore *backingStore, 
	BTreeTraceRecord *traceRecord)
{
	vm_offset_t	pageOffset;
	BTreePage	*btreePage;

	btreePage = traceRecord->btreePage;
#if	defined(DEBUG)
	if (! btreePage)
		_NXRaiseError(NX_BTreeInternalError, "_btreeReadPage", 0);
#endif
	pageOffset = btreePage->pageOffset;
	backingStore->touchPage(backingStore->btreeStore, 0, btreePage);
	if (btreePage->pageOffset != pageOffset)
	{
		traceRecord->traceStatus.shadowed = YES;
		_btreeInitTraceRecord(backingStore->btreeStore, traceRecord);
	}
}

inline static void 
_btreeDestroyPage(BTreeBackingStore *backingStore, 
	BTreeTraceRecord *traceRecord)
{
	backingStore->destroyPage(backingStore->btreeStore, 
		0, traceRecord->btreePage);
	traceRecord->btreePage = (BTreePage *) 0;
}


#endif	_BTREEPRIVATE_H_


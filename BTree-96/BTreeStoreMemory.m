
#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreeErrors.h"
#import	"BTreeStoreMemory.h"
#import	<libc.h>
#import	<mach.h>
#import	<mach_error.h>

static vm_size_t		_pageSize;
static unsigned short		_shiftPerPage;
static unsigned short		_pagesPerEntry;
static unsigned short		_shiftPerEntry;
static unsigned short		_spareCount;
static BTreePageEntryChain	_spareChain;

#if	defined(DEBUG)

inline static void 
_btreeProtectBuffer(vm_address_t pageBuffer, boolean_t writeable)
{
	kern_return_t		error;

	error = vm_protect(task_self(), pageBuffer, _pageSize, NO, 
		(writeable) ? VM_PROT_READ | VM_PROT_WRITE : VM_PROT_READ);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeProtectBuffer", mach_error_string(error));
}

#else

#define	_btreeProtectBuffer(pageBuffer, writeable)	

#endif

inline static void 
_btreeFreeBuffer(vm_address_t pageBuffer)
{
	kern_return_t		error;

	error = vm_deallocate(task_self(), pageBuffer, _pageSize);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeFreeBuffer", mach_error_string(error));
}

inline static void 
_btreeCopyBuffer(vm_address_t fromAddress, vm_address_t toAddress)
{
	kern_return_t		error;

	error = vm_copy(task_self(), fromAddress, _pageSize, toAddress);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeCopyBuffer", mach_error_string(error));
}

inline static void 
_btreeFreePageBuffer(BTreePage *btreePage)
{
	if (btreePage->pageBuffer)
	{
		_btreeFreeBuffer(btreePage->pageBuffer);
		btreePage->pageBuffer = (vm_address_t) 0;
	}
}

inline static void 
_btreeFreeCopyBuffer(BTreePageEntry *pageEntry)
{
	if (pageEntry->copyBuffer)
	{
		_btreeFreeBuffer(pageEntry->copyBuffer);
		pageEntry->copyBuffer = (vm_address_t) 0;
	}
}

inline static BTreePageEntry 
*_btreeAppendEntry(BTreePageEntryChain *entryChain, 
	BTreePageEntry *pageEntry)
{
	pageEntry->prevEntry = entryChain->prevEntry;
	if (pageEntry->prevEntry)
		pageEntry->prevEntry->nextEntry = pageEntry;
	else
		entryChain->nextEntry = pageEntry;

	entryChain->prevEntry = pageEntry;
	pageEntry->nextEntry = (BTreePageEntry *) 0;
	pageEntry->entryChain = entryChain;
	return pageEntry;
}

inline static void 
_btreeRemoveEntry(BTreePageEntry *pageEntry)
{
	BTreePageEntryChain	*entryChain;

	entryChain = pageEntry->entryChain;
	if (entryChain)
	{
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
}

inline static void 
_btreeFreeEntry(BTreePage *btreePage)
{
	BTreePageEntry		*pageEntry;

	pageEntry = btreePage->RESERVED;
	if (pageEntry)
	{
		_btreeFreeCopyBuffer(pageEntry);
		btreePage->RESERVED = (void *) 0;
		if (_spareCount < 48)
		{
			_btreeAppendEntry(&_spareChain, pageEntry);
			++_spareCount;
		} else
			free((void *) pageEntry);
	}
}

static BTreePageEntry 
*_btreeMakeEntry(BTreePage *btreePage)
{
	BTreePageEntry		*pageEntry;

	pageEntry = btreePage->RESERVED;
	if (! pageEntry)
	{
		pageEntry = _spareChain.nextEntry;
		if (pageEntry)
		{
			_btreeRemoveEntry(pageEntry);
			--_spareCount;
		} else
		{
			pageEntry = (BTreePageEntry *) 
				malloc(sizeof(BTreePageEntry));
			if (! pageEntry)
				_NXRaiseError(NX_BTreeNoMemory, 
					"_btreeMakeEntry", 0);
		}

		bzero((char *) pageEntry, sizeof(BTreePageEntry));
		pageEntry->btreePage = btreePage;
		btreePage->RESERVED = pageEntry;
	}

	return pageEntry;
}

static BTreePageEntry 
*_btreeLoadEntry(BTreePage *btreePage)
{
	BTreePageEntry		*pageEntry;

	pageEntry = _btreeMakeEntry(btreePage);
	if (! pageEntry->copyBuffer)
	{
		kern_return_t	error;

		error = vm_allocate(task_self(), 
			&pageEntry->copyBuffer, _pageSize, YES);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"touchPage:", mach_error_string(error));
	}

	_btreeCopyBuffer(btreePage->pageBuffer, pageEntry->copyBuffer);
	return pageEntry;
}

static void 
_btreeClearPageTable(BTreeStoreMemory *btreeStore, boolean_t deallocate)
{
	kern_return_t		error;
	unsigned long		i;
	unsigned long		j;
	unsigned long		tableEntries;
	unsigned long		validPages;
	BTreePage		*btreePage;

	tableEntries = btreeStore->tableEntries;
	for (i = 0; i < tableEntries; ++i)
	{
		validPages = _pagesPerEntry;
		if (validPages > btreeStore->validPages)
			validPages = btreeStore->validPages;

		btreeStore->validPages -= validPages;
		btreePage = btreeStore->pageTable[i];
		for (j = 0; j < validPages; ++j, ++btreePage)
		{
			_btreeFreePageBuffer(btreePage);
			_btreeFreeEntry(btreePage);
		}

		if (deallocate)
		{
			error = vm_deallocate(task_self(), (vm_offset_t) 
				btreeStore->pageTable[i], vm_page_size);
			if (error != KERN_SUCCESS)
				_NXRaiseError(NX_BTreeMachError + error, 
					"_btreeClearPageTable", 
						mach_error_string(error));
		} else
			bzero((char *) btreeStore->pageTable[i], vm_page_size);

		btreeStore->totalPages -= _pagesPerEntry;
		--btreeStore->tableEntries;
	}

	btreeStore->discardChain.nextEntry = (BTreePageEntry *) 0;
	btreeStore->discardChain.prevEntry = (BTreePageEntry *) 0;
	btreeStore->destroyChain.nextEntry = (BTreePageEntry *) 0;
	btreeStore->destroyChain.prevEntry = (BTreePageEntry *) 0;
	btreeStore->recycleChain.nextEntry = (BTreePageEntry *) 0;
	btreeStore->recycleChain.prevEntry = (BTreePageEntry *) 0;
}

inline static void 
_btreeBuildPageTable(BTreeStoreMemory *btreeStore)
{
	kern_return_t		error;
	unsigned long		tableEntries;
	unsigned long		actualSize;
	unsigned long		formerSize;
	BTreePage		**tempTable;

	formerSize = malloc_size(btreeStore->pageTable);
	tableEntries = formerSize / sizeof(BTreePage *);
	if (tableEntries > btreeStore->tableEntries)
		tempTable = btreeStore->pageTable;
	else
	{
		tempTable = (BTreePage **) 
			realloc(btreeStore->pageTable, formerSize << 1);
		if (! tempTable)
			_NXRaiseError(NX_BTreeNoMemory, 
				"_btreeBuildPageTable", 0);

		actualSize = malloc_size(tempTable);
		bzero((char *) 
			tempTable + formerSize, actualSize - formerSize);
		btreeStore->pageTable = tempTable;
	}

	tempTable += btreeStore->tableEntries;
	error = vm_allocate(task_self(), 
		(vm_offset_t *) tempTable, vm_page_size, YES);
	if (error != KERN_SUCCESS)
		_NXRaiseError(NX_BTreeMachError + error, 
			"_btreeBuildPageTable", mach_error_string(error));

	btreeStore->totalPages += _pagesPerEntry;
	++btreeStore->tableEntries;
}

inline static BTreePage 
*_btreePageFromIndex(BTreeStoreMemory *btreeStore, unsigned long index)
{
	unsigned long		tableEntry;
	unsigned long		pageOffset;

#if	defined(DEBUG)
	if (index >= btreeStore->validPages + 1)
		_NXRaiseError(NX_BTreeInvalidArguments, 
			"_btreePageFromIndex", 0);
#endif

	pageOffset = index & (_pagesPerEntry - 1);
	tableEntry = index >> _shiftPerEntry;
	return btreeStore->pageTable[tableEntry] + pageOffset;
}

inline static BTreePage 
*_btreeNewPage(BTreeStoreMemory *btreeStore)
{
	BTreePageEntry		*pageEntry;
	BTreePage		*btreePage;

	pageEntry = btreeStore->recycleChain.nextEntry;
	if (pageEntry)
	{
		_btreeRemoveEntry(pageEntry);
		btreePage = pageEntry->btreePage;
		_btreeFreeEntry(btreePage);
	} else
	{
		if (btreeStore->validPages >= btreeStore->totalPages)
			_btreeBuildPageTable(btreeStore);

		btreePage = _btreePageFromIndex(btreeStore, 
			btreeStore->validPages);
		btreePage->pageOffset = _pageSize * btreeStore->validPages;
		++btreeStore->validPages;
	}

	return btreePage;
}

inline static void 
_btreeDestroyPage(BTreeStoreMemory *btreeStore, BTreePage *btreePage)
{
	BTreePageEntry		*pageEntry;

	_btreeFreePageBuffer(btreePage);
	pageEntry = (BTreePageEntry *) btreePage->RESERVED;
	_btreeAppendEntry(&btreeStore->recycleChain, pageEntry);
}

static void 
_btreeUndoChain(BTreeStoreMemory *btreeStore, BTreePageEntryChain *entryChain)
{
	BTreePage		*btreePage;
	BTreePageEntry		*pageEntry;

	pageEntry = entryChain->nextEntry;
	for (; pageEntry; pageEntry = entryChain->nextEntry)
	{
		_btreeRemoveEntry(pageEntry);
		btreePage = pageEntry->btreePage;
		if (pageEntry->copyBuffer)
		{
			_btreeCopyBuffer(pageEntry->copyBuffer, 
				btreePage->pageBuffer);
			_btreeFreeEntry(btreePage);
		} else
			_btreeDestroyPage(btreeStore, btreePage);
	}
}

@implementation BTreeStoreMemory

+ initialize
{
	_pageSize = vm_page_size;
	if (vm_page_size == 8192)
		_shiftPerPage = 13;
	else
	if (vm_page_size == 4096)
		_shiftPerPage = 11;
	else
		_NXRaiseError(NX_BTreeInternalError, "initialize", 0);

	_pagesPerEntry = vm_page_size / sizeof(BTreePage);
	if (_pagesPerEntry == 512)
		_shiftPerEntry = 9;
	else
	if (_pagesPerEntry == 256)
		_shiftPerEntry = 8;
	else
		_NXRaiseError(NX_BTreeInternalError, "initialize", 0);

	malloc_good_size(sizeof(BTreePageEntry));
	return self;
}

+ new
{
	kern_return_t		error;
	NXHandler		jumpHandler;
	unsigned long		actualSize;

	self = [super new];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[self free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		storeStatus.writeable = YES;
		pageSize = _pageSize;
		pageTable = (BTreePage **) malloc(sizeof(BTreePage *));
		if (! pageTable)
			_NXRaiseError(NX_BTreeNoMemory, "new", 0);

		actualSize = malloc_size(pageTable);
		bzero((char *) pageTable, actualSize);
		error = vm_allocate(task_self(), 
			(vm_offset_t *) pageTable, vm_page_size, YES);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"new", mach_error_string(error));

		tableEntries = 1;
		totalPages = _pagesPerEntry;
		[self createPage];
		_NXRemoveHandler(&jumpHandler);
	}

	return self;
}

- free
{
	if (pageTable)
	{
		_btreeClearPageTable((BTreeStoreMemory *) self, YES);
		free(pageTable);
	}

	return [super free];
}

- (BTreePage *) openPageAt: (vm_offset_t) pageOffset
{
	BTreePage	*btreePage;

	btreePage = _btreePageFromIndex(self, pageOffset >> _shiftPerPage);
	_btreeProtectBuffer(btreePage->pageBuffer, NO);
	return btreePage;
}

- (void) touchPage: (BTreePage *) btreePage
{
	_btreeProtectBuffer(btreePage->pageBuffer, YES);
	if (saveEnabled && ! btreePage->RESERVED)
		_btreeAppendEntry(&discardChain, _btreeLoadEntry(btreePage));
}

- (BTreePage *) createPage
{
	kern_return_t		error;
	BTreePage		*btreePage;

	btreePage = _btreeNewPage((BTreeStoreMemory *) self);
	error = vm_allocate(task_self(), 
		&btreePage->pageBuffer, pageSize, YES);
	if (error != KERN_SUCCESS)
	{
		_btreeAppendEntry(&recycleChain, _btreeMakeEntry(btreePage));
		_NXRaiseError(NX_BTreeMachError + error, 
			"createPage", mach_error_string(error));
	}

	if (saveEnabled)
		_btreeAppendEntry(&discardChain, _btreeMakeEntry(btreePage));

	return btreePage;
}

- (void) readPage: (BTreePage *) btreePage
{
	_NXRaiseError(NX_BTreeInternalError, "readPage:", 0);
}

- (void) destroyPage: (BTreePage *) btreePage
{
	_btreeRemoveEntry(_btreeMakeEntry(btreePage));
	if (saveEnabled)
		_btreeAppendEntry(&destroyChain, _btreeLoadEntry(btreePage));
	else
		_btreeDestroyPage((BTreeStoreMemory *) self, btreePage);
}

- (void) readPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount into: (vm_address_t) toAddress
{
	kern_return_t		error;
	vm_task_t		_task_self;
	vm_offset_t		nextAddress;
	unsigned long		readIndex;
	unsigned long		startIndex;
	BTreePage		*startPage;
	BTreePage		*btreePage;

	pageOffset = pageOffset / pageSize;
	btreePage = _btreePageFromIndex(self, pageOffset);
	_task_self = task_self();
	for (readIndex = 0; readIndex < pageCount; toAddress += startIndex)
	{
		startPage = btreePage;
		nextAddress = btreePage->pageBuffer;
		startIndex = readIndex;
		for (++readIndex; readIndex < pageCount; ++readIndex)
		{
			++pageOffset;
			btreePage = _btreePageFromIndex(self, pageOffset);
			nextAddress += pageSize;
			if (btreePage->pageBuffer != nextAddress)
				break;
		}

		startIndex = (readIndex - startIndex) * pageSize;
		error = vm_copy(_task_self, 
			startPage->pageBuffer, startIndex, toAddress);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"readPageGroup:count:into:", 
				mach_error_string(error));
	}
}

- (void) writePageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount from: (vm_address_t) fromAddress
{
	kern_return_t		error;
	vm_task_t		_task_self;
	vm_offset_t		nextAddress;
	unsigned long		writeIndex;
	unsigned long		startIndex;
	BTreePage		*startPage;
	BTreePage		*btreePage;
	void			(*touchPage)();

	_task_self = task_self();
	touchPage = (void (*)()) [self methodFor: @selector(touchPage:)];
	pageOffset = pageOffset / pageSize;
	btreePage = _btreePageFromIndex(self, pageOffset);
	touchPage(self, @selector(touchPage:), btreePage);
	for (writeIndex = 0; writeIndex < pageCount; fromAddress += startIndex)
	{
		startPage = btreePage;
		nextAddress = btreePage->pageBuffer;
		startIndex = writeIndex;
		for (++writeIndex; writeIndex < pageCount; ++writeIndex)
		{
			++pageOffset;
			btreePage = _btreePageFromIndex(self, pageOffset);
			touchPage(self, @selector(touchPage:), btreePage);
			nextAddress += pageSize;
			if (btreePage->pageBuffer != nextAddress)
				break;
		}

		startIndex = (writeIndex - startIndex) * pageSize;
		if (fromAddress % pageSize == 0)
		{
			error = vm_copy(_task_self, fromAddress, 
				startIndex, startPage->pageBuffer);
			if (error != KERN_SUCCESS)
				_NXRaiseError(NX_BTreeMachError + error, 
					"writePageGroup:count:from:", 
						mach_error_string(error));
		} else
			bcopy((char *) fromAddress, 
				(char *) startPage->pageBuffer, startIndex);
	}
}

- (void) undo
{
	if (saveEnabled)
	{
		_btreeUndoChain((BTreeStoreMemory *) self, &discardChain);
		_btreeUndoChain((BTreeStoreMemory *) self, &destroyChain);
	} else
		_btreeClearPageTable((BTreeStoreMemory *) self, NO);
}

- (void) save
{
	BTreePageEntry		*pageEntry;

	if (saveEnabled)
	{
		pageEntry = discardChain.nextEntry;
		for (; pageEntry; pageEntry = discardChain.nextEntry)
		{
			_btreeRemoveEntry(pageEntry);
			_btreeFreeEntry(pageEntry->btreePage);
		}
	
		pageEntry = destroyChain.nextEntry;
		for (; pageEntry; pageEntry = destroyChain.nextEntry)
		{
			_btreeRemoveEntry(pageEntry);
			_btreeDestroyPage((BTreeStoreMemory *) 
				self, pageEntry->btreePage);
		}
	} else
		saveEnabled = YES;
}


@end


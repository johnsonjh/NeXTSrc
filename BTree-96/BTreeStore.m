
#ifdef	SHLIB
#import	"shlib.h"
#endif

#import	"BTreeStore.h"

@implementation BTreeStore

- (unsigned) count
{
	return (unsigned) recordCount;
}

- (BTreePage *) openPageAt: (vm_offset_t) pageOffset
{
	[self subclassResponsibility: _cmd];
	return (BTreePage *) 0;
}

- (BTreePage *) createPage
{
	[self subclassResponsibility: _cmd];
	return (BTreePage *) 0;
}

- (void) readPage: (BTreePage *) btreePage
{
	[self subclassResponsibility: _cmd];
}

- (void) destroyPage: (BTreePage *) btreePage
{
	[self subclassResponsibility: _cmd];
}

- (void) destroyPageAt: (vm_offset_t) pageOffset
{
	[self destroyPage: [self openPageAt: pageOffset]];
}

- (void) touchPage: (BTreePage *) btreePage
{
	[self subclassResponsibility: _cmd];
}

- (void) createPageGroup: (vm_offset_t *) pageOffset 
	count: (unsigned long *) pageCount
{
	vm_offset_t		nextOffset;
	unsigned long		pageLimit;
	BTreePage		*(*createPage)();
	BTreePage		*btreePage;
	NXHandler		jumpHandler;

	if (*pageCount)
	{
		pageLimit = *pageCount;
		createPage = (BTreePage *(*)()) 
			[self methodFor: @selector(createPage)];
		btreePage = createPage(self, @selector(createPage));
		*pageCount = 1;
		*pageOffset = btreePage->pageOffset;
		_NXAddHandler(&jumpHandler);
		if (_setjmp(jumpHandler.jumpState))
		{
			[self destroyPageGroup: *pageOffset count: *pageCount];
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}
	
		for (; *pageCount < pageLimit; ++(*pageCount))
		{
			nextOffset = btreePage->pageOffset + pageSize;
			btreePage = createPage(self, @selector(createPage));
			if (btreePage->pageOffset != nextOffset)
			{
				[self destroyPage: btreePage];
				break;
			}
		}
	
		_NXRemoveHandler(&jumpHandler);
	}
}

- (void) readPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount into: (vm_address_t) toAddress
{
	[self subclassResponsibility: _cmd];
}

- (void) writePageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount from: (vm_address_t) fromAddress
{
	[self subclassResponsibility: _cmd];
}

- (void) destroyPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount
{
	vm_offset_t	nextOffset;
	BTreePage	*(*destroyPageAt)();

	destroyPageAt = (BTreePage *(*)()) 
		[self methodFor: @selector(destroyPageAt:)];
	nextOffset = pageOffset + (pageCount * pageSize);
	for (; pageCount; --pageCount)
	{
		nextOffset -= pageSize;
		destroyPageAt(self, @selector(destroyPageAt:), nextOffset);
	}
}

- (void) save
{
	[self subclassResponsibility: _cmd];
}

- (void) undo
{
	[self subclassResponsibility: _cmd];
}

- (void) bind
{
	[self subclassResponsibility: _cmd];
}


@end


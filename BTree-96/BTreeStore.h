/*
BTreeStore.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield
 
DEFINED AS: A private class
HEADER FILES: BTreeStore.h
*/

#import	<mach.h>
#import	<objc/Object.h>

/*
The BTreePage class manages virtual memory images of backing store pages for instances of the BTreeStore class.  Each instance of BTreePage manages the virtual memory image of one backing store page.  The implementation of the BTreePage class directly determines the performance of the BTree class.  The BTreePage class is therefore implemented as a public data structure, and its instance variables are manipulated directly by the BTree class.  

The pageBuffer instance variable records the address of the virtual memory image of the page represented by the BTreePage.  The pageOffset instance variable records the position of the page within the backing store.  The BTreeStore class uses this value to uniquely identify a page in the backing store.  The BTree class therefore uses this value in the tree structure as a physical page address.  The pageLevel instance variable is set by the BTree class to indicate the level number of a page when the page is created.  The level number of the root page may change at any time.  The backing store should record the contents of the pageLevel instance variable when writing the page; it should also recover the contents from the backing store and initialize the instance variable when rereading the page.

For efficiency, the interface to the BTreeStore class assumes that a BTreePage remains valid for the lifetime of the BTreeStore.
*/

typedef struct BTreePage	{
	void			*RESERVED;	/* This field reserved for 
						use by the backing store */
	unsigned long		pageLevel;	/* The level of the page, 
						with zero at the bottom */
	vm_offset_t		pageOffset;	/* The offset of the page 
						within the backing store */
	vm_address_t		pageBuffer;	/* A pointer to a buffer 
						holding an image of the page */
} BTreePage;

@interface BTreeStore : Object
/*
The BTreeStore class is as an abstract superclass for backing store back ends.  Each instance of BTreeStore represents a single BTree; an instance of BTreeStore is passed to every BTree through the semi-private factory method _newWith:.  Classes that maintain multiple BTrees in a single backing store should create and maintain a unique instance of BTreeStore for each BTree in the backing store.  

A BTree obtains BTreePages representing backing store pages from its BTreeStore.  The BTree builds and maintains private data structures in the backing store pages; it computes the address of its private data structure for a given instance of BTreePage by adding the value returned by the headerSize method to the contents of the pageBuffer instance variable.  For optimal performance, the backing store page size should be an integral multiple of the virtual memory page size, since the implementation of the BTree class is page oriented.  A BTree computes the size of its private data structures for a given instance of BTreeStore by subtracting the value returned by the headerSize method from the value returned by the pageSize method.  

An implementation of BTreeStore for a physical device should use a virtual memory cache to ensure good performance.  In order to support its cursor based architecture in an efficient manner, the BTree class makes liberal use of the backing store on the assumption that repeated requests for the same page are inexpensive.  The implementation of the BTreeStore class directly determines the performance of the BTree class.  For this reason, the BTreeStore is a public data structure, and most of its instance variables are manipulated directly by the BTree class.  Also, implementations should optimize the time required to resolve a page offset with openPageAt:, since this method may be called many times for each operation.  With the exception of \fBbind\fR and \fBsave\fR, all of the methods defined by this class must be implemented.
*/
{
@public
	struct			{
		unsigned	writeable:1;	// True if the backing store 
						// is writeable
		unsigned	performSave:1;	// True if automatic saving 
						// after every operation
		unsigned	RESERVED:14;
	}			storeStatus;
	unsigned short		pageSize;	// The physical page size
	unsigned short		headerSize;	// The size of the page header 
	vm_offset_t		rootOffset;	// The offset of the root page 
						// within the backing store
	unsigned long		codeVersion;	// The highest version of the 
						// BTree used on the store
	unsigned long		recordCount;	// The number of key/record 
						// pairs in the BTree
}

/* 
METHOD TYPES
Creating, freeing and using pages
Creating, freeing and using page groups
Saving and undoing changes
*/

- (unsigned) count;
/* 
Returns the number of \fIkey/record\fR pairs in the BTree.  This method is typically much faster than enumerating and counting the \fIkey/record\fR pairs.
*/

- (BTreePage *) openPageAt: (vm_offset_t) pageOffset;
/* 
TYPE: Creating, freeing and using pages; Opens a page 

Returns a pointer to the BTreePage managing the physical page which resides at offset \fIpageOffset\fR in the backing store.
*/

- (BTreePage *) createPage;
/* 
TYPE: Creating, freeing and using pages; Creates a new page 

Creates a new page in the backing store, and returns a pointer to the BTreePage object managing the page.  New pages are marked modified when initially created, and do not require the use of \fBtouchPage:\fR.
*/

- (void) readPage: (BTreePage *) btreePage;
/* 
TYPE: Creating, freeing and using pages; Reads a page 

Reads the physical page managed by \fIbtreePage\fR and sets the \fIpageBuffer\fR.  This method is used to reload a page that has been cleared by the backing store.
*/

- (void) destroyPage: (BTreePage *) btreePage;
/* 
TYPE: Creating, freeing and using pages; Frees an open page

Frees the page managed by \fIbtreePage\fR.
*/

- (void) destroyPageAt: (vm_offset_t) pageOffset;
/* 
TYPE: Creating, freeing and using pages; Frees a page by offset

Frees the page which resides at offset \fIpageOffset\fR in the backing store.
*/

- (void) touchPage: (BTreePage *) btreePage;
/* 
TYPE: Creating, freeing and using pages; Prepares a page for modification

Called when the BTree intends to modify the contents of the page managed by \fIbtreePage\fR.  This method provides a convient opportunity for copying the original contents of the page for a shadowing integrity maintenance scheme.  In this case, the method may need to change the offset of the page.  After sending this message, the BTree class will examine the \fIpageOffset\fR instance variable to discover this fact.  This method may also change the contents of the \fIpageBuffer\fR instance variable under these circumstances.
*/

- (void) createPageGroup: (vm_offset_t *) pageOffset 
	count: (unsigned long *) pageCount;
/*
TYPE: Creating, freeing and using page groups; Creates a page group

Creates a group of \fI*pageCount\fR or fewer contiguous pages in the backing store.  Returns the offset of the first page in the group in \fIpageOffset\fR, and the number of pages created in \fIpageCount\fR.  Multiple invocations of this method may be necessary to fulfill a requested allocation.
*/

- (void) readPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount into: (vm_address_t) toAddress;
/*
TYPE: Creating, freeing and using page groups; Reads a page group

Maps a group of \fIpageCount\fR contiguous pages from offset \fIpageOffset\fR to the virtual memory location identified by \fItoAddress\fR.
*/

- (void) writePageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount from: (vm_address_t) fromAddress;
/*
TYPE: Creating, freeing and using page groups; Writes a page group

Writes a group of \fIpageCount\fR contiguous pages to offset \fIpageOffset\fR in the backing store from the virtual memory location identified by \fIfromAddress\fR.  Typically, this method should maintain a write back list, and should defer physical write operations until the list becomes large, or until required by the integrity maintenance strategy.
*/

- (void) destroyPageGroup: (vm_offset_t) pageOffset 
	count: (unsigned long) pageCount;
/*
TYPE: Creating, freeing and using page groups; Frees a page group

Frees the group of \fIpageCount\fR contiguous pages residing at offset \fIpageOffset\fR in the backing store.  
*/

- (void) save;
/* 
TYPE: Saving and undoing changes; Saves changes to permanent storage

Saves all changes made since the last call to \fBsave\fR, and commits them to permanent storage, if applicable.  The first call to \fBsave\fR saves all changes made since the BTree was opened.

This method may be safely omitted from the implementation if not applicable.
*/

- (void) undo;
/* 
TYPE: Saving and undoing changes; Undoes changes to permanent storage

Undoes all changes made since the last call to \fBsave\fR.  If \fBsave\fR has not been called since the BTree was opened, then \fBundo\fR undoes all changes made since the BTree was opened, if possible.

\fBundo\fR is used internally by the BTree and BTreeCursor classes to recover from errors encountered during operations that modify the BTree structure.
*/

- (void) bind;
/* 
Provides an opportunity for the backing store to perform any processing required following operations that modify the BTree.  Called before \fBsave\fR if autosaving is enabled.

This method may be safely omitted from the implementation if not applicable.
*/


@end


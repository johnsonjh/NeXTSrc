/*
BTreeStoreMemory.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield
 
DEFINED AS: A private class
HEADER FILES: BTreeStoreMemory.h
*/

#import	"BTreeStore.h"

typedef struct BTreePageEntryChain	{
	struct BTreePageEntry	*nextEntry;
	struct BTreePageEntry	*prevEntry;
} BTreePageEntryChain;

typedef struct BTreePageEntry	{
	BTreePage		*btreePage;
	vm_address_t		copyBuffer;	/* A copy of the original 
						contents of \fIpageBuffer\fR */
	struct BTreePageEntry	*nextEntry;
	struct BTreePageEntry	*prevEntry;
	BTreePageEntryChain	*entryChain;
} BTreePageEntry;

@interface BTreeStoreMemory : BTreeStore
/*
This class implements a BTree backing store in virtual memory. Note that when the store is freed, the contents of the store are lost, unless they have been saved to some other location via BTree's \fBwrite\fR method.
*/
{
@public
	unsigned char		saveEnabled;	// True if save enabled
	unsigned long		validPages;	// The number of BTreePages 				
						// currently in use
	unsigned long		totalPages;	// The number of BTreePages 
						// available in \fIpageTable\fR
	unsigned long		tableEntries;	// The number of entries 
						// in \fIpageTable\fR
	BTreePage		**pageTable;	// A table of pointers to 
						// allocated BTreePages
	BTreePageEntryChain	recycleChain;	// A linked list of entrys for
						// deallocated pages
	BTreePageEntryChain	destroyChain;	// A linked list of entrys for 
						// pages marked for destruction
	BTreePageEntryChain	discardChain;	// A linked list of entrys for 
						// pages marked modified
}

/* 
METHOD TYPES
Saving and undoing changes
*/

- (void) save;
/*
TYPE: Saving and undoing changes; Saves changes to permanent storage

Saves changes made since the last call to \fBsave\fR.  This class does not implement \fBbind\fR.
*/


@end


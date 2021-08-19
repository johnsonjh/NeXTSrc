/*
BTreeStoreFile.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield

DEFINED AS: A private class
HEADER FILES: BTreeStoreFile.h
*/

#import	"BTreeFile.h"
#import	"BTreeStore.h"

@interface BTreeStoreFile : BTreeStore
/*
The BTreeStoreFile class implements a BTreeStore for a single BTree 
residing in a BTreeFile.
*/
{
	void	*_storeCache;
}

/* 
METHOD TYPES
Creating and freeing instances
Saving and undoing changes
*/

+ newFile: (BTreeFile *) btreeFile 
	entry: (unsigned short) countEntry offset: (vm_offset_t) rootOffset;
/*
TYPE: Creating and freeing instances; Returns a BTreeStoreFile object

Creates and returns a BTreeStoreFile object for a store in \fIbtreeFile\fR.  \fIbtreeIndex\fR is the store's index in the count table in the file header, and \fIrootOffset\fR is the offset of the root page of the store.
*/

- free;
/*
TYPE: Creating and freeing instances; Frees the BTreeStoreFile object

Saves changes, frees the BTreeStoreFile object and notifies the BTreeFile through BTreeFile's semi-private \fB_btreeClosed:\fR method.
*/

- (unsigned) count;
/*
Returns the number of \fIkey/record\fR pairs in the BTree.  If the \fIcountEntry\fR of the store is less than the number of entries in the count table, this method returns the contents of \fIrecordCount\fR.  Otherwise, it calls BTree's private \fB_count\fR method to count the \fIkey/record\fR pairs.
*/

- (void) _save;

- (void) _undo;

- (void) save;
/* 
TYPE: Saving and undoing changes; Saves changes to permanent storage

Saves all changes made since the last call to \fBsave\fR, and commits them to permanent storage by synchronizing the virtual memory image of the file with the device on which the file resides.  The first call to \fBsave\fR saves all changes made since the BTree was opened, and ensures the integrity of the BTree in the event of error recovery during operations that modify the BTree.
*/

- (void) undo;
/* 
TYPE: Saving and undoing changes; Undoes changes to permanent storage

Undoes all changes made since the last call to \fBsave\fR.  If \fBsave\fR has not been called since the BTree was opened, \fBundo\fR does nothing.

\fBundo\fR is used internally by the BTree and BTreeCursor classes to recover from errors encountered during operations that modify the BTree structure.  If this occurs prior to the first call to \fBsave\fR, then the state of the BTree is not guaranteed to be valid, and the BTree may be rendered unuseable.
*/

- (void) bind;
/* 
Provides an opportunity for the backing store to perform post processing required following operations that modify the BTree.  Called before \fBsave\fR if autosaving is enabled.
*/


@end


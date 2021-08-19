/*
BTree.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield
 
DEFINED AS: A common class
HEADER FILES: BTree.h
*/

#import	"BTreeCursor.h"
#import	"BTreeErrors.h"

typedef int 
NXBTreeComparator(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareStrings(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareMonocaseStrings(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareBytes(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareShorts(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareLongs(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareUnsignedBytes(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareUnsignedShorts(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

extern int 
NXBTreeCompareUnsignedLongs(const void *data1, unsigned short length1, 
	const void *data2, unsigned short length2);

@interface BTree : Object
/*
The BTree class implements key based associative memories using the well-known B* tree algorithm.  By default, each BTree object manages a single B *tree that resides in virtual memory.

This class is used by the BTreeFile class to implement BTrees in files.  Other types of backing stores are also possible.  The BTreeCursor class defines objects that perform operations on BTrees.  The BTree class reports errors with NX_RAISE().

For a description of the B* tree algorithm and its properties, see \fIAlgorithms\fR by Sedgewick, or \fIThe Art of Computer Programming, Vol.  3/Sorting and Searching\fR, by Knuth.  Both sources provide additional references to the literature.
*/
{
	unsigned short		pageSize;	/* The size of 
						a backing store page */
	void			*backingStore;	/* A handle to 
						the backing store */
	NXBTreeComparator	*comparator;	/* The key comparator */
	unsigned long		btreeDepth;	/* The number of levels 
						in the tree */
	unsigned long		_syncVersion;
	unsigned long		_codeVersion;
}

/*
METHOD TYPES
Creating and freeing instances
Ordering the keys
Saving and undoing changes
Archiving
*/

+ new;
/*
TYPE: Creating and freeing instances; Returns a new BTree object 

Returns a new BTree object for an empty BTree residing in virtual memory.  To create a BTree in a file, use the BTreeFile class.

CF: + new (BTreeCursor), - createBTreeNamed: (BTreeFile)
*/

+ _newWith: (void *) private;

- free;
/*
TYPE: Creating and freeing instances; Frees the BTree object 

Saves changes, frees the storage occupied by the BTree object, and reclaims any storage used to cache the BTree in the backing store.
*/

- (unsigned) _count;

- (void) setComparator: (NXBTreeComparator *) comparator;
/*
TYPE: Ordering the keys; Sets the key comparator 

Sets the comparator used to order keys in the BTree.  The type used to declare \fIcomparator\fR is defined as follows.

.nf
typedef int 
NXBTreeComparator(const void *data1, unsigned short length1, 
.in +5
const void *data2, unsigned short length2);
.in -5
.fi

If this method is never called, \fBNXBTreeCompareStrings()\fR is used as the default comparator.  \fBNXBTreeCompareStrings()\fR performs case sensitive comparsions between strings, with or without null termination.

This class supplies the following comparators.

.nf
\fBNXBTreeCompareStrings()\fR
\fBNXBTreeCompareMonocaseStrings()\fR
\fBNXBTreeCompareBytes()\fR
\fBNXBTreeCompareUnsignedBytes()\fR
\fBNXBTreeCompareShorts()\fR
\fBNXBTreeCompareUnsignedShorts()\fR
\fBNXBTreeCompareLongs()\fR
\fBNXBTreeCompareUnsignedLongs()\fR
.fi

Each of the supplied comparators compares two arrays, each containing zero or more elements of the base type.  If the two arrays are identical in the length of the shorter, then the longer is considered the greater of the two.

CF: - comparator
*/

- (NXBTreeComparator *) comparator;
/*
TYPE: Ordering the keys; Returns the key comparator 

Returns the comparator used to order keys in the BTree.  The default comparator is \fBNXBTreeCompareStrings()\fR.

CF: - setComparator:
*/

- cursor;
/*
Returns a new BTreeCursor object for the receiver.  This method may be called more than once to obtain multiple cursors.  See the BTreeCursor class for more information on BTreeCursor objects.

CF: + new (BTreeCursor)
*/ 

- (unsigned) count;
/*
Returns the number of \fIkey/record\fR pairs in the BTree.  This method is much faster than enumerating and counting the \fIkey/record\fR pairs.
*/

- (void) empty;
/*
Empties the BTree.  All storage allocated to the BTree is freed, but the BTree object is not freed; the BTree is \fInot\fR removed from the backing store.  This method is much faster than enumerating and explicitly removing all of the records with BTreeCursor's \fBremove\fR method.

CF: - free, - removeBTreeNamed: (BTreeFile), - remove (BTreeCursor)
*/

- (void) save;
/*
TYPE: Saving and undoing changes; Saves changes

Saves all changes made since the last call to \fBsave\fR.  The first call to \fBsave\fR saves all changes made since the BTree was opened.

CF: - undo, - save (BTreeFile)
*/

- (void) undo;
/*
TYPE: Saving and undoing changes; Undoes changes

Undoes all changes made since the last call to \fBsave\fR.  If \fBsave\fR has not been called since the BTree was opened, then \fBundo\fR undoes all changes made since the BTree was opened, if possible.

\fBundo\fR is used internally by the BTree and BTreeCursor classes to recover from errors encountered during operations that modify the BTree structure.

CF: - save, - undo (BTreeFile)
*/

- read: (NXTypedStream *) stream;
/*
TYPE: Archiving; Reads a BTree description from a typed stream 

Empties the receiver if necessary, and then instantiates in the receiver a copy of the BTree described by \fIstream\fR.  The \fBread:\fR and \fBwrite:\fR methods should not be used as substitutes for a persistent BTree backing store like BTreeFile.

CF: - write:
*/

- write: (NXTypedStream *) stream;
/*
TYPE: Archiving; Writes a BTree description to a typed stream 

Writes a description of the BTree managed by the receiver to \fIstream\fR.  The \fBread:\fR method can use the description to instantiate a copy of the BTree.  The \fBread:\fR and \fBwrite:\fR methods should not be used as substitutes for a persistent BTree backing store like BTreeFile.

CF: - read:
*/


@end


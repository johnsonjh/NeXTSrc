/*
BTreeCursor.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield
 
DEFINED AS: A common class
HEADER FILES: BTree.h
*/

#import	<objc/Object.h>

@interface BTreeCursor : Object
/*
The BTreeCursor class defines objects that perform operations on BTrees.

A BTree is logically an ordered set of \fIkey/record\fR pairs, and a position in a BTree is defined by a specific key.  Each BTreeCursor records a position in a BTree; the key defining its position is stored in a per instance key buffer.  This means that two or more BTreeCursors may be opened on a BTree without difficulty.

The lengths of the keys stored in a BTree are limited by the page size of the backing store.  The maximum key length is returned by the \fBmaxKeyLength\fR method.  For efficiency, however, keys should be kept as small as possible.  In principle, the lengths of the records stored in a BTree are limited only by the resolution of a long unsigned integer.  In practice, record lengths are limited by the amount of available disk space. Records smaller than the virtual memory page size are copied; records larger than the page size are memory mapped.

For efficiency in working with large records, the BTreeCursor exports several methods for reading and writing ranges of bytes within records.  These methods read and write only the pages affected by the byte range operations.

The BTreeCursor class reports errors with NX_RAISE().
*/
{
	struct _cstatus		{
		unsigned	performSave:1;
		unsigned	traceNeeded:1;
		unsigned	modified:1;
		unsigned	firstPosition:1;
		unsigned	lastPosition:1;
		unsigned	spanning:1;
		unsigned	rightRotation:1;
		unsigned	leftRotation:1;
		unsigned	RESERVED:8;
	}			_cursorStatus;
	void			*keyBuffer;	/* The key buffer */
	unsigned short		bufferLength;	/* The current length 
						of the key buffer */
	unsigned short		maxKeyLength;	/* The maximum key length 
						for this BTree */
	unsigned short		keyLength;	/* The length of the key 
						in the key buffer */
	void			*_tracePrivate;
	id			btree;		/* The BTree object addressed 
						by this cursor */
	unsigned long		positionHint;	/* A hint for accelerating 
						key alignment */
	unsigned long		_syncVersion;
	unsigned long		_traceDepth;
}

/* 
METHOD TYPES
Creating and freeing instances
Setting the cursor position
Reading and writing keys and records
Reading and writing byte ranges
Using hints
*/

+ new;
/*
TYPE: Creating and freeing instances; Returns a new BTreeCursor object 

Returns a BTreeCursor object for a new BTree residing in virtual memory.  To obtain a BTreeCursor for any other type of BTree, use the \fBcursor\fR method of the BTree class.

CF: + new (BTree), - cursor (BTree)
*/

+ _newWith: (void *) private;

- (void) _setSpanning: (BOOL) spanning;

- (void) _setRightRotation: (BOOL) rightRotation;

- (void) _setLeftRotation: (BOOL) leftRotation;

- free;
/*
TYPE: Creating and freeing instances; Frees the BTreeCursor object 

Frees the storage occupied by the BTreeCursor object.
*/

- (void) setAutoSave: (BOOL) autoSave;
/*
Enables or disables automatic saving after every modification. Automatic saving may be enforced by the backing store.

CF: - autoSave
*/

- (BOOL) autoSave;
/*
Returns boolean true if automatic saving is enabled. Automatic saving may be enforced by the backing store.

CF: - setAutoSave:
*/

- btree;
/*
Returns a pointer to the BTree addressed by this BTreeCursor.
*/

- (unsigned short) maxKeyLength;
/*
Returns the maximum key length for the BTree addressed by the cursor.  This  is computed as the page size of the backing store less per page overhead.  For maximum efficiency, keys should be kept as small as possible.
*/

- (void) setKey: (void *) data length: (unsigned short) dataLength;
/*
TYPE: Setting the cursor position; Sets the cursor position

Sets the cursor position to the key defined by \fIdata\fR and \fIdataLength\fR.  The key is blind data; key ordering is determined by a user supplied key comparator.  See the BTree class for more information on the key comparator.

CF: - setKey:length:hint
*/

- (BOOL) getKey: (void **) data length: (unsigned short *) dataLength 
	valid: (BOOL *) validFlag;
/*
TYPE: Reading and writing; Reads the cursor position

Aligns the cursor with the \fIkey/record\fR pair at the current cursor position.  If there is no \fIkey/record\fR pair at the current cursor position, this method aligns the cursor with the \fIkey/record\fR pair immediately following the current cursor position, and repositions the cursor to match the alignment.

This method then returns the key defining the cursor position in \fI*data\fR and \fI*dataLength\fR, and returns boolean false in \fI*validFlag\fR when the cursor is beyond the last \fIkey/record\fR pair in the BTree.  The return value is boolean true if the cursor postiion did not change.

The pointer returned in \fI*data\fR points to the per instance key buffer; it is not guaranteed to remain valid beyond subsequent messages, since the buffer may be moved by \fBrealloc(3)\fR.
*/

- (BOOL) setFirst;
/*
TYPE: Setting the cursor position; Sets the first position

If there is at least one \fIkey/record\fR pair in the BTree, this method returns true and positions the cursor at the first \fIkey/record\fR pair.
*/

- (void) setEnd;
/*
TYPE: Setting the cursor position; Sets the last position

Positions the cursor at the end of the BTree, \fIbeyond\fR the last \fIkey/record\fR pair.  A subsequent \fBsetPrevious\fR message will position the cursor \fIat\fR the last \fIkey/record\fR pair in the BTree.
*/

- (BOOL) setPrevious;
/*
TYPE: Setting the cursor position; Sets the previous position

Positions the cursor at the \fIkey/record\fR pair immediately preceding the current cursor position, and returns boolean false when the cursor is already at the first \fIkey/record\fR pair in the BTree.
*/

- (BOOL) setNext;
/*
TYPE: Setting the cursor position; Sets the next position

Positions the cursor at the \fIkey/record\fR pair immediately following the current cursor position, and returns boolean false when the cursor moves beyond the last \fIkey/record\fR pair in the BTree.
*/

- (BOOL) read: (void **) data length: (unsigned long *) dataLength
	valid: (BOOL *) validFlag;
/*
TYPE: Reading and writing; Reads the record at the cursor 

Aligns the cursor with the \fIkey/record\fR pair at the current cursor position.  If there is no \fIkey/record\fR pair at the current cursor position, this method aligns the cursor with the \fIkey/record\fR pair immediately following the current cursor position, and repositions the cursor to match the alignment.

This method then reads the record at the cursor position into \fI*data\fR and \fI*dataLength\fR, and returns boolean false in \fI*validFlag\fR when the cursor is beyond the last \fIkey/record\fR pair in the BTree.  The return value is boolean true if the cursor postiion did not change.

If both \fIdata\fR and \fI*data\fR are non-zero, then \fI**data\fR must be a valid memory location, and \fI*dataLength\fR must describe the number of bytes of storage available at \fI**data\fR.  Up to \fI*dataLength\fR bytes of the record are then read or mapped into \fI**data\fR.

If \fIdata\fR is non-zero and \fI*data\fR is zero, then a buffer large enough to hold the record is allocated with \fBmalloc(3)\fR.  The entire record is then read or mapped into the buffer, and the record length is returned in \fI*dataLength\fR.  The address of the buffer is returned in \fI*data\fR.  The sender is responsible for freeing the buffer with \fBfree(3)\fR.

If \fIdata\fR is zero, then the record length is returned in \fI*dataLength\fR.
*/

- (BOOL) insert: (void *) data length: (unsigned long) dataLength;
/*
TYPE: Reading and writing; Inserts a record at the cursor 

If there is no \fIkey/record\fR pair at the cursor position, this method inserts a new \fIkey/record\fR pair into the BTree at the cursor position.  The record is defined by \fIdata\fR and \fIdataLength\fR; \fIdataLength\fR may be zero.  This method returns boolean true if the insertion was performed.
*/

- (void) replace: (void *) data length: (unsigned long) dataLength;
/*
TYPE: Reading and writing; Replaces the record at the cursor 

Replaces the record at the cursor position using \fIdata\fR and \fIdataLength\fR.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.
*/

- (void) remove;
/*
TYPE: Reading and writing; Removes the record at the cursor 

Removes the \fIkey/record\fR pair at the cursor position.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.
*/

- (void) append: (void *) data length: (unsigned long) dataLength;
/*
TYPE: Reading and writing byte ranges; Appends a byte range

Appends the range of bytes defined by \fIdata\fR and \fIdataLength\fR to the record at the cursor position.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.

CF: - insert:length:at:
*/

- (void) insert: (void *) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset;
/*
TYPE: Reading and writing byte ranges; Inserts a byte range

Inserts the range of bytes defined by \fIdata\fR and \fIdataLength\fR at \fIbyteOffset\fR in the record at the cursor position.  The portion of the record above \fIbyteOffset\fR is logically shifted to the right.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.
*/

- (void) read: (void **) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset;
/*
TYPE: Reading and writing byte ranges; Reads a byte range

Reads the range of bytes defined by \fIbyteOffset\fR and \fIdataLength\fR from the record at the cursor position into \fI*data\fR.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.

If both \fIdata\fR and \fI*data\fR are non-zero, then \fI**data\fR must be a the starting address of a valid region of memory at least \fIdataLength\fR bytes long.  \fIdataLength\fR bytes of the record beginning at \fIbyteOffset\fR are then read or mapped into \fI**data\fR.

If \fIdata\fR is non-zero and \fI*data\fR is zero, then a buffer at least \fIdataLength\fR bytes long is allocated with \fBmalloc(3)\fR.  \fIdataLength\fR bytes of the record beginning at \fIbyteOffset\fR are then read or mapped into the buffer.  The address of the buffer is returned in \fI*data\fR.  The sender is responsible for freeing the buffer with \fBfree(3)\fR.

If \fIdata\fR is zero, this method raises an exception.
*/

- (void) remove: (unsigned long) dataLength at: (unsigned long) byteOffset;
/*
TYPE: Reading and writing byte ranges; Removes a byte range

Removes the range of bytes defined by \fIbyteOffset\fR and \fIdataLength\fR from the record at the cursor position.  The portion of the record above \fIbyteOffset\fR is logically shifted to the left.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.
*/

- (void) replace: (void *) data length: (unsigned long) dataLength 
	at: (unsigned long) byteOffset;
/*
TYPE: Reading and writing byte ranges; Replaces a byte range

Replaces the range of bytes defined by \fIbyteOffset\fR and \fIdataLength\fR in the record at the cursor position with \fIdata\fR.  If the range overruns the end of the record, the excess is silently appended.  If there is no \fIkey/record\fR pair at the cursor position, this method raises an exception.
*/

- (unsigned long) hint;
/*
TYPE: Using hints; Returns a position hint 

Returns a hint describing the cursor position.  The hint may be used with the \fBsetKey:length:hint:\fR method to accelerate the key alignment operations performed by \fBgetKey:length:valid:\fR and \fBread:length:valid:\fR.

CF: - setKey:length:hint
*/

- (void) setKey: (void *) data length: (unsigned short) dataLength 
	hint: (unsigned long) aHint;
/*
TYPE: Using hints; Sets the cursor position with a position hint 

Similar to \fBsetKey:length:\fR, but accepts a hint returned by \fBhint\fR.  For relatively static BTrees, the use of hints will significantly reduce the average running time of the key alignment operation performed by \fBgetKey:length:valid:\fR and \fBread:length:valid:\fR.  Hints become less effective as BTrees are modified.  The use of hints may actually increase the average running time of the key alignment operation for highly dynamic BTrees.

Hints are especially valuable for improving the performance of secondary key resolution for relatively static BTrees.  Retrieve the hint for each primary \fIkey/record\fR pair after every \fBgetKey:length:valid:\fR, \fBread:length:valid:\fR or \fBinsert:length:\fR message sent to the primary BTree.  If the hint has changed, store the new hint in the secondary record.  At retrieval time, use this method instead of \fBsetKey:length:\fR.

CF: - setKey:length:
*/


@end


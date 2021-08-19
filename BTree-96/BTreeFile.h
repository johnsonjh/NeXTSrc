/*
BTreeFile.h
Copyright 1990, NeXT, Inc.
Responsibility: Jack Greenfield
 
DEFINED AS: A common class
HEADER FILES: BTreeFile.h
*/

#import	"BTree.h"
#import	<objc/hashtable.h>

extern const char *NXBTreeFileExtension;
extern const char *NXBTreeFileDirectory;

@interface BTreeFile : Object
/*
Using the BTree class, the BTreeFile class implements one or more BTrees in a single Unix file.  The implementation of the BTree has been separated from the implementation of its backing store to allow for alternative backing store implementations.  The BTreeFile class is an example of an alternative backing store implementation.

Every BTreeFile maintains a single default BTree; the \fBopenBTree\fR method opens the default BTree.  The BTreeFile class can also manage additional, named BTrees.  The maximum number of BTrees supported by a file is determined by the virtual memory page size in effect when the file was created.  The BTreeFile class reports errors with the NX_RAISE().
*/
{
	char		*fileName;		/* The name of the file */
	void		*_backingStore;
	NXHashTable	*_btreesByStore;
	NXHashTable	*btreesByName;		/* A table containing the names 
						of the BTrees in the file */
	id		defaultBTree;		/* The BTree object managing 
						the default BTree */
}

/* 
METHOD TYPES
Creating and freeing instances
Managing BTrees
Saving and undoing changes
Handling unexpected process termination
Restarting
*/

+ new;
/*
TYPE: Creating and freeing instances; Creates a temporary file

Creates a BTree backing store in a temporary file named by \fBmktemp(3)\fR, and returns a BTreeFile object.  If the file cannot be created, this method raises an exception.  In the event of unexpected process termination, the temporary file will be destroyed by the boot process.  The temporary file is NOT removed when the BTreeFile object is freed.
*/

+ newFile: (const char *) fileName;
/*
TYPE: Creating and freeing instances; Creates a named file 

Creates a BTree backing store in \fIfileName\fR, and returns a BTreeFile object.  If \fIfileName\fR already exists, or if it cannot be created, this method raises an exception.  If the process is terminated unexpectedly while the file is open for writing, the file may be left in an unrecoverable state.
*/

+ newFile: (const char *) fileName isInstant: (BOOL) instantFlag 
	canUndo: (BOOL) undoFlag;
/*
TYPE: Creating and freeing instances; Creates a recoverable file 

Creates a recoverable BTree backing store in \fIfileName\fR.  Files created by this method are guaranteed to be recoverable if the process is terminated unexpectedly.  This guarantee is achieved at a modest performance cost.

If \fIinstantFlag\fR is true, the file is guaranteed to be instantly recoverable; an explicit recovery operation will not be required before the file may be successfully opened for writing following unexpected process termination.  The guarantee of instant recovery incurs a more substantial performance cost, but avoids the possibility of a lengthy recovery following unexpected process termination.

If \fIundoFlag\fR is true, the file will support basic transaction management through the \fBsave\fR and \fBundo\fR methods.  In combination with instant recovery, this provides instant roll forward and backward capability through the \fBrestartFrom:\fR method.

If the file is created successfully, this method attempts to retain exclusive access to the file with \fBflock(2)\fR.
*/

+ openFile: (const char *) fileName forWriting: (BOOL) writingFlag;
/*
TYPE: Creating and freeing instances; Opens a named file 

Opens \fIfileName\fR, and returns a BTreeFile object.  If \fIwritingFlag\fR is true, \fIfileName\fR is opened for reading and writing, and the method attempts to secure exclusive access to \fIfileName\fR with \fBflock(2)\fR.  Otherwise, \fIfileName\fR is opened for reading only, and the method attempts to secure shared access to \fIfileName\fR with \fBflock(2)\fR.

If \fIfileName\fR does not exist, or if it cannot be accessed, or if it does not contain a BTree backing store, this method raises an exception.  This method will also raise an exception if \fIwritingFlag\fR is true, and \fIfileName\fR is internally inconsistent and not instantly recoverable, or if \fIwritingFlag\fR is true, and \fIfileName\fR is internally inconsistent and not recoverable.  If \fIfileName\fR is recoverable, the \fBrecover:\fR method may be used to recover its contents.
*/

+ (void) recoverFile: (const char *) fileName;
/*
TYPE: Handling unexpected process termination; Recovers from unexpected process termination 

Recovers the contents of \fIfileName\fR.  This method raises an exception if \fIfileName\fR does not exist, or if it cannot be accessed, or if it does not contain a BTree backing store, or if it is internally inconsistent and not recoverable.  This method attempts to secure exclusive access to \fIfileName\fR with \fBflock(2)\fR.
*/

+ (void) compressFile: (const char *) fileName;
/*
Removes all unused space from \fIfileName\fR.  This method should not be used with files which are frequently modified, since it will reduce update performance.  The space savings provided by this method may exceed 35 per cent.  This method may take a long time to run.
*/

- _btreeByStore: (void *) private;

- (void) _btreeClosed: (void *) private;

- free;
/*
TYPE: Creating and freeing instances; Returns a new BTreeCursor object 

Saves changes, closes the file, and frees the BTreeFile object.
*/

- (const char *) fileName;
/*
Returns the name of the file associated with the BTreeFile object.
*/

- (BTree *) openBTree;
/*
TYPE: Managing BTrees; Returns a BTree object for the default BTree

Returns a BTree object for the default BTree.  This method returns the same BTree object every time it is called.  To dispose of the BTree object and close the BTree, use BTree's \fBfree\fR method.
*/

- (BTree *) openBTreeNamed: (const char *) btreeName;
/*
TYPE: Managing BTrees; Opens the BTree named \fIbtreeName\fR

Opens and returns a BTree object for the BTree named \fIbtreeName\fR.  This method raises an exception if the file does not contain a BTree named \fIbtreeName\fR.  This method returns the same BTree object every time it is called with the same name.  To dispose of the BTree object and close the BTree, use BTree's \fBfree\fR method.
*/

- (BTree *) createBTreeNamed: (const char *) btreeName;
/*
TYPE: Managing BTrees; Creates and opens a BTree named \fIbtreeName\fR 

Creates and returns a BTree object for a BTree named \fIbtreeName\fR.  This method raises an exception if there is already a BTree named \fIbtreeName\fR in the file.  To dispose of the BTree object and close the BTree, use BTree's \fBfree\fR method.
*/

- (void) removeBTreeNamed: (const char *) btreeName;
/*
TYPE: Managing BTrees; Destroys the BTree named \fIbtreeName\fR 

Removes the BTree named \fIbtreeName\fR from the file.  This method raises an exception if the BTree named \fIbtreeName\fR is in use, or if the file does not contain a BTree named \fIbtreeName\fR.
*/

- (char **) contents;
/*
TYPE: Managing BTrees; Returns a null-terminated list of BTree names 

Returns a null-terminated array of pointers to strings containing the names of all of the named BTrees in the file.  The caller is responsible for freeing the array and its contents with \fBfree(3)\fR.
*/

- (void) save;
/*
TYPE: Saving and undoing changes; Saves changes to disk 

Saves changes to disk.  On return from this method, the file is guaranteed to be internally consistent.  If the file supports transaction management, this method commits the outstanding transaction, and begins a new one.

CF: - canUndo, undo
*/

- (BOOL) canUndo;
/*
TYPE: Saving and undoing changes; Returns a new BTreeCursor object 

Returns boolean true if the file supports transaction management.  A file supports transaction management if it was created with the \fBnew:isInstant:canUndo:\fR method, with the \fIcanUndo\fR argument true.

CF: - save, undo
*/

- (void) undo;
/*
TYPE: Saving and undoing changes; Returns a new BTreeCursor object 

If the file supports transaction management, this method aborts any outstanding transaction, and returns the file to the state created by the last \fBsave\fR message, or to the state in which it was opened or created, if \fBsave\fR has not been performed.  If the file does not support transaction management, this method raises an exception.

CF: - canUndo, save
*/

- (BOOL) isRestartable;
/*
TYPE: Restarting; Returns boolean true if the BTreeFile is restartable 

Returns boolean true if the BTreeFile is restartable.  A file is restartable if it was created with the \fBnew:isInstant:canUndo:\fR method, with both boolean arguments true.

CF: - discardPriorTo:, - restartFrom:
*/

- (void) restartFrom: (struct timeval) startTime;
/*
TYPE: Restarting; Returns a new BTreeCursor object 

If the BTreeFile is restartable, this method restarts it instantly from \fIstartTime\fR.  \fIstartTime\fR is given as a \fIstruct timeval\fR describing a Greenwich mean time, as returned by \fBgettimeofday(2)\fR.  This method does not permanently alter the file, and is much faster than permanently discarding file history with \fBdiscardPriorTo:\fR.  If the BTreeFile is not restartable, this method raises an exception.

CF: - isRestartable
*/

- (void) discardPriorTo: (struct timeval) threshhold;
/*
TYPE: Restarting; Discards file history older than \fIthreshhold\fR 

If the BTreeFile is restartable, this method discards file history older than \fIthreshhold\fR.  This involves a single pass over the entire file, and may be very time consuming for large files.  \fIthreshhold\fR is given as a \fIstruct timeval\fR describing a Greenwich mean time, as returned by \fBgettimeofday(2)\fR.  If the BTreeFile is not restartable, this method raises an exception.

CF: - restartFrom:, - isRestartable
*/


@end


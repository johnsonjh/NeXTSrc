/*
    typedstream.h
    Copyright 1989 NeXT, Inc.
*/


/*
 *	This modules saves data structures for later retrieval.  You can use typedstreams to save arbitrary C data, including Objective C objects, onto an arbitrary stream (see \fBNXStream\fR), such as a file, memory buffer, or pasteboard buffer.  The process of saving data is calling archiving, the process of retrieving the data unarchiving.
 *
 *	Not only data structures are written, but also their type, allowing for retrieval even in the absence of the code used for saving them.  Objective C objects are stored along with the full class hierarchy, allowing for translation of old classes into new classes.
 *	At writing time, a tight encoding is possible, because all the type information is present.  The encoding is not too tight, to allow for fast reading.  A check is done when reading data to confirm conformance of the stored data to the expected data.
 *	Any Objective C class can define its own write: and read: methods.  By default, nothing is saved. 
 *	Three types of versions are supported to allow for code evolution, not revolutions: typedstream versioning (used by the implementation), system versioning (used by NeXT), per class versioning (available for clients).
 *	Shared objects as well as shared C pointers remain shared after reading.
 *	Data structures to be saved can be carved at the level of their backpointers.
 *
 */
  

#import <streams/streams.h>
#import "objc.h"
#import <zone.h>

typedef void NXTypedStream;	/* NXTypedStream is made opaque */


/***********************************************************************
 *	Creation, destruction and other global operations
 **********************************************************************/

/*
 *	When dealing with streams, it is important to keep in mind that the creator of a stream is responsible for destroying it after destroying the NXTypedStream.  In particular, NXTypedStreams do not destroy the underlying stream when they are closed, and only the NXTypedStream data structure is released.  One exception to this scheme, however, has been provided for convenience with NXOpenTypedStreamForFile.
 *
 */
 
extern NXTypedStream *NXOpenTypedStream (NXStream *physical, int mode);
	/* If mode is NX_WRITEONLY, creates a NXTypedStream, ready for writing, given a physical stream on which to actually put the bytes. If mode is NX_READONLY, creates a NXTypedStream, ready for reading, given a physical stream on which to actually get the bytes.  The caller is responsible for closing physical.  If the file format mismatches right from the start with typedstream format, NULL is returned, otherwise an exception might be raised.  */
	
extern void NXSetTypedStreamZone(NXTypedStream *stream, NXZone *zone);
extern NXZone *NXGetTypedStreamZone(NXTypedStream *stream);

extern BOOL NXEndOfTypedStream (NXTypedStream *stream);
	/* For a stream opened for reading indicates whether or not more data follows. */

extern void NXFlushTypedStream (NXTypedStream *stream);
	/* For a stream opened for writing, flushes the underlying physical stream. */

extern void NXCloseTypedStream (NXTypedStream *stream);
	/* Flushes underlying stream and frees stream data structures.  Physical stream is not destroyed, except if stream was created with NXOpenTypedStreamForFile.  */


/***********************************************************************
 *	Writing and reading arbitrary data
 **********************************************************************/

/*
 *	The type of the data written follows the Objective C type string conventions <<Refer to Obj C documentation>>.  Supported descriptors:
 *		'c': char
 *		's': short
 *		'i': int
 *		'f': float
 *		'd': double
 *		'@': id
 *		'*': char *
 *		'%': NXAtom
 *		':': SEL
 *		'#': Class
 *		'!': int ignored
 *		{<type>} : struct
 *		[<count><itemType>] : array
 *
 *	Example, "{csi*@}" means a struct containing a char, a short, an int, a string and an object.  Type descriptor '%' implies at read time that the string is uniqued (using \fBNXUniqueString()\fR).
 *
 *	Objective C objects are written by sending write: to them, and read by sending read:.
 *	Write methods should look like:
 *	- write: (NXTypedStream *) s {
 *		[super write s];
 *		...code for writing instance variables defined at this class level ...
 *		};
 *
 *	Read methods should look like:
 *	- read: (NXTypedStream *) s {
 *		[super read s];
 *		...code for writing instance variables defined at this class level ...
 *		};
 *
 *	For being able to convert old files, it is critical that write: methods always perform [super write: s] before writing any other data.  In a similar fashion, read: methods must always perform [super read: s] before reading any other data.
 *	The current implementation performs write: methods twice, so no side effect should be performed in a write:. 
 *
 *	After reading an object, an awake message is performed, followed by an finishUnarchiving.  The purpose of the awake message is to give a chance to establish invariants within the object, and the purpose of startArchiving: (resp. finishUnarchiving) is to give a chance to replace the object to be written (resp. just read) by some other object.
 *
 */

extern void NXWriteType (NXTypedStream *stream, const char *type, const void *data);
	/* data specifies the address of the types to write. */
	
extern void NXReadType (NXTypedStream *stream, const char *type, void *data);
	/* data specifies the address of the types to read.  Expected type is checked against the type actually present on the stream. */


extern void NXWriteTypes (NXTypedStream *stream, const char *type, ...);
	/* Last arguments specify addresses of values to be written.  It might seem surprising to specify values by address, but this is extremely convenient for copy-paste with NXReadTypes calls.  A more down-to-the-earth cause for this passing of addresses is that values of arbitrary size is not well supported in ANSI C for functions with variable number of arguments. */
	
extern void NXReadTypes (NXTypedStream *stream, const char *type, ...);
	/* Last arguments specify addresses of values to be read.  Expected type is checked against the type actually present on the stream. */


/***********************************************************************
 *	Conveniences for writing and reading common types of data.
 **********************************************************************/
 
extern void NXWriteArray (NXTypedStream *stream, const char *itemType, int count, const void *data);
	/* Expects data to be an array of count elements of type itemType.  Writes the type specification and the array contents onto the stream.  Equivalent to NXWriteType (stream, "[<count><itemType>]", data).  Example of use: NXWriteArray (stream, "c", 99, data) writes 99 bytes of data. */
	
extern void NXReadArray (NXTypedStream *stream, const char *itemType, int count, void *data);
	/* Expects data to be a previously allocated array of count elements of type itemType. */


extern void NXWriteObject (NXTypedStream *stream, id object);
	/* Equivalent to NXWriteTypes (stream, "@", &object) */
extern id NXReadObject (NXTypedStream *stream);
	
/***********************************************************************
 *	Writing and reading back pointers.
 **********************************************************************/

/*
 *	Backpointers within the saved part of the data structure are stored, while backpointers pointing to outside the saved data structure are not stored.  To specify that a data structure might contain such backpointers, NXWriteRootObject must be called. 
 */

extern void NXWriteRootObject (NXTypedStream *stream, id object);
	/* Specifies that a data structure might contain backpointers, and writes it as with NXWriteObject.  This function cannot be called recursively.  The physical stream is flushed after object is written.  The implementation works in 2 passes, which implies that write: methods will be performed twice. */

extern void NXWriteObjectReference (NXTypedStream *stream, id object);
	/* Indicates that object is a back pointer, and writes it as with NXWriteObject or writes nil depending on whether object is written by an unconditional NXWriteObject otherwise.  Implies a previous call to NXWriteRootObject. */


/***********************************************************************
 *	Conveniences for writing and reading files and buffers.
 **********************************************************************/
 
extern NXTypedStream *NXOpenTypedStreamForFile (const char *fileName, int mode);
	/* Opens a file named fileName, and associates it with the NXTypedStream. mode should be NX_READONLY or NX_WRITEONLY.  On closing with NXCloseTypedStream, all buffers and file descriptors are released.  In case of opening error, NULL is returned.  */

extern char *NXWriteRootObjectToBuffer (id object, int *length);
	/* Creates a new memory buffer, a corresponding NXTypedStream, writes object using NXWriteRootObject, frees the NXTypedStream, and returns that buffer as well as the number of bytes written.  Use NXFreeObjectBuffer to free buffer. */

extern id NXReadObjectFromBuffer (const char *buffer, int length);
extern id NXReadObjectFromBufferWithZone (const char *buffer, int length, NXZone *zone);
	/* Creates a reading NXTypedStream, reads using NXReadObject, frees the NXTypedStream and returns the object.  buffer is not freed. */

extern void NXFreeObjectBuffer (char *buffer, int length);
	/* Frees buffer previously created by NXWriteRootObjectToBuffer. */


/***********************************************************************
 *	Dealing with versions
 **********************************************************************/

/*
 *	Three types of versions are supported: typedstream versions (used internally), system versions (accessible when reading a stream), class versions (available for clients).
 *
 */

#define NXSYSTEMVERSION082	82
#define NXSYSTEMVERSION083	83
#define NXSYSTEMVERSION090	90
#define NXSYSTEMVERSION0900	900
#define NXSYSTEMVERSION0901	901
#define NXSYSTEMVERSION0905	905
#define NXSYSTEMVERSION0930	930
#define NXSYSTEMVERSION		NXSYSTEMVERSION0930

extern int NXSystemVersion (NXTypedStream *stream);
	/* Returns the version used for writing the stream.  Only applicable to reading streams. */

extern int NXTypedStreamClassVersion (NXTypedStream *stream, const char *className);

/***********************************************************************
 *	Dealing with errors
 **********************************************************************/

/* several exceptions can occur; the format of all exceptions raised is a label, a message string and maybe some extra information */

#define TYPEDSTREAM_ERROR_RBASE 8000

enum TypedstreamErrors {
    TYPEDSTREAM_CALLER_ERROR = TYPEDSTREAM_ERROR_RBASE,
    TYPEDSTREAM_FILE_INCONSISTENCY,
    TYPEDSTREAM_CLASS_ERROR,
    TYPEDSTREAM_TYPE_DESCRIPTOR_ERROR,
    TYPEDSTREAM_WRITE_REFERENCE_ERROR,
    TYPEDSTREAM_INTERNAL_ERROR
};





#ifndef _BTREEERRORS_H_
#define _BTREEERRORS_H_

#import <objc/error.h>

#define NX_BTREEUSERERRBASE	(9000)
#define NX_BTREEMACHERRBASE	NX_BTREEUSERERRBASE + (300)
#define NX_BTREEUNIXERRBASE	NX_BTREEUSERERRBASE + (600)

typedef enum BTreeError	{
	NX_BTreeNoError = NX_BTREEUSERERRBASE, 
	NX_BTreeStoreEmpty, 
	NX_BTreeStoreFull, 
	NX_BTreeReadOnlyStore, 
	NX_BTreeDuplicateName, 
	NX_BTreeRecordNotMapped, 
	NX_BTreeInvalidCursor, 
	NX_BTreeNoMemory, 
	NX_BTreeRecordTooLarge, 
	NX_BTreeKeyNotFound, 
	NX_BTreeInvalidArguments, 
	NX_BTreeFileLocked, 
	NX_BTreeFileInconsistent, 
	NX_BTreeInvalidVersion, 
	NX_BTreeInternalError, 
	NX_BTreeUnixError = NX_BTREEUNIXERRBASE, 
	NX_BTreeMachError = NX_BTREEMACHERRBASE
} BTreeError;

#endif _BTREEERRORS_H_


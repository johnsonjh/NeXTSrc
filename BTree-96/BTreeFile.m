
#ifdef	SHLIB
#import	"shlib.h"
#endif


#import	<sys/time_stamp.h>
#import	"BTreeFilePrivate.h"

static int _NXBTreeFileVersion = 1;

static BTreeFileBTreeHandle 
*_btreeFileOpenBTree(BTreeFileTemplate *btreeFile, const char *btreeName);
static BTreeFileBTreeHandle 
*_btreeFileCreateBTree(BTreeFileTemplate *btreeFile, const char *btreeName);
static void 
_btreeFileRemoveBTree(BTreeFileTemplate *btreeFile, const char *btreeName);

/* 
routines for maintaining the table of contents
*/

static void 
_btreeFileDirectoryInsert(BTreeFileTemplate *btreeFile, 
	const char *btreeName, BTreeFileBTreeRecord *btreeRecord)
{
	BOOL			matchFlag;
	BOOL			validFlag;
	NXHandler		jumpHandler;
	unsigned short		keyLength;
	unsigned short		recLength;
	void			*keyValue;
	BTreeFileBTreeHandle	*btreeHandle;
	BTreeCursor		*btreeCursor;

	keyLength = 1 + strlen(btreeName);
	btreeHandle = _btreeFileOpenBTree(btreeFile, NXBTreeFileDirectory);
	btreeCursor = [btreeHandle->btree cursor];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeCursor free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		[btreeCursor setKey: (void *) btreeName length: keyLength];
		matchFlag = [btreeCursor getKey: &keyValue 
			length: &recLength valid: &validFlag];
		if (matchFlag)
			_NXRaiseError(NX_BTreeDuplicateName, 
				"_btreeFileDirectoryInsert", 0);
	
		recLength = sizeof(BTreeFileBTreeRecord);
		[btreeCursor setKey: (void *) btreeName length: keyLength];
		if (! [btreeCursor insert: btreeRecord length: recLength])
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeFileDirectoryInsert", 0);
	
		_NXRemoveHandler(&jumpHandler);
	}

	[btreeCursor free];
}

static BTreeFileBTreeRecord 
*_btreeFileDirectoryRead(BTreeFileTemplate *btreeFile, const char *btreeName)
{
	NXHandler		jumpHandler;
	BOOL			matchFlag;
	BOOL			validFlag;
	unsigned long		recLength;
	BTreeFileBTreeRecord	*btreeRecord;
	BTreeCursor		*btreeCursor;
	BTreeFileBTreeHandle	*btreeHandle;

	btreeHandle = _btreeFileOpenBTree(btreeFile, NXBTreeFileDirectory);
	btreeCursor = [btreeHandle->btree cursor];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeCursor free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		recLength = sizeof(BTreeFileBTreeRecord);
		btreeRecord = (BTreeFileBTreeRecord *) 0;
		[btreeCursor setKey: (void *) btreeName 
			length: 1 + strlen(btreeName)];
		matchFlag = [btreeCursor read: (void **) &btreeRecord 
			length: &recLength valid: &validFlag];
		if (! matchFlag)
		{
			free(btreeRecord);
			_NXRaiseError(NX_BTreeKeyNotFound, 
				"_btreeFileDirectoryRead", 0);
		}

		_NXRemoveHandler(&jumpHandler);
	}

	[btreeCursor free];
	return btreeRecord;
}

static void 
_btreeFileDirectoryRemove(BTreeFileTemplate *btreeFile, const char *btreeName)
{
	NXHandler		jumpHandler;
	BTreeCursor		*btreeCursor;
	BTreeFileBTreeHandle	*btreeHandle;

	btreeHandle = _btreeFileOpenBTree(btreeFile, NXBTreeFileDirectory);
	btreeCursor = [btreeHandle->btree cursor];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeCursor free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		[btreeCursor setKey: (void *) btreeName 
			length: 1 + strlen(btreeName)];
		[btreeCursor remove];
		_NXRemoveHandler(&jumpHandler);
	}

	[btreeCursor free];
}

/* 
routines for checking names 
*/

inline static void 
_btreeFileCheckBTreeName(const char *btreeName, const char *methodName)
{
	if (! btreeName || ! *btreeName || 
			! strcmp(btreeName, NXBTreeFileDirectory))
		_NXRaiseError(NX_BTreeInvalidArguments, methodName, 0);
}

static char 
*_btreeFileBuildFileName(const char *fileName)
{
	int	mallocSize;
	char	*resultName;
	char	*dotAddress;

	mallocSize = 2 + strlen(fileName) + strlen(NXBTreeFileExtension);
	resultName = malloc(mallocSize);
	if (! resultName)
		_NXRaiseError(NX_BTreeNoMemory, "_btreeFileBuildFileName", 0);

	strcpy(resultName, fileName);
	dotAddress = strrchr(resultName, '.');
	if (! dotAddress || strcmp(&dotAddress[1], NXBTreeFileExtension))
		strcat(strcat(resultName, "."), NXBTreeFileExtension);

	return resultName;
}

/*
routines for initializing the cache
*/

static char 
*_btreeFileOpenFile(BTreeFileCache *fileCache, 
	char *fileName, int openFlags, int lockType)
{
	fileCache->fileDescriptor = open(fileName, openFlags | O_NDELAY, 0666);
	if (fileCache->fileDescriptor < 0)
	{
		fileName = _btreeFileBuildFileName(fileName);
		fileCache->fileDescriptor = 
			open(fileName, openFlags | O_NDELAY, 0666);
		if (fileCache->fileDescriptor < 0)
		{
			free((void *) fileName);
			_NXRaiseError(NX_BTreeUnixError + errno, 
				"_btreeFileOpenFile", strerror(errno));
		}
	} else
		fileName = strcpy(malloc(1 + strlen(fileName)), fileName);

	if (flock(fileCache->fileDescriptor, lockType | LOCK_NB) < 0)
	{
		free((void *) fileName);
		_NXRaiseError(NX_BTreeFileLocked, 
			"_btreeFileOpenFile", fileName);
	}

	return fileName;
}

inline static BTreeFileCache 
*_btreeFileMakeCache(BTreeFileTemplate *btreeFile)
{
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) calloc(sizeof(BTreeFileCache), 1);
	if (! fileCache)
		_NXRaiseError(NX_BTreeNoMemory, "_btreeFileMakeCache", 0);

	btreeFile->_backingStore = (void *) fileCache;
	return fileCache;
}

/*
routines for opening or initializing the file header
*/

inline static void 
_btreeFileInitHeader(BTreeFileCache *fileCache, 
	boolean_t recoverable, boolean_t instantFlag, boolean_t undoFlag)
{
	unsigned long		i;
	BTreeFileCountTable	*countTable;
	BTreeFileOffsetCache	*tempOffsetCache;
	BTreeFileOffsetCache	*freeOffsetCache;
	BTreeFileHeader		*fileHeader;

	fileHeader = (BTreeFileHeader *) 0;
	_btreeFileRead(fileCache->fileDescriptor, (vm_offset_t) 0, 
		(vm_address_t *) &fileHeader, vm_page_size, YES);
	if (ftruncate(fileCache->fileDescriptor, vm_page_size))
		_NXRaiseError(NX_BTreeUnixError + errno, 
			"_btreeFileInitHeader", strerror(errno));

	fileCache->fileSize = vm_page_size;
	fileHeader->fileStatus.modified = YES;
	fileHeader->fileStatus.recoverable = recoverable;
	fileHeader->fileStatus.instantly = instantFlag;
	fileHeader->fileStatus.transactions = undoFlag;
	strncat(strcpy(fileHeader->magicString, "!%"), 
		NXBTreeFileExtension, sizeof(fileHeader->magicString) - 2);
	fileHeader->codeVersion[0] = [BTreeFile version];
	fileHeader->pageSize = vm_page_size;
	fileHeader->fileSize = vm_page_size;
	freeOffsetCache = &fileHeader->freeOffsetCache;
	tempOffsetCache = &fileHeader->tempOffsetCache;
	freeOffsetCache->offsetTable = vm_page_size >> 1;
	if (recoverable)
		freeOffsetCache->offsetLimit = vm_page_size >> 2;
	else
		freeOffsetCache->offsetLimit = vm_page_size >> 1;

	tempOffsetCache->offsetTable = 
		freeOffsetCache->offsetTable + freeOffsetCache->offsetLimit;
	freeOffsetCache->offsetLimit -= sizeof(BTreeFileOffsetTable);
	freeOffsetCache->offsetLimit /= sizeof(BTreeFileOffsetEntry);
	if (recoverable)
		tempOffsetCache->offsetLimit = freeOffsetCache->offsetLimit;

	countTable = &fileHeader->countTable;
	countTable->tableLimit = vm_page_size >> 1;
	countTable->tableLimit -= sizeof(BTreeFileHeader);
	countTable->tableLimit /= sizeof(BTreeFileCountEntry);
	for (i = 0; i < countTable->tableLimit; ++i)
		countTable->countTable[i].recordCount = i + 1;

	fileCache->fileHeader = fileHeader;
	fileCache->fileStatus = fileHeader->fileStatus;
	fileCache->fileStatus.writeable = YES;
	fileCache->fileStatus.creating = YES;
}

inline static void 
_btreeFileOpenHeader(BTreeFileCache *fileCache)
{
	boolean_t		writeable;
	static char		magicString[6];
	kern_return_t		error;
	NXHandler		jumpHandler;
	vm_size_t		pageSize;
	BTreeFileHeader		*fileHeader;

	writeable = fileCache->fileStatus.writeable;
	fileHeader = (BTreeFileHeader *) 0;
	_btreeFileRead(fileCache->fileDescriptor, (vm_offset_t) 0, 
		(vm_address_t *) &fileHeader, vm_page_size, writeable);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		vm_deallocate(task_self(), 
			(vm_address_t) fileHeader, vm_page_size);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	if (magicString[0] != '!')
		strncat(strcpy(magicString, "!%"), 
			NXBTreeFileExtension, sizeof(magicString) - 2);

	if (strcmp(magicString, fileHeader->magicString))
		_NXRaiseError(NX_BTreeInvalidArguments, 
			"_btreeFileOpenHeader", 0);

	if (fileHeader->codeVersion[0] > [BTreeFile version])
		_NXRaiseError(NX_BTreeInvalidVersion, 
			"_btreeFileOpenHeader", 0);

	if (fileHeader->fileStatus.modified)
		_NXRaiseError(NX_BTreeFileInconsistent, 
			"_btreeFileOpenHeader", 0);

	_NXRemoveHandler(&jumpHandler);
	if (fileHeader->pageSize > vm_page_size)
	{
		pageSize = fileHeader->pageSize;
		error = vm_deallocate(task_self(), 
			(vm_address_t) fileHeader, vm_page_size);
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"_btreeFileOpenHeader", 
					mach_error_string(error));

		fileHeader = (BTreeFileHeader *) 0;
		_btreeFileRead(fileCache->fileDescriptor, (vm_offset_t) 0, 
			(vm_address_t *) &fileHeader, pageSize, writeable);
	}

	fileCache->fileHeader = fileHeader;
	fileCache->fileStatus = fileHeader->fileStatus;
	fileCache->fileStatus.writeable = writeable;
	if (writeable)
	{
		fileCache->fileSize = 
			lseek(fileCache->fileDescriptor, 0, L_XTND);
		if (fileCache->fileSize < 0)
			_NXRaiseError(NX_BTreeUnixError + errno, 
				"_btreeFileOpenHeader", strerror(errno));
	
		_btreeFileAdjustFileSize(fileCache);
	}
}

/*
routines for maintaining the BTree table
*/

inline static short 
_btreeFileNewCountEntry(BTreeFileHeader *fileHeader)
{
	static BTreeFileCountEntry	countEntry;
	unsigned short			tableIndex;
	BTreeFileCountTable		*countTable;

	countTable = &fileHeader->countTable;
	if (countTable->tableCount >= countTable->tableLimit)
		return -1;

	++countTable->tableCount;
	tableIndex = countTable->startEntry;
	countTable->startEntry = 
		countTable->countTable[tableIndex].recordCount;
	countTable->countTable[tableIndex] = countEntry;
	return tableIndex;
}

inline static void 
_btreeFileFreeCountEntry(BTreeFileHeader *fileHeader, 
	unsigned short tableIndex)
{
	BTreeFileCountTable	*countTable;

	countTable = &fileHeader->countTable;
	if (tableIndex < countTable->tableLimit)
	{
		--countTable->tableCount;
		countTable->countTable[tableIndex].recordCount = 
			countTable->startEntry;
		countTable->startEntry = tableIndex;
	}
}

/*
routines for creating and destroying BTree backing stores
*/

static void 
_btreeFileCreateBTreeStore(BTreeFileCache *fileCache, 
	BTreeFileBTreeRecord *btreeRecord)
{
	NXHandler			jumpHandler;
	BTreeFilePageHeader		*pageHeader;

	volatile vm_offset_t		rootOffset;
	volatile unsigned short		tableIndex;
	BTreeFileHeader			*volatile fileHeader;

	fileHeader = fileCache->fileHeader;
	rootOffset = _btreeFileNewOffset(fileCache);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		error: 
		_btreeFileFreeOffset(fileCache, rootOffset);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	tableIndex = _btreeFileNewCountEntry(fileHeader);
	if (_setjmp(jumpHandler.jumpState))
	{
		_btreeFileFreeCountEntry(fileHeader, tableIndex);
		goto error;
	}

	pageHeader = (BTreeFilePageHeader *) 
		_btreeFilePageBufferFromOffset(fileCache, rootOffset, YES);
	pageHeader->pageLevel = 0;
	pageHeader->verification = rootOffset;
	pageHeader->rootOffset = rootOffset;
	gettimeofday(&pageHeader->timeStamp, 0);
	_NXRemoveHandler(&jumpHandler);
	btreeRecord->tableIndex = tableIndex;
	btreeRecord->rootOffset = rootOffset;
}

static void 
_btreeFileRemoveBTreeStore(BTreeFileCache *fileCache, 
	BTreeFileBTreeRecord *btreeRecord)
{
	_btreeFileFreeOffset(fileCache, btreeRecord->rootOffset);
	_btreeFileFreeCountEntry(fileCache->fileHeader, 
				btreeRecord->tableIndex);
}

/*
routines for hashing names and store pointers
*/

static void 
_btreeFileFreeHandle(const void *info, void *data)
{
	BTreeFileBTreeHandle	*btreeHandle;

	btreeHandle = (BTreeFileBTreeHandle *) data;
	if (btreeHandle->btreeName)
		free((void *) btreeHandle->btreeName);

	free((void *) btreeHandle);
}

static unsigned int 
_btreeFileHashName(const void *info, const void *data)
{
	const BTreeFileBTreeHandle	*btreeHandle;

	btreeHandle = (const BTreeFileBTreeHandle *) data;
	return NXStrHash(info, btreeHandle->btreeName);
}

static int 
_btreeFileCompareNames(const void *info, const void *data1, const void *data2)
{
	const BTreeFileBTreeHandle	*btreeHandle1;
	const BTreeFileBTreeHandle	*btreeHandle2;

	btreeHandle1 = (const BTreeFileBTreeHandle *) data1;
	btreeHandle2 = (const BTreeFileBTreeHandle *) data2;
	return NXStrIsEqual(info, 
		btreeHandle1->btreeName, btreeHandle2->btreeName);
}

static unsigned int
_btreeFileHashStore(const void *info, const void *data)
{
	const BTreeFileBTreeHandle	*btreeHandle;

	btreeHandle = (const BTreeFileBTreeHandle *) data;
	return NXPtrHash(info, btreeHandle->btreeStore);
}

static int 
_btreeFileCompareStores(const void *info, const void *data1, const void *data2)
{
	const BTreeFileBTreeHandle	*btreeHandle1;
	const BTreeFileBTreeHandle	*btreeHandle2;

	btreeHandle1 = (const BTreeFileBTreeHandle *) data1;
	btreeHandle2 = (const BTreeFileBTreeHandle *) data2;
	return NXPtrIsEqual(info, 
		btreeHandle1->btreeStore, btreeHandle2->btreeStore);
}

static NXHashTablePrototype _btreeFileByName = {
	_btreeFileHashName, _btreeFileCompareNames, NXNoEffectFree
};

static NXHashTablePrototype _btreeFileByStore = {
	_btreeFileHashStore, _btreeFileCompareStores, _btreeFileFreeHandle
};

@implementation BTreeFile

+ initialize
{
	[self setVersion: _NXBTreeFileVersion];
	return self;
}

+ _newFile: (const char *) filePath recoverable: (BOOL) recoverable 
		isInstant: (BOOL) instantFlag canUndo: (BOOL) undoFlag
{
	NXHandler		jumpHandler;
	BTreeFileCache		*fileCache;

	self = [super new];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[self free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	fileCache = _btreeFileMakeCache((BTreeFileTemplate *) self);
	fileName = _btreeFileOpenFile(fileCache, 
		(char *) filePath, O_CREAT | O_EXCL | O_RDWR, LOCK_EX);
	_btreeFileInitHeader(fileCache, recoverable, instantFlag, undoFlag);
	btreesByName = NXCreateHashTable(_btreeFileByName, 0, 0);
	_btreesByStore = NXCreateHashTable(_btreeFileByStore, 0, 0);
	[self save];
	_NXRemoveHandler(&jumpHandler);
	fileCache->fileStatus.creating = NO;
	return self;
}

+ newFile: (const char *) filePath
{
	return [self _newFile: filePath 
		recoverable: NO isInstant: NO canUndo: NO];
}

+ new
{
	struct tsval	timestamp;
	char		template[128];

	kern_timestamp(&timestamp);
	sprintf(template, "%s/%lu.%lu.XXXXXX", NXBTreeFileTemporary, 
		timestamp.high_val, timestamp.low_val);
	mktemp(template);
	return [self newFile: 
		strcat(strcat(template, "."), NXBTreeFileExtension)];
}

+ openFile: (const char *) filePath forWriting: (BOOL) writingFlag
{
	NXHandler		jumpHandler;
	int			lockType;
	int			openFlags;
	BTreeFileCache		*fileCache;

	self = [super new];
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[self free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	fileCache = _btreeFileMakeCache((BTreeFileTemplate *) self);
	if (fileCache->fileStatus.writeable = writingFlag)
	{
		lockType = LOCK_EX;
		openFlags = O_RDWR;
	} else
	{
		lockType = LOCK_SH;
		openFlags = O_RDONLY;
	}

	fileName = _btreeFileOpenFile(fileCache, 
		(char *) filePath, openFlags, lockType);
	_btreeFileOpenHeader(fileCache);
	btreesByName = NXCreateHashTable(_btreeFileByName, 0, 0);
	_btreesByStore = NXCreateHashTable(_btreeFileByStore, 0, 0);
	_NXRemoveHandler(&jumpHandler);
	return self;
}

+ (void) compressFile: (const char *) filePath
{
	[self notImplemented: _cmd];
}

inline static void 
_btreeFileClearPageArray(BTreeFileCache *fileCache)
{
	kern_return_t		error;
	vm_size_t		tableSize;
	unsigned short		i;
	unsigned short		j;
	BTreePage		*btreePage;
	BTreeFilePageTable	*pageTable;
	BTreeFileHeader		*fileHeader;

	fileHeader = fileCache->fileHeader;
	tableSize = fileHeader->pageSize;
	tableSize *= BTreeFilePagesPerPageTable;
	for (i = 0; i < BTreeFilePageBlocksPerBTreeFile; ++i)
	{
		pageTable = fileCache->pageArray[i];
		if (! pageTable)
			continue;

		for (j = 0; j < BTreeFilePageTablesPerPageBlock; ++j)
		{
			btreePage = pageTable[j].btreePage;
			if (! btreePage || ! btreePage->pageBuffer)
				continue;

			error = vm_deallocate(task_self(), 
				btreePage->pageBuffer, tableSize);
			if (error != KERN_SUCCESS)
				_NXRaiseError(NX_BTreeMachError + error, 
					"_btreeFileClearPageArray", 
						mach_error_string(error));
		}
	}
}

- free
{
	NXHashState		hashState;
	NXHashTable		*_btreesClosing;
	BTreeFileCache		*fileCache;
	BTreeFileBTreeHandle	*btreeHandle;
	BTreeFileHeader		*fileHeader;

	fileCache = (BTreeFileCache *) _backingStore;
	if (fileCache)
	{
		fileHeader = fileCache->fileHeader;
		if (_btreesByStore)
		{
			_btreesClosing = _btreesByStore;
			hashState = NXInitHashState(_btreesClosing);
			_btreesByStore = (NXHashTable *) 0;
			while (NXNextHashState(_btreesClosing, 
					&hashState, (void **) &btreeHandle))
				[btreeHandle->btree free];

			NXFreeHashTable(_btreesClosing);
		}

		if (btreesByName)
			NXFreeHashTable(btreesByName);

		if (fileCache->copyHeader)
			vm_deallocate(task_self(), (vm_address_t) 
				fileCache->copyHeader, fileHeader->pageSize);

		if (fileCache->fileHeader)
		{
			_btreeFileClearPageArray(fileCache);
			vm_deallocate(task_self(), (vm_address_t) 
				fileCache->fileHeader, fileHeader->pageSize);
		}

		if (fileCache->fileDescriptor >= 0)
		{
			flock(fileCache->fileDescriptor, LOCK_UN);
			close(fileCache->fileDescriptor);
			if (fileCache->fileStatus.creating)
				unlink(fileName);
		}

		free((void *) fileCache);
		if (fileName) free((void *) fileName);
	}

	return [super free];
}

- (const char *) fileName
{
	return (const char *) fileName;
}

inline static BTreeFileBTreeHandle 
*_btreeFileOpenBTreeWithStore(BTreeFileCache *fileCache, 
	BTreeFileBTreeHandle *btreeHandle)
{
	BTreeStore		*btreeStore;

	btreeStore = btreeHandle->btreeStore;
	btreeHandle->btree = (BTree *) [BTree _newWith: btreeStore];
	if (fileCache->fileStatus.writeable)
		fileCache->fileHeader->codeVersion[1] = 
			btreeStore->codeVersion;

	return btreeHandle;
}

- (BTree *) openBTree
{
	NXHandler		jumpHandler;
	BTreeFileHeader		*fileHeader;
	BTreeFileCache		*fileCache;
	BTreeFileBTreeRecord	*defaultRecord;

	BTreeFileBTreeHandle	*volatile btreeHandle;

	if (! defaultBTree)
	{
		fileCache = (BTreeFileCache *) _backingStore;
		fileHeader = fileCache->fileHeader;
		defaultRecord = &fileHeader->defaultRecord;
		if (! defaultRecord->rootOffset)
		{
			if (! fileCache->fileStatus.writeable)
				_NXRaiseError(NX_BTreeReadOnlyStore, 
					"openBTree", 0);

			_btreeFileCreateBTreeStore(fileCache, defaultRecord);
		}

		btreeHandle = calloc(sizeof(BTreeFileBTreeHandle), 1);
		if (! btreeHandle)
			_NXRaiseError(NX_BTreeNoMemory, "openBTree", 0);
	
		_NXAddHandler(&jumpHandler);
		if (_setjmp(jumpHandler.jumpState))
		{
			free(btreeHandle);
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}
	
		btreeHandle->btreeStore = [BTreeStoreFile newFile: self 
			entry: defaultRecord->tableIndex 
			offset: defaultRecord->rootOffset];
		if (_setjmp(jumpHandler.jumpState))
		{
			[btreeHandle->btreeStore free];
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		} else
		{
			NXHashInsert(_btreesByStore, btreeHandle);
			_btreeFileOpenBTreeWithStore(fileCache, btreeHandle);
			_NXRemoveHandler(&jumpHandler);
		}

		defaultBTree = (void *) btreeHandle;
	}

	btreeHandle = (BTreeFileBTreeHandle *) defaultBTree;
	return btreeHandle->btree;
}

static BTreeFileBTreeHandle 
*_btreeFileOpenBTree(BTreeFileTemplate *btreeFile, const char *btreeName)
{
	NXHandler		jumpHandler;
	BTreeFileBTreeHandle	dummyHandle;
	BTreeFileCache		*fileCache;
	BTreeFileHeader		*fileHeader;

	BTreeFileBTreeHandle	*volatile btreeHandle;
	BTreeFileBTreeRecord	*volatile btreeRecord;

	dummyHandle.btreeName = (char *) btreeName;
	btreeHandle = NXHashGet(btreeFile->btreesByName, &dummyHandle);
	if (! btreeHandle)
	{
		btreeHandle = calloc(sizeof(BTreeFileBTreeHandle), 1);
		if (! btreeHandle)
			_NXRaiseError(NX_BTreeNoMemory, 
				"_btreeFileOpenBTree", 0);
	
		_NXAddHandler(&jumpHandler);
		if (_setjmp(jumpHandler.jumpState))
		{
			free(btreeHandle);
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}
	
		fileCache = (BTreeFileCache *) btreeFile->_backingStore;
		fileHeader = fileCache->fileHeader;
		if (! strcmp(btreeName, NXBTreeFileDirectory))
		{
			btreeRecord = &fileHeader->directoryRecord;
			if (! btreeRecord->rootOffset)
				_NXRaiseError(NX_BTreeInvalidArguments, 
					"_btreeFileOpenBTree", 0);
		} else
		{
			btreeRecord = 
				_btreeFileDirectoryRead(btreeFile, btreeName);
			if (_setjmp(jumpHandler.jumpState))
			{
				free(btreeRecord);
				free(btreeHandle);
				_NXRaiseError(jumpHandler.code, 
					jumpHandler.data1, jumpHandler.data2);
			}
		}

		btreeHandle->btreeName = strsave((char *) btreeName);
		btreeHandle->btreeStore = 
			[BTreeStoreFile newFile: (BTreeFile *) btreeFile 
				entry: btreeRecord->tableIndex 
				offset: btreeRecord->rootOffset];
		if (strcmp(btreeName, NXBTreeFileDirectory))
			free(btreeRecord);

		if (_setjmp(jumpHandler.jumpState))
		{
			[btreeHandle->btreeStore free];
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}

		NXHashInsert(btreeFile->_btreesByStore, btreeHandle);
		NXHashInsert(btreeFile->btreesByName, btreeHandle);
		_btreeFileOpenBTreeWithStore(fileCache, btreeHandle);
		_NXRemoveHandler(&jumpHandler);
	}

	return btreeHandle;
}

- (BTree *) openBTreeNamed: (const char *) btreeName
{
	BTreeFileBTreeHandle	*btreeHandle;

	_btreeFileCheckBTreeName(btreeName, "openBTreeNamed:");
	btreeHandle = 
		_btreeFileOpenBTree((BTreeFileTemplate *) self, btreeName);
	return btreeHandle->btree;
}

- _btreeByStore: (void *) private
{
	BTreeFileBTreeHandle	dummyHandle;
	BTreeFileBTreeHandle	*btreeHandle;

	dummyHandle.btreeStore = (BTreeStoreFile *) private;
	btreeHandle = NXHashGet(_btreesByStore, &dummyHandle);
	if (! btreeHandle)
		_NXRaiseError(NX_BTreeInternalError, 
			"_btreeClosed:", 0);

	return btreeHandle->btree;
}

- (void) _btreeClosed: (void *) private
{
	BTreeFileBTreeHandle	dummyHandle;
	BTreeFileBTreeHandle	*btreeHandle;

	if (_btreesByStore)
	{
		dummyHandle.btreeStore = (BTreeStoreFile *) private;
		btreeHandle = NXHashRemove(_btreesByStore, &dummyHandle);
		if (! btreeHandle)
			_NXRaiseError(NX_BTreeInternalError, 
				"_btreeClosed:", 0);

		if ((void *) btreeHandle == defaultBTree)
			defaultBTree = (void *) 0;
		else
			NXHashRemove(btreesByName, btreeHandle);

		_btreeFileFreeHandle((const void *) 0, (void *) btreeHandle);
	}
}

static BTreeFileBTreeHandle 
*_btreeFileCreateBTree(BTreeFileTemplate *btreeFile, const char *btreeName)
{
	NXHandler		jumpHandler;
	BTreeFileBTreeRecord	staticRecord;
	BTreeFileHeader		*fileHeader;

	BTreeFileCache		*volatile fileCache;
	BTreeFileBTreeHandle	*volatile btreeHandle;
	BTreeFileBTreeRecord	*volatile btreeRecord;

	fileCache = (BTreeFileCache *) btreeFile->_backingStore;
	if (! fileCache->fileStatus.writeable)
		_NXRaiseError(NX_BTreeReadOnlyStore, 
			"_btreeFileCreateBTree", 0);

	btreeHandle = calloc(sizeof(BTreeFileBTreeHandle), 1);
	if (! btreeHandle)
		_NXRaiseError(NX_BTreeNoMemory, "_btreeFileCreateBTree", 0);

	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		error1: free(btreeHandle);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	fileHeader = fileCache->fileHeader;
	btreeRecord = &fileHeader->directoryRecord;
	if (strcmp(btreeName, NXBTreeFileDirectory))
	{
		if (! btreeRecord->rootOffset)
			_btreeFileCreateBTree(btreeFile, NXBTreeFileDirectory);

		btreeRecord = &staticRecord;
		_btreeFileCreateBTreeStore(fileCache, btreeRecord);
		if (_setjmp(jumpHandler.jumpState))
		{
			error2: 
			_btreeFileRemoveBTreeStore(fileCache, btreeRecord);
			goto error1;
		}

		_btreeFileDirectoryInsert(btreeFile, btreeName, btreeRecord);
		if (_setjmp(jumpHandler.jumpState))
		{
			_btreeFileDirectoryRemove(btreeFile, btreeName);
			goto error2;
		}
	} else
		_btreeFileCreateBTreeStore(fileCache, btreeRecord);

	btreeHandle->btreeName = strsave((char *) btreeName);
	btreeHandle->btreeStore = 
		[BTreeStoreFile newFile: (BTreeFile *) btreeFile 
			entry: btreeRecord->tableIndex 
			offset: btreeRecord->rootOffset];
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeHandle->btreeStore free];
		if (strcmp(btreeName, NXBTreeFileDirectory))
		{
			_btreeFileDirectoryRemove(btreeFile, btreeName);
			_btreeFileRemoveBTreeStore(fileCache, btreeRecord);
		}

		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	NXHashInsert(btreeFile->_btreesByStore, btreeHandle);
	NXHashInsert(btreeFile->btreesByName, btreeHandle);
	_btreeFileOpenBTreeWithStore(fileCache, btreeHandle);
	_NXRemoveHandler(&jumpHandler);
	return btreeHandle;
}

- (BTree *) createBTreeNamed: (const char *) btreeName
{
	BTreeFileBTreeHandle	*btreeHandle;

	_btreeFileCheckBTreeName(btreeName, "createBTreeNamed:");
	btreeHandle = 
		_btreeFileCreateBTree((BTreeFileTemplate *) self, btreeName);
	return btreeHandle->btree;
}

static void 
_btreeFileRemoveBTree(BTreeFileTemplate *btreeFile, const char *btreeName)
{
	NXHandler		jumpHandler;
	BTreeFileBTreeHandle	dummyHandle;
	BTreeFileCache		*fileCache;
	BTreeFileHeader		*fileHeader;

	BTreeFileBTreeHandle	*volatile btreeHandle;
	BTreeFileBTreeRecord	*volatile btreeRecord;

	fileCache = (BTreeFileCache *) btreeFile->_backingStore;
	if (! fileCache->fileStatus.writeable)
		_NXRaiseError(NX_BTreeReadOnlyStore, 
			"_btreeFileRemoveBTree", 0);

	dummyHandle.btreeName = (char *) btreeName;
	if (NXHashMember(btreeFile->btreesByName, &dummyHandle))
		_NXRaiseError(NX_BTreeInvalidArguments, 
			"_btreeFileRemoveBTree", 0);

	btreeHandle = _btreeFileOpenBTree(btreeFile, btreeName);
	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeHandle->btree free];
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	} else
	{
		[btreeHandle->btree empty];
		_NXRemoveHandler(&jumpHandler);
	}

	[btreeHandle->btree free];
	fileCache = (BTreeFileCache *) btreeFile->_backingStore;
	fileHeader = fileCache->fileHeader;
	if (! strcmp(btreeName, NXBTreeFileDirectory))
	{
		btreeRecord = &fileHeader->directoryRecord;
		_btreeFileRemoveBTreeStore(fileCache, btreeRecord);
		btreeRecord->rootOffset = (vm_offset_t) 0;
		[(BTreeFile *) btreeFile save];
	} else
	{
		btreeRecord = _btreeFileDirectoryRead(btreeFile, btreeName);
		_NXAddHandler(&jumpHandler);
		if (_setjmp(jumpHandler.jumpState))
		{
			free(btreeRecord);
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}

		_btreeFileDirectoryRemove(btreeFile, btreeName);
		if (_setjmp(jumpHandler.jumpState))
		{
			_btreeFileDirectoryInsert(btreeFile, 
				btreeName, btreeRecord);
			free(btreeRecord);
			_NXRaiseError(jumpHandler.code, 
				jumpHandler.data1, jumpHandler.data2);
		}

		_btreeFileRemoveBTreeStore(fileCache, btreeRecord);
		_NXRemoveHandler(&jumpHandler);
		free(btreeRecord);
		btreeHandle = 
			_btreeFileOpenBTree(btreeFile, NXBTreeFileDirectory);
		if (! btreeHandle->btreeStore->recordCount)
		{
			[btreeHandle->btree free];
			_btreeFileRemoveBTree(btreeFile, NXBTreeFileDirectory);
		}
	}
}

- (void) removeBTreeNamed: (const char *) btreeName
{
	_btreeFileCheckBTreeName(btreeName, "removeBTreeNamed:");
	_btreeFileRemoveBTree((BTreeFileTemplate *) self, btreeName);
}

- (char **) contents;
{
	NXHandler		jumpHandler;
	BOOL			validFlag;
	unsigned short		keyLength;
	char			*keyString;
	char			**btreeName;
	BTreeFileBTreeHandle	*btreeHandle;
	BTreeFileCache		*fileCache;

	char			**volatile btreeNameArray;
	BTreeCursor		*volatile btreeCursor;

	fileCache = (BTreeFileCache *) _backingStore;
	if (! fileCache->fileHeader->directoryRecord.rootOffset)
		return (char **) 0;

	btreeCursor = (BTreeCursor *) 0;
	btreeHandle = _btreeFileOpenBTree((BTreeFileTemplate *) self, 
		NXBTreeFileDirectory);
	keyLength = [btreeHandle->btree count];
	btreeNameArray = (char **) calloc((1 + keyLength), sizeof(char *));
	if (! btreeNameArray)
		_NXRaiseError(NX_BTreeNoMemory, "contents", 0);

	_NXAddHandler(&jumpHandler);
	if (_setjmp(jumpHandler.jumpState))
	{
		[btreeCursor free];
		for (btreeName = btreeNameArray; *btreeName; ++btreeName)
			free(*btreeName);

		free(btreeNameArray);
		_NXRaiseError(jumpHandler.code, 
			jumpHandler.data1, jumpHandler.data2);
	}

	btreeName = btreeNameArray;
	btreeCursor = [btreeHandle->btree cursor];
	validFlag = [btreeCursor setFirst];
	for (; validFlag; validFlag = [btreeCursor setNext])
	{
		[btreeCursor getKey: (void **) &keyString 
			length: &keyLength valid: &validFlag];
		if (strcmp(keyString, NXBTreeFileDirectory))
		{
			*btreeName = (char *) malloc(keyLength);
			if (! *btreeName)
				_NXRaiseError(NX_BTreeNoMemory, "contents", 0);

			bcopy(keyString, *btreeName, keyLength);
			++btreeName;
		}
	}

	[btreeCursor free];
	_NXRemoveHandler(&jumpHandler);
	return btreeNameArray;
}

void 
_btreeFileWritePageArray(BTreeFileCache *fileCache)
{
	long			pageCount;
	unsigned long		thisBlock;
	unsigned long		thisTable;
	BTreeFilePageTable	*pageBlock;
	BTreeFilePageTable	*pageTable;
	BTreePage		*btreePage;

	thisBlock = 0;
	for (; thisBlock < BTreeFilePageBlocksPerBTreeFile; ++thisBlock)
	{
		if (! (pageBlock = fileCache->pageArray[thisBlock]))
			continue;

		thisTable = 0;
		for (; thisTable < BTreeFilePageTablesPerPageBlock; 
			++thisTable)
		{
			pageTable = pageBlock + thisTable;
			if (! pageTable->btreePage)
				continue;

			if ((pageCount = pageTable->maximumPage - 
				pageTable->minimumPage) < 1)
				continue;

			btreePage = pageTable->btreePage + 
				pageTable->minimumPage;
			_btreeFileWrite(btreePage->pageBuffer, 
				pageCount * fileCache->fileHeader->pageSize);
			pageTable->maximumPage = 0;
			pageTable->minimumPage = BTreeFilePagesPerPageTable;
		}
	}
}

inline static void 
_btreeFileSynchronize(BTreeFileCache *fileCache)
{
	kern_return_t		error;
	BTreeFileHeader		*fileHeader;

	_btreeFileWritePageArray(fileCache);
	if (fileCache->fileStatus.recoverable)
		_btreeFileMergeOffsetCache(fileCache);

	fileCache->fileStatus.modified = NO;
	_btreeFileWriteHeader(fileCache);
	_btreeFileAdjustFileSize(fileCache);
	if (fileHeader = fileCache->copyHeader)
	{
		error = vm_deallocate(task_self(), 
			(vm_address_t) fileHeader, fileHeader->pageSize);
		fileCache->copyHeader = (BTreeFileHeader *) 0;
		if (error != KERN_SUCCESS)
			_NXRaiseError(NX_BTreeMachError + error, 
				"_btreeFileSynchronize", 
					mach_error_string(error));
	}
}

inline static void 
_btreeFileFlushStores(NXHashTable *hashTable, SEL selector)
{
	NXHashState		hashState;
	BTreeFileBTreeHandle	*btreeHandle;

	hashState = NXInitHashState(hashTable);
	while (NXNextHashState(hashTable, &hashState, (void **) &btreeHandle))
		[btreeHandle->btreeStore perform: selector];
}

- (void) save
{
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) _backingStore;
	if (fileCache->fileStatus.modified)
	{
		_btreeFileFlushStores(_btreesByStore, @selector(_save));
		_btreeFileSynchronize(fileCache);
	}
}

- (void) undo
{
	BTreeFileHeader		*fileHeader;
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) _backingStore;
	if (fileCache->fileStatus.modified)
	{
		_btreeFileFlushStores(_btreesByStore, @selector(_undo));
		if (fileHeader = fileCache->copyHeader)
			bcopy((char *) fileHeader, (char *) 
				fileCache->fileHeader, fileHeader->pageSize);

		_btreeFileSynchronize(fileCache);
	}
}

+ newFile: (const char *) btreeName isInstant: (BOOL) instantFlag 
	canUndo: (BOOL) undoFlag
{
	return [self _newFile: btreeName 
		recoverable: YES isInstant: instantFlag canUndo: undoFlag];
}

+ (void) recoverFile: (const char *) btreeName
{
	[self notImplemented: _cmd];
}

- (void) discardPriorTo: (struct timeval) threshhold
{
	[self notImplemented: _cmd];
}

- (BOOL) recoverable
{
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) _backingStore;
	return fileCache->fileStatus.recoverable;
}

- (BOOL) canUndo
{
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) _backingStore;
	return fileCache->fileStatus.recoverable && 
		fileCache->fileStatus.transactions;
}

- (BOOL) isRestartable
{
	BTreeFileCache		*fileCache;

	fileCache = (BTreeFileCache *) _backingStore;
	return fileCache->fileStatus.recoverable && 
		fileCache->fileStatus.transactions && 
		fileCache->fileStatus.instantly;
}

- (void) restartFrom: (struct timeval) time
{
	[self notImplemented: _cmd];
}


@end



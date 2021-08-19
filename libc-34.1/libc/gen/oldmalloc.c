#define NDEBUG
#define SIZETABLE
#define MSTATS
#define MTHREAD
#define FREEKLUDGE

#include <stdio.h>
#include <mach.h>
#include <errno.h>
#ifdef MTHREAD
#include <cthreads.h>
#endif MTHREAD
#include <assert.h>
#include <string.h>

#ifdef REPLACABLE
#define valloc _new_valloc
#define calloc _new_calloc
#define realloc _new_realloc
#define malloc _new_malloc
#define free _new_free
#define malloc_size _new_malloc_size
#define malloc_good_size _new_malloc_good_size
#endif

/* external definitions */

void *malloc(unsigned int byteSize);
void free(void *ptr);
void *realloc(void *oldptr, unsigned int newsize);
void *calloc(unsigned int numElems, unsigned int byteSize);
void cfree(void *ptr);
void *valloc(unsigned int byteSize);
void vfree(void *ptr);
unsigned int malloc_size(void *ptr);
unsigned int malloc_good_size(unsigned int byteSize);

/* Craig Hansen, NeXT, Inc. - 8-September-88 */

#define BITSPERWORD 32
#define BYTESPERWORD 4
#define BITSPERBYTE 8
#define LOGBITSPERWORD 5
#define LOGBITSPERBYTE 3
#define LOGBYTESPERWORD 2

#define ROUNDBITS 3

#define PAGESIZEBYTES 8192
#define PAGESIZEWORDS (PAGESIZEBYTES/BYTESPERWORD)

#define DATAPAGESIZEBYTES PAGESIZEBYTES
#define MAPPAGESIZEBYTES (PAGESIZEBYTES/8)

#define CHUNKSIZEBYTES 4
#define CHUNKSIZEWORDS (CHUNKSIZEBYTES/BYTESPERWORD)

#define DATASPACEBYTES (PAGESIZEBYTES-DATAOVERHEADBYTES)
#define DATASPACECHUNKS (DATASPACEBYTES/CHUNKSIZEBYTES)
#define HALFSPACE (DATASPACEBYTES/2)
#ifdef SIZETABLE
#define MAXBYTES HALFSPACE
#else
#define ROUND1 (HALFSPACE>>ROUNDBITS)
#define ROUND2 (ROUND1|(ROUND1>>1))
#define ROUND4 (ROUND2|(ROUND2>>2))
#define ROUND8 (ROUND4|(ROUND4>>4))
#define ROUND16 (ROUND8|(ROUND8>>8))
#define MAXBYTES (HALFSPACE & ~ ROUND16)
#endif

typedef unsigned int UInt, *PToUInt;
typedef unsigned short UShort, *PToUShort;
typedef unsigned char UByte, *PToUByte;

#define ui(x) ((UInt) x)
#define us(x) ((UShort) x)
#define ub(x) ((UByte) x)

#define MAPINDEXMASK ( ui(MAPPAGESIZEBYTES-1) )
#define DATAINDEXMASK ( ui(DATAPAGESIZEBYTES-1) )

#define HIBIT (ui(1<<(BITSPERWORD-1)))
#define CHUNKS(size,chunksize) (((size)+(chunksize)-1)/(chunksize))

typedef struct FreeListStruct *PToFreeList;
typedef struct FreeListStruct {
    PToFreeList next;
} FreeList;

typedef struct PageMapStruct *PToPageMap;
typedef struct DataMapStruct *PToDataMap;

#define MAPOVERHEADBYTES 12
typedef struct PageMapStruct {
    UInt capacity;
    UInt exist;
    UInt stuffed;
    PToDataMap data[20];
} PageMap;

#define DATAOVERHEADBYTES 16
typedef struct DataMapStruct {
    UInt link;
    UInt count;
    UInt size;
    PToFreeList freelist;
} DataMap;

#ifdef SIZETABLE
static UByte goodIndex[] = {
 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3,
 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,
 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,
 9, 9, 9,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,
10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,10,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,11,
11,11,11,11,11,11,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,
12,12,12,12,12,12,12,12,12};
#define MAXCHUNKS 30
#define BYTESINCHUNK(x) (masterMap[x].goodSize*CHUNKSIZEBYTES)
#define CHUNKSINBYTE(x) (goodIndex[(x)/CHUNKSIZEBYTES])
#define PAD NULL,NULL
static struct {
    UInt goodSize;
    PToDataMap dataPage;
    PToPageMap mapPage;
} masterMap[MAXCHUNKS] = {
   4,PAD,   8,PAD,  16,PAD,  32,PAD,  63,PAD,
  85,PAD, 127,PAD, 170,PAD, 255,PAD, 340,PAD,
 511,PAD, 681,PAD,1022,PAD};
static sizesInMasterMap = 13;
#else
#define MAXCHUNKS (MAXBYTES/CHUNKSIZEBYTES)
#define BYTESINCHUNK(x) (x*CHUNKSIZEBYTES)
#define CHUNKSINBYTE(x) (x/CHUNKSIZEBYTES)
static struct {
    PToDataMap dataPage;
    PToPageMap mapPage;
} masterMap[MAXCHUNKS+1];
#define sizesInMasterMap (MAXCHUNKS+1)
#endif

#ifdef MTHREAD
static int malloc_lock;
#define LOCK spin_lock(&malloc_lock);
#define UNLOCK spin_unlock(&malloc_lock);
#define STATIC static
STATIC void *MALLOC(UInt byteSize);
STATIC void FREE(void *ptr);
STATIC UInt MALLOC_SIZE(void *ptr);
#else
#define LOCK
#define UNLOCK
#define STATIC
#define MALLOC malloc
#ifdef FREEKLUDGE
STATIC void FREE(void *ptr);
#else
#define FREE free
#endif
#define MALLOC_SIZE malloc_size
#endif

#ifdef FREEKLUDGE
static void *lastfree;
#endif

#define bfffo(x) \
	({ UInt __value, __arg = (x); \
	asm ("bfffo %1{#0:#0},%0": "=d" (__value): "d" (__arg)); \
	__value; })

static void DefaultMallocError() {
    errno = ENOMEM;
}

static void (*MallocErrorFunc)(void) = DefaultMallocError;

/* The routines pagemalloc and pagefree handle allocation of regions
* that are sized in units of virtual memory pages. Mach does most of
* the work, but it does not keep track of the size of allocation requests
* and the standard UNIX routine "free" isn't told the size of the
* region to free. So, we keep track of the information in a hash
* table, in which the sizes of all allocation requests larger than a
* single page are kept. Anything not in this hash table is assumed
* to be only a page.
*/

#define HASHBUCKETS 157

typedef struct HashBucketStruct *PToHashBucket;
typedef struct HashBucketStruct {
    PToHashBucket next;
    void *address;
    UInt size;
} HashBucket;

static PToHashBucket pagehash[HASHBUCKETS];
#ifdef TEST
static UInt allocatedPages = 0;
#endif TEST
static UInt singlePagesAllocated = 0;
#ifdef PAGEBUFFERSIZE
static struct {
	void *buffer[PAGEBUFFERSIZE];
	UInt count;
} pageBuffer;
#endif

static void *pagemalloc(UInt size) {
    void *ptr;
    register UInt hash;
    register PToHashBucket bucket;
    
#ifdef PAGEBUFFERSIZE
    if (size == 1) {
	if (pageBuffer.count != 0) {
	    return pageBuffer.buffer[--pageBuffer.count];
	}	
    }
#endif
    if (vm_allocate(task_self_, &ptr, size*PAGESIZEBYTES, TRUE) != KERN_SUCCESS) {
	MallocErrorFunc();
	return NULL;
    }
#ifdef TEST
    allocatedPages += size;
#endif TEST
    if (size == 1) {
        singlePagesAllocated++;
    } else {
	hash = (ui(ptr)/PAGESIZEBYTES) % HASHBUCKETS;
	bucket = (PToHashBucket) MALLOC (sizeof(HashBucket));
	bucket->next = pagehash[hash];
	bucket->address = ptr;
	bucket->size = size;
	pagehash[hash] = bucket;
    }
    return ptr;
}

static pagefree(void *ptr) {
    register UInt hash, size;
    register PToHashBucket bucket, *trailer;

    if (ptr==NULL) return;
    
    hash = (ui(ptr)/PAGESIZEBYTES) % HASHBUCKETS;
    size = 1;
    trailer = &pagehash[hash];
    while ((bucket = *trailer) != NULL) {
	if (bucket->address == ptr) {
	    size = bucket->size;
	    *trailer = bucket->next;
	    FREE(bucket);
	    break;
	}
	trailer = &bucket->next;
    }
    if (size==1) {
#ifdef PAGEBUFFERSIZE
	if (pageBuffer.count != PAGEBUFFERSIZE) {
	    pageBuffer.buffer[pageBuffer.count++] = ptr;
	    return;
	}	
#endif
	singlePagesAllocated--;
    }
#ifdef TEST
    allocatedPages -= size;
#endif TEST
    if (vm_deallocate(task_self_, ptr, size*PAGESIZEBYTES) != KERN_SUCCESS) {
	MallocErrorFunc();
    }
}

static UInt pageallocsize(void *ptr) {
    register UInt hash, size;
    register PToHashBucket bucket, *trailer;

    if (ptr==NULL) return 0;
    
    hash = (ui(ptr)/PAGESIZEBYTES) % HASHBUCKETS;
    size = 1;
    bucket = pagehash[hash];
    while (bucket != NULL) {
	if (bucket->address == ptr) {
	    size = bucket->size;
	    break;
	}
	bucket = bucket->next;
    }
    return size;
}

static PToDataMap newdatapage(UInt chunkSize) {
    register PToDataMap newPage;
    register UShort count;
    register UInt byteSize;
    
    if ((newPage = (PToDataMap) pagemalloc(1)) != NULL) {
        byteSize = BYTESINCHUNK(chunkSize);
	count = us(DATASPACEBYTES) / us(byteSize);
	newPage->link = 0;
	newPage->count = PAGESIZEBYTES - count * byteSize;
	newPage->size = byteSize;
	newPage->freelist = NULL;
	masterMap[chunkSize].dataPage = newPage;
    }
    assert ((ui(newPage) & ui(PAGESIZEBYTES-1)) == 0);
    return newPage;
}

static PToPageMap newmap (UInt capacity) {
    register PToPageMap newPage;
    register UInt i;

    if ((newPage = (PToPageMap) MALLOC(capacity * BYTESPERWORD + MAPOVERHEADBYTES)) != NULL) {
	newPage->capacity = (MALLOC_SIZE(newPage) - MAPOVERHEADBYTES) / BYTESPERWORD;
	newPage->exist = 0;
	newPage->stuffed = 0;
#ifndef NDEBUG
	bzero(newPage->data, newPage->capacity*BYTESPERWORD);
#endif
    }
    return newPage;
}

static PToPageMap growmap (PToPageMap oldMap, UInt chunkSize) {
    register PToPageMap newMap;
    register UInt i;

    if ((newMap = (PToPageMap) MALLOC( (oldMap->capacity+1) * BYTESPERWORD + MAPOVERHEADBYTES)) != NULL) {
	newMap->capacity = (MALLOC_SIZE(newMap) - MAPOVERHEADBYTES) / BYTESPERWORD;
	newMap->exist = oldMap->exist;
	newMap->stuffed = oldMap->stuffed;
	bcopy (oldMap->data, newMap->data, oldMap->capacity*BYTESPERWORD);
#ifndef NDEBUG
	bzero (newMap->data+oldMap->capacity, (newMap->capacity-oldMap->capacity)*BYTESPERWORD);
#endif
	masterMap[chunkSize].mapPage = newMap;
	FREE (oldMap);
    }
    return newMap;
}

static PToDataMap switchdatapage(PToDataMap dataPage, UInt chunkSize) {
    register PToPageMap mapPage;
    register UInt index, stuffed;
    register PToDataMap newDataPage;

    mapPage = masterMap[chunkSize].mapPage;
    if (mapPage == NULL) {
        newDataPage = newdatapage(chunkSize);
	mapPage = masterMap[chunkSize].mapPage = newmap(2);
	mapPage->exist = 2;
	mapPage->stuffed = 1;
	mapPage->data[0] = dataPage;
	assert (dataPage->link == 0);
	(mapPage->data[1] = newDataPage)->link = 1;
    } else {
	index = dataPage->link;
	assert (mapPage->stuffed <= mapPage->exist);
	assert (mapPage->exist != 1);
	stuffed = mapPage->stuffed;
	assert (index < mapPage->exist);
	assert (mapPage->data[index] == dataPage);
	if (index > stuffed) {
	    (mapPage->data[index] = mapPage->data[stuffed])->link = index;
	    (mapPage->data[stuffed] = dataPage)->link = stuffed;
	    mapPage->stuffed++;
	} else if (index == stuffed) {
	    mapPage->stuffed = ++index;
	} else {
	    index = stuffed;
	}
	if (index == mapPage->exist) {
	    newDataPage = newdatapage(chunkSize);
	    if (mapPage->exist == mapPage->capacity) {
		mapPage = growmap(mapPage, chunkSize);
	    }
	    mapPage->exist++;
	    (mapPage->data[index] = newDataPage)->link = index;
	} else {
	    masterMap[chunkSize].dataPage = newDataPage = mapPage->data[index];
	}
	assert (mapPage->stuffed <= mapPage->exist);
	assert (mapPage->stuffed < mapPage->capacity);
	assert (mapPage->exist <= mapPage->capacity);
    }
    assert (mapPage->exist != 1);
    assert ((ui(newDataPage) & ui(PAGESIZEBYTES-1)) == 0);
    assert (newDataPage->count < PAGESIZEBYTES);
    assert (masterMap[chunkSize].dataPage == newDataPage);
    return newDataPage;
}

static PToDataMap freemapentry(PToDataMap dataPage, UInt chunkSize) {
    register PToPageMap mapPage;
    register UInt index, exist;

    mapPage = masterMap[chunkSize].mapPage;
    if (mapPage == NULL) {
	return NULL;
    } else {
	index = dataPage->link;
	assert (mapPage->stuffed <= mapPage->exist);
	assert (mapPage->exist != 1);
	assert (index < mapPage->exist);
	assert (mapPage->data[index] != NULL);
	if (index == (exist = --mapPage->exist)) {
	    if (index < mapPage->stuffed) {
		mapPage->stuffed--;
	    }
#ifndef NDEBUG
	    mapPage->data[index] = NULL;
#endif
	} else {
	    assert (index >= mapPage->stuffed);
	    (mapPage->data[index] = mapPage->data[exist])->link = index;
#ifndef NDEBUG
	    mapPage->data[exist] = NULL;
#endif
	}
	assert (mapPage->exist == exist);
	assert (mapPage->stuffed <= mapPage->exist);
	if (exist == 1) {
	    dataPage = mapPage->data[0];
	    FREE (mapPage);
	    masterMap[chunkSize].mapPage = NULL;
	} else {
	    dataPage = mapPage->data[exist-1];
	}
	assert ((ui(dataPage) & ui(PAGESIZEBYTES-1)) == 0);
	return dataPage; 
    }
}

static freedatapage(PToDataMap dataPage) {
    register PToPageMap mapPage;
    register UInt index, chunkSize;
    register PToDataMap newDataPage;

    chunkSize = CHUNKSINBYTE(dataPage->size);
    newDataPage = freemapentry(dataPage, chunkSize);
    assert ((ui(newDataPage) & ui(PAGESIZEBYTES-1)) == 0);
    if (masterMap[chunkSize].dataPage == dataPage) {
	masterMap[chunkSize].dataPage = newDataPage;
    }
    pagefree (dataPage);
}

static enablepage (PToDataMap dataPage) {
    register PToPageMap mapPage;
    register UInt index, stuffed;
    register UInt chunkSize;

    chunkSize = CHUNKSINBYTE(dataPage->size);
    if (dataPage != masterMap[chunkSize].dataPage) {
	mapPage = masterMap[chunkSize].mapPage;
	if (mapPage != NULL) {
	    index = dataPage->link;
	    assert (mapPage->stuffed <= mapPage->exist);
	    assert (mapPage->exist != 1);
	    assert (index < mapPage->stuffed);
	    assert (mapPage->data[index] == dataPage);
	    stuffed = --mapPage->stuffed;
	    if (index != stuffed) {
		(mapPage->data[index] = mapPage->data[stuffed])->link = index;
		(mapPage->data[stuffed] = dataPage)->link = stuffed;
	    }
	    assert (mapPage->stuffed <= mapPage->exist);
	}
    }
}

UInt malloc_good_size (UInt byteSize) {
    register UInt rawChunks, chunkSize, roundoff;

    if (byteSize>MAXBYTES) {
	return CHUNKS(byteSize, PAGESIZEBYTES)*PAGESIZEBYTES;
    } else {
#ifdef SIZETABLE
	LOCK {
	    rawChunks = (byteSize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	    chunkSize = goodIndex[rawChunks];
	    if (rawChunks != 0) {
		register UInt rawChunksPerPage, chunksPerPage, goodSize, i;
		rawChunksPerPage = DATASPACECHUNKS / rawChunks;
		chunksPerPage = DATASPACECHUNKS / masterMap[chunkSize].goodSize;
		if ((rawChunksPerPage > chunksPerPage) && (sizesInMasterMap != MAXCHUNKS)) {
		    goodSize = DATASPACECHUNKS / rawChunksPerPage;
		    for (i=rawChunks; goodIndex[i] == chunkSize; i--) {
			goodIndex[i] = sizesInMasterMap;
		    }
		    masterMap[sizesInMasterMap].dataPage = NULL;
		    masterMap[sizesInMasterMap].mapPage = NULL;
		    masterMap[sizesInMasterMap].goodSize = goodSize;
		    chunkSize = sizesInMasterMap;
		    sizesInMasterMap++;
		}
	    }
	} UNLOCK;
#else
	rawChunks = (byteSize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	roundoff = (ui(-1)>>ROUNDBITS) >> bfffo(rawChunks);
	chunkSize = (rawChunks+roundoff)&(~roundoff);
#endif
	return BYTESINCHUNK(chunkSize);
    }
}

STATIC void *MALLOC(UInt byteSize) {
    register UInt rawChunks, chunkSize, roundoff;
    register PToDataMap dataPage;
    register void *ptr;

    if (byteSize>MAXBYTES) {
	return pagemalloc(CHUNKS(byteSize, PAGESIZEBYTES));
    } else {
#ifdef SIZETABLE
	rawChunks = (byteSize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	chunkSize = goodIndex[rawChunks];
#else
	rawChunks = (byteSize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	roundoff = (ui(-1)>>ROUNDBITS) >> bfffo(rawChunks);
	chunkSize = (rawChunks+roundoff)&(~roundoff);
#endif
	if ((dataPage = masterMap[chunkSize].dataPage) == NULL) {
	    if ((dataPage = newdatapage(chunkSize)) == NULL) return dataPage;
	} else if (dataPage->count == PAGESIZEBYTES) {
	    if ((dataPage = switchdatapage(dataPage, chunkSize)) == NULL) return dataPage;
	}
	assert ((ui(dataPage) & ui(PAGESIZEBYTES-1)) == 0);
	if ((ptr = dataPage->freelist) == NULL) {
	    ptr = ((char *)dataPage) + dataPage->count;
	} else {
	    dataPage->freelist = ((PToFreeList) ptr)->next;
	}
	dataPage->count += dataPage->size;
        assert (dataPage->count <= PAGESIZEBYTES);
	assert (dataPage->size == BYTESINCHUNK(chunkSize));
	assert (dataPage->size >= byteSize);
	return ptr;
    }
}

STATIC void FREE(void *ptr) {
    register PToDataMap dataPage;
    register UInt count;
    register UInt index;

    index = ui(ptr) & DATAINDEXMASK;
    if (index != 0) {
	dataPage = (PToDataMap) (ui(ptr) - index);
	assert({
		PToFreeList fp = dataPage->freelist;
		while (fp != NULL) {
			if (fp == ptr) break;
			fp = fp->next;
		}
		fp == NULL;
	});
	((PToFreeList)ptr)->next = dataPage->freelist;
	dataPage->freelist = ((PToFreeList)ptr);
	assert (dataPage->size != 0);
	count = dataPage->count;
	if ((dataPage->count = count - dataPage->size) < dataPage->size+DATAOVERHEADBYTES) {
	    freedatapage (dataPage);
	} else if (count == PAGESIZEBYTES) {
	    enablepage (dataPage);
	}
    } else {
	pagefree(ptr);
    }
}

STATIC UInt MALLOC_SIZE(void *ptr) {
    register PToDataMap dataPage;
    register UInt index;
    
    index = ui(ptr) & DATAINDEXMASK;
    if (index != 0) {
	dataPage = (PToDataMap) (ui(ptr) - index);
	return dataPage->size;
    } else {
	return pageallocsize(ptr) * PAGESIZEBYTES;
    }
}

static UInt realloc_size(UInt newsize) {
    register UInt rawChunks, chunkSize, roundoff;
    
    if (newsize>MAXBYTES) {
	return CHUNKS(newsize, PAGESIZEBYTES)*PAGESIZEBYTES;
    } else {
#ifdef SIZETABLE
	rawChunks = (newsize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	chunkSize = goodIndex[rawChunks];
#else
	rawChunks = (newsize+CHUNKSIZEBYTES-1) / CHUNKSIZEBYTES;
	roundoff = (ui(-1)>>ROUNDBITS) >> bfffo(rawChunks);
	chunkSize = (rawChunks+roundoff)&(~roundoff);
#endif SIZETABLE
	return BYTESINCHUNK(chunkSize);
    }
}

void *realloc(void *oldptr, UInt newsize) {
    register UInt oldsize;
    register void *newptr, *p;

    LOCK {
#ifdef FREEKLUDGE
	if (lastfree == oldptr) {
	    lastfree = NULL;
	}
#endif
	oldsize = MALLOC_SIZE(oldptr);
	if (newsize <= oldsize) {
	    newsize = realloc_size(newsize);
	    if (newsize >= oldsize) {
		UNLOCK; return oldptr;
	    } else {
		oldsize = newsize;
	    }
	}
	if ((newptr = MALLOC (newsize)) != NULL) {
	    if (oldsize > MAXBYTES) {
		if (vm_copy (task_self_, oldptr, oldsize, newptr) != KERN_SUCCESS) {
		    MallocErrorFunc();
		    UNLOCK; return NULL;
		}
	    } else {
		bcopy (oldptr, newptr, oldsize);
	    }
	    FREE (oldptr);
	}
    } UNLOCK;
    return newptr;
}

void *calloc(UInt numElems, UInt byteSize) {
    register void *p;

    if (numElems != 1) {
	byteSize *= numElems;
    }
    p = malloc(byteSize);
    /* If the block is larger than MAXBYTES (or PAGESIZEBYTES
     * if single pages are buffered), it was allocated
     * via a vm_allocate, so the block is already zeroed.
     * This check also prevents touching the pages when
     * a large block is allocated.
     */
#ifdef PAGEBUFFERSIZE
    if (byteSize <= PAGESIZEBYTES) {
#else
    if (byteSize <= MAXBYTES) {
#endif
	bzero(p, byteSize);
    }
    return p;
}

void *valloc(UInt byteSize) {
    register void *ptr;

    assert (PAGESIZEBYTES == getpagesize());
    LOCK {
	ptr = pagemalloc(CHUNKS(byteSize, PAGESIZEBYTES));
    } UNLOCK;
    return ptr;
}

#ifdef MTHREAD
void *malloc(UInt byteSize) {
    register void *ptr;

    LOCK {
	ptr = MALLOC(byteSize);
    } UNLOCK;
    return ptr;
}
#endif

#if defined(MTHREAD) || defined(FREEKLUDGE)
void free(void *ptr) {
    LOCK {
#ifdef FREEKLUDGE
	FREE(lastfree);
	lastfree = ptr;
#else
	FREE(ptr);
#endif
    } UNLOCK;
}
#endif

#ifdef MTHREAD
UInt malloc_size(void *ptr) {
    LOCK {
	MALLOC_SIZE(ptr);
    } UNLOCK;
}
#endif

void (*malloc_error(void (*func)(void)))(void) {
    void (*oldfunc)(void);

    oldfunc = MallocErrorFunc;
    MallocErrorFunc = func;
}

#ifdef MSTATS
typedef struct TotalStruct *PToTotal;
typedef struct TotalStruct {
    UInt alloc, free, pages;
} Total;

static void totalDataPage(PToTotal totals, PToDataMap dataPage) {
    UInt free;

    free = (PAGESIZEBYTES - dataPage->count) / dataPage->size;
    totals->pages++;
    totals->alloc += (DATASPACEBYTES / dataPage->size) - free;
    totals->free += free;
}

static void totalMapPage(PToTotal totals, PToPageMap mapPage, PToDataMap dataPage) {
    UInt i;

    for (i=0; i != mapPage->exist; i++) {
        if ((mapPage->data[i] == dataPage) || (i>=mapPage->stuffed)) {
	    totalDataPage(totals, mapPage->data[i]);
	} else {
            totals->pages++;
	    totals->alloc += (DATASPACEBYTES / dataPage->size);
	}
    }
}

UInt mstats() {
    Total totals;
    UInt size, i, hash, nextlargest, larger;
    UInt grandTotalBytes, grandTotalPages;
    PToDataMap dataPage;
    PToPageMap mapPage;
    PToHashBucket ptr;

#ifdef FREEKLUDGE
    free(NULL);
#endif
    LOCK {
	fprintf (stderr, "    bytes allocated      free     pages\n");
	grandTotalBytes = 0;
	grandTotalPages = 0;
	for (size=0; size <= MAXBYTES; size += CHUNKSIZEBYTES) {
	    i = CHUNKSINBYTE(size);
	    if ((dataPage=masterMap[i].dataPage) != NULL) {
		totals.alloc = totals.free = totals.pages = 0;
		if ((mapPage=masterMap[i].mapPage) == NULL) {
		    totalDataPage(&totals, dataPage);
		} else {
		    totalMapPage(&totals, mapPage, dataPage);
		}
		fprintf (stderr, "%9d %9d %9d %9d\n", dataPage->size, totals.alloc, totals.free, totals.pages);
		grandTotalBytes += totals.alloc * dataPage->size;
		grandTotalPages += totals.pages;
	    }
	    size = BYTESINCHUNK(i);
	}
#ifdef PAGEBUFFERSIZE
#    define singlePagesBuffered (pageBuffer.count)
#else
#    define singlePagesBuffered 0
#endif
	totals.pages = singlePagesAllocated - grandTotalPages;
	if (totals.pages != 0) {
	    fprintf (stderr, "%9d %9d %9d %9d\n", PAGESIZEBYTES, totals.pages - singlePagesBuffered, singlePagesBuffered, totals.pages);
	    grandTotalBytes += totals.pages * PAGESIZEBYTES;
	    grandTotalPages += totals.pages;
	}
	i=1;
	do {
	    larger = 0;
	    nextlargest = ui(-1);
	    totals.alloc = 0;
	    for (hash=0; hash!=HASHBUCKETS; hash++) {
		for (ptr = pagehash[hash]; ptr != NULL; ptr = ptr->next) {
		    if (ptr->size == i) {
			totals.alloc++;
		    } else if (ptr->size > i) {
			larger = 1;
			if (ptr->size < nextlargest) nextlargest = ptr->size;
		    }
		}
	    }
	    if (totals.alloc != 0) {
		fprintf (stderr, "%9d %9d %9d %9d\n", PAGESIZEBYTES * i, totals.alloc, 0, totals.alloc * i);
		grandTotalBytes += totals.alloc * i * PAGESIZEBYTES;
		grandTotalPages += totals.alloc * i;
	    }
	    i = nextlargest;
	} while (larger);
    } UNLOCK;
    if (grandTotalPages == 0) {
	fprintf (stderr, "nothing allocated!\n");
    } else {
	fprintf (stderr, "total: %d bytes in %d pages. efficiency: %.1f%%\n", grandTotalBytes, grandTotalPages, 100.0 * ((double)grandTotalBytes) / ((double)grandTotalPages*PAGESIZEBYTES));
    }
    return grandTotalPages*PAGESIZEBYTES;
}
#endif

#ifdef TEST

#define TESTBLOCKS 1024
#define TESTMINSIZE 0
#define TESTMAXSIZE 8192*4
#define REPETITIONS 5000

static char *block[TESTBLOCKS];
static UInt requests[TESTBLOCKS];
static UInt sizes[TESTBLOCKS];
#ifdef MTHREAD
static UInt locks[TESTBLOCKS];
static UInt statlock;
#define THREADS 2
#endif
static void malloc_tester(int arg);

main(int argc, char **argv) {
    UInt i;

    malloc_good_size(12);
    for (i=0; i!=TESTBLOCKS; i++) {	
	block[i] = NULL;
    }
#ifdef MTHREAD
    for (i=0; i!=THREADS; i++) {
        cthread_fork(malloc_tester, i);
    }
#endif
    malloc_tester(i);
}

static void malloc_tester(int arg) {
    UInt i, j, size, request;
    UInt totalRequested = 0;
    UInt totalAllocated = 0;

    for (;;) {
	for (i=0; i!=REPETITIONS; i++) {
	    j = ui(random()) % TESTBLOCKS;
#ifdef MTHREAD
	    spin_lock(&locks[j]);
#endif
	    if (block[j] != NULL) {
		totalAllocated -= sizes[j];
		totalRequested -= requests[j];
		/*bzero(block[j], requests[j]);*/
		if (random() & ui(1)) {
		    request = (ui(random()) % TESTMAXSIZE) + TESTMINSIZE;
		    block[j] = realloc(block[j], request);
		    requests[j] = request;
		    sizes[j] = size = malloc_size(block[j]);
		    if (size < request) {
			fprintf (stderr, "block was too small: request = %d, size = %d\n", request, size);
		    }
		    totalAllocated += size;
		    totalRequested += request;		    
		} else {
		    free(block[j]);
		    block[j] = NULL;
		}
	    } else {
		request = (ui(random()) % TESTMAXSIZE) + TESTMINSIZE;
		block[j] = malloc(request);
		requests[j] = request;
		sizes[j] = size = malloc_size(block[j]);
		if (size < request) {
		    fprintf (stderr, "block was too small: request = %d, size = %d\n", request, size);
		}
		totalAllocated += size;
		totalRequested += request;
	    }
#ifdef MTHREAD
	    spin_unlock(&locks[j]);
#endif
	}
#ifdef MTHREAD
	spin_lock(&statlock);
	fprintf (stderr, "%d ", arg);
#else
#ifdef MSTATS
	(void) mstats();
#endif
#endif
	fprintf (stderr, "* pages: %d for %d, %d bytes: efficiencies = %.3f%%, %.3f%%\n", allocatedPages, totalAllocated, totalRequested, 100.0 * (double)totalAllocated / ((double)allocatedPages*PAGESIZEBYTES), 100.0 * (double)totalRequested / ((double)allocatedPages*PAGESIZEBYTES));
	fflush (stderr);
#ifdef MTHREAD
	spin_unlock(&statlock);
#endif
    }
}
#endif

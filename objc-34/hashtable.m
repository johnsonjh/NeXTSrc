/*
	hashtable.m
  	Copyright 1989 NeXT, Inc.
	Created by Bertrand Serlet, Feb 89
 */

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <mach.h>
#import <cthreads.h>

#import "hashtable.h"
#import "objc-private.h"

/* In order to improve efficiency, buckets contain a pointer to an array or directly the data when the array size is 1 */
typedef union {
    const void	*one;
    const void	**many;
    } oneOrMany;
    /* an optimization consists of storing directly data when count = 1 */
    
typedef struct	{
    unsigned 	count; 
    oneOrMany	elements;
    } HashBucket;
    /* private data structure; may change */
    
/*************************************************************************
 *
 *	Macros and utilities
 *	
 *************************************************************************/

static unsigned log2 (unsigned x) { return (x<2) ? 0 : log2 (x>>1)+1; };

static unsigned exp2m1 (unsigned x) { return (1 << x) - 1; };

#define	PTRSIZE		sizeof(void *)

#define	ALLOCTABLE(z)	((NXHashTable *) NXZoneMalloc (z,sizeof (NXHashTable)))
#define	ALLOCBUCKETS(z,nb)((HashBucket *) NXZoneCalloc (z, nb, sizeof (HashBucket)))
#define	ALLOCPAIRS(z,nb) ((const void **) NXZoneCalloc (z, nb, sizeof (void *)))

/* iff necessary this modulo can be optimized since the nbBuckets is of the form 2**n-1 */
#define	BUCKETOF(table, data) (((HashBucket *)table->buckets)+((*table->prototype->hash)(table->info, data) % table->nbBuckets))

#define ISEQUAL(table, data1, data2) ((data1 == data2) || (*table->prototype->isEqual)(table->info, data1, data2))
	/* beware of double evaluation */
	
/*************************************************************************
 *
 *	Global data and bootstrap
 *	
 *************************************************************************/
 
static unsigned hashPrototype (const void *info, const void *data) {
    NXHashTablePrototype	*proto = (NXHashTablePrototype *) data;
    
    return NXPtrHash(info, proto->hash) ^ NXPtrHash(info, proto->isEqual) ^ NXPtrHash(info, proto->free) ^ (unsigned) proto->style;
    };

static int isEqualPrototype (const void *info, const void *data1, const void *data2) {
    NXHashTablePrototype	*proto1 = (NXHashTablePrototype *) data1;
    NXHashTablePrototype	*proto2 = (NXHashTablePrototype *) data2;
    
    return (proto1->hash == proto2->hash) && (proto1->isEqual == proto2->isEqual) && (proto1->free == proto2->free) && (proto1->style == proto2->style);
    };
    
static NXHashTablePrototype protoPrototype = {
    hashPrototype, isEqualPrototype, NXNoEffectFree, 0
    };

static NXHashTable *prototypes = NULL;
	/* table of all prototypes */

static void bootstrap (void) {
    free(malloc(8));
    prototypes = ALLOCTABLE (NXDefaultMallocZone());
    prototypes->prototype = &protoPrototype; 
    prototypes->count = 1;
    prototypes->nbBuckets = 1; /* has to be 1 so that the right bucket is 0 */
    prototypes->buckets = ALLOCBUCKETS(NXDefaultMallocZone(),  1);
    prototypes->info = NULL;
    ((HashBucket *) prototypes->buckets)[0].count = 1;
    ((HashBucket *) prototypes->buckets)[0].elements.one = &protoPrototype;
    };

/*************************************************************************
 *
 *	On z'y va
 *	
 *************************************************************************/

NXHashTable *NXCreateHashTable (NXHashTablePrototype prototype, unsigned capacity, const void *info) {
    return NXCreateHashTableFromZone(prototype, capacity, info, NXDefaultMallocZone());
}

NXHashTable *NXCreateHashTableFromZone (NXHashTablePrototype prototype, unsigned capacity, const void *info, NXZone *zone) {
    NXHashTable			*table;
    NXHashTablePrototype	*proto;
    
    table = ALLOCTABLE(zone);
    if (! prototypes) bootstrap ();
    if (! prototype.hash) prototype.hash = NXPtrHash;
    if (! prototype.isEqual) prototype.isEqual = NXPtrIsEqual;
    if (! prototype.free) prototype.free = NXNoEffectFree;
    if (prototype.style) {
	fprintf (stderr, "*** NXCreateHashTable: invalid style\n");
	return NULL;
	};
    proto = NXHashGet (prototypes, &prototype); 
    if (! proto) {
	proto
	= (NXHashTablePrototype *) NXZoneMalloc (NXDefaultMallocZone(),
	                                         sizeof (NXHashTablePrototype));
	bcopy (&prototype, proto, sizeof (NXHashTablePrototype));
    	(void) NXHashInsert (prototypes, proto);
	proto = NXHashGet (prototypes, &prototype);
	if (! proto) {
	    fprintf (stderr, "*** NXCreateHashTable: bug\n");
	    return NULL;
	    };
	};
    table->prototype = proto; table->count = 0; table->info = info;
    table->nbBuckets = exp2m1 (log2 (capacity)+1);
    table->buckets = ALLOCBUCKETS(zone, table->nbBuckets);
    return table;
    }

static void freeBucketPairs (void (*freeProc)(const void *info, void *data), HashBucket bucket, const void *info) {
    unsigned	j = bucket.count;
    const void	**pairs;
    
    if (j == 1) {
	(*freeProc) (info, (void *) bucket.elements.one);
	return;
	};
    pairs = bucket.elements.many;
    while (j--) {
	(*freeProc) (info, (void *) *pairs);
	pairs ++;
	};
    free (bucket.elements.many);
    };
    
static void freeBuckets (NXHashTable *table, int freeObjects) {
    unsigned		i = table->nbBuckets;
    HashBucket		*buckets = (HashBucket *) table->buckets;
    
    while (i--) {
	if (buckets->count) {
	    freeBucketPairs ((freeObjects) ? table->prototype->free : NXNoEffectFree, *buckets, table->info);
	    buckets->count = 0;
	    buckets->elements.one = NULL;
	    };
	buckets++;
	};
    };
    
void NXFreeHashTable (NXHashTable *table) {
    freeBuckets (table, YES);
    free (table->buckets);
    free (table);
    };
    
void NXEmptyHashTable (NXHashTable *table) {
    freeBuckets (table, NO);
    table->count = 0;
    }

void NXResetHashTable (NXHashTable *table) {
    freeBuckets (table, YES);
    table->count = 0;
}

BOOL NXIsEqualHashTable (NXHashTable *table1, NXHashTable *table2) {
    if (table1 == table2) return YES;
    if (NXCountHashTable (table1) != NXCountHashTable (table2)) return NO;
    else {
	void		*data;
	NXHashState	state = NXInitHashState (table1);
	while (NXNextHashState (table1, &state, &data)) {
	    if (! NXHashMember (table2, data)) return NO;
	}
	return YES;
    }
}

BOOL NXCompareHashTables (NXHashTable *table1, NXHashTable *table2) {
    if (table1 == table2) return YES;
    if (NXCountHashTable (table1) != NXCountHashTable (table2)) return NO;
    else {
	void		*data;
	NXHashState	state = NXInitHashState (table1);
	while (NXNextHashState (table1, &state, &data)) {
	    if (! NXHashMember (table2, data)) return NO;
	}
	return YES;
    }
}

NXHashTable *NXCopyHashTable (NXHashTable *table) {
    NXHashTable		*new;
    NXHashState		state = NXInitHashState (table);
    void		*data;
    NXZone 		*zone = NXZoneFromPtr(table);
    
    new = ALLOCTABLE(zone);
    new->prototype = table->prototype; new->count = 0;
    new->info = table->info;
    new->nbBuckets = table->nbBuckets;
    new->buckets = ALLOCBUCKETS(zone, new->nbBuckets);
    while (NXNextHashState (table, &state, &data))
	(void) NXHashInsert (new, data);
    return new;
    }

unsigned NXCountHashTable (NXHashTable *table) {
    return table->count;
    }

int NXHashMember (NXHashTable *table, const void *data) {
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    
    if (! j) return 0;
    if (j == 1) {
    	return ISEQUAL(table, data, bucket->elements.one);
	};
    pairs = bucket->elements.many;
    while (j--) {
	/* we don't cache isEqual because lists are short */
    	if (ISEQUAL(table, data, *pairs)) return 1; 
	pairs ++;
	};
    return 0;
    }

void *NXHashGet (NXHashTable *table, const void *data) {
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    
    if (! j) return NULL;
    if (j == 1) {
    	return ISEQUAL(table, data, bucket->elements.one)
	    ? (void *) bucket->elements.one : NULL; 
	};
    pairs = bucket->elements.many;
    while (j--) {
	/* we don't cache isEqual because lists are short */
    	if (ISEQUAL(table, data, *pairs)) return (void *) *pairs; 
	pairs ++;
	};
    return NULL;
    }

static void _NXHashRehash (NXHashTable *table) {
    /* Rehash: we create a pseudo table pointing really to the old guys,
    extend self, copy the old pairs, and free the pseudo table */
    NXHashTable	*old;
    NXHashState	state;
    void	*aux;
    NXZone 	*zone = NXZoneFromPtr(table);
    
    old = ALLOCTABLE(zone);
    old->prototype = table->prototype; old->count = table->count; 
    old->nbBuckets = table->nbBuckets; old->buckets = table->buckets;
    table->nbBuckets += table->nbBuckets + 1; /* 2 times + 1 */
    table->count = 0; table->buckets = ALLOCBUCKETS(zone, table->nbBuckets);
    state = NXInitHashState (old);
    while (NXNextHashState (old, &state, &aux))
	(void) NXHashInsert (table, aux);
    freeBuckets (old, NO);
    if (old->count != table->count)
	fprintf(stderr,"*** hashtable: count differs after rehashing; probably indicates a broken invariant: there are x and y such as isEqual(x, y) is TRUE but hash(x) != hash (y)\n");
    free (old->buckets); 
    free (old);
    };

void *NXHashInsert (NXHashTable *table, const void *data) {
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**new;
    NXZone 	*zone = NXZoneFromPtr(table);
    
    if (! j) {
	bucket->count++; bucket->elements.one = data; 
	table->count++; 
	return NULL;
	};
    if (j == 1) {
    	if (ISEQUAL(table, data, bucket->elements.one)) {
	    const void	*old = bucket->elements.one;
	    bucket->elements.one = data;
	    return (void *) old;
	    };
	new = ALLOCPAIRS(zone, 2);
	new[1] = bucket->elements.one;
	*new = data;
	bucket->count++; bucket->elements.many = new; 
	table->count++; 
	if (table->count > table->nbBuckets) _NXHashRehash (table);
	return NULL;
	};
    pairs = bucket->elements.many;
    while (j--) {
	/* we don't cache isEqual because lists are short */
    	if (ISEQUAL(table, data, *pairs)) {
	    const void	*old = *pairs;
	    *pairs = data;
	    return (void *) old;
	    };
	pairs ++;
	};
    /* we enlarge this bucket; and put new data in front */
    new = ALLOCPAIRS(zone, bucket->count+1);
    if (bucket->count) bcopy (bucket->elements.many, new+1, bucket->count * PTRSIZE);
    *new = data;
    free (bucket->elements.many);
    bucket->count++; bucket->elements.many = new; 
    table->count++; 
    if (table->count > table->nbBuckets) _NXHashRehash (table);
    return NULL;
    }

void *NXHashInsertIfAbsent (NXHashTable *table, const void *data) {
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**new;
    NXZone 	*zone = NXZoneFromPtr(table);
    
    if (! j) {
	bucket->count++; bucket->elements.one = data; 
	table->count++; 
	return (void *) data;
	};
    if (j == 1) {
    	if (ISEQUAL(table, data, bucket->elements.one))
	    return (void *) bucket->elements.one;
	new = ALLOCPAIRS(zone, 2);
	new[1] = bucket->elements.one;
	*new = data;
	bucket->count++; bucket->elements.many = new; 
	table->count++; 
	if (table->count > table->nbBuckets) _NXHashRehash (table);
	return (void *) data;
	};
    pairs = bucket->elements.many;
    while (j--) {
	/* we don't cache isEqual because lists are short */
    	if (ISEQUAL(table, data, *pairs))
	    return (void *) *pairs;
	pairs ++;
	};
    /* we enlarge this bucket; and put new data in front */
    new = ALLOCPAIRS(zone, bucket->count+1);
    if (bucket->count) bcopy (bucket->elements.many, new+1, bucket->count * PTRSIZE);
    *new = data;
    free (bucket->elements.many);
    bucket->count++; bucket->elements.many = new; 
    table->count++; 
    if (table->count > table->nbBuckets) _NXHashRehash (table);
    return (void *) data;
    }

void *NXHashRemove (NXHashTable *table, const void *data) {
    HashBucket	*bucket = BUCKETOF(table, data);
    unsigned	j = bucket->count;
    const void	**pairs;
    const void	**new;
    NXZone 	*zone = NXZoneFromPtr(table);
    
    if (! j) return NULL;
    if (j == 1) {
	if (! ISEQUAL(table, data, bucket->elements.one)) return NULL;
	data = bucket->elements.one;
	table->count--; bucket->count--; bucket->elements.one = NULL;
	return (void *) data;
	};
    pairs = bucket->elements.many;
    if (j == 2) {
	int	del;
	
    	if (ISEQUAL(table, data, pairs[0])) {
	    bucket->elements.one = pairs[1]; data = pairs[0];
	    }
	else if (ISEQUAL(table, data, pairs[1])) {
	    bucket->elements.one = pairs[0]; data = pairs[1];
	    }
	else return NULL;
	free (pairs);
	table->count--; bucket->count--;
	return (void *) data;
	};
    while (j--) {
    	if (ISEQUAL(table, data, *pairs)) {
	    data = *pairs;
	    /* we shrink this bucket */
	    new = (bucket->count-1) 
		? ALLOCPAIRS(zone, bucket->count-1) : NULL;
	    if (bucket->count-1 != j)
		    bcopy (bucket->elements.many, new, PTRSIZE*(bucket->count-j-1));
	    if (j)
		    bcopy (bucket->elements.many + bucket->count-j, new+bucket->count-j-1, PTRSIZE*j);
	    free (bucket->elements.many);
	    table->count--; bucket->count--; bucket->elements.many = new;
	    return (void *) data;
	    };
	pairs ++;
	};
    return NULL;
    }

NXHashState NXInitHashState (NXHashTable *table) {
    NXHashState	state;
    
    state.i = table->nbBuckets;
    state.j = 0;
    return state;
    };
    
int NXNextHashState (NXHashTable *table, NXHashState *state, void **data) {
    HashBucket		*buckets = (HashBucket *) table->buckets;
    
    while (state->j == 0) {
	if (state->i == 0) return NO;
	state->i--; state->j = buckets[state->i].count;
	}
    state->j--;
    buckets += state->i;
    *data = (void *) ((buckets->count == 1) 
    		? buckets->elements.one : buckets->elements.many[state->j]);
    return YES;
    };

/*************************************************************************
 *
 *	Conveniences
 *	
 *************************************************************************/

unsigned NXPtrHash (const void *info, const void *data) {
    return (((unsigned) data) >> 16) ^ ((unsigned) data);
    };
    
unsigned NXStrHash (const void *info, const void *data) {
    register unsigned		hash = 0;
    register unsigned char	*s = (unsigned char *) data;
    /* unsigned to avoid a sign-extend */
    /* unroll the loop */
    if (s) for (; ; ) { 
	if (*s == '\0') break;
	hash ^= *s++;
	if (*s == '\0') break;
	hash ^= *s++ << 8;
	if (*s == '\0') break;
	hash ^= *s++ << 16;
	if (*s == '\0') break;
	hash ^= *s++ << 24;
	}
    return hash;
    };
    
int NXPtrIsEqual (const void *info, const void *data1, const void *data2) {
    return data1 == data2;
    };

int NXStrIsEqual (const void *info, const void *data1, const void *data2) {
    if (data1 == data2) return YES;
    if (! data1) return ! strlen ((char *) data2);
    if (! data2) return ! strlen ((char *) data1);
    if (((char *) data1)[0] != ((char *) data2)[0]) return NO;
    return (strcmp ((char *) data1, (char *) data2)) ? NO : YES;
    };
    
void NXNoEffectFree (const void *info, void *data) {};

void NXReallyFree (const void *info, void *data) {
    free (data);
    };

/* All the following functions are really private, made non-static only for the benefit of shlibs */
unsigned hashPtrStructKey (const void *info, const void *data) {
    return NXPtrHash(info, *((void **) data));
    };
int isEqualPtrStructKey (const void *info, const void *data1, const void *data2) {
    return NXPtrIsEqual (info, *((void **) data1), *((void **) data2));
    };
unsigned hashStrStructKey (const void *info, const void *data) {
    return NXStrHash(info, *((char **) data));
    };
int isEqualStrStructKey (const void *info, const void *data1, const void *data2) {
    return NXStrIsEqual (info, *((char **) data1), *((char **) data2));
    };


/*************************************************************************
 *
 *	Unique strings
 *	
 *************************************************************************/

/* the implementation could be made faster at the expense of memory if the size of the strings were kept around */
static NXHashTable *uniqueStrings = NULL;

static int accessUniqueString = 0;

static vm_address_t	zone = 0;
static vm_size_t	zoneSize = 0;
static mutex_t		lock = NULL;

static const char *CopyIntoReadOnly (const char *str) {
    int		len = strlen (str) + 1;
    char	*new;
    
    if (! lock) lock = mutex_alloc ();
    mutex_lock (lock);
    if (zoneSize < len) {
	zoneSize = vm_page_size *((len + vm_page_size - 1) / vm_page_size);
	/* not enough room, we try to allocate.  If no room left, too bad */
	(void) vm_allocate (task_self(), &zone, zoneSize, 1);
	};
    
    new = (char *) zone;
    bcopy (str, new, len);
    zone += len;
    zoneSize -= len;
    mutex_unlock (lock);
    return new;
    };
    
NXAtom NXUniqueString (const char *buffer) {
    const char	*previous;
    
    if (! buffer) return buffer;
    accessUniqueString++;
    if (! uniqueStrings)
    	uniqueStrings = NXCreateHashTable (NXStrPrototype, 0, NULL);
    previous = (const char *) NXHashGet (uniqueStrings, buffer);
    if (previous) return previous;
    previous = CopyIntoReadOnly (buffer);
    if (NXHashInsert (uniqueStrings, previous)) {
	fprintf (stderr, "*** NXUniqueString: invariant broken\n");
	return NULL;
	};
    return previous;
    };

NXAtom NXUniqueStringNoCopy (const char *string) {
    accessUniqueString++;
    if (! uniqueStrings)
    	uniqueStrings = NXCreateHashTable (NXStrPrototype, 0, NULL);
    return (const char *) NXHashInsertIfAbsent (uniqueStrings, string);
    };

static char		*globalBuffer = NULL;
static int		globalBufferSize = 0;
static mutex_t		globalLock = NULL;

NXAtom NXUniqueStringWithLength (const char *buffer, int length) {
    NXAtom	atom;
    if (! globalLock) globalLock = mutex_alloc ();
    mutex_lock (globalLock);
    if (length+1 > globalBufferSize) {
	globalBufferSize = length+1;
	globalBuffer = (char *) realloc (globalBuffer, globalBufferSize);
	};
    bcopy (buffer, globalBuffer, length);
    globalBuffer[length] = '\0';
    atom = NXUniqueString (globalBuffer);
    mutex_unlock (globalLock);
    return atom;
    };

char *NXCopyStringBufferFromZone (const char *str, NXZone *zone) {
    return strcpy ((char *) NXZoneMalloc (zone, strlen (str) + 1), str);
    };
    
char *NXCopyStringBuffer (const char *str) {
    return NXCopyStringBufferFromZone(str, NXDefaultMallocZone());
    };
    
static void uniqueStringsStatistics (void) {
    unsigned	i = uniqueStrings->nbBuckets;
    HashBucket	*buckets = (HashBucket *) uniqueStrings->buckets;
    
    printf ("Size: %d\tcount: %d\taccesses: %d\n", i, uniqueStrings->count, accessUniqueString);
    while (i--) {
	if (buckets->count) {
	    if (buckets->count == 1)
		printf ("1\t%s\n", (char *) buckets->elements.one);
	    else {
		int	j = buckets->count;
		char	**pairs = (char **) buckets->elements.many;
		printf ("%d\t", buckets->count);
		while (j--) printf ("%s ", *(pairs++));
		printf ("\n");
		};
	    };
	buckets++;
	};
    };







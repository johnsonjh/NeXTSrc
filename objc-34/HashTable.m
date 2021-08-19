/*
	HashTable.m
  	Copyright 1988, 1989 NeXT, Inc.
	Written by Bertrand Serlet, Dec 88
	Responsibility: Bertrand Serlet
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "HashTable.h"
#import "NXStringTable.h"

extern unsigned _strhash(const char *);

typedef struct {const void *key; void *value;} Pair;
typedef	Pair	*Pairs;
typedef struct	{unsigned count; Pairs pairs;} Bucket;
typedef	Bucket	*Buckets;

#define INITCAPA	1	/* _nbBuckets has to be a 2**N-1, and at least 1 */
#define PAIRSSIZE(count) ((count) * sizeof(Pair))
#define HASHSTR(str) (_strhash (str))
#define HASHINT(x) ((((unsigned) x) >> 16) ^ ((unsigned) x))

static unsigned log2 (unsigned x) { return (x<2) ? 0 : log2 (x>>1)+1; };

static unsigned exp2m1 (unsigned x) { return (1 << x) - 1; };

static unsigned hash (const char *keyDesc, const void *aKey, unsigned mod) {
    switch (keyDesc[0]) {
    	case '@': return [(id) aKey hash] % mod;
	case '%':
	case '*': return (aKey && *((char *) aKey)) ? HASHSTR (aKey) % mod : 0;
	default: return HASHINT (aKey) % mod;
	};
    };  
    
static unsigned isEqual (const char *keyDesc, const void *key1, const void *key2) {
    if (key1 == key2) return YES;
    switch (keyDesc[0]) {
    	case '@': return [(id) key1 isEqual: (id) key2];
	case '%':
	case '*':
		if (! key1) return (strlen (key2) == 0);
		if (! key2) return (strlen (key1) == 0);
		if (((char *) key1)[0] != ((char *) key2)[0]) return NO;
		return (strcmp (key1, key2) == 0);
	default: return NO;
	};
    };
    
static void freeObject (void *item) {
    [(id) item free];
    };
    
static void noFree (void *item) {
    };
    
@implementation  HashTable

+ initialize
{
    [self setVersion: 1];
    return self;
}

- _initBare: (const char *) aKeyDesc : (const char *) aValueDesc : (unsigned) capacity
/* used for efficient implementation of rehashing: does create a semi ok table */
{
    count = 0; _nbBuckets = capacity;
    keyDesc = (aKeyDesc) ? aKeyDesc : "@"; 
    valueDesc = (aValueDesc) ? aValueDesc : "@";
    _buckets = nil;
    return self;
}

+ _newBare: (const char *) aKeyDesc : (const char *) aValueDesc : (unsigned) capacity
/* used for efficient implementation of rehashing: does create a semi ok table */
{
    return [[self allocFromZone: NXDefaultMallocZone()] 
	        _initBare:aKeyDesc:aValueDesc:capacity];
}

- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc
{
    return [self initKeyDesc: aKeyDesc valueDesc: aValueDesc capacity: INITCAPA];
}

- initKeyDesc: (const char *) aKeyDesc
{
    return [self initKeyDesc: aKeyDesc valueDesc: NULL];
}

- init
{
    return [self initKeyDesc: NULL];
}
- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc capacity: (unsigned) aCapacity
{
    [self _initBare: aKeyDesc: aValueDesc: exp2m1 (log2 (aCapacity)+1)];
    _buckets = NXZoneCalloc ([self zone], _nbBuckets, sizeof(Bucket));
    return self;
}

+ newKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc capacity: (unsigned) aCapacity
{
    return [[self allocFromZone: NXDefaultMallocZone()]
	initKeyDesc:aKeyDesc valueDesc:aValueDesc capacity:aCapacity];
}

+ newKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc
{
    return [self newKeyDesc: aKeyDesc valueDesc: aValueDesc capacity: INITCAPA];
}

+ newKeyDesc: (const char *) aKeyDesc
{
    return [self newKeyDesc: aKeyDesc valueDesc: NULL];
}

+ new
{
    return [self newKeyDesc: NULL];
}

- free {
    [self freeKeys: noFree values: noFree];
    free(_buckets);
    return [super free];
    }

- freeObjects {
    return [self 
	freeKeys: (keyDesc[0] == '@') ? freeObject : noFree 
	values: (valueDesc[0] == '@') ? freeObject : noFree 
	];
    }

- freeKeys: (void (*) (void *)) keyFunc values: (void (*) (void *)) valueFunc {
    unsigned	i = _nbBuckets;
    Buckets	buckets = (Buckets) _buckets;
    while (i--) {
	if (buckets->count) {
	    unsigned	j = buckets->count;
	    Pairs	pairs = buckets->pairs;
	    while (j--) {
		(*keyFunc) ((void *) pairs->key);
		(*valueFunc) (pairs->value);
		pairs++;
		};
	    free(buckets->pairs);
	    buckets->count = 0;
	    buckets->pairs = (Pairs) nil;
	    };
	buckets++;
	};
    count = 0;
    return self;
    };
    
- empty
{
    unsigned	i = _nbBuckets;
    Buckets	buckets = (Buckets) _buckets;
    while (i--) {
	if (buckets->count) free(buckets->pairs);
	buckets->count = 0;
	buckets->pairs = NULL;
	buckets++;
	};
    count = 0;
    return self;
}

- copy
{
    return [self copyFromZone: [self zone]];
}


- copyFromZone:(NXZone *)zone
/* we enumerate the table instead of blind copy to create a hash table of the right size */
{
    HashTable	*new = [[[self class] allocFromZone: zone] 
	                                   initKeyDesc: keyDesc
							 		    valueDesc: valueDesc
									     capacity: count];
    NXHashState	state = [self initState];
    const void	*key;
    void	*value;
    while ([self nextState: &state key: &key value: &value])
    	[new insertKey: key value: value];
    return new;
}

- (unsigned) count
{
    return count;
}

- (BOOL) isKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs;
    if (j == 0) return NO;
    pairs = bucket.pairs;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) return YES; 
	pairs++;
	};
    return NO;
}

- (void *) valueForKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs;
    if (j == 0) return nil;
    pairs = bucket.pairs;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) return pairs->value; 
	pairs++;
	};
    return nil;
}

- (void *) _insertKeyNoRehash: (const void *) aKey value: (void *) aValue
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs = bucket.pairs;
    Pairs	new;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) {
		void	*old = pairs->value;
		pairs->key = aKey;
		pairs->value = aValue;
		return old;
		};
	pairs++;
	};
    /* we enlarge this bucket; this could be optimized by using realloc */
    new = (Pairs) NXZoneMalloc ([self zone], PAIRSSIZE(bucket.count+1));
    if (bucket.count) bcopy (bucket.pairs, new+1, PAIRSSIZE(bucket.count));
    new->key = aKey; new->value = aValue;
    if (bucket.count) free (bucket.pairs);
    buckets[index].count++; buckets[index].pairs = new; count++; 
    return nil;
}

- (void *) insertKey: (const void *) aKey value: (void *) aValue
{
    void	*result = [self _insertKeyNoRehash: aKey value: aValue];
    if (result) return result; /* it was a replace */
    if (count > _nbBuckets) {
    	/* Rehash: we create a pseudo table pointing really to the old guys,
	extend self, copy the old pairs, and free the pseudo table */
	HashTable	* old = [[HashTable allocFromZone: [self zone]]
	                     _initBare: keyDesc: valueDesc: _nbBuckets];
        NXHashState	state;
        const void	*key;
	void		*value;
	
	old->count = count; old->_buckets = _buckets;
	_nbBuckets += _nbBuckets + 1;
	count = 0;
	_buckets = NXZoneCalloc ([self zone], _nbBuckets, sizeof(Bucket));
	state = [old initState];
	while ([old nextState: &state key: &key value: &value])
	    [self insertKey: key value: value];
	if (old->count != count)
	    fprintf(stderr,"*** HashTable: count differs after rehashing; probably indicates a broken invariant: there are x and y such as isEqual(x, y) is TRUE but hash(x) != hash (y)\n");
	[old free];
	};
    return nil;
}

- (void *) removeKey: (const void *) aKey
{
    Buckets	buckets = (Buckets) _buckets;
    unsigned	index = hash (keyDesc, aKey, _nbBuckets);
    Bucket	bucket = buckets[index];
    unsigned	j = bucket.count;
    Pairs	pairs = bucket.pairs;
    Pairs	new;
    while (j--) {
    	if (isEqual (keyDesc, aKey, pairs->key)) {
		/* we shrink this bucket; this could be optimized by using realloc */
		void	*oldValue = pairs->value;
		new = (bucket.count-1) ? (Pairs) NXZoneMalloc (
		    [self zone], PAIRSSIZE(bucket.count-1)) : 0;
		if (bucket.count-1 != j)
			bcopy (bucket.pairs, new, PAIRSSIZE(bucket.count-j-1));
		if (j)
			bcopy (bucket.pairs+bucket.count-j, new+bucket.count-j-1,PAIRSSIZE (j));
	        free (bucket.pairs); 
		count--; buckets[index].count--; buckets[index].pairs = new;
		return oldValue;
		};
	pairs++;
	};
    return nil;
}

- (NXHashState) initState {
    NXHashState	state;
    state.i = _nbBuckets;
    state.j = 0;
    return state;
    };
    
- (BOOL) nextState: (NXHashState *) aState key: (const void **) aKey value: (void **) aValue {
    Buckets	buckets = (Buckets) _buckets;
    Bucket	bucket;
    Pair	pair;
    while (aState->j == 0) {
	if (aState->i == 0) return NO;
	aState->i--; aState->j = buckets[aState->i].count;
	}
    aState->j--;
    pair = buckets[aState->i].pairs[aState->j];
    *aKey = pair.key; *aValue = pair.value;
    return YES;
    };

- write:(NXTypedStream *) stream
{
    NXHashState		state = [self initState];
    const void	*key;
    void	*value;
    [super write: stream];
    NXWriteTypes (stream, "i%%", &count, &keyDesc, &valueDesc);
    while ([self nextState: &state key: &key value: &value]) {
	NXWriteType (stream, keyDesc, &key);
	NXWriteType (stream, valueDesc, &value);
	};
    return self;
    }

- read:(NXTypedStream *) stream
{
    unsigned	nb; /* we set count as 0 but read nb elements */
    if (NXTypedStreamClassVersion (stream, "HashTable") == 0) {
        NXReadTypes (stream, "i**", &nb, &keyDesc, &valueDesc);
    } else {
	[super read: stream];
        NXReadTypes (stream, "i%%", &nb, &keyDesc, &valueDesc);
    }
    if (! keyDesc) exit (1);
    if (! valueDesc) exit (2);
    count = 0;
    _nbBuckets = exp2m1 (log2 (nb)+1);
    _buckets = NXZoneCalloc ([self zone], _nbBuckets, sizeof(Bucket));
    while (nb--) {
    	const void	*key;
	void	*value;
	NXReadType (stream, keyDesc, &key);
	NXReadType (stream, valueDesc, &value);
	[self _insertKeyNoRehash: key value: value];
        };
    return self;
    }

static void DebugPrint (NXStream *stream, const char *desc, const void *data) {
    switch (desc[0]) {
    	case '@': 
	    NXPrintf (stream, "%s[0x%x]", [(id) data name], (int) data);
	    return;
	case 'i':
	    NXPrintf (stream, "%d[0x%x]", (int) data, (int) data);
	    return;
	case '%':
	case '*': 
	    NXPrintf (stream, "\"%s\"", (char *) data);
	    return;
	default: 
	    NXPrintf (stream, "0x%x", (int) data);
	    return;
	};
    };  
    
- _debugPrint: (NXStream *) stream {
    unsigned	i = _nbBuckets;
    Buckets	buckets = (Buckets) _buckets;

    NXPrintf (stream, "Table [%s -> %s]: \tcount: %d\tcapacity: %d\n", 
	keyDesc, valueDesc, count, _nbBuckets);
    while (i--) {
	if (buckets->count) {
	    unsigned	j = buckets->count;
	    Pairs	pairs = buckets->pairs;

	    NXPrintf (stream, "%d\t", j);
	    while (j--) {
		DebugPrint (stream, keyDesc, pairs->key);
		NXPrintf (stream, ": ");
		DebugPrint (stream, valueDesc, pairs->value);
		NXPrintf (stream, "\t");
		pairs++;
		};
	    NXPrintf (stream, "\n");
	    };
	buckets++;
	};
    NXPrintf (stream, "\n");
    NXFlush (stream);
    return self;
    };
    
@end

static short pad_to_align_global_data_to_4_byte_alignment = 0;







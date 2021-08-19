/*
    HashTable.h
    Copyright 1988, 1989 NeXT, Inc.
	
    DEFINED AS: A common class
    HEADER FILES: objc/HashTable.h

*/

#import "Object.h"
#import "hashtable.h"
#import "typedstream.h"

@interface HashTable: Object
{
    unsigned	count;		/* Current number of associations */
    const char	*keyDesc;	/* Description of keys */
    const char	*valueDesc;	/* Description of values */
    unsigned	_nbBuckets;	/* Current size of the array */
    void	*_buckets;	/* Data array */
}

/* Initializing */

- init;
- initKeyDesc: (const char *)aKeyDesc;
- initKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc;
- initKeyDesc: (const char *) aKeyDesc valueDesc: (const char *) aValueDesc 
	capacity: (unsigned) aCapacity;

/* Freeing */

- free;	
- freeObjects;
- freeKeys:(void (*) (void *))keyFunc values:(void (*) (void *))valueFunc;
- empty;

/* Copying */

- copy;
- copyFromZone:(NXZone *)zone;
  
/* Manipulating */

- (unsigned)count;
- (BOOL)isKey:(const void *)aKey;
- (void *)valueForKey:(const void *)aKey;
- (void *)insertKey:(const void *)aKey value:(void *)aValue;
- (void *)removeKey:(const void *)aKey;

/* Iterating */

- (NXHashState)initState;
- (BOOL)nextState:(NXHashState *)aState key:(const void **)aKey 
	value:(void **)aValue;

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new;
+ newKeyDesc: (const char *)aKeyDesc;
+ newKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc;
+ newKeyDesc:(const char *)aKeyDesc valueDesc:(const char *)aValueDesc 
	capacity:(unsigned)aCapacity;

@end


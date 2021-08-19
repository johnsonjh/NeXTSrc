/*
    List.h
    Copyright 1988, 1989 NeXT, Inc.

    DEFINED AS:	A common class
    HEADER FILES:	objc/List.h

*/

#import "Object.h"
#import "typedstream.h"

@interface List : Object
{
@public
    id 		*dataPtr;	/* data of the List object */
    unsigned 	numElements;	/* Actual number of elements */
    unsigned 	maxElements;	/* Total allocated elements */
}

/* Creating, freeing */

- free;
- freeObjects;
- copyFromZone:(NXZone *)zone;
- copy;
  
/* Initializing */

- init;
- initCount:(unsigned)numSlots;

/* Comparing two lists */

- (BOOL)isEqual: anObject;
  
/* Managing the storage capacity */

- (unsigned)capacity;
- setAvailableCapacity:(unsigned)numSlots;

/* Manipulating objects by index */

- (unsigned)count;
- objectAt:(unsigned)index;
- lastObject;
- addObject:anObject;
- insertObject:anObject at:(unsigned)index;
- removeObjectAt:(unsigned)index;
- removeLastObject;
- replaceObjectAt:(unsigned)index with:newObject;

/* Manipulating objects by id */

- (unsigned)indexOf:anObject;
- addObjectIfAbsent:anObject;
- removeObject:anObject;
- replaceObject:anObject with:newObject;

/* Emptying the list */

- empty;

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/* Sending messages to elements of the list */

- makeObjectsPerform:(SEL)aSelector;
- makeObjectsPerform:(SEL)aSelector with:anObject;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new;
+ newCount:(unsigned)numSlots;

@end

typedef struct {
    @defs(List)
} NXListId;

#define NX_ADDRESS(x) (((NXListId *)(x))->dataPtr)

#define NX_NOT_IN_LIST	0xffffffff



/*
    Storage.h
    Copyright 1988, 1989 NeXT, Inc.

    DEFINED AS:	A common class
    HEADER FILES:	objc/Storage.h
*/

#import "Object.h"
#import "typedstream.h"

@interface Storage : Object
{
@public
    void	*dataPtr;	/* Data of the Storage object */
    const char	*description;	/* Encoded data type of the stored elements */
    unsigned	numElements;	/* Number of elements actually in the array */
    unsigned	maxElements;	/* Total allocated elements */
    unsigned	elementSize;	/* Size of each element in the array */
}

/* Creating, freeing, initializing, and emptying */

- init;
- initCount:(unsigned)count elementSize:(unsigned)sizeInBytes 
	description:(const char *)descriptor;
- free; 
- empty;
- copyFromZone:(NXZone *)zone;
- copy;
  
/* Manipulating the elements */

- (BOOL)isEqual: anObject;
- (const char *)description; 
- (unsigned)count; 
- (void *)elementAt:(unsigned)index; 
- replace:(void *)anElement at:(unsigned)index;
- setNumSlots:(unsigned)numSlots; 
- setAvailableCapacity:(unsigned)numSlots;
- addElement:(void *)anElement; 
- removeLastElement; 
- insert:(void *)anElement at:(unsigned)index; 
- removeAt:(unsigned)index; 

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new; 
+ newCount:(unsigned)count elementSize:(unsigned)sizeInBytes 
	description:(const char *)descriptor; 

@end

typedef struct {
    @defs(Storage)
} NXStorageId;


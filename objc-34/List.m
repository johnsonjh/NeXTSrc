/*
	List.m
  	Copyright 1988, 1989 NeXT, Inc.
	Written by: Bryan Yamamoto
	Responsibility: Bertrand Serlet
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdlib.h>
#import <stdio.h>
#import <string.h>
#import "List.h"

#define DATASIZE(count) ((count) * sizeof(id))

@implementation  List

+ initialize
{
    [self setVersion: 1];
    return self;
}

- initCount:(unsigned)numSlots
{
    maxElements = numSlots;
    if (maxElements) 
	dataPtr = (id *)NXZoneMalloc([self zone], DATASIZE(maxElements));
    return self;
}

+ newCount:(unsigned)numSlots
{
    return [[self allocFromZone: NXDefaultMallocZone()] initCount:numSlots];
}

+ new
{
    return [self newCount:0];
}

- init
{
    return [self initCount:0];
}

- free
{
    free(dataPtr);
    return [super free];
}

- freeObjects
{
    id element;
    while (element = [self removeLastObject]) 
	[element free];
    return self;
}

- copy
{
    return [self copyFromZone: [self zone]];
}

- copyFromZone:(NXZone *)zone
{
    List	*new = [[[self class] allocFromZone:zone] initCount: numElements];
    new->numElements = numElements;
    bcopy (dataPtr, new->dataPtr, DATASIZE(numElements));
    return new;
}

- (BOOL) isEqual: anObject
{
    List	*other;
    if (! [anObject isKindOf: [self class]]) return NO;
    other = (List *) anObject;
    return (numElements == other->numElements) 
    	&& (bcmp (dataPtr, other->dataPtr, DATASIZE(numElements)) == 0);
}

- (unsigned)capacity
{
    return maxElements;
}

- (unsigned)count
{
    return numElements;
}

- objectAt:(unsigned)index
{
    if (index >= numElements)
	return nil;
    return dataPtr[index];
}

- (unsigned)indexOf:anObject
{
    register id *this = dataPtr;
    register id *last = this + numElements;
    while (this < last) {
        if (*this == anObject)
	    return this - dataPtr;
	this++;
    }
    return NX_NOT_IN_LIST;
}

- lastObject
{
    if (! numElements)
	return nil;
    return dataPtr[numElements - 1];
}

- setAvailableCapacity:(unsigned)numSlots
{
    if (numSlots < numElements) return nil;
    dataPtr = (id *) NXZoneRealloc ([self zone], dataPtr, DATASIZE(numSlots));
    maxElements = numSlots;
    return self;
}

- insertObject:anObject at:(unsigned)index
{
    register id *this, *last, *prev;
    if (! anObject) return nil;
    if (index > numElements)
        return nil;
    if ((numElements + 1) > maxElements) {
	/* we double the capacity, also a good size for malloc */
	maxElements += maxElements + 1;
	dataPtr = (id *) NXZoneRealloc (
	                  [self zone], dataPtr, DATASIZE(maxElements));
    }
    this = dataPtr + numElements;
    prev = this - 1;
    last = dataPtr + index;
    while (this > last) 
	*this-- = *prev--;
    *last = anObject;
    numElements++;
    return self;
}

- addObject:anObject
{
    return [self insertObject:anObject at:numElements];
    
}


- addObjectIfAbsent:anObject
{
    register id *this, *last;
    if (! anObject) return nil;
    this = dataPtr;
    last = dataPtr + numElements;
    while (this < last) {
        if (*this == anObject)
	    return self;
	this++;
    }
    return [self insertObject:anObject at:numElements];
    
}


- removeObjectAt:(unsigned)index
{
    register id *this, *last, *next;
    id retval;
    if (index >= numElements)
        return nil;
    this = dataPtr + index;
    last = dataPtr + numElements;
    next = this + 1;
    retval = *this;
    while (next < last)
	*this++ = *next++;
    numElements--;
    return retval;
}

- removeObject:anObject
{
    register id *this, *last;
    this = dataPtr;
    last = dataPtr + numElements;
    while (this < last) {
	if (*this == anObject)
	    return [self removeObjectAt:this - dataPtr];
	this++;
    }
    return nil;
}

- removeLastObject
{
    register id *this, *last;
    if (! numElements)
	return nil;
    return [self removeObjectAt: numElements - 1];
}

- empty
{
    numElements = 0;
    return self;
}

- replaceObject:anObject with:newObject
{
    register id *this, *last;
    if (! newObject)
        return nil;
    this = dataPtr;
    last = dataPtr + numElements;
    while (this < last) {
	if (*this == anObject) {
	    *this = newObject;
	    return anObject;
	}
	this++;
    }
    return nil;
}

- replaceObjectAt:(unsigned)index with:newObject
{
    register id *this, *last;
    id retval;
    if (! newObject)
        return nil;
    if (index >= numElements)
        return nil;
    this = dataPtr + index;
    retval = *this;
    *this = newObject;
    return retval;
}

- write:(NXTypedStream *) stream {
    [super write: stream];
    NXWriteTypes (stream, "i", &numElements);
    if (numElements)
	NXWriteArray (stream, "@", numElements, dataPtr);
    };

- read:(NXTypedStream *) stream {
    NXZone *zone = [self zone];
    [super read: stream];
    if (NXTypedStreamClassVersion (stream, "List") == 0) {
	int		_growAmount = 0;
	NXReadTypes (stream, "ii", &_growAmount, &numElements);
	dataPtr = (id *) NXZoneMalloc (zone, numElements*sizeof(id));
	maxElements = numElements;
	NXReadArray (stream, "@", numElements, dataPtr);
    } else {
	NXReadTypes (stream, "i", &numElements);
	maxElements = numElements;
	if (numElements) {
	    dataPtr = (id *) NXZoneMalloc (zone, numElements*sizeof(id));
	    NXReadArray (stream, "@", numElements, dataPtr);
	}
    }
}

- makeObjectsPerform:(SEL)aSelector
{
    unsigned	count = numElements;
    while (count--)
	[dataPtr[count] perform: aSelector];
    return self;
}

- makeObjectsPerform:(SEL)aSelector with:anObject
{
    unsigned	count = numElements;
    while (count--)
	[dataPtr[count] perform: aSelector with: anObject];
    return self;
}

@end


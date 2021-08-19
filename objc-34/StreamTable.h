/*
    StreamTable.h
    Copyright 1989 NeXT, Inc.
    
    DEFINED AS:	A common class
    HEADER FILES:	objc/StreamTable.h

*/
 
#import "HashTable.h"	

@interface StreamTable: HashTable 
	
/* Creating, freeing, and initializing */

- free;	
- freeObjects;	
- init;
- initKeyDesc:(const char *)aKeyDesc;

/* Manipulating */

- valueForStreamKey:(const void *)aKey;
- insertStreamKey:(const void *)aKey value:aValue;
- removeStreamKey:(const void *)aKey;

/* Iterating */

- (NXHashState)initStreamState;
- (BOOL)nextStreamState:(NXHashState *)aState key:(const void **)aKey 
	value:(id *)aValue;

/* Archiving */

- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;

/*
 * The following new... methods are now obsolete.  They remain in this 
 * interface file for backward compatibility only.  Use Object's alloc method 
 * and the init... methods defined in this class instead.
 */

+ new;
+ newKeyDesc:(const char *)aKeyDesc;

@end


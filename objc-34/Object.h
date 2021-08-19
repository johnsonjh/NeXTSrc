/*
	Object.h
	Copyright 1988, 1989 NeXT, Inc.
  
	DEFINED AS:	A common class
	HEADER FILES:	<objc/Object.h>

*/
#import "objc.h"
#import "objc-class.h"
#import "typedstream.h"
#import <zone.h>

@interface Object
{
	Class isa;	/* A pointer to the instance's class structure */
}

/* Initializing classes and instances */

+ initialize;
- init;

/* Creating, copying, and freeing instances */

+ new;
+ free;
- free;
+ alloc;
- copy;
+ allocFromZone:(NXZone *)zone;
- copyFromZone:(NXZone *)zone;
- (NXZone *)zone;

/* Identifying classes */

+ class;
+ superClass;
- class;
- superClass;
- (const char *) name;

/* Identifying and comparing instances */

- self;
- (unsigned int) hash;
- (BOOL) isEqual:anObject;

/* Testing inheritance relationships */

- (BOOL) isKindOf: aClassObject;
- (BOOL) isMemberOf: aClassObject;
- (BOOL) isKindOfGivenName: (STR)aClassName;
- (BOOL) isMemberOfGivenName: (STR)aClassName;

/* Testing class functionality */

+ (BOOL) instancesRespondTo:(SEL)aSelector;
- (BOOL) respondsTo:(SEL)aSelector;

/* Obtaining method handles */

- (IMP) methodFor:(SEL)aSelector;
+ (IMP) instanceMethodFor:(SEL)aSelector;

/* Sending messages determined at run time */

- perform:(SEL)aSelector;
- perform:(SEL)aSelector with:anObject;
- perform:(SEL)aSelector with:object1 with:object2;

/* Posing */

+ poseAs: aClassObject;

/* Enforcing intentions */
 
- subclassResponsibility:(SEL)aSelector;
- notImplemented:(SEL)aSelector;

/* Error handling */

- doesNotRecognize:(SEL)aSelector;
- error:(STR)aString, ...;

/* Archiving */

- awake;
- write:(NXTypedStream *)stream;
- read:(NXTypedStream *)stream;
+ (int) version;
+ setVersion: (int) aVersion;

/* Forwarding */

- forward: (SEL)sel : (marg_list)args;
- performv: (SEL)sel : (marg_list)args;

@end

/* Abstract Protocol for Archiving */

@interface Object (Archiving)

- startArchiving: (NXTypedStream *)stream;
- finishUnarchiving;

@end

/* Abstract Protocol for Dynamic Loading */

@interface Object (DynamicLoading)

+ finishLoading:(struct mach_header *)header;
+ startUnloading;

@end

id object_dispose(Object *anObject);
id object_copy(Object *anObject, unsigned nBytes);
id object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *);
id object_realloc(Object *anObject, unsigned nBytes);
id object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *);

Ivar object_setInstanceVariable(id, STR name, void *);
Ivar object_getInstanceVariable(id, STR name, void **);

/*
	Object.m
	Copyright 1988, 1989 NeXT, Inc.
*/

#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import <stdarg.h> 
#import <string.h> 
#import "objc-private.h"

#import "objc-runtime.h"	
#import "typedstream.h"

extern id (*_cvtToId)(STR);
extern id (*_poseAs)();

// Error Messages
static char
	_errNoMem[] = "failed -- out of memory(%s, %u)",
	_errReAllocNil[] = "reallocating nil object",
	_errReAllocFreed[] = "reallocating freed object",
	_errReAllocTooSmall[] = "(%s, %u) requested size too small",
	_errShouldHaveImp[] = "should have implemented the '%s' method.",
	_errShouldNotImp[] = "should NOT have implemented the '%s' method.",
	_errLeftUndone[] = "method '%s' not implemented",
	_errBadSel[] = "method %s given invalid selector %s",
	_errDoesntRecognize[] = "does not recognize selector %c%s";

#import "Object.h"

@implementation Object 


+ initialize
{
	return self; 
}

- awake 
{
	return self; 
}

+ poseAs: aFactory
{ 
	return (*_poseAs)(self, aFactory); 
}

+ new
{
	id newObject = (*_alloc)((Class)self, 0);
	Class metaClass = self->isa;
	if (metaClass->version > 1)
	    return [newObject init];
	else
	    return newObject;
}

+ alloc
{
	return (*_zoneAlloc)((Class)self, 0, NXDefaultMallocZone()); 
}

+ allocFromZone:(NXZone *) zone
{
	return (*_zoneAlloc)((Class)self, 0, zone); 
}

- init
{
    return self;
}

- findClass:(STR)aClassName
{
	return (*_cvtToId)(aClassName);
}

- (const char *)name
{
	return isa->name; 
}

- (unsigned)hash
{
	return ((unsigned)self) >> 2; 
}

- (BOOL)isEqual:anObject
{
	return anObject == self; 
}

- free 
{ 
	return (*_dealloc)(self); 
}

+ free
{
	return nil; 
}

- self
{
	return self; 
}

- class
{
	return (id)isa; 
}

+ class 
{
	return self;
}

- (NXZone *)zone
{
	NXZone *zone = NXZoneFromPtr(self);
	return zone ? zone : NXDefaultMallocZone();
}

+ superClass 
{ 
	return (id)((Class)self)->super_class; 
}

- superClass 
{ 
	return (id)isa->super_class; 
}

+ (int) version
{
	Class	class = (Class) self;
	return class->version;
}

+ setVersion: (int) aVersion
{
	Class	class = (Class) self;
	class->version = aVersion;
	return self;
}

- (BOOL)isKindOf:aClass
{
	register Class cls;
	for (cls = isa; cls; cls = cls->super_class) 
		if (cls == (Class)aClass)
			return YES;
	return NO;
}

- (BOOL)isMemberOf:aClass
{
	return isa == (Class)aClass;
}

- (BOOL)isKindOfGivenName:(STR)aClassName
{
	register Class cls;
	for (cls = isa; cls; cls = cls->super_class) 
		if (strcmp(aClassName, cls->name) == 0)
			return YES;
	return NO;
}

- (BOOL)isMemberOfGivenName:(STR)aClassName 
{
	return strcmp(aClassName, isa->name) == 0;
}

+ (BOOL)instancesRespondTo:(SEL)aSelector 
{
	return class_respondsToMethod((Class)self, aSelector);
}

- (BOOL)respondsTo:(SEL)aSelector 
{
	return class_respondsToMethod(isa, aSelector);
}

- copy 
{
	return (*_copy)(self, 0); 
}

- copyFromZone:(NXZone *)zone
{
	return (*_zoneCopy)(self, 0, zone); 
}

- (IMP)methodFor:(SEL)aSelector 
{
	return class_lookupMethod(isa, aSelector);
}

+ (IMP)instanceMethodFor:(SEL)aSelector 
{
	return class_lookupMethod((Class)self, aSelector);
}

- perform:(SEL)aSelector 
{ 
	if (aSelector)
		return objc_msgSend(self, aSelector); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- perform:(SEL)aSelector with:anObject 
{
	if (aSelector)
		return objc_msgSend(self, aSelector, anObject); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- perform:(SEL)aSelector with:obj1 with:obj2 
{
	if (aSelector)
		return objc_msgSend(self, aSelector, obj1, obj2); 
	else
		return [self error:_errBadSel, SELNAME(_cmd), aSelector];
}

- subclassResponsibility:(SEL)aSelector 
{
	return [self error:_errShouldHaveImp, sel_getName(aSelector)];
}

- shouldNotImplement:(SEL)aSelector
{
	return [self error:_errShouldNotImp, sel_getName(aSelector)];
}

- notImplemented:(SEL)aSelector
{
	return [self error:_errLeftUndone, sel_getName(aSelector)];
}

- doesNotRecognize:(SEL)aMessage
{
	return [self error:_errDoesntRecognize, 
		CLS_GETINFO(isa, CLS_META) ? '+' : '-', SELNAME(aMessage)];
}

- error:(STR)aCStr, ... 
{
	va_list ap;
	va_start(ap,aCStr); 
	(*_error)(self, aCStr, ap); 
	va_end(ap);
}

- write:(NXTypedStream *) stream 
{
	return self;
}

- read:(NXTypedStream *) stream 
{
	return self;
}

- forward: (SEL) sel : (marg_list) args 
{
    return [self doesNotRecognize: sel];
}

/* this method is not part of the published API */

- (unsigned)methodArgSize:(SEL)sel 
{
    Method	method = class_getInstanceMethod((Class)isa, sel);
    if (! method) return 0;
    return method_getSizeOfArguments(method);
}

- performv: (SEL) sel : (marg_list) args 
{
    unsigned	size;
    
    if (! self) return nil;
    size = [self methodArgSize: sel];
    if (! size) return [self doesNotRecognize: sel];
    return objc_msgSendv (self, sel, size, args); 
}

@end

/* System Primitive...declared as `private externs' in shared library */

id _object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	id obj;
	register unsigned siz;

	if (anObject == nil)
		return nil;

	obj = (*_zoneAlloc)(anObject->isa, nBytes, zone);
	siz = anObject->isa->instance_size + nBytes;
	bcopy(anObject, obj, siz);
	return obj;
}

id _object_copy(Object *anObject, unsigned nBytes) 
{
    NXZone *zone = NXZoneFromPtr(anObject);
    return _object_copyFromZone(anObject, 
				nBytes,
				zone ? zone : NXDefaultMallocZone());
}

id _object_dispose(Object *anObject) 
{
	if (anObject==nil) return nil;
	anObject->isa = Nil; 
	free((STR)anObject);
	return nil;
}

id _object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	id newObject; 
	Class tmp;

	if (anObject == nil)
		__objc_error(nil, _errReAllocNil, 0);

	if (anObject->isa == Nil)
		__objc_error(anObject, _errReAllocFreed, 0);

	if (nBytes < ((Class)anObject->isa)->instance_size)
		__objc_error(anObject, _errReAllocTooSmall, 
				object_getClassName(anObject), nBytes);

	// Make sure not to modify space that has been declared free
	tmp = anObject->isa; 
	anObject->isa = Nil;
	newObject = (id)NXZoneRealloc(zone, (STR)anObject, nBytes);
	if (newObject) {
		newObject->isa = tmp;
		return newObject;
	}
	else 
		__objc_error(anObject, _errNoMem, 
				object_getClassName(anObject), nBytes);
}

id _object_realloc(Object *anObject, unsigned nBytes) 
{
    NXZone *zone = NXZoneFromPtr(anObject);
    return _object_reallocFromZone(anObject,
				   nBytes,
				   zone ? zone : NXDefaultMallocZone());
}

/* Functional Interface to system primitives */

id object_copy(Object *anObject, unsigned nBytes) 
{
	return (*_copy)(anObject, nBytes); 
}

id object_copyFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneCopy)(anObject, nBytes, zone); 
}

id object_dispose(Object *anObject) 
{
	return (*_dealloc)(anObject); 
}

id object_realloc(Object *anObject, unsigned nBytes) 
{
	return (*_realloc)(anObject, nBytes); 
}

id object_reallocFromZone(Object *anObject, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneRealloc)(anObject, nBytes, zone); 
}

Ivar object_setInstanceVariable(id obj, STR name, void *value)
{
	Ivar ivar = 0;

	if (obj && name) {
		void **ivaridx;

		if (ivar = class_getInstanceVariable(obj->isa, name)) {
		       ivaridx = (void **)((char *)obj + ivar->ivar_offset);
		       *ivaridx = value;
		}
	}
	return ivar;
}

Ivar object_getInstanceVariable(id obj, STR name, void **value)
{
	Ivar ivar = 0;

	if (obj && name) {
		void **ivaridx;

		if (ivar = class_getInstanceVariable(obj->isa, name)) {
		       ivaridx = (void **)((char *)obj + ivar->ivar_offset);
		       *value = *ivaridx;
		} else
		       *value = 0;
	}
	return ivar;
}











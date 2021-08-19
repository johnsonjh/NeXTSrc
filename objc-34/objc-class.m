/*
 *	objc-class.m
 *	Copyright 1988, NeXT, Inc.
 *	Author:	s. naroff
 *
 */
#ifdef SHLIB
#import "shlib.h"
#endif SHLIB

#import "objc.h"
#import "objc-runtime.h"
#import "Object.h"

#import "objc-private.h"

#import "vectors.h"	

#import <mach.h>

/* CACHE_SIZE and META_CACHE_SIZE must be a power of two */
#define CACHE_SIZE		16	
#define META_CACHE_SIZE		8	

#define ISCLASS(cls)		((cls)->info & CLS_CLASS)
#define ISMETA(cls)		((cls)->info & CLS_META) 
#define GETMETA(cls)		(ISMETA(cls) ? cls : cls->isa)

#define ISINITIALIZED(cls)	(GETMETA(cls)->info & CLS_INITIALIZED)
#define MARKINITIALIZED(cls)	(GETMETA(cls)->info |= CLS_INITIALIZED)

static char _freedName[] = "FREED(id)";
static char _nilName[] = "nil";

static NXZone *_objc_zone = 0;

Cache _cache_create(Class);
static void _cache_fill(Class, Method);
static Cache _cache_expand(Class);

// Error Messages
static char
	_errNoMem[] = "failed -- out of memory(%s, %u)",
	_errAllocNil[] = "allocating nil object",
	_errFreedObject[] = "message %s sent to freed object=%lx",
	_errBadSel[] = "invalid selector %s",
	_errDoesntRecognize[] = "Does not recognize selector %s",
	_errNotSuper[]	= "[%s poseAs:%s]: target not immediate superclass",
	_errNewVars[]	= "[%s poseAs:%s]: %s defines new instance variables";

STR object_getClassName(id obj)
{
	if (obj == nil) 
		return _nilName;
	else if (obj->isa == Nil)
		return _freedName;
	else
		return ((Class)obj->isa)->name;
}

void *object_getIndexedIvars(id obj)
{
	return ((STR)obj) + obj->isa->instance_size;
}

// Allocate new instance of aClass with nBytes bytes of indexed vars

id _class_createInstanceFromZone(Class aClass, unsigned nBytes, NXZone *zone) 
{
	id obj; 
	register unsigned siz;

	if (aClass == Nil)
		__objc_error((id)aClass, _errAllocNil, 0);

	siz = aClass->instance_size + nBytes;

	if (obj = (id)NXZoneMalloc(zone, siz)) { 
		bzero((char *)obj, siz);
		obj->isa = aClass; 
		return obj;
	} else 
		__objc_error((id)aClass, _errNoMem, aClass->name, nBytes);
} 

id class_createInstanceFromZone(Class aClass, unsigned nBytes, NXZone *zone) 
{
	return (*_zoneAlloc)(aClass, nBytes, zone);
} 

id _class_createInstance(Class aClass, unsigned nBytes) 
{
    return _class_createInstanceFromZone(aClass, nBytes, NXDefaultMallocZone());
} 

id class_createInstance(Class aClass, unsigned nBytes) 
{
	return (*_alloc)(aClass, nBytes);
} 

void class_setVersion(Class aClass, int version)
{
	aClass->version = version;
}

int class_getVersion(Class aClass)
{
	return aClass->version;
}

static inline Method class_getMethod(Class cls, SEL sel)
{
	do {
		register int n;
		register Method smt;
		struct objc_method_list *mlist;

		for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		   smt = mlist->method_list;
		   n = mlist->method_count;

		   while (--n >= 0) {
		      if (sel == smt->method_name)
			return smt;
		      smt++;
		   }
		}
	} while (cls = cls->super_class);

	return 0;
}

Method class_getInstanceMethod(Class aClass, SEL aSelector)
{
	if (aClass && aSelector)
		return class_getMethod(aClass, aSelector);
       	else 
		return 0;
}

Method class_getClassMethod(Class aClass, SEL aSelector)
{
	if (aClass && aSelector)
		return class_getMethod(GETMETA(aClass), aSelector);
       	else 
		return 0;
}

static Ivar class_getVariable(Class cls, STR name)
{
	do {
		if (cls->ivars) {
			int i;
			Ivar ivars = cls->ivars->ivar_list;

			for (i = 0; i < cls->ivars->ivar_count; i++)	
				if (strcmp(name,ivars[i].ivar_name) == 0)
					return &ivars[i];
		}
	} while (cls = cls->super_class);

	return 0;
}

Ivar class_getInstanceVariable(Class aClass, STR name)
{
	if (aClass && name)
		return class_getVariable(aClass, name);	
	else
		return 0;
}

static inline void cache_flush(struct objc_cache *cache)
{
	int i;
	for (i = 0; i < cache->mask + 1; i++) 
		cache->buckets[i] = 0;
}

static void flush_caches(Class cls, BOOL meta)
{
	NXHashTable *class_hash = objc_getClasses();
	NXHashState state = NXInitHashState(class_hash);
	Class clsObject;

	while (NXNextHashState(class_hash, &state, (void **)&clsObject)) {

		Class clsIter = clsObject;

		while (clsIter) {

			if (clsIter == cls) {
				// it is `aKindOf' the class that has
				// been modified - flush!
				if (meta) {
					if (clsObject->isa->cache)
					  cache_flush(clsObject->isa->cache);
				} else {
					if (clsObject->cache)
					  cache_flush(clsObject->cache);
				}
				clsIter = 0;
			} else
				clsIter = clsIter->super_class;
		}
	}
}

void class_addInstanceMethods(Class cls, struct objc_method_list *meths)
{
	meths->method_next = cls->methods;
        cls->methods = meths;
	// must flush when dynamically adding methods.
	flush_caches(cls, 0);
}

void class_addClassMethods(Class cls, struct objc_method_list *meths)
{
	meths->method_next = cls->isa->methods;
        cls->isa->methods = meths;
	// must flush when dynamically adding methods.
	flush_caches(cls, 1);
}

void class_removeMethods(Class cls, struct objc_method_list *meths)
{
	if (cls->methods == meths) {
		/* it is at the front of the list - take it out */
		cls->methods = meths->method_next;
	} else {
        	struct objc_method_list *mlist, *prev;

		/* it is not at the front of the list - look for it */
		prev = cls->methods;
		mlist = cls->methods->method_next;

		while (mlist) {
			if (mlist == meths) {
				prev->method_next = mlist->method_next;
				mlist = 0;
			} else {
				prev = mlist;
				mlist = mlist->method_next;
			}
		}
	}
	// must flush when dynamically removing methods.
	flush_caches(cls, 0); 
}

Class class_poseAs(Class imposter, Class original) 
{
	Class clsObject;
	char imposterName[256], *imposterNamePtr; 
	NXHashTable *class_hash;
	NXHashState state;

	if (imposter == original) 
		return imposter;

	if (imposter->super_class != original)
		return (Class)[(id)imposter error:_errNotSuper, 
					imposter->name, original->name];
	if (imposter->ivars)
		return (Class)[(id)imposter error:_errNewVars, imposter->name, 
					original->name, imposter->name];

	class_hash = objc_getClasses();
        state = NXInitHashState(class_hash);

	while (NXNextHashState(class_hash, &state, (void **)&clsObject)) {

		if (clsObject == imposter) 
			continue;

		do {
			if (clsObject->super_class == original) {
				clsObject->super_class = imposter;
				clsObject->isa->super_class = imposter->isa;
				break;
			}
			clsObject = clsObject->super_class;
		} while ((clsObject) && (clsObject != imposter));
	}

	strcpy (imposterName, "_%"); 
	strcat (imposterName, original->name);

	imposterNamePtr = malloc (strlen(imposterName)+1); 
	strcpy (imposterNamePtr, imposterName);

	NXHashRemove(class_hash, imposter);
	NXHashRemove(class_hash, original);
	objc_addClass((Class)object_copy((Object *)imposter, 0));

	imposter->name = original->name;
	imposter->isa->name = original->isa->name;
	CLS_SETINFO(imposter, CLS_POSING);
	CLS_SETINFO(imposter->isa, CLS_POSING);
	
	original->name = imposterNamePtr+1;
	original->isa->name = imposterNamePtr;

	NXHashInsert(class_hash, imposter);
	NXHashInsert(class_hash, original);

	return imposter;
}

// provide a default error handler for un-recognized messages
static id _forward (id self, SEL sel, ...) 
{
    id		retval;

    // the following test is not necessary for Objects (instances of Object)
    // because forward:: is recognized.
    if(sel == @selector(forward::)) {
	__objc_error(self, _errDoesntRecognize, SELNAME(sel));
	return nil;
    }
    retval = [self forward: sel : &self];

    return retval;
}

static void _freedHandler(id self, SEL sel) 
{
	__objc_error(self, _errFreedObject, SELNAME(sel), self);
}

/*
 *	Purpose: Send the 'initialize' message on demand to any un-initialized 
 *		 class. Force initialization of superclasses first.
 *	Options:
 *	Assumptions:
 *	Side Effects: 
 *	Returns: 
 */
static id class_initialize(Class clsDesc)
{
	Class super = clsDesc->super_class;

	if (ISINITIALIZED(clsDesc))
		return (id )clsDesc;

	// force initialization of superclasses first
	if (super != Nil && !ISINITIALIZED(super))
		class_initialize(super);

	if (ISINITIALIZED(clsDesc))
		return (id )clsDesc;

	MARKINITIALIZED(clsDesc);
	[(id) clsDesc initialize];

	return (id )clsDesc;
}

/* make a private extern in shlib */

void _class_install_relationships(Class class, long version)
{
	Class meta, clstmp;
	int errflag = 0;

	meta = class->isa;
	
	meta->version = version;

	if (class->super_class) {
	  if (clstmp = (Class)objc_getClass((STR)class->super_class))
	    class->super_class = clstmp;
	  else 
	    errflag = 1;
	}

	if (clstmp = (Class)objc_getClass((STR)meta->isa))
	  meta->isa = clstmp->isa;
	else 
	  errflag = 1;

	if (meta->super_class) {
	  if (clstmp = (Class)objc_getClass((STR)meta->super_class))
	    meta->super_class = clstmp->isa;
	  else
	    errflag = 1;
	}
	else /* `tie' the meta class down to its class */
	  meta->super_class = class;

	if (errflag)
		_objc_fatal("please link appropriate classes in your program");
}

static int objc_malloc(int size)
{
	int space = (int)NXZoneMalloc(_objc_zone, size);

	if (space == 0)
	  _objc_fatal("unable to allocate space");

	return space;
}

BOOL class_respondsToMethod(Class savCls, SEL sel)
{
	Class cls = savCls;
	Method *buckets;
	int index, mask;
	
	if (sel == 0) 
		return NO;

	if (!cls->cache) {
		cls->cache = _cache_create(cls);
		goto cacheMiss;
	}
	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = (unsigned int) sel & mask;

	for (;;) {
	    if (buckets[index] && (buckets[index]->method_name == sel))
	      {
		if (buckets[index]->method_imp == (IMP) _forward)
		  return NO;
	        else
		  return YES;
	      }
	    if (! buckets[index]) goto cacheMiss;
	    index++;
	    index &= mask;
	}

cacheMiss:
	
	do {
		register int n;
		register Method smt;
		struct objc_method_list *mlist;

		for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		   smt = mlist->method_list;
		   n = mlist->method_count;

		   while (--n >= 0) {
			if (sel == smt->method_name)
			  {
			    _cache_fill(savCls, smt);
			    return YES;
			  }
			smt++;
		   }
		}
	} while (cls = cls->super_class);
		
	{
	  Method smt = (Method) objc_malloc (sizeof (struct objc_method));
	  
	  smt->method_name = sel;
	  smt->method_types = "";
	  smt->method_imp = (IMP) _forward;
	  _cache_fill(savCls, smt);
	}
	
	return NO;
}

IMP class_lookupMethod(Class cls, SEL sel)
{
	Method *buckets;
	int index, mask;
	
	if (sel == 0) 
		[(id)cls error:_errBadSel, sel];

	if (!cls->cache) {
		cls->cache = _cache_create(cls);
		goto cacheMiss;
	}
	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = (unsigned int) sel & mask;

	for (;;) {
	    if (buckets[index] && (buckets[index]->method_name == sel))
	      {
	        return buckets[index]->method_imp;
	      }
	    if (! buckets[index]) goto cacheMiss;
	    index++;
	    index &= mask;
	}

cacheMiss:
	return _class_lookupMethodAndLoadCache(cls, sel, NULL);
}

IMP class_lookupMethodInMethodList(struct objc_method_list *mlist, SEL sel)
{
	register int n;
	register Method smt;

	smt = mlist->method_list;
	n = mlist->method_count;

	while (--n >= 0) {
		if (sel == smt->method_name)
			return smt->method_imp;
		smt++;
	}
	return 0;
}

#define BUCKETSIZE(size) 	((size-1) * sizeof(Method))

Cache _cache_create(Class class)
{
	Cache new_cache;
	int size, i;

	size = (ISMETA (class)) ? META_CACHE_SIZE : CACHE_SIZE;

	// allocate and initialize table...
	new_cache = (Cache)objc_malloc(sizeof(struct objc_cache) + 
				BUCKETSIZE(size)); 

	for (i = 0; i < size; i++)
	   new_cache->buckets[i] = 0;
	new_cache->occupied = 0;
	new_cache->mask = size - 1;

	return new_cache;
}

static Cache _cache_expand(Class class)
{
	Cache old_cache, new_cache;
	int size, i;

	/* make sure size is a power of 2 */
	size = (class->cache->mask + 1) << 1;

	/* allocate and initialize table... */
	new_cache = (Cache)objc_malloc(sizeof(struct objc_cache) + 
				BUCKETSIZE(size));
	for (i = 0; i < size; i++)
	   new_cache->buckets[i] = 0;

	new_cache->occupied = 0;
	new_cache->mask = size - 1;

	old_cache = class->cache;

	// free any malloced method descriptors...class_respondsToMethod()
	// does this for negative caching.
	for (i = 0; i < old_cache->mask + 1; i++) {
		if (old_cache->buckets[i] &&
		    old_cache->buckets[i]->method_imp == (IMP) _forward) {
		  free(old_cache->buckets[i]);
	   	}
	}   
	class->cache = new_cache;

	NXZoneFree(_objc_zone, old_cache);
	return new_cache;
}

static void _cache_fill(Class class, Method smt)
{
	Cache cache = class->cache;
	SEL sel = smt->method_name;
	Method *buckets;
	int index, mask;

	buckets = cache->buckets;
	mask = cache->mask;

	/* First try to find an empty bucket within CHAIN of the natural 
	   bucket.
	 */
	index = (unsigned int)sel & mask;

	for (;;) {
	    if (! buckets[index]) {
		buckets[index] = smt;
		cache->occupied++;
		if ((cache->occupied << 2) >= (cache->mask+1) * 3) {
			class->cache = _cache_expand(class);
		}
		return;
	    }
	    index++;
	    index &= mask;
	}
}

void _cache_flush(Cache cache)
{
	int i;

	for (i = 0; i < cache->mask + 1; i++)
	   cache->buckets[i] = 0;
	cache->occupied = 0;
}

static inline Method _class_lookupMethod(Class cls, SEL sel)
{
	register int n;
	register Method smt;
	struct objc_method_list *mlist;

	for (mlist = cls->methods; mlist; mlist = mlist->method_next) {
		smt = mlist->method_list;
		n = mlist->method_count;

		while (--n >= 0) {
			if (sel == smt->method_name) {
				return smt;
			}
			smt++;
		}
	}
	return 0;
}

static inline Method _class_lookupMethodInCache(Class cls, SEL sel)
{
	Method *buckets;
	int index, mask;
	
	if (sel == 0) 
		[(id)cls error:_errBadSel, sel];

	if (!cls->cache) {
		cls->cache = _cache_create(cls);
		return 0;
	}
	mask = cls->cache->mask;
	buckets = cls->cache->buckets;

	index = (unsigned int) sel & mask;

	for (;;) {
	    if (buckets[index] && (buckets[index]->method_name == sel)) {
	        return buckets[index];
	    }	

	    if (! buckets[index]) 
		return 0;
	    index++;
	    index &= mask;
	}
}

IMP _class_lookupMethodAndLoadCache(Class savCls, SEL sel, id *refSelf)
{
	register Class cls = savCls;
	Method method;

	if (cls == Nil)
		return (IMP)_freedHandler;

	// lazy initialization...
	if (CLS_GETINFO(cls,CLS_META) && !ISINITIALIZED(cls) && refSelf)
		class_initialize((Class)*refSelf);

	do {
		method = _class_lookupMethod(cls, sel);
		if (method) {
			_cache_fill(savCls, method);
			return method->method_imp;
		}
	} while (cls = cls->super_class);
#if 0
	// the class does not directly respond...try forward:

	if (class_respondsToMethod(savCls, @selector(forward:))) {
		id dest = [*refSelf forward:sel];
		if (dest) {
			cls = dest->isa;
			method = _class_lookupMethodInCache(cls, sel);
			if (!method) {
			  do {
				method = _class_lookupMethod(cls, sel);
				if (method) {
					_cache_fill(dest->isa, method);
					*refSelf = dest;	
					return method->method_imp;
				}
			  } while (cls = cls->super_class);
			} else {
				*refSelf = dest;	
				return method->method_imp;
			}
		}
	} 
#endif
	// the class does not respond to forward: (or, did not supply a dest)
	{
		Method smt = (Method)objc_malloc(sizeof(struct objc_method));
		smt->method_name = sel;
		smt->method_types = "";
		smt->method_imp = (IMP) _forward;
		_cache_fill(savCls, smt);
	}
	return (IMP) _forward;
}

/* delegation */

static NXAtom SubTypeUntil (const char *type, char end) {
    int		level = 0;
    const char	*head = type;
    while (*type) {
	if (!*type || (! level && (*type == end)))
	    return NXUniqueStringWithLength (head, type - head);
	switch (*type) {
	    case ']': case '}': case ')': level--; break;
	    case '[': case '{': case '(': level++; break;
	}
	type ++;
    }
    printf ("Object: SubTypeUntil: end of type encountered prematurely\n");
    return NULL;
}

static const char *SkipFirstType (const char *type) {
    switch (*type++) {
    	case '[':
	    while ('0' <= *type && '9' >= *type) type++;
	    return type + strlen (SubTypeUntil (type, ']')) + 1;
	case '^':
	    if ((*type++) == '{')
		return type + strlen (SubTypeUntil (type, '}')) + 1;
	    else 
		return type;
	case '{':
	    while (*type++ != '}') ;
	default: return type;
    }
}

unsigned method_getNumberOfArguments(Method method)
{
	const char *typedesc = method->method_types;
	unsigned nargs = 0;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	/* next, skip stack size */
	while ('0' <= *typedesc && '9' >= *typedesc) 
		typedesc++;

	/* now, we have the arguments - count how many */
	while (*typedesc) {

		/* skip argument type */
		typedesc = SkipFirstType(typedesc);
		
		/* next is the argument offset, blow it off */
		while ('0' <= *typedesc && '9' >= *typedesc) 
			typedesc++;

		nargs++;
	}
	return nargs;
}

unsigned method_getSizeOfArguments(Method method)
{
	const char *typedesc = method->method_types;
	unsigned stack_size = 0;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	while ('0' <= *typedesc && '9' >= *typedesc) 
		stack_size = stack_size * 10 + (*typedesc++ - '0');

	return stack_size;
}

unsigned method_getArgumentInfo(Method method, int arg, 
				const char **type, int *offset)
{
	const char *typedesc = method->method_types;
	unsigned nargs = 0;
	unsigned self_offset = 0;

	/* first, skip the return type */
	typedesc = SkipFirstType(typedesc);

	/* next, skip stack size */
	while ('0' <= *typedesc && '9' >= *typedesc) 
		typedesc++;

	/* now, we have the arguments - position typedesc to the appropriate argument */
	while (*typedesc && nargs != arg) {

		/* skip argument type */
		typedesc = SkipFirstType(typedesc);
		
		if (nargs == 0) {
			while ('0' <= *typedesc && '9' >= *typedesc) 
				self_offset = self_offset * 10 + (*typedesc++ - '0');
		} else {
			/* next is the argument offset, blow it off */
			while ('0' <= *typedesc && '9' >= *typedesc) 
				typedesc++;
		}
		nargs++;
	}
	if (*typedesc) {
		unsigned arg_offset = 0;

		*type = typedesc;
		typedesc = SkipFirstType(typedesc);

		if (arg == 0) {
			*offset = 0;
		} else {
			while ('0' <= *typedesc && '9' >= *typedesc) 
				arg_offset = arg_offset * 10 + (*typedesc++ - '0');
			*offset = arg_offset - self_offset;
		}
	} else {
		*type = 0; *offset = 0;
	}
	return nargs;
}

void _objc_create_zone()
{
	if (!_objc_zone) {
	    _objc_zone = NXCreateZone(vm_page_size, vm_page_size, YES);
	    NXNameZone(_objc_zone, "ObjC");
	}
}

static short pad_to_align_global_data_to_4_byte_alignment = 0;

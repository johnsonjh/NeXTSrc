
|	objc-dispatch.s
|	Copyright 1988, NeXT, Inc.
|	Author: s.naroff & s.stone.
|	recoded in assembler by John Anderson

| 	Purpose: Compute implementation for method, and dispatch there,
|		 leaving all arguments undisturbed on the stack.

|	- caches are maintained on a `per-class' basis.
|	- caches are allocated dynamically, sized proportional to all the
|	methods that the class can respond to.
|	- cache entries cost 4 bytes each. This saves memory and enables
|	a cache load to be accomplished in an atomic assignment operation,
|	removing the expensive `tas' operation. Because a cache load in the
|	Stepstone messager consisted of 3 assignment operations <class,
|	selector,implementation> they needed this instruction to enforce
|	concurrency lockouts on cache slot access. This insured the `integrity'
|	of the cache when sending messages from interrupt handlers. This
|	makes the dispatcher more portable, considering many machines do not
|	have or support (i.e. apollo) this instruction.
|	- this cache supports collision resolution. Because we can estimate
|	the number of elements in the cache (and are concerned about data
|	space overhead), we use a simple open-addressing method called
|	`linear probing'. If all probes are filled, we replace one of them.
|	In this implementation, the length of a chain will be 8.
|	- _msgSuper DOES NOT depend on retreiving `self' from the previous
|	stack frame.

|	These routine could be faster and simpler if the selectors were offsets
|	into the cache tables rather than indexes.  This would require that
|	selectors be premultiplied by 4 (the size of a bucket) and never have
|	values of 0 and 4.  Values of 0 and 4 correspond to the offset to mask
|	and occupied fields of the objc_cache structure.  The size of the entire
|	objc_cache structure should then be a power of two instead of the bucket
|	array.


|Note:	The following assembler equates corresponding to the C typedefs must
|	be modified each time the corresponding C structures change.

|typedef struct objc_class *Class;

|struct objc_class {
|	struct objc_class *isa;
|	struct objc_class *super_class;
|	char *name;
|	long version;
|	long info;
|	long instance_size;
|	struct objc_ivar_list *ivars;
|	struct objc_method_list *methods;
|	struct objc_cache *cache;
|};

isa		=	0			|struct objc_class *
super_class	=	(isa + 4) 		|struct objc_class *
name		=	(super_class + 4)	|char *
version		=	(name + 4)		|long
info		=	(version + 4)		|long
instance_size	=	(info + 4)		|long
ivars		=	(instance_size + 4) 	|struct objc_ivar_list *
methods		=	(ivars + 4)		|struct objc_method_list *
cache		=	(methods + 4)		|struct objc_cache *
SZcache		=	(cache + 4)

|struct objc_cache {
|	unsigned int mask;		/* total = mask + 1 */
|	unsigned int occupied;
|	Method buckets[1];		/* variable sized arrary */
|};

mask		=	0			|unsigned int
occupied	=	(mask + 4)		|unsigned int
buckets		=	(occupied + 4)		|Method []

|typedef struct objc_method *Method;

|struct objc_method_list {
|	struct objc_method_list *method_next;
|	int method_count;
|	struct objc_method {
|		SEL method_name;
|		char *method_types;
|                IMP method_imp;
|	} method_list[1];		/* variable length structure */
|};

method_name	=	0			|SEL
method_types	=	(method_name + 4)	|char *
method_imp	=	(method_types + 4)	|IMP

|struct objc_super {
|	id receiver;
|	Class class;
|};

receiver	=	0			|id
class		=	(receiver + 4)		|Class

|Imported externals

.globl	__cache_create
.globl	__class_lookupMethodAndLoadCache


	.globl	_objc_msgSend
_objc_msgSend:
|/\/\/\/\10/\/\/\/\20/\/\/\/\30/\/\/\/\40/\/\/\/\50/\/\/\/\60/\/\/\/\70/\/\/\/\80/\/\/\/\90/\/\/\/\100\
| This routine implements the message call for Objective C (e.g. [anObject aSelector]).  It's C calling
|conventions are:

|id objc_msgSend(id self, SEL selector, ...)

| It finds the method in the object "self" corresponding to the SEL "selector".  On entry the arguments
|have been pushed on the stack according to the usual C conventions:

| +-----------------------+
| |    .     .	    .	  | <-- Variable number of argument appropriate for the method.
| |    .     .	    .	  |
| |    .     .	    .	  |
| +-----------------------+
| |      selector   	  | <-- selector (SEL)
| +-----------------------+
| |        self   	  | <-- self (id)
| +-----------------------+
| |    return address     | <-- return address (Long)  Lowest item on the stack.
| +-----------------------+

|a0/d0-d1 are used as scratch registers and are not preserved across the call.  a1 is not
|modified since the compiler uses it for pointing to returned structure greater than 8 bytes.

|Local Vars
returnAddress	=	0			|Long
self		=	(returnAddress + 4)	|id
selector	=	(self + 4)		|SEL

				   |{
					|Method	    *bucketPtr;
					|byte	    *cacheEndPtr;
					|objc_cache *cachePtr;
					|Class	     class;
					|int	     index;
					|byte	    *length;

	movl	sp@(self),d0		|if (self == nil)
	beqs	L1.1			|    return nil;
	movl	d0,a0			|class = self->isa;
	movl	a0@,a0
	movl	a0@(cache),d0		|if (!class->cache) { /* cache absent */
	beqs	L1.2			|    class->cache = _cache_create(class);
					|    IMP implementation = class_lookupMethodAndLoadCache
					|			  (class, selector, &self);
					|    Jump (implementation);
					|}
	movl	d0,a0			|cachePtr = class->cache;
	movl	sp@(selector),d1	|index = selector & cachePtr->mask;
	movl	d1,d0
	andl	a0@,d1
	asll	#2,d1			|bucketPtr = cachePtr->bucket [index];
	addl	d1,a0
	movl	a0@(buckets),d1		|if (!(*bucketPtr)) { /* cache miss */
	beqs	L1.4			|    IMP implementation = class_lookupMethodAndLoadCache
					|			  (class, selector, &self);
					|    Jump (implementation);
					|}
	movl	d1,a0			|if ((*bucketPtr)->method_name == selector) { /* cache hit */
	cmpl	a0@,d0
	bnes	L1.6
	movl	a0@(method_imp),a0	|    IMP implementation = (*bucketPtr)->method_imp;
	jmp	a0@			|    Jump (implementation);
					|}
L1.1:	moveq	#0,d0
	rts
L1.2:	movl	a1,sp@-
	movl	a0,sp@-
	jsr	__cache_create
	movel	sp@+,a0
	movel	d0,a0@(cache)
	bras	L1.5
L1.3:	movl	sp@+,d2
	bras	L1.5
L1.4:	movl	a1,sp@-
L1.5:	pea	sp@(self+4)
	movl	sp@(selector+4+4),sp@-
	movl	sp@(self+4+4+4),a0
	movl	a0@,sp@-
	jsr	__class_lookupMethodAndLoadCache
	addw	#3*4,sp
	movl	sp@+,a1
	movl	d0,a0
	jmp	a0@
					|else { /* cache collision */
L1.6:	movl	a1,sp@-			| /* Save regs */
	movl	d2,sp@-
	movl	sp@(self+8),a1		|    cachePtr = self->isa->cache;
	movl	a1@,a1
	movl	a1@(cache),a1
	movl	a1@,d1			|    length = (cachePtr->mask + 3) * 4;
					| /*
	addql	#3,d1			|  * 2 to skip over first 2 longs plus 1 since the mask is
	asll	#2,d1			|  * 1 short of the length in longs.
					|  */
	addl	a1,d1			|    cacheEndPtr = (byte *) cachePtr + length;
	movl	sp@(selector+8),d0	|    index = selector & cachePtr->mask;
	movl	d0,d2
	andl	a1@,d0
	addql	#3,d0
	asll	#2,d0			|    bucketPtr = cachePtr->bucket [index + 1];
	addl	d0,a1
					| /*
					|  * Look at successive buckets, wrapping around if
					|  * necessary until we find it or come across and empty
					|  * bucket.
					|  */
	bras	L1.8			|    do {
					|	if (NotFirstPass) {
L1.7:	movl	a1@+,d0			|	    IMP theMethod = *bucketPtr++;
	beqs	L1.3			|	    if (!theMethod) { /* cache Miss */
					|		/* Restore regs */
					|    		IMP implementation =
					|		     class_lookupMethodAndLoadCache
					|		     (class, selector, &self);
					|		Jump (implementation);
					|	    }
	movl	d0,a0			|	    if (theMethod->method_name == selector)) {
	cmpl	a0@,d2
	beqs	L1.9			|	    	/* Restore regs */
					|	    	IMP implementation = bucketPtr->method_imp;
					|	    	Jump (implementation);
					|	    }
L1.8:	cmpl	d1,a1			|	if ((byte *) bucketPtr >= cacheEndPtr)
	bcss	L1.7
	movl	sp@(self+8),a1		|	    bucketPtr = &(self->isa->cache.buckets[0]);
	movl	a1@,a1
	movl	a1@(cache),a1
	addql	#8,a1
	bras	L1.7			|    } while (1);
L1.9:	movl	sp@+,d2
	movl	sp@+,a1
	movl	a0@(method_imp),a0
	jmp	a0@
					|}
				   |}


	.globl	_objc_msgSendSuper
_objc_msgSendSuper:
|/\/\/\/\10/\/\/\/\20/\/\/\/\30/\/\/\/\40/\/\/\/\50/\/\/\/\60/\/\/\/\70/\/\/\/\80/\/\/\/\90/\/\/\/\100\
| This routine is essentially identical to objc_msgSend except that the caller is an argument instead
|of self.  Instead of passing self we use caller->receiver as self is used in objc_msgSend.  Instead
|of using the class = self->isa, we use the class = caller->class.

|id objc_msgSendSuper(struct objc_super *caller, SEL sel, ...)

| Finds the method in the object caller->class corresponding to the SEL "selector".  On entry
|the arguments have been pushed on the stack according to the usual C conventions:

| +-----------------------+
| |    .     .	    .	  | <-- Variable number of argument appropriate for the method.
| |    .     .	    .	  |
| |    .     .	    .	  |
| +-----------------------+
| |      selector   	  | <-- selector (SEL)
| +-----------------------+
| |        caller   	  | <-- caller (struct objc_super *)
| +-----------------------+
| |    return address     | <-- return address (Long)  Lowest item on the stack.
| +-----------------------+

|a0/d0-d1 are used as scratch registers and are not preserved across the call.  a1 is not
|modified since the compiler uses it for pointing to returned structure greater than 8 bytes.

|Local Vars
returnAddress	=	0			|Long
caller		=	(returnAddress + 4)	|id
selector	=	(caller + 4)		|SEL

				   |{
					|Method	    *bucketPtr;
					|byte	    *cacheEndPtr;
					|objc_cache *cachePtr;
					|Class	     class;
					|int	     index;
					|byte	    *length;
					|id	     self;

	movl	a2,sp@-			|/* save regs */
	movl	sp@(caller+4),a2	|originalCaller = caller;
	movl	a2@+,sp@(caller+4)	|caller = originalCaller->receiver;
	movl	a2@,a2			|class = originalCaller->class;
	movl	a2@(cache),d0		|if (!class->cache) { /* cache absent */
	beqs	L2.1			|    class->cache = _cache_create(class);
					|    IMP implementation = class_lookupMethodAndLoadCache
					|			  (class, selector, &caller);
					|    Jump (implementation);
					|}
	movl	d0,a0			|cachePtr = class->cache;
	movl	sp@(selector+4),d1	|index = selector & cachePtr->mask;
	movl	d1,d0
	andl	a0@,d1
	asll	#2,d1			|bucketPtr = cachePtr->bucket [index];
	addl	d1,a0
	movl	a0@(buckets),d1		|if (!(*bucketPtr)) { /* cache miss */
	beqs	L2.3			|    IMP implementation = class_lookupMethodAndLoadCache
					|			  (class, selector, &self);
					|    Jump (implementation);
					|}
	movl	d1,a0			|if ((*bucketPtr)->method_name == selector) { /* cache hit */
	cmpl	a0@,d0
	bnes	L2.5			|{
	movl	sp@+,a2			|    /* restore regs */
	movl	a0@(method_imp),a0	|    IMP implementation = (*bucketPtr)->method_imp;
	jmp	a0@			|    Jump (implementation);
					|}
L2.1:	movl	a1,sp@-
	movl	a2,sp@-
	jsr	__cache_create
	addql	#4,sp
	movel	d0,a2@(cache)
	bras	L2.4
L2.2:	movl	sp@+,d2
	bras	L2.4
L2.3:	movl	a1,sp@-
L2.4:	pea	sp@(caller+4+4)
	movl	sp@(selector+4+4+4),sp@-
	movl	a2,sp@-
	jsr	__class_lookupMethodAndLoadCache
	addw	#3*4,sp
	movl	sp@+,a1
	movl	sp@+,a2
	movl	d0,a0
	jmp	a0@
					|else { /* cache collision */
L2.5:	movl	a1,sp@-			| /* Save regs */
	movl	d2,sp@-
	movl	a2@(cache),a1		|    cachePtr = class->cache;
	movl	a1@,d1			|    length = (cachePtr->mask + 3) * 4;
					| /*
	addql	#3,d1			|  * 2 to skip over first 2 longs plus 1 since the mask is
	asll	#2,d1			|  * 1 short of the length in longs.
					|  */
	addl	a1,d1			|    cacheEndPtr = (byte *) cachePtr + length;
	movl	sp@(selector+4+4+4),d0	|    index = selector & cachePtr->mask;
	movl	d0,d2
	andl	a1@,d0
	addql	#3,d0
	asll	#2,d0			|    bucketPtr = cachePtr->bucket [index + 1];
	addl	d0,a1
					| /*
					|  * Look at successive buckets, wrapping around if
					|  * necessary until we find it or come across and empty
					|  * bucket.
					|  */
	bras	L2.7			|    do {
					|	if (NotFirstPass) {
L2.6:	movl	a1@+,d0			|	    IMP theMethod = *bucketPtr++;
	beqs	L2.2			|	    if (!theMethod) { /* cache Miss */
					|		/* Restore regs */
					|    		IMP implementation =
					|		     class_lookupMethodAndLoadCache
					|		     (class, selector, &self);
					|		Jump (implementation);
					|	    }
	movl	d0,a0			|	    if (theMethod->method_name == selector)) {
	cmpl	a0@,d2
	beqs	L2.8			|	    	/* Restore regs */
					|	    	IMP implementation = bucketPtr->method_imp;
					|	    	Jump (implementation);
					|	    }
L2.7:	cmpl	d1,a1			|	if ((byte *) bucketPtr >= cacheEndPtr)
	bcss	L2.6
	movl	a2@(cache),a1		|	    bucketPtr = &(class->cache.buckets[0]);
	addql	#8,a1
	bras	L2.6			|    } while (1);
L2.8:	moveml	sp@+,d2/a1-a2
	movl	a0@(method_imp),a0
	jmp	a0@
					|}
				   |}


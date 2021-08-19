/*
 *	objc-runtime.h
 *	Copyright 1988, NeXT, Inc.
 */

#import "objc.h"
#import "objc-class.h"
#import "hashtable.h"
#import "Object.h"

typedef struct objc_symtab *Symtab;

struct objc_symtab {
	unsigned long 	sel_ref_cnt;
	SEL 		*refs;		
	unsigned short 	cls_def_cnt;
	unsigned short 	cat_def_cnt;
	void  		*defs[1];	/* variable size */
};

typedef struct objc_module *Module;

struct objc_module {
	unsigned long	version;
	unsigned long	size;
	STR		name;
	Symtab 		symtab;	
};

struct objc_super {
	id receiver;
	Class class;
};

/* kernel operations */

id objc_getClass(STR name);
id objc_getMetaClass(STR name);
id objc_msgSend(id self, SEL op, ...);
id objc_msgSendSuper(struct objc_super *super, SEL op, ...);

/* forwarding operations */

id objc_msgSendv(id self, SEL op, unsigned arg_size, marg_list arg_frame);

/* 
   iterating over all the classes in the application...
  
   NXHashTable *class_hash = objc_getClasses();
   NXHashState state = NXInitHashState(class_hash);
   Class class;
  
   while (NXNextHashState(class_hash, &state, &class)
     ...;
 */
NXHashTable *objc_getClasses();
Module *objc_getModules();
void objc_addClass(Class myClass);

/* customizing the error handling for objc_getClass/objc_getMetaClass */

void objc_setClassHandler(int (*)(STR));

/* overriding the default object allocation and error handling routines */

extern id	(*_alloc)(Class, unsigned int);
extern id	(*_copy)(Object *, unsigned int);
extern id	(*_realloc)(Object *, unsigned int);
extern id	(*_dealloc)(Object *);
extern id	(*_zoneAlloc)(Class, unsigned int, NXZone *);
extern id	(*_zoneRealloc)(Object *, unsigned int, NXZone *);
extern id	(*_zoneCopy)(Object *, unsigned int, NXZone *);
extern void	(*_error)(Object *, char *, va_list);

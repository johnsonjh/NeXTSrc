/*
 *	objc-class.h
 *	Copyright 1988, NeXT, Inc.
 */
#import "objc.h"
#import <zone.h>
/* 
 *	Class Template
 */
struct objc_class {			
	struct objc_class *isa;	
	struct objc_class *super_class;	
	char *name;		
	long version;
	long info;
	long instance_size;
	struct objc_ivar_list *ivars;
	struct objc_method_list *methods;
	struct objc_cache *cache;
};
#define CLS_GETINFO(cls,infomask)	((cls)->info & infomask)
#define CLS_SETINFO(cls,infomask)	((cls)->info |= infomask)

#define CLS_CLASS		0x1L
#define CLS_META		0x2L
#define CLS_INITIALIZED		0x4L
#define CLS_POSING		0x8L
#define CLS_MAPPED		0x10L
/* 
 *	Category Template
 */
typedef struct objc_category *Category;

struct objc_category {
	char *category_name;
	char *class_name;
	struct objc_method_list *instance_methods;
	struct objc_method_list *class_methods;
};
/* 
 *	Instance Variable Template
 */
typedef struct objc_ivar *Ivar;

struct objc_ivar_list {
	int ivar_count;
	struct objc_ivar {
		char *ivar_name;
		char *ivar_type;
		int ivar_offset;
	} ivar_list[1];			/* variable length structure */
};
/* 
 *	Method Template
 */
typedef struct objc_method *Method;

struct objc_method_list {
	struct objc_method_list *method_next;
	int method_count;
	struct objc_method {
		SEL method_name;
		char *method_types;
                IMP method_imp;
	} method_list[1];		/* variable length structure */
};

/* Definitions of filer types */

#define _C_ID		'@'
#define _C_CLASS	'#'
#define _C_SEL		':'
#define _C_CHR		'c'
#define _C_UCHR		'C'
#define _C_SHT		's'
#define _C_USHT		'S'
#define _C_INT		'i'
#define _C_UINT		'I'
#define _C_LNG		'l'
#define _C_ULNG		'L'
#define _C_FLT		'f'
#define _C_DBL		'd'
#define _C_BFLD		'b'
#define _C_VOID		'v'
#define _C_UNDEF	'?'
#define _C_PTR		'^'
#define _C_CHARPTR	'*'
#define _C_ARY_B	'['
#define _C_ARY_E	']'
#define _C_UNION_B	'('
#define _C_UNION_E	')'
#define _C_STRUCT_B	'{'
#define _C_STRUCT_E	'}'

/* Structure for method cache - allocated/sized at runtime */

typedef struct objc_cache *Cache;

struct objc_cache {
	unsigned int mask;            /* total = mask + 1 */
	unsigned int occupied;        
	Method buckets[1];
};

/* operations */

id	class_createInstance(Class, unsigned idxIvars);
id	class_createInstanceFromZone(Class, unsigned idxIvars, NXZone *zone);

void 	class_setVersion(Class, int);
int 	class_getVersion(Class);

Ivar 	class_getInstanceVariable(Class, STR);
Method 	class_getInstanceMethod(Class, SEL);
Method 	class_getClassMethod(Class, SEL);

void 	class_addInstanceMethods(Class, struct objc_method_list *);
void 	class_addClassMethods(Class, struct objc_method_list *);
void 	class_removeMethods(Class, struct objc_method_list *);

Class 	class_poseAs(Class imposter, Class original);

unsigned method_getNumberOfArguments(Method);
unsigned method_getSizeOfArguments(Method);
unsigned method_getArgumentInfo(Method m, int arg, char **type, int *offset);

typedef void *marg_list;

#define marg_getRef(margs, offset, type) \
	( (type *)((char *)margs + offset) )

#define marg_getValue(margs, offset, type) \
	( *marg_getRef(margs, offset, type) )

#define marg_setValue(margs, offset, type, value) \
	( marg_getValue(margs, offset, type) = (value) )


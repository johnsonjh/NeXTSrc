/*
 *	objc.h
 *	Copyright 1988, NeXT, Inc.
 */
#ifndef  OBJC_INCL
#define  OBJC_INCL

typedef struct objc_class *Class;

typedef struct objc_object {
	Class isa;
} *id;

typedef struct objc_selector 	*SEL;    
typedef char *			STR;
typedef id 			(*IMP)(id, SEL, ...); 
typedef char			BOOL;

extern BOOL sel_isMapped(SEL sel);
extern STR sel_getName(SEL sel);
extern SEL sel_getUid(STR str);
extern STR object_getClassName(id obj);
extern void *object_getIndexedIvars(id obj);

#define YES             (BOOL)1
#define NO              (BOOL)0
#define ISSELECTOR(sel) sel_isMapped(sel)
#define SELNAME(sel)	sel_getName(sel)
#define SELUID(str)	sel_getUid(str)
#define NAMEOF(obj)     object_getClassName(obj)
#define IV(obj)         object_getIndexedIvars(obj)

#define Nil (Class)0           /* id of Nil class */
#define nil (id)0              /* id of Nil instance */

#endif /* OBJC_INCL */

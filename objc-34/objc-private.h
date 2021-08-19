/*
 *	objc-private.h
 *	Copyright 1988, NeXT, Inc.
 */

#import <stdlib.h>
#import <stdarg.h>
#import <stdio.h>
#import <string.h>
#import <ctype.h>

#import "objc.h"
#import "objc-runtime.h"
#import "hashtable.h"

#import <sys/time.h>

/* utilities */
extern unsigned _strhash (unsigned char *);
extern SEL _sel_getMaxUid();

/* debug */
extern int gettime();
extern void print_time(char *, int, struct timeval *);

/* initialize */
extern void _sel_init();
extern SEL _sel_registerName(STR);
extern SEL _sel_registerNameUseString(STR);
extern void _sel_unloadSelectors(char *, char *);
extern void _class_install_relationships(Class, long);
extern void _objc_add_category(Category);
extern void _objc_remove_category(Category);
extern void _objc_create_zone();

/* method lookup */
extern BOOL class_respondsToMethod(Class, SEL);
extern IMP class_lookupMethod(Class, SEL);
extern IMP class_lookupMethodInMethodList(struct objc_method_list *mlist, 
                                          SEL sel);

/* message dispatcher */
Cache _cache_create(Class);
IMP _class_lookupMethodAndLoadCache(Class, SEL, id *);

/* errors */
extern void _objc_fatal(char *message);
extern void _objc_error(id, char *, va_list);
extern void __objc_error(id, char *, ...);

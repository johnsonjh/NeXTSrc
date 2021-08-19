#ifdef SHLIB
/*
 * This file is only used when building a shared version of libsys.
 * It's purpose is to turn references to symbols not defined in the
 * library into indirect references, via a pointer that will be defined
 * in the library (see the file pointers.c).  These macros are expected
 * to be substituted for both the references and the declarations.  In
 * the case the user supplies the declaration (in his code or and include
 * file) the type for the pointer is declared when the macro is substitued.
 * The user MUST supply a declaration for each of these symbols he uses in
 * his code.
 *
 * MAJOR ASSUMPTION here is that the object files of this library are not
 * expected to be substituted by the user.  This means that the inter module
 * references are linked directly to each other and if the user attempted to
 * replace an object file he would get a multiply defined error.
 */

/*
 * In libc (but needs to be overridable so postscript can be linked with this)
 */
#define _setjmp(a)		(*_libsys__setjmp)(a)
#define _longjmp(a,b)		(*_libsys__longjmp)(a,b)
#define setjmp(a)		(*_libsys_setjmp)(a)
#define longjmp(a,b)		(*_libsys_longjmp)(a,b)

/*
 * In libc and in gcrt0.o.  Needs to be overridable so that this can be linked
 * with gcrt0.o for profiling.
 */
#define exit(a)			(*_libsys_exit)(a)

/* In crt0.o */
#define environ			(*_libsys_environ)

/* Link editor defined symbols */
#define _mh_execute_header	(*_libsys__mh_execute_header)

/* 
 * In libc - Needs to be overridable for malloc_debug
 */
#define malloc(a)		(*_libsys_malloc)(a)
#define calloc(a,b)		(*_libsys_calloc)(a,b)
#define realloc(a,b)		(*_libsys_realloc)(a,b)
#define valloc(a)		(*_libsys_valloc)(a)
#define mstats			(*_libsys_mstats)
#define free(a)			(*_libsys_free)(a)
#define malloc_size(a)		(*_libsys_malloc_size)(a)
#define malloc_debug(a)		(*_libsys_malloc_debug)(a)
#define malloc_error(a)		(*_libsys_malloc_error)(a)
#define malloc_good_size(a)	(*_libsys_malloc_good_size)(a)

/* In crt0.o */
#define NXArgv 			(*_libsys_NXArgv)
#define NXArgc 			(*_libsys_NXArgc)

/*
 * In libc (but needs to be overridable so gdb(1) can be linked with this)
 */
#define catch_exception_raise (*_libsys_catch_exception_raise)

#define NXCreateZone		(*_libsys_NXCreateZone)
#define NXCreateChildZone	(*_libsys_NXCreateChildZone)
#define NXMergeZone		(*_libsys_NXMergeZone)
#define NXZoneCalloc		(*_libsys_NXZoneCalloc)
#define NXZoneFromPtr		(*_libsys_NXZoneFromPtr)
#define NXZonePtrInfo		(*_libsys_NXZonePtrInfo)
#define NXDefaultMallocZone	(*_libsys_NXDefaultMallocZone)
#define NXAddRegion		(*_libsys_NXAddRegion)
#define NXMallocCheck		(*_libsys_NXMallocCheck)
#define NXRemoveRegion		(*_libsys_NXRemoveRegion)
#define malloc_singlethreaded	(*_libsys_malloc_singlethreaded)
#define malloc_freezedry	(*_libsys_malloc_freezedry)
#define malloc_jumpstart	(*_libsys_malloc_jumpstart)
#define vfree			(*_libsys_vfree)
#define _NXMallocDumpFrees	(*_libsys__NXMallocDumpFrees)
#define _NXMallocDumpZones	(*_libsys__NXMallocDumpZones)
#define _malloc_fork_prepare	(*_libsys__malloc_fork_prepare)
#define _malloc_fork_parent	(*_libsys__malloc_fork_parent)
#define _malloc_fork_child	(*_libsys__malloc_fork_child)
#define NXNameZone		(*_libsys_NXNameZone)

#endif SHLIB


/*
 * This file contains the pointers to symbols not defined in libsys but used
 * by the code in libsys.  For the pointers who's symbols are expected to be
 * in libsys they are initialized to the addresses in libsys (for routines this
 * is the address of a branch table slot not the actual address of the routine).
 *
 * New pointers are to be added at the end of this file before the symbol 
 * _libsys_pointers_pad and the pad to be reduced by the number of pointers
 * added.  This keeps the global data size of this file constant and thus does
 * not effect the addresses of other symbols.
 */

/*
 * In libc (but needs to be overridable so postscript can be linked with this)
 */
void * _libsys__setjmp =  		(void *)0x050020de;
void * _libsys__longjmp = 		(void *)0x050020b4;

/* 
 * in crt0.o 
 * a data symbol unpredictable address 
 */
void * _libsys_environ = 		(void *)0x0;

/* link editor defined symbols */
/* the default starting address */
void * _libsys__mh_execute_header = 	(void *)0x2000;

void * _libsys_exit = 0x0;

/* 
 * In libc - Needs to be overridable for malloc_debug
 */
void * _libsys_malloc =  		(void *)0x050028fa;	
void * _libsys_calloc =  		(void *)0x0500220a;
void * _libsys_realloc = 		(void *)0x05002d7a;
void * _libsys_valloc =  		(void *)0x05003284;
void * _libsys_mstats =  		(void *)0x050029e4;
void * _libsys_free =    		(void *)0x05002546;
void * _libsys_malloc_size =  		(void *)0x05002912;
void * _libsys_malloc_debug =  		(void *)0x05002900; 
void * _libsys_malloc_error =  		(void *)0x05002906;
void * _libsys_malloc_good_size =  	(void *)0x0500290c;

/* 
 * in crt0.o 
 * a data symbol unpredictable address 
 */
void * _libsys_NXArgv = 		(void *)0x0; 

/*
 * In libc (but needs to be overridable so gdb(1) can be linked with this)
 */
void * _libsys_catch_exception_raise = 	(void *)0x05002216;

/*
 * In libc (but needs to be overridable so postscript can be linked with this)
 */
void * _libsys_setjmp =  		(void *)0x05002ec4;
void * _libsys_longjmp = 		(void *)0x05002864;

void * _libsys_NXCreateZone = 		(void *)0x0500240e;
void * _libsys_NXCreateChildZone = 	(void *)0x05002414;
void * _libsys_NXMergeZone = 		(void *)0x0500241a;
void * _libsys_NXZoneCalloc = 		(void *)0x05002420;
void * _libsys_NXZoneFromPtr = 		(void *)0x05002426;
void * _libsys_NXZonePtrInfo = 		(void *)0x0500242c;
void * _libsys_NXDefaultMallocZone = 	(void *)0x05002444;
void * _libsys_NXAddRegion = 		(void *)0x05002438;
void * _libsys_NXMallocCheck = 		(void *)0x05002432;
void * _libsys_NXRemoveRegion = 	(void *)0x0500243e;
void * _libsys_malloc_singlethreaded = 	(void *)0x05002456;
void * _libsys_vfree = 			(void *)0x05003296;
void * _libsys__NXMallocDumpFrees = 	(void *)0x0500244a;
void * _libsys__NXMallocDumpZones = 	(void *)0x05002450;
void * _libsys__malloc_fork_prepare = 	(void *)0x05003d52;
void * _libsys__malloc_fork_parent = 	(void *)0x05003d58;
void * _libsys__malloc_fork_child = 	(void *)0x05003d5e;
void * _libsys_NXNameZone = 		(void *)0x0500245c;

/* 
 * in crt0.o 
 * a data symbol unpredictable address 
 */
void * _libsys_NXArgc = 		(void *)0x0; 

void * _libsys_malloc_freezedry = 	(void *)0x0;  /* FIXME */
void * _libsys_malloc_jumpstart = 	(void *)0x0;  /* FIXME */
/*
 * New pointers are added before this symbol.  This must remain at the end of
 * this file.  When a new pointers are added the pad must be reduced by the
 * number of new pointers added.
 */
static void *_libsys_pointers_pad[88] = { 0 };

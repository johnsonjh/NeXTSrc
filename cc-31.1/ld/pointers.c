#ifdef SHLIB
/*
 * This file contains the pointers to symbols not defined in librld but used
 * by the code in librld.  For the pointers who's symbols are expected to be
 * in librld they are initialized to the addresses in libsys (for routines this
 * is the address of a branch table slot not the actual address of the routine).
 *
 * New pointers are to be added at the end of this file before the symbol 
 * _librld_pointers_pad and the pad to be reduced by the number of pointers
 * added.  This keeps the global data size of this file constant and thus does
 * not effect the addresses of other symbols.
 */
void * _librld__mh_execute_header = (void *)0x2000;
void * _librld_NXArgv = (void *)0x0;
void * _librld_NXVPrintf = (void *)0x00003cf8;
void * _librld_bcopy = (void *)0x050021c8;
void * _librld_bsearch = (void *)0x050021fe;
void * _librld_close = (void *)0x0500229a;
void * _librld_errno = (void *)0x040105b0;
void * _librld_free = (void *)0x05002546;
void * _librld_fstat = (void *)0x0500256a;
void * _librld_getpagesize = (void *)0x05002678;
void * _librld_getsegbyname = (void *)0x05002714;
void * _librld_longjmp = (void *)0x05002864;
void * _librld_mach_error_string = (void *)0x050028ca;
void * _librld_malloc = (void *)0x050028fa;
void * _librld_map_fd = (void *)0x0500291e;
void * _librld_memcpy = (void *)0x05002948;
void * _librld_memset = (void *)0x05002954;
void * _librld_open = (void *)0x05002bc4;
void * _librld_realloc = (void *)0x05002d7a;
void * _librld_setjmp = (void *)0x05002ec4;
void * _librld_strcmp = (void *)0x05003008;
void * _librld_strcpy = (void *)0x0500301a;
void * _librld_strerror = (void *)0x0500302c;
void * _librld_strlen = (void *)0x05003038;
void * _librld_strncmp = (void *)0x05003044;
void * _librld_strncpy = (void *)0x0500304a;
void * _librld_task_self_ = (void *)0x04010290;
void * _librld_vfprintf = (void *)0x05003290;
void * _librld_vm_allocate = (void *)0x050032a8;
void * _librld_vm_deallocate = (void *)0x050032ba;
void * _librld_vm_protect = (void *)0x050032c6;
void * _librld_unlink = (void *)0x0500324e;
void * _librld_write = (void *)0x0500330e;
void * _librld_strtol = (void *)0x05003074;
void * _librld_qsort = (void *)0x05002d1a;
void * _librld_strrchr = (void *)0x05003056;
void * _librld_strchr = (void *)0x05003002;
void * _librld_strtoul = (void *)0x0500307a;
/*
 * New pointers are added before this symbol.  This must remain at the end of
 * this file.  When a new pointers are added the pad must be reduced by the
 * number of new pointers added.
 */
void *_librld_pointers_pad[90] = { 0 };
#endif SHLIB

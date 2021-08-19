#ifdef SHLIB
/*
 * This file is only used when building a shared version of the C
 * library.  It's purpose is to turn references to symbols not defined in the
 * library into indirect references, via a pointer that will be defined in the
 * library (see the file crt0_pointers.c).  These macros
 * are expected to be substituted for both the references and the declarations.
 * In the case the user supplies the declaration (in his code or and include
 * file) the type for the pointer is declared when the macro is substitued.
 * If there is no explicited declaration for the symbol, typical when the
 * symbol is a  function that returns an int, then it would cause an error
 * if one was not supplied here.  Type conflicts can arise if the user does
 * declare it with a different type.  So we try here not to declare a type
 * unless we think the user is using an implicit declration.
 *
 * MAJOR ASSUMPTION here is that the object files of this library are not
 * expected to be substituted by the user.  This means that the inter module
 * references are linked directly to each other and if the user attempted to
 * replace an object file he would get a multiply defined error.  This
 * assumption can be changed by putting macros like these into each file that
 * makes a reference outside the file.  Also pointers uses by those macros would
 * have to be declared somewhere.
 */

/* In crt0.o */
#define environ (*_libc_environ)
/* declaration expected to be supplied since its an external data item */
/* extern char **environ; */

/* In mach-o */
#define end (*_libc_end)
/* declaration expected to be supplied since its an external data item */
/* extern char *end; */

/* In mach-o */
#define _mh_execute_header (*_libc__mh_execute_header)
/* declaration expected to be supplied since its an external data item */
/* extern mh_execute_header; */

#endif SHLIB

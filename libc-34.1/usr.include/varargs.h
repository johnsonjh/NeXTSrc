/*	varargs.h	4.1	83/05/03	*/

#ifndef _VARARGS_H
#define _VARARGS_H

#ifdef __STRICT_ANSI__
#error varargs.h should not be included in an ANSI C program
#endif /* __STRICT_ANSI__ */

#ifdef _STDARG_H
#undef va_start
#undef va_end
#undef va_arg
#else
typedef char *va_list;
#endif /* _STDARG_H */

# define va_dcl int va_alist;
# define va_start(list) list = (char *) &va_alist
# define va_end(list)
# define va_arg(list,mode) ((mode *)(list += sizeof(mode)))[-1]

#endif _VARARGS_H

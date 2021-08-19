/* Copyright (c) 1988 NeXT, Inc. - 9/8/88 CCH */

#ifndef _STDDEF_H
#define _STDDEF_H

#ifndef NULL
#define NULL ((void *)0)
#endif

typedef long ptrdiff_t;
#ifndef _SIZE_T
#define _SIZE_T
typedef unsigned long size_t;
#endif
typedef unsigned short wchar_t;

#define offsetof(type,identifier) ((size_t)&((type *)0)->identifier)

#ifndef _ERRNO_
extern int errno;
#endif /* _ERRNO_ */

#endif /* _STDDEF_H */

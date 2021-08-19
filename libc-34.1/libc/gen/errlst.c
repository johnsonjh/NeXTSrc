#if !defined(lint) && defined(SCCSIDS)
static char sccsid[] = 	"@(#)errlst.c	1.4 88/05/10 4.0NFSSRC SMI"; /* from UCB 5.2 3/9/86 */
#endif

/*
 * From SMI 1.15
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#pragma CC_NO_MACH_TEXT_SECTIONS
/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  This
 * file has been padded so that more error strings can be added without
 * changing it's size.
 */

#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)errlst.c	5.2 (Berkeley) 3/9/86";
#endif /* LIBC_SCCS and not lint */

#define el(str, num, err) \
extern const char _sys_errlist_##err[];

#include "errlst.h"

#undef el

#define PADDING 120

#define el(str, num, err) \
	_sys_errlist_##err,

const char 	*const sys_errlist[] = {
#include "errlst.h"
};
static const char      *const __padding_sys_errlist[PADDING - (sizeof sys_errlist/sizeof sys_errlist[0])] = { 0 };
const int	sys_nerr = { sizeof sys_errlist/sizeof sys_errlist[0] };


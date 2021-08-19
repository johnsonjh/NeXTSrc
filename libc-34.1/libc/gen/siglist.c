#pragma CC_NO_MACH_TEXT_SECTIONS
/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  This
 * file has been padded so that more signal strings can be added without
 * changing it's size.  Also see next/sys/sigcatch.c
 */
#include <signal.h>

#define ss(str, num, sig) \
extern const char _sys_siglist_##sig[];

#include "siglist.h"

#undef ss

#define PADDING 50
#if (NSIG>PADDING)
#error NSIG is too big to keep shared library compatible - change PADDING
#endif

#define ss(str, num, sig) \
	_sys_siglist_##sig,

const char * const sys_siglist[PADDING] = {
#include "siglist.h"
};

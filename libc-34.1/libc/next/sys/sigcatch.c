/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  This
 * data has been padded so new signals can be added.  Also see gen/siglist.c
 */
#include <signal.h>

#define PADDING 50
#if (NSIG>PADDING)
#error NSIG is too big to keep shared library compatible - change PADDING
#endif

extern int	(*sigcatch[PADDING])() = { 0 };

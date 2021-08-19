#ifndef _SIGNAL_H
#define _SIGNAL_H

#ifndef __STRICT_BSD__
typedef char sig_atomic_t;
#define SIG_ERR		(void (*)())-1
int raise(int sig);
#endif /* __STRICT_BSD__ */

#ifdef __STRICT_ANSI__
#define SIG_DFL		(void (*)())0
#define SIG_IGN		(void (*)())1

#define SIGINT		2
#define SIGILL		4
#define SIGABRT		6
#define SIGFPE		8
#define SIGSEGV		11
#define SIGTERM		15

void (*signal(int sig, void (*func)(int)))(int);
#else
/*
 * note: this include file has been moved from this directory to the
 * directory indicated by the include directive, below.  this file
 * is present to preserve compatibility with previous system releases.
 * unlike the bsd releases, symbolic links are not used to maintain
 * compatibility because they would not preserve correct path searching
 * with cpp when -i flags are present.
 */
#include <sys/signal.h>
#endif /* __STRICT_ANSI__ */

#endif /* _SIGNAL_H */

/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * mig_support.c  - by Mary Thompson
 *
 * Routines to set and deallocate the mig reply port for the current thread.
 * Called from mig-generated interfaces.
 */
extern void exit(int);
/*
 * Routines to set and deallocate the mig reply port for the current thread.
 * Called from mig-generated interfaces.
 */
#include <mach.h>
#include <cthreads.h>
#include "cthread_internals.h"

#if	MTASK
#undef	task_self  /* Must call the function since the variable is shared */
#endif	MTASK

private struct mutex reply_port_lock = MUTEX_INITIALIZER;
#if	NeXT
#else	NeXT
private int multithreaded = 0;
#endif	NeXT

/*
 * Called by mach_init with 0 before cthread_init is
 * called and again with 1 at the end of cthread_init.
 */
void
mig_init(init_done)
	int init_done;
{
#if	NeXT
#else	NeXT
	multithreaded = init_done;
#endif	NeXT
}

/*
 * Called by mig interface code whenever a reply port is needed.
 * Tracing is masked during this call; otherwise, a call to printf()
 * can result in a call to malloc() which eventually reenters
 * mig_get_reply_port() and deadlocks.
 */
port_t
mig_get_reply_port()
{
	register cproc_t self;
	register kern_return_t r;
	port_t port;
#ifdef	DEBUG
	int d = cthread_debug;
#endif	DEBUG

#if	NeXT
#else	NeXT
	if (! multithreaded)
		return thread_reply();
#endif	NeXT
#ifdef	DEBUG
	cthread_debug = FALSE;
#endif	DEBUG
	self = cproc_self();
#if	NeXT
	if (self == NO_CPROC) {
#ifdef	DEBUG
		cthread_debug = d;
#endif	DEBUG
		return(thread_reply());
	}
#endif	NeXT
	if (self->reply_port == PORT_NULL) {
		mutex_lock(&reply_port_lock);
		self->reply_port = thread_reply();
		MACH_CALL(port_allocate(task_self(), &port), r);
		self->reply_port = port;
		mutex_unlock(&reply_port_lock);
	}
#ifdef	DEBUG
	cthread_debug = d;
#endif	DEBUG
	return self->reply_port;
}

/*
 * Called by mig interface code after a timeout on the reply port.
 * May also be called by user.
 */
void
mig_dealloc_reply_port()
{
	register cproc_t self;
	register port_t port;
#ifdef	DEBUG
	int d = cthread_debug;
#endif	DEBUG

#if	NeXT
#else	NeXT
	if (! multithreaded)
		return;
#endif	NeXT
#ifdef	DEBUG
	cthread_debug = FALSE;
#endif	DEBUG
	self = cproc_self();
#if	NeXT
	if (self == NO_CPROC) {
#ifdef	DEBUG
		cthread_debug = d;
#endif	DEBUG
		return;
	}
#endif	NeXT
	ASSERT(self != NO_CPROC);
	port = self->reply_port;
	if (port != PORT_NULL && port != thread_reply()) {
		mutex_lock(&reply_port_lock);
		self->reply_port = thread_reply();
		(void) port_deallocate(task_self(), port);
		self->reply_port = PORT_NULL;
		mutex_unlock(&reply_port_lock);
	}
#ifdef	DEBUG
	cthread_debug = d;
#endif	DEBUG
}

/* 
 * Copyright (c) 1990 NeXT Computer, Inc.  All rights reserved.
 *
 * Cthreads priority interface.
 */
/*
 * HISTORY
 * 14-Aug-90  Gregg Kellogg (gk) at NeXT
 *	Created.
 */
/*
 * cthreads_sched.c - by Gregg Kellogg
 *
 * Cthreads layer on top of mach scheduling primitives.
 */
#include <cthreads.h>

kern_return_t
cthread_priority(cthread_t t, int priority, boolean_t set_max)
{
	return thread_priority(cthread_thread(t), priority, set_max);
}

kern_return_t
cthread_max_priority(cthread_t t, processor_set_t pset, int max_priority)
{
	return thread_max_priority(cthread_thread(t), pset, max_priority);
}

kern_return_t
cthread_abort(cthread_t t)
{
	return thread_abort(cthread_thread(t));
}

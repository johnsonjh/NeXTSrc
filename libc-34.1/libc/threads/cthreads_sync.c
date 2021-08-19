#include <cthreads.h>
#include "cthread_internals.h"

/*
 * Spin locks.
 *
 * Only the COROUTINE implementations of spin_lock and spin_unlock
 * are always machine independent.
 */

#if	COROUTINE

void
spin_lock(p)
	register int *p;
{
	while (*p)
		cthread_yield();
	*p = 1;
}

#ifndef	spin_unlock
void
spin_unlock(p)
	register int *p;
{
	*p = 0;
}
#endif	spin_unlock

#endif	COROUTINE

/*
 * Mutex objects.
 *
 * Mutex_wait_lock is implemented in terms of mutex_try_lock().
 * Mutex_try_lock() and mutex_unlock() are machine-dependent,
 * except in the COROUTINE implementation.
 */

extern int mutex_spin_limit;


void
mutex_wait_lock(m)
	register mutex_t m;
{
	register int i;

	TRACE(printf("[%s] lock(%s)\n", cthread_name(cthread_self()), mutex_name(m)));
	for (i = 0; i < mutex_spin_limit; i += 1)
		if (m->lock == 0 && mutex_try_lock(m))
			return;
		else
			/* spin */;
	for (;;)
		if (m->lock == 0 && mutex_try_lock(m))
			return;
		else
			cthread_yield();
}

#if	COROUTINE
int
mutex_try_lock(m)
	register mutex_t m;
{
	TRACE(printf("[%s] try_lock(%s)\n", cthread_name(cthread_self()), mutex_name(m)));
	if (m->lock)
		return FALSE;
	m->lock = 1;
	return TRUE;
}

#ifndef	mutex_unlock
void
mutex_unlock(m)
	mutex_t m;
{
	TRACE(printf("[%s] unlock(%s)\n", cthread_name(cthread_self()), mutex_name(m)));
	m->lock = 0;
}
#endif	mutex_unlock

#endif	COROUTINE

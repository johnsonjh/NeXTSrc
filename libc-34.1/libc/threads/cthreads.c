/*
 * cthreads.c - by Eric Cooper
 *
 * Implementation of cthread_fork, cthread_join, cthread_exit, etc.
 */
#if	NeXT
#include <stdlib.h>
#endif	NeXT
extern void exit(int);
#include <cthreads.h>
#include "cthread_internals.h"

/*
 * C Threads imports:
 */
#if	NeXT
extern void cproc_init();
extern thread_t cproc_create();
#else
extern void cproc_init(), cproc_create();
#endif	NeXT
extern void mig_init();

/*
 * Mach imports:
 */

/*
 * C library imports:
 */
#if	NeXT
extern _setjmp(jmp_buf env), _longjmp(jmp_buf env, int val);
#else
extern _setjmp(), _longjmp();
#endif	NeXT

/*
 * Thread status bits.
 */
#define	T_MAIN		0x1
#define	T_RETURNED	0x2
#define	T_DETACHED	0x4

#ifdef	DEBUG
int cthread_debug = FALSE;
#endif	DEBUG

private struct cthread_queue cthreads = QUEUE_INITIALIZER;
private struct mutex cthread_lock = MUTEX_INITIALIZER;
private struct condition cthread_needed = CONDITION_INITIALIZER;
private struct condition cthread_idle = CONDITION_INITIALIZER;
private int n_cprocs = 0;
private int n_cthreads = 0;
private int max_cprocs = 0;

private cthread_t free_cthreads = NO_CTHREAD;	/* free list */
private int cthread_free_lock = 0;		/* unlocked */

private struct cthread initial_cthread = { 0 };

private cthread_t
cthread_alloc(func, arg)
	cthread_fn_t func;
	any_t arg;
{
#if	NeXT
	thread_t real_thread = THREAD_NULL;
#endif	NeXT
	register cthread_t t = NO_CTHREAD;

	if (free_cthreads != NO_CTHREAD) {
		/*
		 * Don't try for the lock unless
		 * the list is likely to be nonempty.
		 * We can't be sure, though, until we lock it.
		 */
		spin_lock(&cthread_free_lock);
		t = free_cthreads;
		if (t != NO_CTHREAD)
			free_cthreads = t->next;
		spin_unlock(&cthread_free_lock);
#if	NeXT
		real_thread = t->real_thread;
#endif	NeXT
	}
	if (t == NO_CTHREAD) {
		/*
		 * The free list was empty.
		 * We may have only found this out after
		 * locking it, which is why this isn't an
		 * "else" branch of the previous statement.
		 */
		t = (cthread_t) malloc(sizeof(struct cthread));
	}
	*t = initial_cthread;
#if	NeXT
	t->real_thread = real_thread;	/* preserve if necessary */
#endif	NeXT
	t->func = func;
	t->arg = arg;
	return t;
}

private void
cthread_free(t)
	register cthread_t t;
{
	spin_lock(&cthread_free_lock);
	t->next = free_cthreads;
	free_cthreads = t;
	spin_unlock(&cthread_free_lock);
}

void
cthread_init()
{
	static int cthreads_started = FALSE;
	register cthread_t t;

	if (cthreads_started)
		return;
	cproc_init();
	n_cprocs = 1;
	t = cthread_alloc((cthread_fn_t) 0, (any_t) 0);
	n_cthreads = 1;
	t->state |= T_MAIN;
	cthread_set_name(t, "main");
	cthread_assoc(cproc_self(), t);
#if	NeXT
	t->real_thread = thread_self();
#endif	NeXT
	cthreads_started = TRUE;
	mig_init(1);		/* enable multi-threaded mig interfaces */
}

/*
 * Used for automatic initialization by crt0.
 */
extern int (*_cthread_init_routine)();

/*
 * Procedure invoked at the base of each cthread.
 */
void
cthread_body(self)
	cproc_t self;
{
	register cthread_t t;

#if	NeXT
	cthread_set_self(self);
#endif	NeXT
	ASSERT(cproc_self() == self);
	TRACE(printf("[idle] cthread_body(%x)\n", self));
	mutex_lock(&cthread_lock);
	for (;;) {
		/*
		 * Dequeue a thread invocation request.
		 */
		cthread_queue_deq(&cthreads, cthread_t, t);
		if (t != NO_CTHREAD) {
			/*
			 * We have a thread to execute.
			 */
			mutex_unlock(&cthread_lock);
			cthread_assoc(self, t);		/* assume thread's identity */
			if (_setjmp(t->catch) == 0) {	/* catch for cthread_exit() */
				/*
				 * Execute the fork request.
				 */
				t->result = (*(t->func))(t->arg);
			}
			/*
			 * Return result from thread.
			 */
			TRACE(printf("[%s] done()\n", cthread_name(t)));
			mutex_lock(&t->lock);

			if (t->state & T_MAIN) {
				/*
				 * Could have become main thread if we're the
				 * child of a thread that did a fork() call.
				 */
				mutex_unlock(&t->lock);
				cthread_exit(t->result);
			}

			if (t->state & T_DETACHED) {
				mutex_unlock(&t->lock);
				cthread_free(t);
			} else {
				t->state |= T_RETURNED;
				mutex_unlock(&t->lock);
				condition_signal(&t->done);
			}
			cthread_assoc(self, NO_CTHREAD);
			mutex_lock(&cthread_lock);
			n_cthreads -= 1;
		} else {
			/*
			 * Queue is empty.
			 * Signal that we're idle in case the main thread
			 * is waiting to exit, then wait for reincarnation.
			 */
			condition_signal(&cthread_idle);
#if	MTASK
			if (n_cthreads == 0) {
				/* The next broadcast should be redundant */
				condition_broadcast(&cthread_needed);
				mutex_unlock(&cthread_lock);
				_exit(0);  /* Don't clean up stdio again! */
			} else
				condition_wait(&cthread_needed, &cthread_lock);
#else	MTASK
			condition_wait(&cthread_needed, &cthread_lock);
#endif	MTASK
		}
	}
}

cthread_t
cthread_fork(func, arg)
	cthread_fn_t func;
	any_t arg;
{
	register cthread_t t;

	TRACE(printf("[%s] fork()\n", cthread_name(cthread_self())));
	mutex_lock(&cthread_lock);
	t = cthread_alloc(func, arg);
	cthread_queue_enq(&cthreads, t);
	if (++n_cthreads > n_cprocs && (max_cprocs == 0 || n_cprocs < max_cprocs)) {
		n_cprocs += 1;
#if	NeXT
		t->real_thread = cproc_create();
#else
		cproc_create();
#endif	NeXT
	}
	mutex_unlock(&cthread_lock);
	condition_signal(&cthread_needed);
	return t;
}

void
cthread_detach(t)
	cthread_t t;
{
	TRACE(printf("[%s] detach(%s)\n", cthread_name(cthread_self()), cthread_name(t)));
	mutex_lock(&t->lock);
	if (t->state & T_RETURNED) {
		mutex_unlock(&t->lock);
		cthread_free(t);
	} else {
		t->state |= T_DETACHED;
		mutex_unlock(&t->lock);
	}
}

any_t
cthread_join(t)
	cthread_t t;
{
	any_t result;

	TRACE(printf("[%s] join(%s)\n", cthread_name(cthread_self()), cthread_name(t)));
	mutex_lock(&t->lock);
	ASSERT(! (t->state & T_DETACHED));
	while (! (t->state & T_RETURNED))
		condition_wait(&t->done, &t->lock);
	result = t->result;
	mutex_unlock(&t->lock);
	cthread_free(t);
	return result;
}

void
cthread_exit(result)
	any_t result;
{
	register cthread_t t = cthread_self();

	TRACE(printf("[%s] exit()\n", cthread_name(t)));
	t->result = result;
	if (t->state & T_MAIN) {
		mutex_lock(&cthread_lock);
		while (n_cthreads > 1)
			condition_wait(&cthread_idle, &cthread_lock);
#if 	MTASK
		n_cthreads = 0;		/* Tell other tasks to exit */
		condition_broadcast(&cthread_needed);
#endif	MTASK
		mutex_unlock(&cthread_lock);
		exit((int) result);
	} else {
		_longjmp(t->catch, TRUE);
	}
}

/*
 * Used for automatic finalization by crt0.
 */
extern int (*_cthread_exit_routine)();

void
cthread_set_name(t, name)
	cthread_t t;
	char *name;
{
	t->name = name;
}

char *
cthread_name(t)
	cthread_t t;
{
	return (t == NO_CTHREAD
		? "idle"
		: (t->name == 0 ? "?" : t->name));
}

int
cthread_limit()
{
	return max_cprocs;
}

void
cthread_set_limit(n)
	int n;
{
	max_cprocs = n;
}

int
cthread_count()
{
	return n_cthreads;
}

/*
 * Routines for supporting fork() of multi-threaded programs.
 */


#if	NeXT
extern void _malloc_fork_prepare(), _malloc_fork_parent();
extern void _malloc_fork_child();
extern void _cproc_fork_child(), _stack_fork_child();
static cproc_t	saved_self = 0;
#else
extern void malloc_fork_prepare(), malloc_fork_parent(), malloc_fork_child();
extern void cproc_fork_child(), stack_fork_child();
#endif	NeXT

#if	NeXT
void _cthread_fork_prepare()
#else
void cthread_fork_prepare()
#endif	NeXT
/*
 * Prepare cthreads library to fork() a multi-threaded program.  All cthread
 * library critical section locks are acquired before a fork() and released
 * afterwards to insure no cthread data structure is left in an inconsistent
 * state in the child, which comes up with only the forking thread running.
 */
{
	mutex_lock(&cthread_lock);
	spin_lock(&cthread_free_lock);
#if	NeXT
	saved_self = cproc_self();
	_malloc_fork_prepare();
#else
	malloc_fork_prepare();
#endif	NeXT
}

#if	NeXT
void _cthread_fork_parent()
#else
void cthread_fork_parent()
#endif	NeXT
/*
 * Called in the parent process after a fork syscall.
 * Releases locks acquired by cthread_fork_prepare().
 */
{
#if	NeXT
	_malloc_fork_parent();
#else
	malloc_fork_parent();
#endif	NeXT
	spin_unlock(&cthread_free_lock);
	mutex_unlock(&cthread_lock);
}

#if	NeXT
void _cthread_fork_child()
#else
void cthread_fork_child()
#endif	NeXT
/*
 * Called in the child process after a fork syscall.  Releases locks acquired
 * by cthread_fork_prepare().  Deallocates cthread data structures which
 * described other threads in our parent.  Makes this thread the main thread.
 * 
 * The mach_init() routine must be called in the child before this routine.
 */
{
	cthread_t t;

#if	NeXT
	cthread_set_self(saved_self);
	_malloc_fork_child();
#else
	malloc_fork_child();
#endif	NeXT
	spin_unlock(&cthread_free_lock);
	mutex_unlock(&cthread_lock);

	condition_init(&cthread_needed);
	condition_init(&cthread_idle);
	max_cprocs = 0;

	/*
	 * Reinit other modules.
	 */

#if	NeXT
	_stack_fork_child();
	_cproc_fork_child();
#else
	stack_fork_child();
	cproc_fork_child();
#endif	NeXT

	while (TRUE) {		/* Free cthread runnable list */
		cthread_queue_deq(&cthreads, cthread_t, t);
		if (t == NO_CTHREAD) break;
		free((char *) t);
	}

	while (free_cthreads != NO_CTHREAD) {	/* Free cthread free list */
		t = free_cthreads;
		free_cthreads = free_cthreads->next;
		free((char *) t);
	}

	/*
	 * From cthread_init() ...
	 */

	t = cthread_self();

	n_cprocs = 1;
	n_cthreads = 1;
	t->state = T_MAIN;
#if	NeXT
	t->real_thread = thread_self();
#endif	NeXT
	cthread_set_name(t, "main");
	mig_init(1);		/* enable multi-threaded mig interfaces */
	
}


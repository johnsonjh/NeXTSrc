/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY
 * 05-April-90  Morris Meyer (mmeyer) at NeXT
 * 	Fixed bug in cproc_fork_child() where the first cproc would
 *	try doing a msg_rpc() with an invalid reply port.
 *
 * $Log:	cprocs.c,v $
 * Revision 1.5  89/06/21  13:02:10  mbj
 * 	Fixed the performance bug that HP reported regarding spin locks
 * 	held around msg_send() calls.
 * 
 * 	Removed the old (non-IPC) form of condition waiting/signalling.
 * 	It hasn't worked since thread_wait/thread_suspend/thread_resume
 * 	changed, many months ago.
 * 
 * Revision 1.4  89/05/19  13:02:28  mbj
 * 	Initialize cproc flags.
 * 
 * Revision 1.3  89/05/05  18:47:55  mrt
 * 	Cleanup for Mach 2.5
 * 
 * 24-Mar-89  Michael Jones (mbj) at Carnegie-Mellon University
 *	Implement fork() for multi-threaded programs.
 *	Made MTASK version work correctly again.
 *
 * 01-Apr-88  Eric Cooper (ecc) at Carnegie Mellon University
 *	Changed condition_clear(c) to acquire c->lock,
 *	to serialize after any threads still doing condition_signal(c).
 *	Suggested by Dan Julin.
 *
 * 19-Feb-88  Eric Cooper (ecc) at Carnegie Mellon University
 * 	Extended the inline scripts to handle spin_unlock() and mutex_unlock().
 *
 * 28-Jan-88  David Golub (dbg) at Carnegie-Mellon University
 *	Removed thread_data argument from thread_create (for new
 *	interface).
 *
 */
/*
 * cprocs.c - by Eric Cooper
 *
 * Implementation of cprocs (lightweight processes)
 * and primitive synchronization operations.
 */
#if	NeXT
#include <stdlib.h>
#endif	NeXT
extern void exit(int);
#include <cthreads.h>
#include "cthread_internals.h"
#if	MTASK
#include <sys/errno.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/resource.h>
private cproc_interrupt();
#endif	MTASK
#if	MTHREAD
#if	NeXT
#include <sys/message.h>
#else
#include <mach/message.h>
#endif	NeXT
#endif	MTHREAD

/*
 * C Threads imports:
 */
extern void stack_init();
#if	MTHREAD || COROUTINE
#if	NeXT
extern void alloc_stack(), _dealloc_stack();
#else
extern void alloc_stack(), dealloc_stack();
#endif	NeXT
#endif	MTHREAD || COROUTINE
#if	COROUTINE
extern void cproc_start(), cproc_switch();
extern kern_return_t msg_send();
extern select();
#endif	COROUTINE

/*
 * Mach imports:
 */
#if	MTHREAD
extern thread_t thread_self();
#endif	MTHREAD
#if	MTHREAD || MTASK
extern boolean_t swtch_pri();
#endif	MTHREAD || MTASK

private int cprocs_started = FALSE;

#if	COROUTINE
/*
 * Queue of runnable cprocs.
 * Currently executing cproc is at the head.
 */
private struct cthread_queue ready = QUEUE_INITIALIZER;
private int n_blocked;		/* number of blocked cprocs */
#endif	COROUTINE

#ifdef	DEBUG
private void
print_cproc(p)
	cproc_t p;
{
	char *s;

	switch (p->state) {
	    case CPROC_RUNNING:
		s = "";
		break;
	    case CPROC_SPINNING:
		s = "+";
		break;
	    case CPROC_BLOCKED:
		s = "*";
		break;
	    default:
		ASSERT(SHOULDNT_HAPPEN);
	}
	printf(" %x(%s)%s",
#if	MTHREAD || MTASK
		p->id,
#endif	MTHREAD || MTASK
#if	COROUTINE
		p,
#endif	COROUTINE
		cthread_name(p->incarnation), s);
}

private void
print_cproc_queue(name, queue)
	string_t name;
	queue_t queue;
{
	printf("[%s] %s:", cthread_name(cthread_self()), name);
	cthread_queue_map(queue, cproc_t, print_cproc);
	printf("\n");
}
#endif	DEBUG

private int cproc_lock = 0;		/* unlocked */
private cproc_t cprocs = NO_CPROC;	/* linked list of cprocs */

#if	MTASK

/*
 * Find cproc with given id
 * in global list of cproc structures.
 * Since cproc structures are only linked onto the front, never removed,
 * there is no need to lock the list while searching.
 */
private cproc_t
cproc_lookup(id)
	int id;
{
	register cproc_t p;

	for (p = cprocs; p != NO_CPROC; p = p->link)
		if (p->id == id)
			break;
	return p;
}
#endif	MTASK

#ifdef	DEBUG
private void
print_all_cprocs()
{
	cproc_t p;

	printf("[%s] cprocs:", cthread_name(cthread_self()));
	for (p = cprocs; p != NO_CPROC; p = p->link)
		print_cproc(p);
	printf("\n");
}
#endif	DEBUG

private cproc_t
cproc_alloc()
{
	register cproc_t p = (cproc_t) malloc(sizeof(struct cproc));
#if	MTHREAD
	kern_return_t r;
#endif	MTHREAD

	p->incarnation = NO_CTHREAD;
	p->state = CPROC_RUNNING;
	p->reply_port = PORT_NULL;

#if	MTHREAD
	MACH_CALL(port_allocate(task_self(), &p->wait_port), r);
	MACH_CALL(port_disable(task_self(), p->wait_port), r);
	MACH_CALL(port_set_backlog(task_self(), p->wait_port, 1), r);
#endif	MTHREAD

#if	MTHREAD || COROUTINE
	p->flags = 0;
#endif	MTHREAD || COROUTINE

	spin_lock(&cproc_lock);
	p->link = cprocs;
	cprocs = p;
	spin_unlock(&cproc_lock);

	return p;
}

void
cproc_init()
{
	cproc_t p = cproc_alloc();

	ASSERT(!cprocs_started);

#if	MTHREAD
	p->id = thread_self();
#endif	MTHREAD

#if	NeXT
	cthread_set_self(p);
#endif	NeXT

#if	COROUTINE
	cthread_queue_enq(&ready, p);
	n_blocked = 0;
#endif	COROUTINE

	stack_init(p);

#if	MTASK
	p->id = getpid();
	(void) signal(SIGCHLD, cproc_interrupt);
#endif	MTASK

	cprocs_started = TRUE;
}

#if	NeXT
#else	NeXT
#ifndef	ur_cthread_self
/*
 * Find self by masking stack pointer
 * to find value stored at base of stack.
 */
ur_cthread_t
ur_cthread_self()
{
	register cproc_t p;

#if	MTHREAD || COROUTINE
	register int sp = cthread_sp();

	p = *((cproc_t *) (sp & cthread_stack_mask));
	ASSERT(p != NO_CPROC && p->stack_base <= sp && sp < p->stack_base + p->stack_size);
#endif	MTHREAD || COROUTINE
#if	MTASK
	p = cproc_lookup(getpid());
	ASSERT(p != NO_CPROC);
#endif	MTASK
	return (ur_cthread_t) p;
}
#endif	ur_cthread_self
#endif	NeXT

#ifdef	unused
void
cproc_exit()
{
	TRACE(printf("[%s] cproc_exit()\n", cthread_name(cthread_self())));
#if	MTHREAD
	/*
	 * Terminate thread.
	 * Note that the stack is never freed.
	 */
	(void) thread_terminate(thread_self());
#endif	MTHREAD
#if	COROUTINE
	/*
	 * Dequeue thread and context switch.
	 * Note that the stack is never freed.
	 */
	{
		cproc_t p;
		struct cproc dummy;	/* for cproc_switch to save state */

		cthread_queue_deq(&ready, cproc_t, p);
		TRACE(print_cproc_queue("cproc_exit", &ready));
		cproc_switch(&dummy.context, &(cthread_queue_head(&ready, cproc_t)->context));
	}
#endif	COROUTINE
#if	MTASK
	/*
	 * Terminate task.
	 */
	exit(0);
#endif	MTASK
}
#endif	unused

#if	MTHREAD
/*
 * Implement C threads using MACH threads.
 */
#if	NeXT
thread_t
#else
void
#endif	NeXT
cproc_create()
{
	register cproc_t child = cproc_alloc();
	register kern_return_t r;
	extern void cproc_setup();

      	alloc_stack(child);
	MACH_CALL(thread_create(task_self(), &child->id), r);
	cproc_setup(child);	/* machine dependent */
	MACH_CALL(thread_resume(child->id), r);
	TRACE(print_all_cprocs());
#if	NeXT
	return(child->id);
#endif	NeXT
}
#endif	MTHREAD

#if	MTASK
/*
 * Simulate threads using separate tasks.
 */
void
cproc_create()
{
	register cproc_t child = cproc_alloc();

	switch (unix_fork()) {
	    case -1:
		perror("fork");
		ASSERT(SHOULDNT_HAPPEN);
		exit(1);
		/* NOTREACHED */
	    case 0:
		/* child */
		child->id = getpid();	/* must be set before child runs */
		break;
	    default:
		/* parent */
		TRACE(print_all_cprocs());
		return;
	}
	/*
	 * Now running in new thread.
	 */
	stack_init(child);		/* Place self pointer in child stack */
	cthread_body(child);		/* pass thread handle as argument */
}
#endif	MTASK

#if	COROUTINE
/*
 * Simulate threads using coroutines.
 */
void
cproc_create()
{
	register cproc_t parent = cproc_self();
	register cproc_t child = cproc_alloc();

	alloc_stack(child);
	cthread_queue_preq(&ready, child);	/* run child first */
	TRACE(print_cproc_queue("cproc_create", &ready));
	cproc_start(&parent->context, child, child->stack_base + child->stack_size);
}
#endif	COROUTINE

#ifdef	NeXT
/*
 * Global data moved to threads_data.c for SHLIB purposes.
 */
extern int condition_spin_limit;
extern int condition_yield_limit;
#else	NeXT
int condition_spin_limit = 0;
int condition_yield_limit = 7;
#endif	NeXT

void
condition_wait(c, m)
	register condition_t c;
	mutex_t m;
{
	register cproc_t p;
#if	COROUTINE
	register cproc_t q;
#endif	COROUTINE
#if	MTHREAD
	register int i;
	register kern_return_t r;
	msg_header_t msg;
#endif	MTHREAD

	TRACE(printf("[%s] wait(%s,%s)\n",
		     cthread_name(cthread_self()),
		     condition_name(c), mutex_name(m)));

#if	MTHREAD || MTASK
	p = cproc_self();
	spin_lock(&c->lock);
	p->state = CPROC_SPINNING;
	cthread_queue_enq(&c->queue, p);
	TRACE(print_cproc_queue("wait", &c->queue));
	spin_unlock(&c->lock);
#endif	MTHREAD || MTASK

#if	COROUTINE
	cthread_queue_deq(&ready, cproc_t, p);
	cthread_queue_enq(&c->queue, p);
	p->state = CPROC_BLOCKED;
	n_blocked += 1;
	TRACE(print_cproc_queue("wait", &c->queue));
#endif	COROUTINE

	/*
	 * Release the mutex while we wait for the condition.
	 */
	mutex_unlock(m);

#if	MTHREAD
#if	SCHED_HINT
	/*
	 * First spin, yielding the processor.
	 * If swtch_pri() returns TRUE, be a good citizen and block.
	 */
	do {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
	} while (! swtch_pri(0));
#else
	/*
	 * First, try busy-waiting.
	 */
	for (i = 0; i < condition_spin_limit; i += 1) {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
	}
	/*
	 * Next, try yielding the processor.
	 */
	for (i = 0; i < condition_yield_limit; i += 1) {
		if (p->state == CPROC_RUNNING) {
			/*
			 * We've been woken up.
			 */
			goto done;
		}
		(void) swtch_pri(0);
	}
#endif	SCHED_HINT
	spin_lock(&c->lock);
	/*
	 * Check again to avoid race.
	 */
	if (p->state == CPROC_RUNNING) {
		/*
		 * We've been woken up.
		 */
		spin_unlock(&c->lock);
		goto done;
	}
	/*
	 * The kernel has someone else to run, so we block.
	 */
	p->state = CPROC_BLOCKED;
	msg.msg_size = sizeof(msg);
	msg.msg_local_port = p->wait_port;
	TRACE(printf("[%s] receive(%x)\n",
		     cthread_name(cthread_self()), p->wait_port));
	spin_unlock(&c->lock);
	MACH_CALL(msg_receive(&msg, MSG_OPTION_NONE, 0), r);
#endif	MTHREAD

#if	COROUTINE
	q = cthread_queue_head(&ready, cproc_t);
	if (q == NO_CPROC) {
		fprintf(stderr, "\n*** All C threads are blocked. ***\n");
		ASSERT(SHOULDNT_HAPPEN);
	}
	cproc_switch(&p->context, &q->context);
#endif	COROUTINE

#if	MTASK
	while (p->state != CPROC_RUNNING)
		cthread_yield();
#endif	MTASK

#if	MTHREAD
done:
#endif	MTHREAD

	ASSERT(p->state == CPROC_RUNNING);
	/*
	 * Re-acquire the mutex and return.
	 */
	mutex_lock(m);
}

/*
 * Continue a waiting thread.
 * Called from signal or broadcast with the condition variable locked.
 */
private void
cproc_continue(p, c)
	register cproc_t p;
	register condition_t c;
{
#if	MTHREAD
	register int old_state;
	register kern_return_t r;
	msg_header_t msg;
	spin_lock(&c->lock);
	old_state = p->state;
#endif	MTHREAD

	p->state = CPROC_RUNNING;
#if	MTHREAD
	spin_unlock(&c->lock);
	/*
	 * If thread is just spinning,
	 * setting its state to CPROC_RUNNING
	 * will suffice to wake it up.
	 */
	if (old_state != CPROC_BLOCKED)
		return;
	msg.msg_simple = TRUE;
	msg.msg_size = sizeof(msg);
	msg.msg_type = MSG_TYPE_NORMAL;
	msg.msg_local_port = PORT_NULL;
	msg.msg_remote_port = p->wait_port;
	msg.msg_id = 0;
	TRACE(printf("[%s] send(%x)\n",
		     cthread_name(cthread_self()), p->wait_port));
	r = msg_send(&msg, SEND_TIMEOUT, 0);
	if (r != SEND_SUCCESS && r != SEND_TIMED_OUT) {
		mach_error("msg_send", r);
		ASSERT(SHOULDNT_HAPPEN);
		exit(1);
	}
#endif	MTHREAD

#if	COROUTINE
	cthread_queue_enq(&ready, p);
	n_blocked -= 1;
#endif	COROUTINE

#if	MTASK
	/* do nothing */
#endif	MTASK
}

void
cond_signal(c)
	register condition_t c;
{
	register cproc_t p;

	TRACE(printf("[%s] signal(%s)\n",
		     cthread_name(cthread_self()), condition_name(c)));
	TRACE(print_cproc_queue("signal", &c->queue));

	spin_lock(&c->lock);
	cthread_queue_deq(&c->queue, cproc_t, p);
	spin_unlock(&c->lock);
	if (p != NO_CPROC)
		cproc_continue(p, c);
}

void
cond_broadcast(c)
	register condition_t c;
{
	register cproc_t p;
	struct cthread_queue queue;

	TRACE(printf("[%s] broadcast(%s)\n",
		     cthread_name(cthread_self()), condition_name(c)));
	TRACE(print_cproc_queue("broadcast", &c->queue));

	spin_lock(&c->lock);
	queue = c->queue;
	cthread_queue_init(&c->queue);
	spin_unlock(&c->lock);
	for (;;) {
		cthread_queue_deq(&queue, cproc_t, p);
		if (p == NO_CPROC)
			break;
		cproc_continue(p, c);
	}
}

void
cthread_yield()
{
#if	COROUTINE
	if (ready.head != ready.tail) {
		/*
		 * Place ourself at the end of the ready queue.
		 */
		register cproc_t p;
		cthread_queue_deq(&ready, cproc_t, p);
		cthread_queue_enq(&ready, p);
		cproc_switch(&p->context, &(cthread_queue_head(&ready, cproc_t)->context));
	}
#endif	COROUTINE

#if	MTHREAD || MTASK
	/* let the kernel reschedule us */
	swtch_pri(0);
#endif	MTHREAD || MTASK
}

#if	COROUTINE
/*
 * Temporary hack to force linking with
 * alternate versions of blocking system calls.
 * This function will never be called,
 * but we rely on the C compiler being too stupid
 * to realize that and producing code for it anyway.
 */
#ifndef	lint
static void
force_linking()
{
	ASSERT(SHOULDNT_HAPPEN);
	msg_send(0, 0, 0);
	select(0, 0, 0, 0);
}
#endif	lint
#endif	COROUTINE

#if	MTASK
private char *signal_name[] = {
	"Hangup",
	"Interrupt",
	"Quit",
	"Illegal instruction",
	"Trace/BPT trap",
	"IOT trap",
	"EMT trap",
	"Floating point exception",
	"Killed",
	"Bus error",
	"Segmentation fault",
	"Bad system call",
	"Broken pipe",
	"Alarm clock",
	"Terminated",
	"Urgent I/O",
	"Stopped (signal)",
	"Stopped (from tty)",
	"Continued",
	"Child exited",
	"Stopped (tty input)",
	"Stopped (tty output)",
	"TTY input interrupt",
	"CPU time limit exceeded",
	"File size limit exceeded",
	"Virtual time alarm clock",
	"Profiling time alarm clock",
	"Window size changed",
	"Signal 29",
	"Emergency message received",
	"Message received",
	"Signal 32",
};

/*
 * Interrupt routine called when child process terminates.
 */
private
cproc_interrupt()
{
	int id;
	union wait w;
	cproc_t p;
	char *name;
	int mention = FALSE;
	extern int errno;

	extern int cthread_last_child_pid;

	TRACE(printf("[%s] cproc_interrupt()\n", cthread_name(cthread_self())));
	TRACE(mention = TRUE);	/* always print message when debugging */
	while ((id = wait3(&w, WNOHANG, (struct rusage *) 0)) != 0) {
		if (id < 0) switch (errno) {
		    case EINTR:
			continue;
		    case ECHILD:
			return;
		    default:
			TRACE(perror("wait3"));
			return;
		}
		p = cproc_lookup(id);
		if (p != NO_CPROC) {
			p->id = -1;
			name = cthread_name(p->incarnation);
			if (w.w_termsig != 0)
				mention = TRUE;
		} else if (id != cthread_last_child_pid) {
			name = "unknown";
			mention = TRUE;
		}
		if (mention) {
			printf("[%s] (process %d) ", name, id);
			if (w.w_termsig != 0)
				printf("%s%s\n", signal_name[w.w_termsig-1],
					w.w_coredump ? " (core dumped)" : "");
			else
				printf("Exited\n");
		}
	}
	TRACE(print_all_cprocs());
}
#endif	MTASK

/*
 * Routines for supporting fork() of multi-threaded programs.
 */

#if	NeXT
void _cproc_fork_child()
#else
void cproc_fork_child()
#endif	NeXT
/*
 * Called in the child after a fork().  Resets cproc data structures to
 * coincide with the reality that we now have a single cproc and cthread.
 */
{
	cproc_t p, n;
#if	MTHREAD
	kern_return_t r;
#endif	MTHREAD

	/*
	 * From cprocs.c ...
	 */

#if	COROUTINE
	cthread_queue_init(&ready);
	n_blocked = 0;		/* number of blocked cprocs */
#endif	COROUTINE
	cproc_lock = 0;		/* unlocked */

#if	NeXT
	/*
	 * Fix for the case where the first cproc in "cprocs" is not
	 * cproc_self() and we go onto dealloc_stack() which does a 
	 * vm_deallocate.  The vm_deallocate will then fail because the
	 * reply port is invalid (SEND_INVALID_PORT).
	 */
	p = cproc_self();
	p->reply_port = PORT_NULL;
#endif	NeXT
	/*
	 * Free all cprocs.
	 */
	for (p = cprocs; p != NO_CPROC; p = n) {
		if (cproc_self() == p) {	/* Found ourself */
			/*
			 * From cproc_alloc() ...
			 */
			p->state = CPROC_RUNNING;
			p->reply_port = PORT_NULL;


#if	MTHREAD
			MACH_CALL(port_allocate(task_self(), &p->wait_port), r);
			MACH_CALL(port_disable(task_self(), p->wait_port), r);
			MACH_CALL(port_set_backlog(task_self(), p->wait_port, 1), r);
#endif	MTHREAD

			cprocs = p;		/* We become the only cproc */
			n = p->link;		/* No next one */
			p->link = NO_CPROC;	/* No more cprocs */

			/*
			 * From cproc_init() ...
			 */

#if	COROUTINE
			cthread_queue_enq(&ready, p);
#endif	COROUTINE

#if	MTASK
			p->id = getpid();
#endif	MTASK
			continue;		/* Don't deallocate self */
		}

		if (p->incarnation != NO_CTHREAD) {
			free((char *) p->incarnation);	/* Free cthread */
		}

#if	MTHREAD || COROUTINE
		/*
		 * Deallocate the cproc's stack.  Note that this destroys any
		 * local variables which might have been there, including argc,
		 * argv, and envp if the forking cproc was not the main one.
		 */
#if	NeXT
		_dealloc_stack(p);
#else
		dealloc_stack(p);
#endif	NeXT
#endif	MTHREAD || COROUTINE

		n = p->link;
		free((char *) p);	/* Free cproc */
	}
}

#if	NeXT
/*
 *	Support for a per-thread UNIX errno.
 */

void cthread_set_errno_self(error)
	int	error;
{
	cproc_t	c;

	c = cproc_self();
	if (c)
		c->error = error;
}

int cthread_errno()
{
	cproc_t	c;

	c = cproc_self();
	if (c)
		return(c->error);
	else
		return(errno);
}
#endif	NeXT


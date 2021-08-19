#include <cthreads.h>
#include "cthread_internals.h"

#if	MTHREAD

#include <mach.h>

#include <next/thread_status.h>

/*
 * C library imports:
 */
extern bzero();

/*
 * Set up the initial state of a MACH thread
 * so that it will invoke cthread_body(child)
 * when it is resumed.
 */
void
cproc_setup(child)
	register cproc_t child;
{
	register int *top = (int *) (child->stack_base + child->stack_size);
	struct NeXT_thread_state_regs state;
	register struct NeXT_thread_state_regs *ts = &state;
	kern_return_t r;
	extern void cthread_body();


	/*
	 * Set up Next call frame and registers.
	 */
	bzero((char *) &state, sizeof(state));
	/*
	 * Inner cast needed since too many C compilers choke on the type void (*)().
	 */
	ts->pc = (int) (int (*)()) cthread_body;
	*--top = (int) child;	/* argument to function */
	*--top = 0;		/* return address */
#if	NeXT
	/* 
	 * a6 should be set to zero, not to the top of stack.  
	 * The debugger stops stack backtraces when the saved fp is zero. 
	 */
	ts->areg[6] = (int) 0;
#else
	ts->areg[6] = (int) top;
#endif	NeXT
	ts->areg[7] = (int) top;

	/*
	 *	Backslashes in next call are necessary to convince
	 *	CPP that the call is one argument to MACH_CALL.
	 */
	MACH_CALL(thread_set_state(child->id, \
				NeXT_THREAD_STATE_REGS, \
				(thread_state_t)&state, \
				NeXT_THREAD_STATE_REGS_COUNT), r);
}

void cthread_set_self(p)
	cproc_t	p;
{
	asm("movl	%1,d0" : "=m" (*(char *)0): "rm" (p));
	asm("trap	#6");	/* set cthread pointer */
}

ur_cthread_t ur_cthread_self()
{
	ur_cthread_t	ret;

	asm("trap	#5");
	asm("movl	d0,%0" : "=dm" (ret));
}
#endif	MTHREAD

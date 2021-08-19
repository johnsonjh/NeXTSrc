	.text

|
| Suspend the current thread and resume the next one.
|
|	void
|	context_switch(cur, next)
|		cproc_t cur;
|		cproc_t next;
|
_context_switch:
	movl	sp@(4),a0	| cur cproc_t
	movl	sp@(8),a1	| next cproc_t
	moveml	#0xfcfc,a0@(4)	| save cur a7-a2, d7-d2
	moveml	a1@(4),#0xfcfc	| restore next a7-a2, d7-d2
	rts			| return via next stack

|
|	void
|	start_cproc(parent, child, func, stackp)
|		cproc_t parent;
|		cproc_t child;
|		void (*func)();
|		int stackp;
|
	.globl	_start_cproc
_start_cproc:
	movl	sp@(4),a0	| parent cproc_t
	movl	sp@(8),d0	| child cproc_t
	movl	sp@(12),a1	| func ptr
	movl	sp@(16),sp	| switch stacks
	moveml	#0xfcfc,a0@(4)	| save parents a7-a2, d7-d2
	movl	#0,a6		| mark bottom of stack
	movl	d0,sp@-		| push child cproc_t
	jsr	a1@		| (*func)(child)
	jsr	_stop_cproc
	|
	| Control never returns from stop_cproc().
	|

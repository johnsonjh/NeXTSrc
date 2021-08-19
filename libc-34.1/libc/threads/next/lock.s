	.text
|	void
|	spin_lock(p)
|		register int *p;
|
|	Lock the lock pointed to by p.  Spin (possibly forever) until
|		the lock is available.  Test and test and set logic used.

	.globl	_spin_lock

_spin_lock:
	movl	sp@(4),a0	| put lock address in a0 
1:
	
	tstb	a0@		| check lock 
	bmi	1b		| lock is on sign bit of byte
	clrb	a0@(1)		| 040 bug: make sure TLB entry modify bit is set
	tas	a0@		| actually try to lock it 
	bmi	1b
	rts

|	void
|	spin_unlock(p)
|		int *p;
|
|	Unlock the lock pointed to by p.

	.globl	_spin_unlock
_spin_unlock:
	movl	sp@(4),a0
	movb	#0,a0@		| unlock the lock 
	rts


|	mutex_try_lock(p)
|		int *p;
|
|	Try to lock p.  Return a non-zero if not successful.

	.globl	_mutex_try_lock
_mutex_try_lock:
	movl	sp@(4),a0	| put lock address in a0 
	tstb	a0@		| check lock 
	clrb	a0@(1)		| 040 bug: make sure TLB entry modify bit is set
	tas	a0@		| actually try to lock it 
	bmi	1f		| lock is on sign bit of byte
	moveq	#1,d0		| successful
	rts
1:
	moveq	#0,d0		| FAILED
	rts


|	mutex_unlock_try(p)
|		int *p;
|
|	Unlock the lock pointed to by p.  Return non-zero if actually unlocked.

	.text
	.globl	_mutex_unlock_try
_mutex_unlock_try:
	movl	sp@(4),a0
	moveq	#80,d0		| compare operand (locked)
	clrl	d1		| update operand (unlocked)
	casb	d0,d1,a0@
	sne	d0		| d0 -> 0 if already unlocked 
	rts


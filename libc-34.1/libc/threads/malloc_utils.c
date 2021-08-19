#include <stdio.h>
#include <mach.h>
#include <cthreads.h>

#ifdef MTHREAD
#define LOCK spin_lock(&malloc_lock);
#define UNLOCK spin_unlock(&malloc_lock);
#else
#define LOCK ;
#define UNLOCK ;
#endif MTHREAD
extern int malloc_lock;

void _malloc_fork_prepare()
/*
 * Prepare the malloc module for a fork by insuring that no thread is in a
 * malloc critical section.
 */
{
	LOCK
}
void _malloc_fork_parent()
/*
 * Called in the parent process after a fork() to resume normal operation.
 */
{
	UNLOCK
}
void _malloc_fork_child()
/*
 * Called in the child process after a fork() to resume normal operation.
 * In the MTASK case we also have to change memory inheritance so that the
 * child does not share memory with the parent.
 */
{
	UNLOCK
}


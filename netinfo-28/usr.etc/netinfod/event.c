/* 
 * Event handling
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * Simple-minded event handling. Just writes things to a pipe and expects
 * to be called back by the main loop when it detects data ready on the
 * pipe (select() is king).
 */
#include <netinfo/ni.h>
#include <stdio.h>
#include "clib.h"
#include "event.h"
#include "socket_lock.h"

int event_pipe[2] = { -1, -1 };

static void (*event_callback)(void);

/*
 * Handle an event
 */
void
event_handle(
	     void
	     )
{
	char c;

	/*
	 * Clear event
	 */
	(void)read(event_pipe[0], &c, sizeof(c));

	/*
	 * And callback
	 */
	(*event_callback)();

}

/*
 * Post an event
 */
void
event_post(
	   void
	   )
{
	char c;

	c = 'x'; /* not really necessary */
	(void)write(event_pipe[1], &c, sizeof(c));
	(void)fsync(event_pipe[1]);
}


/*
 * Initialize things
 */
void
event_init(
	   void (*callback)(void)
	   )
{
	if (event_pipe[0] < 0) {
		socket_lock();
		(void)pipe(event_pipe);
		socket_unlock();
	}
	event_callback = callback;
}

/*
 * Port and memory management for doing lookups to the lookup server
 * Copyright (C) 1989 by NeXT, Inc.
 */
/*
 * HISTORY
 * 27-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Changed to use bootstrap port instead of service port.
 *
 */
#import <stdlib.h> 			/* imports NULL */
#import <mach.h>
#import "lu_utils.h"
#import <netinfo/lookup_types.h>
#import <servers/bootstrap.h>

port_t _lu_port = PORT_NULL;

void
_lu_setport(port_t desired)
{
	if (_lu_port != PORT_NULL) {
		port_deallocate(task_self(), _lu_port);
	}
	_lu_port = desired;
}

static int
port_valid(port_t port)
{
	port_set_name_t enabled;
	int num_msgs;
	int backlog;
	boolean_t owner;
	boolean_t receiver;

	return (port_status(task_self(), port, &enabled, &num_msgs, &backlog,
			    &owner, &receiver) == KERN_SUCCESS);
}

int
_lu_running(
	    void
	    )
{
	if (_lu_port != PORT_NULL) {
		if (port_valid(_lu_port)) {
			return (1);
		}
		_lu_port = PORT_NULL;
	}
	if (bootstrap_port == PORT_NULL) {
		return (0);
	}
	if (bootstrap_look_up(bootstrap_port, LOOKUP_SERVER_NAME, 
			      &_lu_port) != KERN_SUCCESS) {
		_lu_port = PORT_NULL;
		return (0);
	}
	return (1);
}




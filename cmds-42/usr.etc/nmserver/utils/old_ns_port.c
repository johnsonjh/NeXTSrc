/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * old_ns_port.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/utils/RCS/old_ns_port.c,v $
 *
 */

#ifndef	lint
static char     rcsid[] = "$Header: old_ns_port.c,v 1.1 88/09/30 15:47:20 osdev Exp $";
#endif not lint

/*
 * Replaces the new name server port by the old name server port
 * in the registered ports of our parent.
 */

/*
 * HISTORY:
 * 11-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Started.
 *
 */

#define DEBUG		1
#define MACH_INIT_SLOTS	1

#include <mach.h>
#include <mach_init.h>
#include <stdio.h>

main()
{
    kern_return_t	kr;
    port_t		old_ns_port;
    port_t		parent_task;
    unsigned int	reg_ports_cnt;
    port_array_t	reg_ports;
    int			parent_pid;

    /*
     * Look up the registered ports.
     */
    if ((kr = mach_ports_lookup(task_self(), &reg_ports, &reg_ports_cnt)) != KERN_SUCCESS) {
	fprintf(stderr, "mach_ports_lookup fails, kr = %d.\n", kr);
	exit(-1);
    }

    /*
     * Get the kernel port for our parent.
     */
    parent_pid = getppid();
    if ((parent_task = task_by_pid(parent_pid)) == PORT_NULL) {
	fprintf(stderr, "task_by_pid fails.\n");
	exit(-1);
    }
#if	DEBUG
    printf("Parent pid = %d, parent port = %x.\n", parent_pid, parent_task);
#endif	DEBUG

    /*
     * Get the old name_server_port from the unused registered slot
     * and place it back in the old name server slot.
     */
    reg_ports[NAME_SERVER_SLOT] = reg_ports[MACH_PORTS_SLOTS_USED];
#if	DEBUG
    printf("reg_ports[%d] = %x.\n", NAME_SERVER_SLOT, reg_ports[NAME_SERVER_SLOT]);
#endif	DEBUG

    /*
     * Register the changed ports with our parent.
     */
    if ((kr = mach_ports_register(parent_task, reg_ports, reg_ports_cnt)) != KERN_SUCCESS) {
	fprintf(stderr, "mach_ports_register fails, kr = %d.\n", kr);
	exit(-1);
    }
#if	DEBUG
    printf("done\n");
#endif	DEBUG

}

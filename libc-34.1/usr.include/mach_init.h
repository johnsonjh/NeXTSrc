/* 
 * Mach Operating System
 * Copyright (c) 1989 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * HISTORY 
 * $Log:	mach_init.h,v $
 * 08-May-90  Morris Meyer (mmeyer) at NeXT
 *	Added prototypes for thread_reply(), host_self(), 
 *	host_priv_self().task_notify(), thread_self(), init_process(),
 *	swtch_pri(), swtch(), thread_switch(), mach_swapon().
 *
 * 22-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Removed task_notify() macro (can't use it on NeXT, as it must
 *	be allocated by those who needed.  The trap is the best way to
 *	retrieve this.
 *
 *	Added bootstrap port.
 *
 * Revision 1.3  89/06/13  16:45:00  mrt
 * 	Defined macros for thread_reply and made task_data be another
 * 	name for thread_reply, as task_data() is no longer exported from
 * 	the kernel.
 * 	[89/05/28            mrt]
 * 
 * 	Moved definitions of round_page and trunc_page to
 * 	here from mach/vm_param.h
 * 	[89/05/18            mrt]
 * 
 * Revision 1.2  89/05/05  18:45:39  mrt
 * 	Cleanup and change includes for Mach 2.5
 * 	[89/04/28            mrt]
 * 
 */
/*
 *	Items provided by the Mach environment initialization.
 */

#ifndef	_MACH_INIT_H_
#define	_MACH_INIT_H_	1

#include <kern/mach_types.h>

/*
 *	Kernel-related ports; how a task/thread controls itself
 */

extern	port_t	task_self_;

#define	task_self()	task_self_
extern port_t		thread_reply(void);
#define	current_task()	task_self()

extern port_t		task_notify(void);
extern port_t		thread_self(void);

#ifndef	_KERN_SYSCALL_SUBR_H_
extern	kern_return_t	init_process(void);
extern	boolean_t  	swtch_pri(int pri);
extern	boolean_t  	swtch(void);
extern	kern_return_t	thread_switch(int thread_name, int opt, int opt_time);
#endif	_KERN_SYSCALL_SUBR_H_

extern int		mach_swapon(char *filename, int flags, 
					long lowat, long hiwat);

extern host_t		host_self(void);
extern host_priv_t	host_priv_self(void);

#define	task_data()	thread_reply()

extern void slot_name(cpu_type_t cpu_type, cpu_subtype_t cpu_subtype, 
	char **cpu_name, char **cpu_subname);
/*
 *	Other important ports in the Mach user environment
 */

#define	NameServerPort	name_server_port	/* compatibility */

extern	port_t	bootstrap_port;
extern	port_t	name_server_port;

/*
 *	Globally interesting numbers
 */

extern	vm_size_t	vm_page_size;

#define round_page(x)	((((vm_offset_t)(x) + (vm_page_size - 1)) / vm_page_size) * vm_page_size)
#define trunc_page(x)	((((vm_offset_t)(x)) / vm_page_size) * vm_page_size)

#endif	_MACH_INIT_H_



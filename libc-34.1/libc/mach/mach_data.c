/*
 * This file contains global data and the size of the global data can NOT
 * change or otherwise it would make the shared library incompatable.  It
 * is padded so that new data can take the place of storage occupied by part
 * of it.
 */
#define	MACH_INIT_SLOTS		1
#include "mach_init.h"
#include <mach.h>

extern int mach_init();

port_t		task_self_ = 1;
port_t		name_server_port = PORT_NULL;
port_t		xxx_environment_port = PORT_NULL;	/* Obsolete */
/*
 * Bootstrap port.
 */
port_t		bootstrap_port = PORT_NULL;	/* Was service_port */
vm_size_t	vm_page_size = 0;
port_set_name_t	PORT_ENABLED = PORT_NULL;
port_array_t	_old_mach_init_ports = { 0 };
unsigned int	_old_mach_init_ports_count = 0;
int		(*mach_init_routine)() = mach_init;

asm(".data");
asm("_old_next_minbrk:");
#ifdef SHLIB
asm("old_next_minbrk:	.long	0");
#else
asm("old_next_minbrk:	.long	0");
#endif
asm("_old_next_curbrk:");
#ifdef SHLIB
asm("old_next_curbrk:	.long	0");
#else
asm("old_next_curbrk:	.long	0");
#endif

/* global data padding, must NOT be static */
char _mach_data_padding[212] = { 0 };


#include <mach.h>

extern port_set_name_t PORT_ENABLED;

void
compat_init()
{
	(void) port_set_allocate(task_self(), &PORT_ENABLED);
}

kern_return_t
port_enable(task, port_name)
	task_t task;
	port_name_t port_name;
{
	if (port_name == PORT_NULL)
		return KERN_SUCCESS;

	return port_set_add(task, PORT_ENABLED, port_name);
}

kern_return_t
port_disable(task, port_name)
	task_t task;
	port_name_t port_name;
{
	kern_return_t kr;

	if (port_name == PORT_NULL)
		return KERN_SUCCESS;

	kr = port_set_remove(task, port_name);
	if (kr == KERN_NOT_IN_SET)
		kr = KERN_SUCCESS;
	return kr;
}

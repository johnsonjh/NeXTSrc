/*
 * This will be overridden by user programs, but
 * is provided to resolve library references
 */
#ifdef	SHLIB
#undef	catch_exception_raise
#endif	SHLIB

#include <sys/boolean.h>
#include <sys/message.h>
#include <sys/mig_errors.h>

kern_return_t catch_exception_raise (
	port_t exception_port, 
	port_t thread, 
	port_t task, 
	int exception, 
	int code, int subcode)
{
	abort();
}

/*
 *	File:	mach_error.c
 *	Author:	Avadis Tevanian, Jr.
 *
 *	Standard error printing routines for Mach errors.
 *
 * HISTORY
 * 03-May-90  Morris Meyer (mmeyer) at NeXT
 *	Removed ipcx interface.
 *
 * 21-Mar-90  Gregg Kellogg (gk) at NeXT
 *	Removed env_mgr stuff from file.
 *
 * 12-Dec-88  Craig Hansen (chansen) at NeXT, Inc.
 *      Added an #undef errno to squeak by ANSI C
 *      Removed error code SEND_MSG_SIZE_CHANGE
 *
 * 15-Feb-88  Mary Thompson (mrt) at Carnegie Mellon
 *	Added error codes SEND_MSG_SIZE_CHANGE, RCV_PORT_CHANGE,
 *	MIG_NO_REPLY,MIG_EXCEPTION, MIG_ARRAY_TOO_LARGE
 *
 * 11-May-87  Mary Thompson (mrt) at Carnegie-Mellon
 *	Added error codes for ipcexecd
 *
 * 25-Mar-87  Mary Thompson (mrt) at Carnegie-Mellon
 *	Added missing code from message.h, mig_errors.h and
 *	the server codes from netmsgserver and envmgr.
 *
 * 20-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added SEND_MSG_TOO_{LARGE,SMALL}, deleted SEND_PORT_FULL messages.
 *
 * 19-Sep-86  Michael Young (mwyoung) at Carnegie-Mellon University
 *	Added in message errors, renamed function to conform with other
 *	error string packages.
 *
 *  8-Jun-86  Avadis Tevanian (avie) at Carnegie-Mellon University
 *	Created.
 *
 */

#include <stdio.h>
#include <mach.h>
#include <sys/message.h>
#include <mig_errors.h>
#include <servers/netname.h>
#include <servers/bootstrap_defs.h>

/* errno is effectively a reserved word in ANSI C  - it's defined
   as a macro that expands to a function */
#undef errno

char *mach_error_string(errno)
	kern_return_t	errno;
{
	switch (errno) {
	case KERN_SUCCESS:
		return("successful");
	case KERN_INVALID_ADDRESS:
		return("invalid address");
	case KERN_PROTECTION_FAILURE:
		return("protection failure");
	case KERN_NO_SPACE:
		return("no space available");
	case KERN_INVALID_ARGUMENT:
		return("invalid argument");
	case KERN_FAILURE:
		return("failure");
	case KERN_RESOURCE_SHORTAGE:
		return("resource shortage");
	case KERN_NOT_RECEIVER:
		return("not receiver");
	case KERN_NO_ACCESS:
		return("no access");
#if	NeXT
	case KERN_MEMORY_FAILURE:
		return("memory failure");
	case KERN_MEMORY_ERROR:
		return("memory error");
	case KERN_ALREADY_IN_SET:
		return("port already in set");
	case KERN_NOT_IN_SET:
		return("port not member of set");
	case KERN_NAME_EXISTS:
		return("name translation already exists");
	case KERN_ABORTED:
		return("operation aborted");

	case BOOTSTRAP_NOT_PRIVILEGED:
		return("bootstrap operation not privileged");
	case BOOTSTRAP_NAME_IN_USE:
		return("bootstrap operation name in use");
	case BOOTSTRAP_UNKNOWN_SERVICE:
		return("bootstrap unknown service");
	case BOOTSTRAP_SERVICE_ACTIVE:
		return("bootstrap service already active");
	case BOOTSTRAP_BAD_COUNT:
		return("bootstrap bad count");
	case BOOTSTRAP_NO_MEMORY:
		return("bootstrap no memory");

#endif	NeXT

	case SEND_INVALID_MEMORY:
	case RCV_INVALID_MEMORY:
		return("invalid memory");
	case RCV_INVALID_PORT:
	case SEND_INVALID_PORT:
		return("invalid port");
	case RCV_TIMED_OUT:
	case SEND_TIMED_OUT:
		return("timed out");
	case SEND_WILL_NOTIFY:
		return("will notify");
	case SEND_NOTIFY_IN_PROGRESS:
		return("notify in progress");
	case SEND_KERNEL_REFUSED:
		return("kernel refused message");
	case SEND_INTERRUPTED:
		return("send interrupted");
	case SEND_MSG_TOO_LARGE:
		return("send message too large");
	case SEND_MSG_TOO_SMALL:
		return("send message too small");
/*	case SEND_MSG_SIZE_CHANGE:
		return("message size changed while being copied"); */

	case RCV_TOO_LARGE:
		return("message too large");
	case RCV_NOT_ENOUGH_MEMORY:
		return("no space for message data");
	case RCV_ONLY_SENDER:
		return("only sender remaining");
	case RCV_INTERRUPTED:
		return("receive interrupted");
	case RCV_PORT_CHANGE:
		return("port receiver changed or port became enabled");

	case MIG_TYPE_ERROR:
		return("Type check failure in message interface");
	case MIG_REPLY_MISMATCH:
		return("Wrong return message ID");
	case MIG_REMOTE_ERROR:
		return("Server detected error");
	case MIG_BAD_ID:
		return("Bad message ID");
	case MIG_BAD_ARGUMENTS:
		return("Server found wrong arguments");
	case MIG_NO_REPLY:
		return("No reply should be sent");
	case MIG_EXCEPTION:
		return("Server raised exception");
	case MIG_ARRAY_TOO_LARGE:
		return("User specified array not large enough for return info");

	case NETNAME_NOT_YOURS:
		return("Netnameserver: name is not yours");
	case NETNAME_NOT_CHECKED_IN:
		return("Netnameserver: name not checked in");
	case NETNAME_NO_SUCH_HOST:
		return("Netnameserver: no such host");
	case NETNAME_HOST_NOT_FOUND:
		return("Netnameserver: host not found");

	default:
		return("unknown error");
	}
}

char *mach_errormsg(errno)
	kern_return_t	errno;
{
	return(mach_error_string(errno));
}

void mach_error(str, errno)
	char		*str;
	kern_return_t	errno;
{
	char	*msg;

	msg = mach_errormsg(errno);
	fprintf(stderr, "%s: %s(%d)\n", str, msg, errno);
}


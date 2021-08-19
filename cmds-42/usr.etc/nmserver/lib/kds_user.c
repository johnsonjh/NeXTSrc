/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
#include "kds.h"
#ifdef	KERNEL
#include "../h/message.h"
#include "../kern/mach.h"
#else	KERNEL
#include <sys/message.h>
#include <kern/mach.h>
#include <strings.h>
#endif	KERNEL
#include <mig_errors.h>
#include <sys/types.h>
#include <msg_type.h>

#ifndef	mig_internal
#define	mig_internal	static
#endif	mig_internal

#ifndef	TypeCheck
#define	TypeCheck 1
#endif	TypeCheck

#ifndef	UseExternRCSId
#if	hc
#define	UseExternRCSId		1
#endif	hc
#endif	UseExternRCSId

#ifndef	UseStaticMsgType
#ifndef	hc
#define	UseStaticMsgType	1
#endif	hc
#endif	UseStaticMsgType

static int msg_type_var_kds = MSG_TYPE_NORMAL;

int set_msg_type_kds(newType)
	int newType;
{
	register int oldType = msg_type_var_kds;

	msg_type_var_kds = newType;
	return oldType;
}

static port_t kds_reply_port = PORT_NULL;
static boolean_t kds_reply_port_is_ours = FALSE;

void kds_reply_port_dealloc()
{
	if ((kds_reply_port != PORT_NULL) &&
	    kds_reply_port_is_ours)
		(void) port_deallocate(task_self(), kds_reply_port);
	kds_reply_port = PORT_NULL;
}

#define kds_reply_port_alloc()	((kds_reply_port == PORT_NULL) ? _kds_reply_port_alloc() : kds_reply_port)

static port_t _kds_reply_port_alloc()
{
	port_t	new_thing;

	if (port_allocate(task_self(), &new_thing) == KERN_SUCCESS) {
		/* not needed in new kernels */
		(void) port_disable(task_self(), new_thing);
		kds_reply_port_is_ours = TRUE;
		return kds_reply_port = new_thing;
	} else {
		kds_reply_port_is_ours = FALSE;
		return kds_reply_port = task_data();
	}
}

void init_kds(rep_port)
	port_t rep_port;
{
	kds_reply_port_dealloc();
	kds_reply_port_is_ours = (rep_port == PORT_NULL);
	kds_reply_port = rep_port;
}

/* SimpleRoutine kds_do_key_exchange */
kern_return_t kds_do_key_exchange(server_port, remote_host)
	port_t server_port;
	netaddr_t remote_host;
{
	typedef struct {
		msg_header_t Head;
		msg_type_t remote_hostType;
		netaddr_t remote_host;
	} Request;

	union {
		Request In;
	} Mess;

	register Request *InP = &Mess.In;

#if	UseStaticMsgType
	static msg_type_t remote_hostType = {
		/* msg_type_name = */		MSG_TYPE_INTEGER_32,
		/* msg_type_size = */		32,
		/* msg_type_number = */		1,
		/* msg_type_inline = */		TRUE,
		/* msg_type_longform = */	FALSE,
		/* msg_type_deallocate = */	FALSE,
	};
#endif	UseStaticMsgType

	InP->Head.msg_simple = TRUE;
	InP->Head.msg_size = sizeof(Request);
	InP->Head.msg_type = msg_type_var_kds;
	InP->Head.msg_remote_port = server_port;
	InP->Head.msg_local_port = PORT_NULL;
	InP->Head.msg_id = 11400;

#if	UseStaticMsgType
	InP->remote_hostType = remote_hostType;
#else	UseStaticMsgType
	InP->remote_hostType.msg_type_name = MSG_TYPE_INTEGER_32;
	InP->remote_hostType.msg_type_size = 32;
	InP->remote_hostType.msg_type_number = 1;
	InP->remote_hostType.msg_type_inline = TRUE;
	InP->remote_hostType.msg_type_longform = FALSE;
	InP->remote_hostType.msg_type_deallocate = FALSE;
#endif	UseStaticMsgType

	InP->remote_host /* remote_host */ = /* remote_host */ remote_host;

	return msg_send(&InP->Head, MSG_OPTION_NONE, 0);
}

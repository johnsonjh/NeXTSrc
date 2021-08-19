/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * nm_enable.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/nm_enable.c,v $
 *
 */
#ifndef	lint
char nm_enable_rcsid[] = "$Header: nm_enable.c,v 1.1 88/09/30 15:40:40 osdev Exp $";
#endif not lint

#include <mach.h>
#include <mig_errors.h>
#include <msg_type.h>
#include <sys/message.h>

static port_t enable_reply_port = PORT_NULL;
static port_t disable_reply_port = PORT_NULL;

kern_return_t	nm_port_enable( ServPort,my_port)
	task_t		ServPort;
	port_t		my_port;
{

	typedef struct {
		msg_header_t	Head;
		msg_type_t	IPCNam2;
		port_t	Arg2;
	}	MyMessage;

	typedef struct {
		msg_header_t	Head;
		msg_type_t	RetCodeType;
		kern_return_t	RetCode;
	}	RepMessage;

	msg_return_t	msg_result;
	union {
		MyMessage	Request;
#define	MyMsg		(BothMessages.Request)
		RepMessage	Reply;
#define	RepMsg		(BothMessages.Reply)
	}		BothMessages;

	if (enable_reply_port == PORT_NULL) {
		if ((msg_result = port_allocate(task_self(), &enable_reply_port)) != KERN_SUCCESS)
			return(msg_result);
	}

	MyMsg.Head.msg_simple = FALSE;
	MyMsg.Head.msg_size = sizeof (MyMsg);
	MyMsg.Head.msg_type = MSG_TYPE_RPC;
	MyMsg.Head.msg_remote_port = ServPort;
	MyMsg.Head.msg_local_port = enable_reply_port;
	MyMsg.Head.msg_id = 2002;

	MyMsg.IPCNam2.msg_type_inline = TRUE;
	MyMsg.IPCNam2.msg_type_deallocate =  FALSE;
	MyMsg.IPCNam2.msg_type_longform = FALSE;
	MyMsg.IPCNam2.msg_type_name = MSG_TYPE_PORT;
	MyMsg.IPCNam2.msg_type_size = 32;
	MyMsg.IPCNam2.msg_type_number = (1);
	MyMsg.Arg2 = (my_port);

	if ((msg_result = msg_send(&MyMsg.Head, MSG_OPTION_NONE, 0)) != SEND_SUCCESS)
		return(msg_result);

	RepMsg.Head.msg_size = sizeof (RepMsg);
	RepMsg.Head.msg_local_port = enable_reply_port;
	if ((msg_result = msg_receive(&RepMsg.Head, MSG_OPTION_NONE, 0)) != RCV_SUCCESS)
		return(msg_result);
	if ((RepMsg.Head.msg_id != 2102) || (RepMsg.Head.msg_size != sizeof(RepMsg)))
		return(MIG_REPLY_MISMATCH);
	if (RepMsg.RetCodeType.msg_type_name != MSG_TYPE_INTEGER_32)
		return(MIG_TYPE_ERROR);
	return(RepMsg.RetCode);

}

kern_return_t	nm_port_disable( ServPort,my_port)
	task_t		ServPort;
	port_t		my_port;
{

	typedef struct {
		msg_header_t	Head;
		msg_type_t	IPCNam2;
		port_t	Arg2;
	}	MyMessage;

	typedef struct {
		msg_header_t	Head;
		msg_type_t	RetCodeType;
		kern_return_t	RetCode;
	}	RepMessage;

	msg_return_t	msg_result;
	union {
		MyMessage	Request;
#define	MyMsg		(BothMessages.Request)
		RepMessage	Reply;
#define	RepMsg		(BothMessages.Reply)
	}		BothMessages;

	if (disable_reply_port == PORT_NULL) {
		if ((msg_result = port_allocate(task_self(), &disable_reply_port)) != KERN_SUCCESS)
			return(msg_result);
	}

	MyMsg.Head.msg_simple = FALSE;
	MyMsg.Head.msg_size = sizeof (MyMsg);
	MyMsg.Head.msg_type = MSG_TYPE_RPC;
	MyMsg.Head.msg_remote_port = ServPort;
	MyMsg.Head.msg_local_port = disable_reply_port;
	MyMsg.Head.msg_id = 2003;

	MyMsg.IPCNam2.msg_type_inline = TRUE;
	MyMsg.IPCNam2.msg_type_deallocate =  FALSE;
	MyMsg.IPCNam2.msg_type_longform = FALSE;
	MyMsg.IPCNam2.msg_type_name = MSG_TYPE_PORT;
	MyMsg.IPCNam2.msg_type_size = 32;
	MyMsg.IPCNam2.msg_type_number = (1);
	MyMsg.Arg2 = (my_port);

	if ((msg_result = msg_send(&MyMsg.Head, MSG_OPTION_NONE, 0)) != SEND_SUCCESS)
		return(msg_result);

	RepMsg.Head.msg_size = sizeof (RepMsg);
	RepMsg.Head.msg_local_port = disable_reply_port;
	if ((msg_result = msg_receive(&RepMsg.Head, MSG_OPTION_NONE, 0)) != RCV_SUCCESS)
		return(msg_result);
	if ((RepMsg.Head.msg_id != 2103) || (RepMsg.Head.msg_size != sizeof(RepMsg)))
		return(MIG_REPLY_MISMATCH);
#if TypeCheck
	if (RepMsg.RetCodeType.msg_type_name != MSG_TYPE_INTEGER_32)
		return(MIG_TYPE_ERROR);
#endif
	return(RepMsg.RetCode);

}

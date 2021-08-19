/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ipc_out.c 
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ipc_out.c,v $ 
 *
 */
#ifndef	lint
char ipc_out_rcsid[] = "$Header: ipc_out.c,v 1.1 88/09/30 15:39:34 osdev Exp $";
#endif not lint
/*
 * Operations for outgoing IPC messages. This module is responsible for
 * receiving messages to be transmitted over the network, translating them
 * and giving them to a transport module for transmission. 
 */

/*
 * Note: this code is augmented with comments indicating the lock status of
 * port records at various stages of the computation. The format is:
 *
 *  <port record ptr> LOCK <current lock>/<needed lock>
 *
 * where <current lock> and <needed lock> are one of "RW", "R" or "-".
 * A necessary condition for correctness is that 
 * <current lock> >= <needed lock>.
 */

/*
 * HISTORY: 
 * 09-Sep-88  Avadis Tevanian (avie) at NeXT
 *	Conditionalize encrypt code.
 *
 * 19-Jan-88  Robert Sansom (rds) at Carnegie Mellon University
 *	Use libmach vm_deallocate, port_enable and port_disable now that
 *	they work for multi-threaded applications.
 *
 * 19-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added port statistics.
 *
 *  5-Aug-87  Daniel Julin (dpj) and Robert Sansom (rds) at Carnegie-Mellon University
 *	Put in Camelot support.
 *
 * 22-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed (void) cast from lk_lock calls.
 *
 *  8-Jul-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Removed the use of temp_buff for the allocation of IPC receive
 *	buffers, because it is causing more problems than it is worth
 *	(e.g. when a message must be re-sent).
 *
 * 22-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	ipc_port_dead only destroys a pending transaction if it is not active.
 *	Turn off the RPC msg_type bit if we are not using VMTP.
 *	Handle local crypt failure correctly in RPC case.
 *
 * 19-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Corrected locking strategy in ipc_out_main and ipc_re_send:
 *	we must get an exclusive lock before marking the port as 'active'.
 *
 * 18-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Do a LOGCHECK in ipc_out_main. Lock the reply port on a
 *	RPC client before transmitting the request, to avoid races
 *	with the response.
 *
 * 17-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified ipc_re_send not to freak out when the ior has already
 *	been initialized on a RPC client.
 *
 * 10-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Adapted for use with VMTP.
 *
 *  8-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Handle a completion code of IPC_BAD_SEQ_NO in ipc_out_cleanup.
 *
 *  2-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Only call po_port_rights_commit if there was a NPD.
 *	Use statically allocated sbuf segments if possible.
 *	Recycle the ipcbuff of an ipc_outrec directly (instead of via mem functions).
 *
 * 26-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Set PORT_INFO_ACTIVE whilst a message is in transit to a port.
 *	Fill in the backlog ipc_iorec before placing it in the port record.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Set the destination network port in ipc_out_backlog.
 *	Added some statistics gathering.
 *
 *  5-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Call nm_vm_deallocate to deallocate out-of-line data.
 *	Lock is now inline in port record.
 *
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Only allocate an sbuf for out-of-line data or the NPD if needed.
 *	Remember in the ipc_outrec whether we allocated an sbuf or not.
 *	Only garbage collect ool/NPD sbuf if it exists.
 *	Statically allocate ipc_re_send_lock and ipc_re_send_signal.
 *	Conditionally use thread_lock - ensures only one thread is executing.
 *	Added call to cthread_set_name.  Removed PRINT_ERROR.
 *
 * 17-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Use non-blocking netmsg_receive provided by nm_extra.
 *	No need to panic if the reply port is PORT_NULL.
 *	Call mem_dealloc instead of vm_deallocate.
 *	Only append out-of-line data if we have some!
 *
 * 16-Apr-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added debugging macros.
 *
 * 15-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Removed a little bit of lint.
 *
 * 25-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Removed definition of MSG_TYPE_RPC.
 *
 *  4-Feb-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Split ipc_retry into ipc_retry and ipc_port_dead.
 *
 * 22-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Look at the msg_type field of a message to determine its crypt_level.
 *	Update the value of the portrec_aliveness field if we successfully
 *	send a message to a remote network server.
 *	Added ipc_sequence_number which is incremented for and inserted into
 *	each message sent out over the network.
 *	ipc_out_cleanup calls do_key_exchange if the completion code is TR_CRYPT_FAILURE.
 *	Check return code from the transport send; call ipc_out_cleanup if
 *	it is not TR_SUCCESS.
 *
 *  2-Jan-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Changes for integration with other modules.
 *
 * 13-Dec-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified for locking of port records and for request-response
 *	operations.
 *
 * 15-Nov-86  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 *
 *
 */

#include	<cthreads.h>
#include	<mach.h>
#include	<msg_type.h>
#include	<stdio.h>
#include	<sys/message.h>
#include	<sys/time.h>
#include	<sys/types.h>
#include	<netinet/in.h>

#include	"config.h"
#include	"crypt.h"
#include	"debug.h"
#include	"ipc.h"
#include	"ipc_hdr.h"
#include	"ipc_rec.h"
#include	"key_defs.h"
#include	"keyman.h"
#include	"ls_defs.h"
#include	"mem.h"
#include	"netmsg.h"
#include	"nm_defs.h"
#include	"nm_extra.h"
#include	"port_defs.h"
#include	"portops.h"
#include	"portrec.h"
#include	"portsearch.h"
#include	"rwlock.h"
#include	"transport.h"

#if	CAMELOT
#include "../camelot/cam.h"
extern struct mutex	camelot_lock;
#endif	CAMELOT

void ipc_out_backlog();

PRIVATE struct mutex		ipc_re_send_lock;	/* lock for IPC Re-Send queue */
PRIVATE struct condition	ipc_re_send_signal;	/* condition to wakeup IPC Re-Send */
PRIVATE pointer_t		ipc_re_send_q;		/* IPC Re-Send queue */
PRIVATE unsigned long		ipc_sequence_number;

/*
 * Macro to call a transport-level send routine.
 *
 * For now, it only knows about one transport module.
 */
#define	ipc_xmit(ior_ptr) {							\
	int	tr;								\
	if ((ior_ptr)->trmod == 0) (ior_ptr)->trmod = TR_DELTAT_ENTRY;		\
	if ((tr = transport_switch[(ior_ptr)->trmod].send(			\
			(ior_ptr), &((ior_ptr)->out.msg), (ior_ptr)->out.dest,	\
			ior_ptr->trid,						\
			(ior_ptr)->out.crypt_level, (ipc_out_cleanup)))		\
		!= TR_SUCCESS)							\
	{									\
		(void)ipc_out_cleanup((int)(ior_ptr), tr);			\
		ior_ptr->status = IPC_IOREC_WAIT_BLOCK;				\
	}									\
	else {									\
		ior_ptr->status = (ior_ptr->type == IPC_IOREC_TYPE_CLIENT)	\
					? IPC_IOREC_WAIT_REPLY : IPC_IOREC_WAIT_CLEANUP;	\
	}									\
}


/*
 * ipc_outmsg -- 
 *
 * Parameters: 
 *
 * ior_ptr:	pointer to an ipc_iorec for the message to transmit
 *
 * msg_ptr:	pointer to a buffer containing the inline section of a
 * message to transmit 
 *
 * dest_port_ptr:	pointer to the port record for the destination port 
 *
 * crypt_level:	the encryption level of the message
 *
 * Results: 
 *
 * none
 *
 * Side effects: 
 *
 * Initializes the ipc_outrec and translates the outgoing message into
 * internal format (sbuf). Translates the ports in the message if necessary.
 * Arranges for everything to be correctly garbage-collected when the
 * ipc_outrec is destroyed (including the message buffer itself). 
 *
 * Note: 
 *
 * This procedure does everything that can be done without using the port record
 * for the destination of the message. It is intended to be followed by a
 * call to ipc_xmit(), which should fill-out the network server header and
 * initiate transmission. ipc_outmsg() must be called only once for each
 * message, whereas ipc_xmit() may be called several times if retransmission
 * or redirection are needed. 
 *
 */
PRIVATE
void ipc_outmsg(IN ior_ptr, IN msg_ptr, IN dest_port_ptr, IN crypt_level)
	ipc_iorec_ptr_t		ior_ptr;
	msg_header_t		*msg_ptr;
	port_rec_ptr_t		dest_port_ptr;
	int			crypt_level;
BEGIN("ipc_outmsg")
	register ipc_outrec_ptr_t	or_ptr;

DEBUG3(debug.ipc_out,0,2010,(long)ior_ptr,(long)dest_port_ptr,crypt_level);

or_ptr = & ior_ptr->out;
/*
 * Fill in some important fields in the ipc_outrec.
 */
or_ptr->ipcbuff = msg_ptr;
or_ptr->dest_port_ptr = dest_port_ptr;
or_ptr->ool_exists = FALSE;
or_ptr->npd_exists = FALSE;

/*
 * Prepare the IPC netmsg header 
 */
or_ptr->netmsg_hdr.disp_hdr.disp_type = htons(DISP_IPC_MSG);
or_ptr->netmsg_hdr.disp_hdr.src_format = conf_own_format;
or_ptr->netmsg_hdr.info = 0;
or_ptr->netmsg_hdr.ipc_seq_no = ipc_sequence_number++;
/*
 * Translate the reply port, the destination port has already been translated.
 */
{
	port_rec_ptr_t	rp_rec_ptr;
	if (msg_ptr->msg_remote_port == PORT_NULL) {
		or_ptr->netmsg_hdr.remote_port = null_network_port;
	}
	else if ((rp_rec_ptr = pr_ltran(msg_ptr->msg_remote_port)) == PORT_REC_NULL) {
		panic("ipc_outmsg.pr_ltran");
	}
	else {
		/* rp_rec_ptr LOCK RW/R */
		or_ptr->netmsg_hdr.remote_port = rp_rec_ptr->portrec_network_port;
		lk_unlock(&rp_rec_ptr->portrec_lock);
	}
}
or_ptr->netmsg_hdr.inline_size = msg_ptr->msg_size;

/*
 * This is a good place to do whatever the Camelot communications manager wants to do. 
 */

#if	CAMELOT
if(Cam_Message(Cam_MsgHeader(msg_ptr))) {
	mutex_lock(&camelot_lock);
	if (!Cam_Transmit(Cam_MsgHeader(msg_ptr),
		dest_port_ptr->portrec_network_port.np_receiver)) {
		ERROR((msg,"ipc_out_msg,Cam_Transmit returned FALSE"));
	}
	mutex_unlock(&camelot_lock);
}
#endif	CAMELOT

if (msg_ptr->msg_simple) {
	/*
	 * For a simple message, we have nothing much to do. 
	 */
	or_ptr->netmsg_hdr.info |= IPC_INFO_SIMPLE;
	or_ptr->netmsg_hdr.npd_size = 0;
	or_ptr->msg.end = or_ptr->msg.segs = &or_ptr->segs[0];
	or_ptr->msg.free = or_ptr->msg.size = IPC_OUT_NUM_SEGS;
	SBUF_APPEND(or_ptr->msg, 0, 0);	/* empty spare segment */
	SBUF_APPEND(or_ptr->msg, (pointer_t) & or_ptr->netmsg_hdr, sizeof(ipc_netmsg_hdr_t));
	SBUF_APPEND(or_ptr->msg, (pointer_t) msg_ptr, msg_ptr->msg_size);
} else {
	/*
	 * The message is not simple. We must scan it to find the out-of-line
	 * sections, and build a Network Port Dictionary for the embedded ports. 
	 */

	/*
	 * Variables for scanning the message 
	 */
	msg_type_long_t *scan_ptr;	/* pointer for scanning the msg */
	msg_type_long_t *end_scan_ptr;	/* pointer for end of msg */

	/*
	 * Variables for keeping track of out-of-line sections 
	 */
	unsigned long   ool_size = 0;		/* total size of ool stuff */
	unsigned long   ool_num = 0;		/* number of ool sections */

	/*
	 * Variables for building the Network Port Dictionary 
	 */
	sbuf_t          npd;			/* NPD to copy in msg */
	pointer_t       npd_seg_start = 0;	/* start of current NPD segment */
	pointer_t       npd_seg_next = 0;	/* current position in NPD segment */
	int             npd_seg_free = 0;	/* bytes free in current NPD segment */
	int             npd_size = 0;		/* total size of NPD */

	DEBUG0(debug.ipc_out,0,2011);

	/*
	 * Scan the message by iterating over each descriptor in the inline section 
	 */
#define	ADDSCAN(p,o)	(((char *)p + o))
	scan_ptr = end_scan_ptr = (msg_type_long_t *) msg_ptr;
	scan_ptr = (msg_type_long_t *) ADDSCAN(msg_ptr, sizeof(msg_header_t));
	end_scan_ptr = (msg_type_long_t *) ADDSCAN(msg_ptr, msg_ptr->msg_size);

	while (scan_ptr < end_scan_ptr) {
		unsigned long   	tn;	/* type of current data */
		unsigned long   	elts;	/* number of elements in current descriptor */
		unsigned long   	len;	/* length of current data */
		pointer_t       	dptr;	/* pointer to current data */
#ifdef lint
		msg_type_t		mth;	/* current msg_type_header */
#else lint
		register msg_type_t	mth;	/* current msg_type_header */
#endif lint

		mth = scan_ptr->msg_type_header;
		if (mth.msg_type_longform) {
			tn = scan_ptr->msg_type_long_name;
			elts = scan_ptr->msg_type_long_number;
#if	LongAlign
			len = (((scan_ptr->msg_type_long_size * elts) + 31) >> 5) << 2;
#else	LongAlign
			len = (((scan_ptr->msg_type_long_size * elts) + 15) >> 4) << 1;
#endif	LongAlign
			dptr = (pointer_t) ADDSCAN(scan_ptr, sizeof(msg_type_long_t));
		} else {
			tn = mth.msg_type_name;
			elts = mth.msg_type_number;
#if	LongAlign
			len = (((mth.msg_type_size * elts) + 31) >> 5) << 2;
#else	LongAlign
			len = (((mth.msg_type_size * elts) + 15) >> 4) << 1;
#endif	LongAlign
			dptr = (pointer_t) ADDSCAN(scan_ptr, sizeof(msg_type_t));
		}

		/*
		 * This is a good place to handle imaginary data (copy-on-reference). 
		 */

		/*
		 * Enter out-of-line sections in the sbufs if necessary
		 * and advance to the next descriptor.
		 */
		if (mth.msg_type_inline) {
			scan_ptr = (msg_type_long_t *) ADDSCAN(dptr, len);
		} else {
			if (!or_ptr->ool_exists) {
				SBUF_SEG_INIT(or_ptr->ool, &or_ptr->ool_seg);
				or_ptr->ool_exists = TRUE;
			}
			else if (or_ptr->ool.size == 1) {
				/*
				 * Handle special case of growing a single inline sbuf segment.
				 */
				sbuf_seg_ptr_t	new_segs;
				if ((new_segs = (sbuf_seg_ptr_t)mem_alloc(10 * sizeof(struct sbuf_seg),
										FALSE)) == 0)
				{
					panic("ipc_outmsg.mem_alloc");
				}
				new_segs[0] = or_ptr->ool_seg;
				or_ptr->ool.end = or_ptr->ool.segs = new_segs;
				or_ptr->ool.end ++;
				or_ptr->ool.size = 10;
				or_ptr->ool.free = 9;
			}
			scan_ptr = (msg_type_long_t *) ADDSCAN(dptr, sizeof(char *));
			dptr = *((pointer_t *) dptr);
			SBUF_APPEND(or_ptr->ool, dptr, len);
			ool_size += len;
			ool_num++;
		}

		/*
		 * Translate ports if needed.
		 */
		if (MSG_TYPE_PORT_ANY(tn)) {
			int	i;		/* index for iterating over elements */
			int	npd_entry_size;	/* size of new NPD entry */

			if (!or_ptr->npd_exists) {
				SBUF_INIT(or_ptr->npd, 10);
				SBUF_INIT(npd, 10);
				or_ptr->npd_exists = TRUE;
			}
			for (i = 1; i <= elts; i++) {
				/*
				 * Make sure that there is enough space in
				 * the NPD to put a new entry. If necessary,
				 * move the current segment to the sbufs and
				 * allocate a new one. 
				 */
				if (npd_seg_free < PO_MAX_NPD_ENTRY_SIZE) {
					if (npd_seg_start) {
						/*
						 * Only do the move if there is something to move.
						 */
						SBUF_APPEND(or_ptr->npd, npd_seg_start, PO_NPD_SEG_SIZE);
						SBUF_APPEND(npd, npd_seg_start,
								PO_NPD_SEG_SIZE - npd_seg_free);
					}
					npd_seg_start = mem_alloc(PO_NPD_SEG_SIZE, FALSE);
					npd_seg_free = PO_NPD_SEG_SIZE;
					npd_seg_next = npd_seg_start;
				}
				npd_entry_size = po_translate_lport_rights((int)ior_ptr, *(port_t *) dptr,
						(int)tn, crypt_level,
						or_ptr->netmsg_hdr.local_port.np_receiver,
						(pointer_t)npd_seg_next);
				npd_seg_free -= npd_entry_size;
				npd_size += npd_entry_size;
				npd_seg_next = (pointer_t) (((char *) npd_seg_next) + npd_entry_size);
				dptr = (pointer_t) ADDSCAN(dptr, sizeof(port_t));
			}
		}
	}

	/*
	 * The message has been completely scanned. Enter the last sbuf of
	 * the NPD and finish the netmsg header. 
	 */
	if (npd_seg_start) {
		SBUF_APPEND(or_ptr->npd, npd_seg_start, PO_NPD_SEG_SIZE);
		SBUF_APPEND(npd, npd_seg_start, PO_NPD_SEG_SIZE - npd_seg_free);
	}
	or_ptr->netmsg_hdr.npd_size = npd_size;
	or_ptr->netmsg_hdr.ool_size = ool_size;
	or_ptr->netmsg_hdr.ool_num = ool_num;


	/*
	 * Assemble the msg to transmit out all the constituent parts. 
	 */
	{
		int	num_segs = 3;	/* Spare segment, netmsg header, inline message. */
		if (or_ptr->npd_exists) num_segs += npd.size - npd.free;
		if (or_ptr->ool_exists) num_segs += or_ptr->ool.size - or_ptr->ool.free;
		if (num_segs > IPC_OUT_NUM_SEGS) {
			SBUF_INIT(or_ptr->msg, num_segs);
		}
		else {
			/*
			 * Use the statically allocated sbuf segments.
			 */
			or_ptr->msg.end = or_ptr->msg.segs = &or_ptr->segs[0];
			or_ptr->msg.free = or_ptr->msg.size = IPC_OUT_NUM_SEGS;
		}
	}
	SBUF_APPEND(or_ptr->msg, 0, 0);	/* empty spare segment */
	SBUF_APPEND(or_ptr->msg, (pointer_t) & or_ptr->netmsg_hdr, sizeof(ipc_netmsg_hdr_t));
	if (or_ptr->npd_exists) SBUF_SB_APPEND(or_ptr->msg, npd);
	SBUF_APPEND(or_ptr->msg, (pointer_t) msg_ptr, msg_ptr->msg_size);
	if (or_ptr->ool_exists) SBUF_SB_APPEND(or_ptr->msg, or_ptr->ool);
}

#undef	ADDSCAN

RET;

END



/*
 * ipc_out_gc -- 
 *
 * Parameters: 
 *
 * ior_ptr: pointer to ipc_iorec to destroy 
 *
 * Results: 
 *
 * Side effects: 
 *
 * Deallocates the ipc_iorec and all space associated with the outgoing message
 * in the IPC module: IPC buffer, out-of-line sections, headers, Network Port
 * Dictionary, and dynamically allocated sbuf (if it exists).
 *
 */
PRIVATE ipc_out_gc(IN ior_ptr)
	ipc_iorec_ptr_t ior_ptr;
BEGIN("ipc_out_gc")
	sbuf_seg_ptr_t  	cursb_ptr;	/* pointer in sbuf to GC */
	sbuf_seg_ptr_t  	endsb_ptr;	/* last sbuf_seg to GC */
	ipc_outrec_ptr_t	or_ptr;

DEBUG1(debug.ipc_out,3,2012,(long)ior_ptr);

or_ptr = & ior_ptr->out;

#if	CAMELOT
if(Cam_Message(Cam_MsgHeader(or_ptr->ipcbuff))) {
	mutex_lock(&camelot_lock);
	Cam_Cleanup(Cam_MsgHeader(or_ptr->ipcbuff));
	mutex_unlock(&camelot_lock);
	/*
	 * Set the first ool segment to be null because Camelot freed it.
	 */
	or_ptr->ool.segs->p = 0;
	or_ptr->ool.segs->s = 0;
}
#endif	CAMELOT

mem_deallocobj((pointer_t) or_ptr->ipcbuff, MEM_IPCBUFF);

if (or_ptr->ool_exists) {
	cursb_ptr = or_ptr->ool.segs;
	endsb_ptr = or_ptr->ool.end;
	while (cursb_ptr < endsb_ptr) {
		if (cursb_ptr->s != 0) {
			(void)vm_deallocate(task_self(),(vm_address_t)cursb_ptr->p,(vm_size_t)cursb_ptr->s);
		}
		cursb_ptr++;
	}
	if (or_ptr->ool.size > 1)
		mem_dealloc((pointer_t) or_ptr->ool.segs, (or_ptr->ool.size * sizeof(sbuf_seg_t)));
}

if (or_ptr->npd_exists) {
	cursb_ptr = or_ptr->npd.segs;
	endsb_ptr = or_ptr->npd.end;
	while (cursb_ptr < endsb_ptr) {
		if (cursb_ptr->s != 0) {
			mem_dealloc(cursb_ptr->p, (int) cursb_ptr->s);
		}
		cursb_ptr++;
	}
	mem_dealloc((pointer_t) or_ptr->npd.segs, (or_ptr->npd.size * sizeof(sbuf_seg_t)));
}

if (or_ptr->msg.size > IPC_OUT_NUM_SEGS) {
	/*
	 * Free the dynamically allocated sbuf.
	 */
	SBUF_FREE(or_ptr->msg);
}

mem_deallocobj((pointer_t) ior_ptr, MEM_IPCIOREC);

RET;

END


/*
 * ipc_out_cleanup -- 
 *
 * Parameters: 
 *
 * clid: client ID for the outmsg to clean up = address of the ipc_outrec. 
 *
 * completion: code indicating why a cleanup is needed. 
 *
 * Results: 
 *
 * Error code, or 0 if all is well. 
 *
 * Side effects: 
 *
 * This procedure removes all record of an outgoing IPC message in the the IPC
 * module.  It is called when the transport-level modules have finished the
 * transmission of a message, or when an exceptional event occurs.
 * It deallocates the ipc_outrec and all space associated with the message,
 * including the IPC receive buffer.  It indicates the completion of the
 * transfer to the Port Operations module.  It can initiate transmission
 * of a following message, through a call to ipc_retry. 
 *
 * Note: 
 *
 */
PUBLIC ipc_out_cleanup(IN clid, IN completion)
	int		clid;
	int		completion;
BEGIN("ipc_out_cleanup")
	ipc_iorec_ptr_t	old_ior_ptr;	/* ipc_iorec to retire */
	port_rec_ptr_t	dest_port_ptr;	/* destination port record */
	kern_return_t	kern_ret;	/* return for kernel operations */

DEBUG2(debug.ipc_out,0,2013,clid,completion);

old_ior_ptr = (ipc_iorec_ptr_t) clid;
dest_port_ptr = old_ior_ptr->out.dest_port_ptr;

switch (completion) {
	case IPC_SUCCESS: {
		/*
		 * Indicate the message transfer to the Port Operations module
		 */
		if (old_ior_ptr->out.npd_exists)
			po_port_rights_commit((int)clid,PO_RIGHTS_XFER_SUCCESS,old_ior_ptr->out.dest);

		/*
 		 * Update the value of the portrec_aliveness field.
		 * Mark the destination port as available, and start
		 * transmission of the next message if appropriate. 
		 */
		DEBUG1(debug.ipc_out & 0xf000,0,2046,dest_port_ptr);
		lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* dest_port_ptr LOCK RW/RW */
		DEBUG1(debug.ipc_out & 0xf000,0,2047,dest_port_ptr);
		dest_port_ptr->portrec_aliveness = PORT_ACTIVE;
		if ((dest_port_ptr->portrec_current_ipcrec = dest_port_ptr->portrec_backlog_ipcrec) != 0) {
#if	1
			/* DANGEROUS could dead-lock */
			ipc_iorec_ptr_t	n_ior_ptr = (ipc_iorec_ptr_t)dest_port_ptr->portrec_current_ipcrec;
			dest_port_ptr->portrec_backlog_ipcrec = 0;
			n_ior_ptr->out.dest = dest_port_ptr->portrec_network_port.np_receiver;
			ipc_xmit(n_ior_ptr);
			/* DANGEROUS could dead-lock */
#else
			dest_port_ptr->portrec_backlog_ipcrec = 0;
			(void)ipc_retry(dest_port_ptr);
#endif
			DEBUG1(debug.ipc_out,0,2048,dest_port_ptr->portrec_backlog_ipcrec);
			if (!(dest_port_ptr->portrec_info & (PORT_INFO_SUSPENDED | PORT_INFO_BLOCKED))) {
				kern_ret = port_enable(task_self(), dest_port_ptr->portrec_local_port);
				if (kern_ret == KERN_SUCCESS) {
					dest_port_ptr->portrec_info &= ~PORT_INFO_DISABLED;
				} else {
					ERROR((msg, "ipc_out_cleanup: port_enable returned %d.", kern_ret));
				}
			}
		}
		else dest_port_ptr->portrec_info &= ~PORT_INFO_ACTIVE;
		DEBUG1(debug.ipc_out & 0xf000,0,2049,dest_port_ptr);
		lk_unlock(&dest_port_ptr->portrec_lock);
		/* dest_port_ptr LOCK -/- */

		/*
		 * Garbage-collect the space used by the message and discard the ipc_iorec. 
		 */
		DEBUG1(debug.ipc_out,0,2050,old_ior_ptr);
		ipc_out_gc(old_ior_ptr);
		break;
	}

	case IPC_PORT_NOT_HERE: {
		lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* dest_port_ptr LOCK RW/RW */
		ipc_freeze(dest_port_ptr);
		ps_do_port_search(dest_port_ptr, FALSE, (network_port_ptr_t)0, ipc_retry);
		lk_unlock(&dest_port_ptr->portrec_lock);
		/* dest_port_ptr LOCK -/- */
		break;
	}

	case IPC_PORT_BLOCKED: {
		lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* dest_port_ptr LOCK RW/RW */
		dest_port_ptr->portrec_info &= ~PORT_INFO_ACTIVE;
		dest_port_ptr->portrec_info |= PORT_INFO_BLOCKED;
		ipc_freeze(dest_port_ptr);
		lk_unlock(&dest_port_ptr->portrec_lock);
		/* dest_port_ptr LOCK -/- */
		break;
	}

	case TR_FAILURE: 
	case TR_SEND_FAILURE: {
	    	/*
		 * XXX Should worry about total failure of a transport module,
		 * and avoid retrying forever the same thing after each 
		 * successful port search.
		 */
		ERROR((msg, "ipc_out_cleanup: the transport module failed to get a message to machine %x.",
				old_ior_ptr->out.dest));

		lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* dest_port_ptr LOCK RW/RW */
		ipc_freeze(dest_port_ptr);
		ps_do_port_search(dest_port_ptr, FALSE, (network_port_ptr_t)0, ipc_retry);
		lk_unlock(&dest_port_ptr->portrec_lock);
		/* dest_port_ptr LOCK -/- */
		break;
	}

	case TR_CRYPT_FAILURE: {				
		ERROR((msg, "ipc_out_cleanup: the transport module does not have a key for machine %x.",
				old_ior_ptr->out.dest));

		lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
		/* dest_port_ptr LOCK RW/RW */
		ipc_freeze(dest_port_ptr);
		km_do_key_exchange((int)dest_port_ptr, ipc_retry, old_ior_ptr->out.dest);
		lk_unlock(&dest_port_ptr->portrec_lock);
		/* dest_port_ptr LOCK -/- */
		break;
	}

	case IPC_BAD_SEQ_NO: {
		ERROR((msg, "ipc_out_cleanup: IPC_BAD_SEQ_NO response received from machine %x.",
				old_ior_ptr->out.dest));
		ipc_out_gc(old_ior_ptr);
		break;
	}

	default: {
		ERROR((msg, "ipc_out_cleanup: unknown completion code: %d", completion));
		ipc_out_gc(old_ior_ptr);
		break;
	}
}
RETURN(0);

END


/*
 * ipc_out_main -- 
 *
 * Parameters: 
 *
 * none 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Loops forever, waiting to receive an IPC message on a local port, to be
 * transmitted on the network. Whenever such a message is received, the
 * procedure allocates an ipc_iorec for it, translates it, links it to the
 * destination port record, and initiates transmission if appropriate. 
 *
 * Notes:
 *
 * This procedure is the main loop of the IPC Send thread. 
 *
 * Part of this code is repeated in ipc_out_backlog. If you change one,
 * change the other.
 *
 * We assume that ipc_out records are initialised to zero.
 *
 */
PRIVATE ipc_out_main()
BEGIN("ipc_out_main")
	register msg_header_t		*rcvbuff = 0;	/* buffer for IPC receive */
	register port_rec_ptr_t		dest_port_ptr;	/* port record for destination */
	register ipc_iorec_ptr_t	ior_ptr;	/* record for the message being processed */
	msg_return_t			msg_ret;	/* return from netmsg_receive */
	key_t				key;		/* used to check whether we have a key for a host */
	port_rec_ptr_t			reply_port_ptr;
	boolean_t			unlock_reply_port;

#if	LOCK_THREADS
mutex_lock(thread_lock);
#endif	LOCK_THREADS

for (;;) {
/*
 * Wait to receive a message.
 */
if ((rcvbuff = (msg_header_t *) mem_allocobj(MEM_IPCBUFF)) == 0) {
	panic("ipc_out_main cannot get an IPC receive buffer");
}

LOGCHECK;

rcvbuff->msg_local_port = PORT_ENABLED;
rcvbuff->msg_size = MSG_SIZE_MAX;

msg_ret = netmsg_receive(rcvbuff);
INCSTAT(ipc_out_messages);

if (msg_ret == RCV_SUCCESS) {
	DEBUG2(debug.ipc_out,1,2014,(long)rcvbuff->msg_local_port,rcvbuff->msg_id);
}
else {
	ERROR((msg, "ipc_out_main: netmsg_receive returned %d", msg_ret));
	continue;
}
#if	!USE_VMTP
rcvbuff->msg_type &= ~MSG_TYPE_RPC;
#endif	!USE_VMTP

/*
 * Find the port record for the destination.
 */
dest_port_ptr = pr_lportlookup(rcvbuff->msg_local_port);
/* dest_port_ptr LOCK RW/RW */
if (dest_port_ptr == 0) {
	ERROR((msg, "ipc_out_main: received a message on an unknown port"));
	mem_deallocobj((pointer_t)rcvbuff,MEM_IPCBUFF);
	/*
	 * Should also deallocate any ool areas. XXX
	 */
	continue;
}

DEBUG1(debug.ipc_out,0,2015,(long)dest_port_ptr);
INCPORTSTAT(dest_port_ptr, messages_sent);

/*
 * Get an ipc_iorec for this message. Look for a record already
 * waiting, or allocate a new one if needed.
 */
if ((ior_ptr = (ipc_iorec_ptr_t) dest_port_ptr->portrec_current_ipcrec)) {
	if (ior_ptr->status != IPC_IOREC_WAIT_LOCAL) {
		/*
		 * There is already a message for the port.
		 */
		ipc_out_backlog(rcvbuff,dest_port_ptr);
		continue;
	}
} else {
	/*
	 * No waiting record. Allocate one.
	 */
	ior_ptr = (ipc_iorec_ptr_t) mem_allocobj(MEM_IPCIOREC);
	if (ior_ptr == 0) {
		panic("ipc_out_main cannot get a new ipc_iorec");
	}
	if (rcvbuff->msg_type & MSG_TYPE_RPC) {
		ior_ptr->type = IPC_IOREC_TYPE_CLIENT;
		ior_ptr->trmod = TR_VMTP_ENTRY;
	} else {
		ior_ptr->type = IPC_IOREC_TYPE_SINGLE;
		ior_ptr->trmod = 0;
	}
	ior_ptr->trid = 0;
	dest_port_ptr->portrec_current_ipcrec = (pointer_t) ior_ptr;
}

DEBUG1(debug.ipc_out,0,2016,(long)ior_ptr);

/* Translate the destination port. */
ior_ptr->out.netmsg_hdr.local_port = dest_port_ptr->portrec_network_port;

lk_unlock(&dest_port_ptr->portrec_lock);
/* dest_port_ptr LOCK -/- */

/*
 * Fill in the output record and translate the message.
 */
ior_ptr->out.crypt_level = (rcvbuff->msg_type & MSG_TYPE_ENCRYPTED) ? CRYPT_ENCRYPT : CRYPT_DONT_ENCRYPT;
ipc_outmsg(ior_ptr, rcvbuff, dest_port_ptr, ior_ptr->out.crypt_level);
if (ior_ptr->type == IPC_IOREC_TYPE_CLIENT) {
    	ior_ptr->out.netmsg_hdr.info |= IPC_INFO_RPC;
}

/*
 * Send the message if possible.
 */
lk_lock(&dest_port_ptr->portrec_lock,PERM_READWRITE,TRUE);
/* dest_port_ptr LOCK RW/RW */

if ((dest_port_ptr->portrec_info & (PORT_INFO_SUSPENDED | PORT_INFO_BLOCKED))) {
	ior_ptr->status = IPC_IOREC_WAIT_BLOCK;
	lk_unlock(&dest_port_ptr->portrec_lock);
	DEBUG1(debug.ipc_out,0,2017,(long)dest_port_ptr->portrec_info);
	/* dest_port_ptr LOCK -/- */
}
else {
	ior_ptr->out.dest = dest_port_ptr->portrec_network_port.np_receiver;
	dest_port_ptr->portrec_info |= PORT_INFO_ACTIVE;
	lk_unlock(&dest_port_ptr->portrec_lock);
	/* dest_port_ptr LOCK -/- */

	/*
	 * If the message is to be encrypted, check that we have a key for the destination.
	 */
#if	USE_CRYPT
	if ((ior_ptr->out.crypt_level != CRYPT_DONT_ENCRYPT) && (!(km_get_key(ior_ptr->out.dest, &key)))) {
		DEBUG0(debug.ipc_out,0,2018);
		(void)ipc_out_cleanup((int)ior_ptr, TR_CRYPT_FAILURE);
	}
	else {
#endif	USE_CRYPT
	    	DEBUG0(debug.ipc_out,0,2019);
		/*
		 * For RPC, we must lock the local (reply) port
		 * to avoid races if the response/next request comes
		 * very fast.
		 */
		if (ior_ptr->type != IPC_IOREC_TYPE_SINGLE) {
			reply_port_ptr = pr_lportlookup(rcvbuff->msg_remote_port);
			/* reply_port_ptr LOCK RW/RW */
			unlock_reply_port = TRUE;
		} else {
		    	unlock_reply_port = FALSE;
		}
		ipc_xmit(ior_ptr);
#if	USE_CRYPT
	}
#endif	USE_CRYPT
}

/*
 * Prepare an input record if appropriate.
 *
 */
if (ior_ptr->type == IPC_IOREC_TYPE_CLIENT) {
	if (reply_port_ptr == 0) {
		ERROR((msg, "ipc_out_main cannot find a reply port for an RPC request"));
		ior_ptr->in.dest_port_ptr = 0;
		/*
		 * Should probably do something more drastic here,
		 * because the ``call back'' may never come.
		 */
		continue;
	}
	DEBUG1(debug.ipc_out,0,2042,reply_port_ptr);
	ior_ptr->in.dest_port_ptr = reply_port_ptr;
	if (reply_port_ptr->portrec_current_ipcrec) {
		ERROR((msg, "ipc_out_main: reply port already has a pending record"));
		lk_unlock(&reply_port_ptr->portrec_lock);
		/* reply_port_ptr LOCK -/- */
		/*
		 * Should do some more cleanup, because we may miss
		 * the ``call back''.
		 */
		continue;
	}
	reply_port_ptr->portrec_current_ipcrec = (pointer_t) ior_ptr;
	/*
	 * Possible optimization here: pre-allocate an assembly area.
	 */
}
if (unlock_reply_port) {
	lk_unlock(&reply_port_ptr->portrec_lock);
	/* reply_port_ptr LOCK -/- */
}
}
/* NOTREACHED */

END


/*
 * ipc_freeze -- 
 *
 * Parameters: 
 *
 * dest_port_ptr: pointer to the port record for the destination of the messages
 * to be suspended. 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Looks for an ipc_outrec associated with the specified port, and marks it as
 * suspended if appropriate. 
 *
 * Note:
 *
 * This routine does not need to do anything in the current implementation.
 *
 */
EXPORT void ipc_freeze(dest_port_ptr)
	port_rec_ptr_t   dest_port_ptr;
BEGIN("ipc_freeze")

#ifdef lint
dest_port_ptr;
#endif lint

DEBUG1(debug.ipc_out,0,2020,(long)dest_port_ptr);

RET;

END


/*
 * ipc_port_dead --
 *	informs the IPC module that a local or a network port is dead.
 *
 * Parameters:
 *	dest_port_ptr: pointer to the port record for the dead port.
 *
 * Results:
 *	none
 *
 * Side effects:
 *	May cancel a transmission currently in progress at the transport level.
 *	May call ipc_out_cleanup if the destination port has been destroyed.
 *	Frees up the list of senders waiting if the port was blocked.
 *
 * Note:
 *	assumes that the port record is already locked.
 *	Only get rid of a pending message if it is not active.
 *
 */
EXPORT void ipc_port_dead(IN dest_port_ptr)
	port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_port_dead")
	ipc_iorec_ptr_t	ior_ptr;
	ipc_block_ptr_t	block_ptr, next;

DEBUG1(debug.ipc_out,3,2021,dest_port_ptr->portrec_local_port);

/* dest_port_ptr LOCK RW/RW */
if ((!(dest_port_ptr->portrec_info & PORT_INFO_ACTIVE))
	&& ((ior_ptr = (ipc_iorec_ptr_t)dest_port_ptr->portrec_current_ipcrec) != 0))
{
	/*
	 * Get rid of any pending messages. 
	 */
	if (ior_ptr->out.npd_exists)
		po_port_rights_commit((int)ior_ptr, PO_RIGHTS_XFER_FAILURE, ior_ptr->out.dest);
	ipc_out_gc(ior_ptr);
	if ((ior_ptr = (ipc_iorec_ptr_t) dest_port_ptr->portrec_backlog_ipcrec) != 0) {
		if (ior_ptr->out.npd_exists)
			po_port_rights_commit((int)ior_ptr, PO_RIGHTS_XFER_FAILURE, ior_ptr->out.dest);
		ipc_out_gc(ior_ptr);
	}
}
block_ptr = (ipc_block_ptr_t)dest_port_ptr->portrec_block_queue;
while (block_ptr != IPC_BLOCK_NULL) {
	next = block_ptr->next;
	mem_deallocobj((pointer_t)block_ptr, MEM_IPCBLOCK);
	block_ptr = next;
}
/* dest_port_ptr LOCK RW/RW */

RET;

END


/*
 * ipc_retry --
 *	called to retry the sending of a message.
 *
 * Parameters:
 *	dest_port_ptr: pointer to the port record for the destination
 *			for which messages are to be retried. 
 *
 * Results:
 *	meaningless
 *
 * Side effects:
 *	signal the IPC Re-Send thread to retransmit the current message. 
 *
 * Note:
 *	assumes the port record to be locked. 
 *
 */
EXPORT ipc_retry(IN dest_port_ptr)
	port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_retry")
	ipc_iorec_ptr_t	ior_ptr;

DEBUG1(debug.ipc_out,0,2022,(long)dest_port_ptr);

/* dest_port_ptr LOCK RW/RW */
if ((ior_ptr = (ipc_iorec_ptr_t) dest_port_ptr->portrec_current_ipcrec) != 0) {
	/*
	 * Signal the IPC Re-Send thread to try transmitting again. 
	 */
	mutex_lock(&ipc_re_send_lock);
	ior_ptr->out.re_send_q = ipc_re_send_q;
	ipc_re_send_q = (pointer_t) ior_ptr;
	mutex_unlock(&ipc_re_send_lock);
	condition_signal(&ipc_re_send_signal);
}
/* dest_port_ptr LOCK RW/RW */

RETURN(0);

END



/*
 * ipc_re_send -- 
 *
 * Parameters: 
 *
 * none 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Waits on ipc_re_send_signal, and attempts to transmit all the messages queued
 * on ipc_re_send_q 
 *
 * Note: 
 *
 */
PRIVATE ipc_re_send()
BEGIN("ipc_re_send")
	ipc_iorec_ptr_t ior_ptr;
	port_rec_ptr_t	dest_port_ptr;
	key_t		key;
    	port_rec_ptr_t	reply_port_ptr;
	boolean_t	unlock_reply_port;

#if	LOCK_THREADS
mutex_lock(thread_lock);
#endif	LOCK_THREADS

for (;;)
{
/*
 * Wait for something to do. 
 */
mutex_lock(&ipc_re_send_lock);
#if	LOCK_THREADS
mutex_unlock(thread_lock);
#endif	LOCK_THREADS
while (ipc_re_send_q == 0)
	condition_wait(&ipc_re_send_signal, &ipc_re_send_lock);
#if	LOCK_THREADS
mutex_lock(thread_lock);
#endif	LOCK_THREADS
ior_ptr = (ipc_iorec_ptr_t) ipc_re_send_q;
ipc_re_send_q = ior_ptr->out.re_send_q;
mutex_unlock(&ipc_re_send_lock);

/*
 * Try to transmit the message referenced by ior_ptr. It is OK to just ignore
 * the message here, because we would then be called again when needed (from ipc_retry).
 */
dest_port_ptr = ior_ptr->out.dest_port_ptr;
DEBUG2(debug.ipc_out,0,2023,(long)ior_ptr,(long)dest_port_ptr);
lk_lock(&dest_port_ptr->portrec_lock, PERM_READWRITE, TRUE);
/* dest_port_ptr LOCK RW/RW */
if (!(dest_port_ptr->portrec_info & PORT_INFO_SUSPENDED)) {
	ior_ptr->out.dest = dest_port_ptr->portrec_network_port.np_receiver;
	dest_port_ptr->portrec_info |= PORT_INFO_ACTIVE;
	lk_unlock(&dest_port_ptr->portrec_lock);
	/* dest_port_ptr LOCK -/- */

	/*
	 * If the message is to be encrypted, check that we have a key for the destination.
	 */
#if	USE_CRYPT
	if ((ior_ptr->out.crypt_level != CRYPT_DONT_ENCRYPT) && (!(km_get_key(ior_ptr->out.dest, &key)))) {
		DEBUG0(debug.ipc_out,0,2024);
		(void)ipc_out_cleanup((int)ior_ptr, TR_CRYPT_FAILURE);
	}
#endif	USE_CRYPT
	else {
		DEBUG0(debug.ipc_out,0,2025);
		/*
		 * For RPC, we must lock the local (reply) port
		 * to avoid races if the response/next request comes
		 * very fast.
		 */
		if (ior_ptr->type != IPC_IOREC_TYPE_SINGLE) {
		    	reply_port_ptr = ior_ptr->in.dest_port_ptr;
			lk_lock(&reply_port_ptr->portrec_lock, PERM_READWRITE, TRUE);
			/* reply_port_ptr LOCK RW/RW */
			unlock_reply_port = TRUE;
		} else {
		    	unlock_reply_port = FALSE;
		}
		ipc_xmit(ior_ptr);
#if	USE_CRYPT
	}
#endif	USE_CRYPT
}
else {
	DEBUG1(debug.ipc_out,0,2026,(long)dest_port_ptr->portrec_info);
	lk_unlock(&dest_port_ptr->portrec_lock);
	/* dest_port_ptr LOCK -/- */
}

/*
 * Connect the input record to its port record if appropriate.
 */
if (ior_ptr->type == IPC_IOREC_TYPE_CLIENT) {
	if ((reply_port_ptr->portrec_current_ipcrec) &&
	    (reply_port_ptr->portrec_current_ipcrec != (pointer_t)ior_ptr)) {
		ERROR((msg, "ipc_re_send: reply port already has a (different) pending record"));
		lk_unlock(&reply_port_ptr->portrec_lock);
		/* reply_port_ptr LOCK -/- */
		/*
		 * Should do some more cleanup, because we may miss
		 * the ``call back''.
		 *
		 * We should really have found this earlier, before
		 * transmitting. XXX
		 */
		continue;
	}
	reply_port_ptr->portrec_current_ipcrec = (pointer_t) ior_ptr;
}
if (unlock_reply_port) {
	lk_unlock(&reply_port_ptr->portrec_lock);
	/* reply_port_ptr LOCK -/- */
}
}

END



/*
 * ipc_out_init -- 
 *
 * Parameters: 
 *
 * none 
 *
 * Results: 
 *
 * none 
 *
 * Side effects: 
 *
 * Initializes various global variables used by the ipc_out routines.
 * Starts up threads to execute ipc_out_main and ipc_re_send.
 *
 * Note:
 *	ipc_seqence_number is set to be twenty times the number of seconds
 *	since 1 Jan 1987 (approximately).  The assumes that messages are
 *	not sent at a greater rate than an average twenty per second.
 *
 */
PUBLIC void ipc_out_init()
BEGIN("ipc_out_init")
cthread_t	new_thread;
struct timeval	tp;

mutex_init(&ipc_re_send_lock);
condition_init(&ipc_re_send_signal);
ipc_re_send_q = 0;

(void)gettimeofday(&tp, (struct timezone *)0);
ipc_sequence_number = 20 * (tp.tv_sec - (17 * 365 * 24 * 60 * 60));
DEBUG1(debug.ipc_out,3,2027,ipc_sequence_number);

new_thread = cthread_fork((cthread_fn_t)ipc_out_main, (any_t)0);
cthread_set_name(new_thread, "ipc_out_main");
cthread_detach(new_thread);

new_thread = cthread_fork((cthread_fn_t)ipc_re_send, (any_t)0);
cthread_set_name(new_thread, "ipc_re_send");
cthread_detach(new_thread);

RET;

END


/*
 * ipc_out_backlog --
 *
 * Parameters:
 *
 * rcvbuff: pointer to a buffer containing a newly received IPC message.
 *
 * dest_port_ptr: port record for the destination port for the message.
 *
 * Results:
 *
 * none
 *
 * Side effects:
 *
 * Creates an ipc_iorec for the message, does everything to prepare for
 * transmission, and puts the record in the backlog queue for the destination
 * port.
 *
 * Design:
 *
 * Note:
 *
 * Part of this code is repeated in ipc_out_main.
 * If you change one, change the other.
 *
 */
PRIVATE
void ipc_out_backlog (IN rcvbuff, IN dest_port_ptr)
msg_header_t	*rcvbuff;
port_rec_ptr_t	dest_port_ptr;
BEGIN("ipc_out_backlog")
	ipc_iorec_ptr_t	ior_ptr;
	kern_return_t	kern_ret;

DEBUG1(debug.ipc_out,0,2028,(long)dest_port_ptr);

/* dest_port_ptr LOCK RW/RW */
if (dest_port_ptr->portrec_backlog_ipcrec) {
	ERROR((msg, "ipc_out_backlog: more than two messages pending on a port"));
	return;
}

/*
 * Lock the port to prevent the reception of further messages.
 */
kern_ret = port_disable(task_self(), dest_port_ptr->portrec_local_port);
if (kern_ret == KERN_SUCCESS) {
	dest_port_ptr->portrec_info |= PORT_INFO_DISABLED;
} else {
	ERROR((msg, "ipc_out_backlog: port_disable returned %d", kern_ret));
}

lk_unlock(&dest_port_ptr->portrec_lock);
/* dest_port_ptr LOCK -/- */

ior_ptr = (ipc_iorec_ptr_t) mem_allocobj(MEM_IPCIOREC);
if (ior_ptr == 0) {
	panic("ipc_out_backlog cannot get a new ipc_iorec");
}
if (rcvbuff->msg_type & MSG_TYPE_RPC) {
	ior_ptr->type = IPC_IOREC_TYPE_CLIENT;
	ior_ptr->trmod = TR_VMTP_ENTRY;
} else {
	ior_ptr->type = IPC_IOREC_TYPE_SINGLE;
	ior_ptr->trmod = 0;
}
ior_ptr->trid = 0;

/*
 * Fill in the output record and translate the message.
 */
ior_ptr->out.crypt_level = (rcvbuff->msg_type & MSG_TYPE_ENCRYPTED) ? CRYPT_ENCRYPT : CRYPT_DONT_ENCRYPT;
ipc_outmsg(ior_ptr, rcvbuff, dest_port_ptr, ior_ptr->out.crypt_level);
if (ior_ptr->type == IPC_IOREC_TYPE_CLIENT) {
    	ior_ptr->out.netmsg_hdr.info |= IPC_INFO_RPC;
}

/*
 * Prepare an input record if appropriate.
 */
if (ior_ptr->type == IPC_IOREC_TYPE_CLIENT) {
	port_rec_ptr_t		reply_port_ptr;

	reply_port_ptr = pr_lportlookup(rcvbuff->msg_remote_port);
	/* reply_port_ptr LOCK RW/RW */
	if (reply_port_ptr == 0) {
		ERROR((msg, "ipc_out_backlog cannot find a reply port for an RPC request"));
		ior_ptr->in.dest_port_ptr = 0;
		/*
		 * Should probably do something more drastic here,
		 * because the ``call back'' may never come.
		 */
		RET;
	}
	ior_ptr->in.dest_port_ptr = reply_port_ptr;
	lk_unlock(&reply_port_ptr->portrec_lock);
	/* reply_port_ptr LOCK -/- */
	/*
	 * Possible optimization here: pre-allocate an assembly area.
	 */
}

/*
 * Translate the destination port and place the ior_rec into the backlog slot.
 */
lk_lock(&dest_port_ptr->portrec_lock, PERM_READ, TRUE);
/* dest_port_ptr LOCK RW/RW */
ior_ptr->out.netmsg_hdr.local_port = dest_port_ptr->portrec_network_port;
if (dest_port_ptr->portrec_current_ipcrec == 0) {
	dest_port_ptr->portrec_current_ipcrec = (pointer_t)ior_ptr;
	(void)ipc_retry(dest_port_ptr);
	if (!(dest_port_ptr->portrec_info & (PORT_INFO_SUSPENDED | PORT_INFO_BLOCKED))) {
		if (kern_ret = port_enable(task_self(), dest_port_ptr->portrec_local_port)
			== KERN_SUCCESS)
		{
			dest_port_ptr->portrec_info &= ~PORT_INFO_DISABLED;
		}
		else {
			ERROR((msg, "ipc_out_backlog: port_enable returned %d", kern_ret));
		}
	}
}
else dest_port_ptr->portrec_backlog_ipcrec = (pointer_t) ior_ptr;
lk_unlock(&dest_port_ptr->portrec_lock);
/* dest_port_ptr LOCK -/- */

RET;
END

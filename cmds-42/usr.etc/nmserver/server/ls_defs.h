/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ls_defs.h
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ls_defs.h,v $
 *
 * $Header: ls_defs.h,v 1.1 88/09/30 15:44:10 osdev Exp $
 *
 */

/*
 * Definitions for the logstat module.
 */

/*
 * HISTORY:
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added param.old_nmmonitor.
 *
 * 24-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added debug.mem.
 *
 * 15-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added param.syslog.
 *
 * 27-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added param.compat.
 *
 * 16-Feb-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Removed tcp_copy_thresh.
 *
 * 09-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added tcp_copy_thresh and TCP stats.
 *
 * 06-Dec-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added definitions for the TCP module.
 *
 *  2-Oct-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added auto-configuration parameters and deltat debug.
 *
 * 18-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added definitions for port statistics.
 *	Added timer_quantum parameter.
 *
 * 14-Jul-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added encryption statistics.
 *
 * 21-Jun-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added VMTP statistics.
 *
 * 15-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added netname to debug record.
 *
 *  8-Jun-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added vmtp to debug record.
 *
 * 19-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added statistics values.
 *	Added crypt_algorithm parameter.
 *
 *  5-May-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added DeltaT parameters.
 *
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added SRR parameters.  Added tracing to debug record.
 *
 * 21-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */

#ifndef	_LS_DEFS_
#define	_LS_DEFS_

#include <sys/types.h>

/*
 * Definition for a log record.
 */
typedef	struct	{
	long	code;
	long	thread;
	long	a1;
	long	a2;
	long	a3;
	long	a4;
	long	a5;
	long	a6;
} log_rec_t;

typedef	log_rec_t	*log_ptr_t;

/*
 * Statistics record.
 */
typedef	struct {
	int	datagram_pkts_sent;
	int	datagram_pkts_rcvd;
	int	srr_requests_sent;
	int	srr_bcasts_sent;
	int	srr_requests_rcvd;
	int	srr_bcasts_rcvd;
	int	srr_replies_sent;
	int	srr_replies_rcvd;
	int	srr_retries_sent;
	int	srr_retries_rcvd;
	int	srr_cfailures_sent;
	int	srr_cfailures_rcvd;
	int	deltat_dpkts_sent;
	int	deltat_acks_rcvd;
	int	deltat_dpkts_rcvd;
	int	deltat_acks_sent;
	int	deltat_oldpkts_rcvd;
	int	deltat_oospkts_rcvd;
	int	deltat_retries_sent;
	int	deltat_retries_rcvd;
	int	deltat_cfailures_sent;
	int	deltat_cfailures_rcvd;
	int	deltat_aborts_sent;
	int	deltat_aborts_rcvd;
	int	vmtp_requests_sent;
	int	vmtp_requests_rcvd;
	int	vmtp_replies_sent;
	int	vmtp_replies_rcvd;
	int	ipc_in_messages;
	int	ipc_out_messages;
	int	ipc_unblocks_sent;
	int	ipc_unblocks_rcvd;
	int	pc_requests_sent;
	int	pc_requests_rcvd;
	int	pc_replies_rcvd;
	int	pc_startups_rcvd;
	int	nn_requests_sent;
	int	nn_requests_rcvd;
	int	nn_replies_rcvd;
	int	po_ro_hints_sent;
	int	po_ro_hints_rcvd;
	int	po_token_requests_sent;
	int	po_token_requests_rcvd;
	int	po_token_replies_rcvd;
	int	po_xfer_requests_sent;
	int	po_xfer_requests_rcvd;
	int	po_xfer_replies_rcvd;
	int	po_deaths_sent;
	int	po_deaths_rcvd;
	int	ps_requests_sent;
	int	ps_requests_rcvd;
	int	ps_replies_rcvd;
	int	ps_auth_requests_sent;
	int	ps_auth_requests_rcvd;
	int	ps_auth_replies_rcvd;
	int	mallocs_or_vm_allocates;
	int	mem_allocs;
	int	mem_deallocs;
	int	mem_allocobjs;
	int	mem_deallocobjs;
	int	pkts_encrypted;
	int	pkts_decrypted;
	int	vmtp_segs_encrypted;
	int	vmtp_segs_decrypted;
	int	tcp_requests_sent;
	int	tcp_replies_sent;
	int	tcp_requests_rcvd;
	int	tcp_replies_rcvd;
	int	tcp_send;
	int	tcp_recv;
	int	tcp_connect;
	int	tcp_accept;
	int	tcp_close;
} stat_t;

typedef	stat_t	*stat_ptr_t;


/*
 * Debugging flags record.
 */
typedef	struct {
	int	print_level;
	int	ipc_in;
	int	ipc_out;
	int	tracing;
	int	vmtp;
	int	netname;
	int	deltat;
	int	tcp;
	int	mem;
} debug_t;

typedef	debug_t	*debug_ptr_t;


/*
 * Parameters record.
 */
typedef struct {
    	int	srr_max_tries;
	int	srr_retry_sec;
	int	srr_retry_usec;
    	int	deltat_max_tries;
	int	deltat_retry_sec;
	int	deltat_retry_usec;
	int	deltat_msg_life;
	int	pc_checkup_interval;
	int	crypt_algorithm;
	int	transport_default;
	int	conf_network;
	int	conf_netport;
	int	timer_quantum;
	int	tcp_conn_steady;
	int	tcp_conn_opening;
	int	tcp_conn_max;
	int	compat;
	int	syslog;
	int	old_nmmonitor;
} param_t;

typedef param_t *param_ptr_t;


/*
 * Port statistics record.
 */
typedef struct {
	u_int	port_id;
	u_int	alive;
	u_int	nport_id_high;
	u_int	nport_id_low;
	u_int	nport_receiver;
	u_int	nport_owner;
	u_int	messages_sent;
	u_int	messages_rcvd;
	u_int	send_rights_sent;
	u_int	send_rights_rcvd_sender;
	u_int	send_rights_rcvd_recown;
	u_int	rcv_rights_xferd;
	u_int	own_rights_xferd;
	u_int	all_rights_xferd;
	u_int	tokens_sent;
	u_int	tokens_requested;
	u_int	xfer_hints_sent;
	u_int	xfer_hints_rcvd;
} port_stat_t, *port_stat_ptr_t;

extern port_stat_ptr_t	port_stat_cur;
extern port_stat_ptr_t	port_stat_end;
extern struct mutex	port_stat_lock;


/*
 * Types for the mem_list operation.
 *
 * XXX These must be faked, because we cannot include mem.h here
 * (mutual includes).
 */
typedef char			*mem_class_ptr_t;
typedef char			*mem_nam_ptr_t;
typedef int			*mem_bucket_ptr_t;

	
/*
 * Definitions for print_level.
 */
#define	LS_PRINT_NEVER		5
#define	LS_PRINT_LOG		3
#define	LS_PRINT_ALWAYS		0

#endif	_LS_DEFS_

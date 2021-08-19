/* 
 * Mach Operating System
 * Copyright (c) 1987 Carnegie-Mellon University
 * All rights reserved.  The CMU software License Agreement specifies
 * the terms and conditions for use and redistribution.
 */
/*
 * ls_rec.c
 *
 * $Source: /os/osdev/LIBS/libs-7/etc/nmserver/server/RCS/ls_rec.c,v $
 *
 */
#ifndef	lint
char ls_rec_rcsid[] = "$Header: ls_rec.c,v 1.1 88/09/30 15:40:16 osdev Exp $";
#endif not lint
/*
 * Initial values for some logstat records, and names for the fields.
 */

/*
 * HISTORY: 
 *  3-Sep-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Turned on conf_netport only for NeXT.
 *
 * 23-Jun-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added param.old_nmmonitor.
 *
 * 24-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Added debug.mem.
 *
 * 15-Mar-88  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Modified default values for production version.
 *	Added compat_server. Added param.syslog.
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
 * 19-Aug-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added port statistics names.
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
 *	Added DeltaT parameters and port checkup interval.
 *
 * 23-Apr-87  Robert Sansom (rds) at Carnegie Mellon University
 *	Added SRR parameters.  Added tracing to debug record.
 *
 * 23-Mar-87  Daniel Julin (dpj) at Carnegie-Mellon University
 *	Created.
 *
 */


#include	"netmsg.h"
#include	"nm_defs.h"
#include	"ls_defs.h"


PUBLIC char	*debug_names[] = {
    	"print_level",
	"ipc_in",
	"ipc_out",
	"tracing",
	"vmtp",
	"netname",
	"deltat",
	"tcp",
	"mem",
	0
};

EXPORT debug_t	debug = {
    	LS_PRINT_NEVER,	/* print_level */
	0xffff,		/* ipc_in */
	0xffff,		/* ipc_out */
	0,		/* tracing */
	0,		/* vmtp */
	0xffff,		/* netname */
	0,		/* deltat */
	0x3,		/* tcp */
	0,		/* mem */
};


PUBLIC char	*param_names[] = {
	"srr_max_tries",
	"srr_retry_sec",
	"srr_retry_usec",
	"deltat_max_tries",
	"deltat_retry_sec",
	"deltat_retry_usec",
	"deltat_msg_life",
	"pc_checkup_interval",
	"crypt_algorithm",
	"transport_default",
	"conf_network",
	"conf_netport",
	"timer_quantum",
	"tcp_conn_steady",
	"tcp_conn_opening",
	"tcp_conn_max",
	"compat",
	"syslog",
	"old_nmmonitor",
	0
};

EXPORT param_t	param = {
	3,		/* srr_max_tries */
	3,		/* srr_retry_sec */
	0,		/* srr_retry_usec */
  	60,		/* deltat_max_tries */
	3,		/* deltat_retry_sec */
	0,		/* deltat_retry_usec */
	60,		/* deltat_msg_life */
	60,		/* pc_checkup_interval */
	0,		/* crypt_algorithm */
	0,		/* transport_default */
	1,		/* conf_network */
#if	NeXT
	1,		/* conf_netport */
#else	NeXT
	0,		/* conf_netport */
#endif	NeXT
	500,		/* timer_quantum */
	16,		/* tcp_conn_steady */
	18,		/* tcp_conn_opening */
	20,		/* tcp_conn_max */
	0,		/* compat */
	1,		/* syslog */
	1,		/* old_nmmonitor */
};


/*
 * Record to hold the statistics.
 *
 * Note: there is no lock for the statistics record. Care must be
 * taken to ensure that each element is only written in one thread.
 */
EXPORT	stat_t	nmstat;


PUBLIC char	*stat_names[] = {
	"datagram_pkts_sent",
	"datagram_pkts_rcvd",
	"srr_requests_sent",
	"srr_bcasts_sent",
	"srr_requests_rcvd",
	"srr_bcasts_rcvd",
	"srr_replies_sent",
	"srr_replies_rcvd",
	"srr_retries_sent",
	"srr_retries_rcvd",
	"srr_cfailures_sent",
	"srr_cfailures_rcvd",
	"deltat_dpkts_sent",
	"deltat_acks_rcvd",
	"deltat_dpkts_rcvd",
	"deltat_acks_sent",
	"deltat_oldpkts_rcvd",
	"deltat_oospkts_rcvd",
	"deltat_retries_sent",
	"deltat_retries_rcvd",
	"deltat_cfailures_sent",
	"deltat_cfailures_rcvd",
	"deltat_aborts_sent",
	"deltat_aborts_rcvd",
	"vmtp_requests_sent",
	"vmtp_requests_rcvd",
	"vmtp_replies_sent",
	"vmtp_replies_rcvd",
	"ipc_in_messages",
	"ipc_out_messages",
	"ipc_blocks_sent",
	"ipc_blocks_rcvd",
	"pc_requests_sent",
	"pc_requests_rcvd",
	"pc_replies_rcvd",
	"pc_startups_rcvd",
	"nn_requests_sent",
	"nn_requests_rcvd",
	"nn_replies_rcvd",
	"po_ro_hints_sent",
	"po_ro_hints_rcvd",
	"po_token_requests_sent",
	"po_token_requests_rcvd",
	"po_token_replies_rcvd",
	"po_xfer_requests_sent",
	"po_xfer_requests_rcvd",
	"po_xfer_replies_rcvd",
	"po_deaths_sent",
	"po_deaths_rcvd",
	"ps_requests_sent",
	"ps_requests_rcvd",
	"ps_replies_rcvd",
	"ps_auth_requests_sent",
	"ps_auth_requests_rcvd",
	"ps_auth_replies_rcvd",
	"mallocs_or_vm_allocates",
	"mem_allocs",
	"mem_deallocs",
	"mem_allocobjs",
	"mem_deallocobjs",
	"pkts_encrypted",
	"pkts_decrypted",
	"vmtp_segs_encrypted",
	"vmtp_segs_decrypted",
	"tcp_requests_sent",
	"tcp_replies_sent",
	"tcp_requests_rcvd",
	"tcp_replies_rcvd",
	"tcp_send",
	"tcp_recv",
	"tcp_connect",
	"tcp_accept",
	"tcp_close",
	0
};

PUBLIC char	*port_stat_names[] = {
	"port_id",
	"alive",
	"nport_id_high",
	"nport_id_low",
	"nport_receiver",
	"nport_owner",
	"messages_sent",
	"messages_rcvd",
	"send_rights_sent",
	"send_rights_rcvd_sender",
	"send_rights_rcvd_recown",
	"rcv_rights_xferd",
	"own_rights_xferd",
	"all_rights_xferd",
	"tokens_sent",
	"tokens_requested",
	"xfer_hints_sent",
	"xfer_hints_rcvd",
	0
};


/*
 * Path name for the old-type netmsgserver to use in compatibility mode.
 */
EXPORT char	compat_server[100] = "/usr/mach/etc/old_netmsgserver";


/* 
 * NetInfo binder protocol specification
 * Copyright (C) 1989 by NeXT, Inc.
 */

/* Preamble appearing on all generated output */
#ifndef NOPREAMBLE
%/*
% * Output of the RPC protocol compiler: DO NOT EDIT
% * Copyright (C) 1989 by NeXT, Inc.
% */
#endif

#ifdef RPC_HDR
%#ifndef NI_PROG
%#include <netinfo/ni_prot.h>
%#endif
#else
%#include <stdio.h>
%#include "clib.h"
#endif

const NIBIND_MAXREGS = 32;

struct nibind_addrinfo {
	unsigned udp_port;
	unsigned tcp_port;
};

struct nibind_registration {
	ni_name tag;
	nibind_addrinfo addrs;
};

union nibind_getregister_res switch (ni_status status) {
case NI_OK:
	nibind_addrinfo addrs;
default:
	void;
};

union nibind_listreg_res switch (ni_status status) {
case NI_OK:
	nibind_registration regs<NIBIND_MAXREGS>;
default:
	void;
};

struct nibind_clone_args {
	ni_name tag;
	ni_name master_name;
	unsigned master_addr;
	ni_name master_tag;
};
	
struct nibind_bind_args {
	unsigned client_addr;
	ni_name client_tag;
	ni_name server_tag;
};

program NIBIND_PROG {
	version NIBIND_VERS {
		void
		NIBIND_PING(void) = 0;

		ni_status
		NIBIND_REGISTER(nibind_registration) = 1;

		ni_status
		NIBIND_UNREGISTER(ni_name) = 2;

		nibind_getregister_res
		NIBIND_GETREGISTER(ni_name) = 3;

		nibind_listreg_res
		NIBIND_LISTREG(void) = 4;

		ni_status
		NIBIND_CREATEMASTER(ni_name) = 5;

		ni_status
		NIBIND_CREATECLONE(nibind_clone_args) = 6;

		ni_status
		NIBIND_DESTROYDOMAIN(ni_name) = 7;

		/*
		 * Answers only if the given binding is served
		 */
		void
		NIBIND_BIND(nibind_bind_args) = 8;
	} = 1;
} = 200100001;

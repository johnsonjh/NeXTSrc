/*	@(#)klm_prot.x	1.3 88/05/26 4.0NFSSRC SMI	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.9
 */

/*
 * Kernel/lock manager protocol definition
 * Copyright (C) 1986 Sun Microsystems, Inc.
 *
 * protocol used between the UNIX kernel (the "client") and the
 * local lock manager.  The local lock manager is a deamon running
 * above the kernel.
 */
const	LM_MAXSTRLEN = 1024;

/*
 * lock manager status returns
 */
enum klm_stats {
	klm_granted = 0,	/* lock is granted */
	klm_denied = 1,		/* lock is denied */
	klm_denied_nolocks = 2, /* no lock entry available */
	klm_working = 3 	/* lock is being processed */
};

/*
 * lock manager lock identifier
 */
struct klm_lock {
	string server_name<LM_MAXSTRLEN>;
	netobj fh;		/* a counted file handle */
	int pid;		/* holder of the lock */
	unsigned l_offset;	/* beginning offset of the lock */
	unsigned l_len;		/* byte length of the lock;
				 * zero means through end of file */
};

/*
 * lock holder identifier
 */
struct klm_holder {
	bool exclusive;		/* FALSE if shared lock */
	int svid;		/* holder of the lock (pid) */
	unsigned l_offset;	/* beginning offset of the lock */
	unsigned l_len;		/* byte length of the lock;
				 * zero means through end of file */
};

/*
 * reply to KLM_LOCK / KLM_UNLOCK / KLM_CANCEL
 */
struct klm_stat {
	klm_stats stat;
};

/*
 * reply to a KLM_TEST call
 */
union klm_testrply switch (klm_stats stat) {
	case klm_denied:
		struct klm_holder holder;
	default: /* All other cases return no arguments */
		void;
};


/*
 * arguments to KLM_LOCK
 */
struct klm_lockargs {
	bool block;
	bool exclusive;
	struct klm_lock alock;
};

/*
 * arguments to KLM_TEST
 */
struct klm_testargs {
	bool exclusive;
	struct klm_lock alock;
};

/*
 * arguments to KLM_UNLOCK
 */
struct klm_unlockargs {
	struct klm_lock alock;
};

/*	@(#)rstat.h	1.2 88/05/08 4.0NFSSRC SMI	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.9
 */

#ifndef CPUSTATES
#include <sys/dk.h>
#endif

#ifndef DST_NONE
#include <sys/time.h>
#endif

#define RSTATPROG 100001
#define RSTATVERS_ORIG 1
#define RSTATVERS_SWTCH 2
#define RSTATVERS_TIME  3
#define RSTATVERS 3
#define RSTATPROC_STATS 1
#define RSTATPROC_HAVEDISK 2

struct stats {				/* version 1 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
};

struct statsswtch {				/* version 2 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
};
struct statstime {				/* version 3 */
	int cp_time[CPUSTATES];
	int dk_xfer[DK_NDRIVE];
	unsigned v_pgpgin;	/* these are cumulative sum */
	unsigned v_pgpgout;
	unsigned v_pswpin;
	unsigned v_pswpout;
	unsigned v_intr;
	int if_ipackets;
	int if_ierrors;
	int if_opackets;
	int if_oerrors;
	int if_collisions;
	unsigned v_swtch;
	long avenrun[3];
	struct timeval boottime;
	struct timeval curtime;
};

int xdr_stats();
int xdr_statsswtch();
int xdr_statstime();
int havedisk();

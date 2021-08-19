/*
 * Copyright (c) 1983,1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of California at Berkeley. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef lint
static char sccsid[] = "@(#)mbuf.c	5.3 (Berkeley) 2/3/87";
#endif not lint

#include <stdio.h>
#include <sys/param.h>
#include <sys/mbuf.h>
#define	YES	1
typedef int bool;

struct	mbstat mbstat;
extern	int kmem;

static struct mbtypes {
	int	mt_type;
	char	*mt_name;
} mbtypes[] = {
	{ MT_DATA,	"data" },
	{ MT_HEADER,	"packet headers" },
	{ MT_SOCKET,	"socket structures" },
	{ MT_PCB,	"protocol control blocks" },
	{ MT_RTABLE,	"routing table entries" },
	{ MT_HTABLE,	"IMP host table entries" },
	{ MT_ATABLE,	"address resolution tables" },
	{ MT_FTABLE,	"fragment reassembly queue headers" },
	{ MT_SONAME,	"socket names and addresses" },
	{ MT_ZOMBIE,	"zombie process information" },
	{ MT_SOOPTS,	"socket options" },
	{ MT_RIGHTS,	"access rights" },
	{ MT_IFADDR,	"interface addresses" }, 
	{ 0, 0 }
};

int nmbtypes = sizeof(mbstat.m_mtypes) / sizeof(short);
bool seen[256];			/* "have we seen this type yet?" */

/*
 * Print mbuf statistics.
 */
#if	NeXT
mbpr(mbaddr, pagesizeaddr)
	off_t mbaddr;
	off_t pagesizeaddr;
#else	NeXT
mbpr(mbaddr)
	off_t mbaddr;
#endif	NeXT
{
	register int totmem, totfree, totmbufs;
	register int i;
	register struct mbtypes *mp;
#if	NeXT
	int	pagesize;
#endif	NeXT

	if (nmbtypes != 256) {
		fprintf(stderr, "unexpected change to mbstat; check source\n");
		return;
	}
	if (mbaddr == 0) {
		printf("mbstat: symbol not in namelist\n");
		return;
	}
	klseek(kmem, mbaddr, 0);
	if (read(kmem, (char *)&mbstat, sizeof (mbstat)) != sizeof (mbstat)) {
		printf("mbstat: bad read\n");
		return;
	}
#if	NeXT
	if (pagesizeaddr == 0) {
		printf("NeXT_page_size: symbol not in namelist\n");
		return;
	}
	klseek(kmem, pagesizeaddr, 0);
	if (read(kmem, (char *)&pagesize,
		 sizeof(pagesize)) != sizeof(pagesize)) {
		printf("NeXT_page_size: bad read\n");
		return;
	}
#endif	NeXT
	printf("%u/%u mbufs in use:\n",
		mbstat.m_mbufs - mbstat.m_mtypes[MT_FREE], mbstat.m_mbufs);
	totmbufs = 0;
	for (mp = mbtypes; mp->mt_name; mp++)
		if (mbstat.m_mtypes[mp->mt_type]) {
			seen[mp->mt_type] = YES;
			printf("\t%u mbufs allocated to %s\n",
			    mbstat.m_mtypes[mp->mt_type], mp->mt_name);
			totmbufs += mbstat.m_mtypes[mp->mt_type];
		}
	seen[MT_FREE] = YES;
	for (i = 0; i < nmbtypes; i++)
		if (!seen[i] && mbstat.m_mtypes[i]) {
			printf("\t%u mbufs allocated to <mbuf type %d>\n",
			    mbstat.m_mtypes[i], i);
			totmbufs += mbstat.m_mtypes[i];
		}
	if (totmbufs != mbstat.m_mbufs - mbstat.m_mtypes[MT_FREE])
		printf("*** %u mbufs missing ***\n",
			(mbstat.m_mbufs - mbstat.m_mtypes[MT_FREE]) - totmbufs);
	printf("%u/%u mbuf clusters in use\n",
		mbstat.m_clusters - mbstat.m_clfree, mbstat.m_clusters);
	printf("%u interface pages allocated\n", mbstat.m_space);
#if	NeXT	
	totmem = mbstat.m_mbufs * MSIZE + mbstat.m_clusters * MCLBYTES +
	    mbstat.m_space * pagesize;
#else	NeXT
	totmem = mbstat.m_mbufs * MSIZE + mbstat.m_clusters * MCLBYTES +
	    mbstat.m_space * CLBYTES;
#endif	NeXT
	totfree = mbstat.m_mtypes[MT_FREE]*MSIZE + mbstat.m_clfree * MCLBYTES;
	printf("%u Kbytes allocated to network (%d%% in use)\n",
		totmem / 1024, (totmem - totfree) * 100 / totmem);
	printf("%u requests for memory denied\n", mbstat.m_drops);
	printf("%u requests for memory delayed\n", mbstat.m_wait);
	printf("%u calls to protocol drain routines\n", mbstat.m_drain);
}

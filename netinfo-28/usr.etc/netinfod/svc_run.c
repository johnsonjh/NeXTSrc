/* 
 * svc_run() replacement for NetInfo
 * Copyright (C) 1989 by NeXT, Inc.
 */

/*
 * We provide our own svc_run() here instead of using the standard one
 * provided by the RPC library so that we can do a few useful things:
 *
 * 1. Close off connections on an LRU (least-recently-used) basis.
 * 2. Periodically send out resynchronization notices (master only).
 *
 * TODO: Clean up memory periodically too to avoid fragmentation problems.
 */

#include "ni_server.h"
#include "ni_globals.h"
#include "notify.h"
#include <sys/errno.h>
#include <stdio.h>
#include "clib.h"
#include "event.h"
#include "system.h"
#include "socket_lock.h"
#include "ni_dir.h"
#include "getstuff.h"

#define CLEANUPWAIT (30*60)	/* # of seconds to wait before cleaning up */

/*
 * Data structure used to keep track of file descriptors
 */
typedef struct lrustuff {
	unsigned len;		/* number of descriptors stored here */
	int *val;		/* pointer to array of descriptors */
} lrustuff;


/*
 * perform the bitwise function c = a & b.
 * "max" is the max number of descriptors we expect.
 */
static void
fd_and(
       unsigned max,
       fd_set *a,
       fd_set *b,
       fd_set *c
       )
{
	int i;

	for (i = 0; i < max/NFDBITS; i++) {
		c->fds_bits[i] = a->fds_bits[i] & b->fds_bits[i];
	}
}


#ifdef notdef
/*
 * perform the bitwise function c = a | b.
 * "max" is the max number of descriptors we expect.
 */
static void
fd_or(
       unsigned max,
       fd_set *a,
       fd_set *b,
       fd_set *c
       )
{
	int i;

	for (i = 0; i < max/NFDBITS; i++) {
		c->fds_bits[i] = a->fds_bits[i] | b->fds_bits[i];
	}
}

#endif

/*
 * perform the bitwise function c = ~(a & b).
 * "max" is the max number of descriptors we expect.
 */
static void
fd_clr(
	unsigned max,
	fd_set *a,
	fd_set *b,
	fd_set *c
	)
{
	int i;

	for (i = 0; i < max/NFDBITS; i++) {
		c->fds_bits[i] = ~(a->fds_bits[i]) & b->fds_bits[i];
	}
}

/*
 * How many bits are set in the given word?
 */
static int
bitcount(
	 unsigned u
	 )
{
	int count;

	for (count = 0; u > 0; u >>=1) {
		count += u & 1;
	}
	return (count);
}

/*
 * How many bits are set in the given fd_set?
 */
static int
fd_count(
	 unsigned max,
	 fd_set *fds
	 )
{
	int i;
	int count;

	count = 0;
	for (i = 0; i < max/NFDBITS; i++) {
		count += bitcount((unsigned)fds->fds_bits[i]);
	}
	return (count);
}

/*
 * Allocates and initializes an lru descriptor data structure
 */
static lrustuff
lru_init(
	 unsigned max
	 )
{
	lrustuff lru;

	lru.val = (int *)malloc(sizeof(int) * max);
	lru.len = 0;
	return (lru);
}

/*
 * Mark a single descriptor as being recently used. If this is a new
 * descriptor, add it to the list.
 */
static void
lru_markone(
	    lrustuff *lru,
	    unsigned which
	    )
{
	int i;
	int j;
	int mark;

	mark = lru->len;
	for (i = 0; i < lru->len; i++) {
		if (lru->val[i] == which) {
			mark = i;
			lru->len--; /* don't double count */
			break;
		}
	}
	for (j = mark; j > 0; j--) {
		lru->val[j] = lru->val[j - 1];
	}
	lru->val[0] = which;
	lru->len++;
}


/*
 * Mark each of the descriptors in the given set as being recently used.
 */
static void
lru_mark(
	 unsigned max,
	 lrustuff *lru,
	 fd_set *fds
	 )
{
	int i;
	int j;
	fd_mask mask;
	fd_mask mask2;

	for (i = 0; i < max/NFDBITS; i++) {
		mask = fds->fds_bits[i];
		for (j = 0, mask2 = 1; mask && j < NFDBITS; j++, mask2 <<= 1) {
			if (mask & mask2) {
				lru_markone(lru, NFDBITS * i + j);
				mask ^= mask2;
			}
		}
	}
}

/*
 * The given descriptor has been closed. Delete it from the list.
 */
static void
lru_unmarkone(
	      lrustuff *lru,
	      unsigned which
	      )
{
	int i;

	for (i = 0; i < lru->len; i++) {
		if (lru->val[i] == which) {
			while (i < lru->len) {
				lru->val[i] = lru->val[i + 1];
				i++;
			}
			lru->len--;
			return;
		}
	}
}

/*
 * The given descriptors have been closed. Delete them from the list.
 */
static void
lru_unmark(
	   unsigned max,
	   lrustuff *lru,
	   fd_set *fds
	   )
{
	int i;
	int j;
	fd_mask mask;
	fd_mask mask2;

	for (i = 0; i < max/NFDBITS; i++) {
		mask = fds->fds_bits[i];
		for (j = 0, mask2 = 1; mask && j < NFDBITS; j++, mask2 <<= 1) {
			if (mask & mask2) {
				lru_unmarkone(lru, NFDBITS * i + j);
				mask ^= mask2;
			}
		}
	}
}

/*
 * Close off the LRU descriptor.
 */
static void
lru_close(
	  lrustuff *lru
	  )
{
	fd_set mask;
	int fd;

	fd = lru->val[lru->len - 1];
	lru_unmarkone(lru, fd);
	FD_ZERO(&mask);
	FD_SET(fd, &mask);
	socket_lock();
	(void)close(fd);
	svc_getreqset(&mask);
	socket_unlock();
	if (FD_ISSET(fd, &svc_fdset)) {
		sys_errmsg("closed descriptor is still set");
	}
}

#if CONNECTION_CHECK

void
open_connections(
		 unsigned maxfds,
		 fd_set *fds
		 )
{
	int newfd;
	struct sockaddr_in from;
	int fromlen;
	SVCXPRT *transp;

	if (FD_ISSET(tcp_sock, fds)) {
		FD_CLR(tcp_sock, fds);
		fromlen = sizeof(from);
		socket_lock();
		newfd = accept(tcp_sock,
			       (struct sockaddr *)&from, 
			       &fromlen);
		socket_unlock();
		if (newfd >= 0) {
			if (is_trusted_network(db_ni, &from)) {
				transp = svcfd_create(newfd, 
						      NI_SENDSIZE, 
						      NI_RECVSIZE);
				if (transp != NULL) {
					transp->xp_raddr = from;
					transp->xp_addrlen = fromlen;
					if (!FD_ISSET(newfd, &svc_fdset)) {
						sys_errmsg("new descriptor is not set");
					}
				} else {
					socket_lock();
					close(newfd);
					socket_unlock();
					if (FD_ISSET(newfd, &svc_fdset)) {
						sys_errmsg("closed descriptor is still set");
					}
				}
			} else {
				/*
				 * We don't trust the network that this
				 * guy is from. Close him off.
				 */
				socket_lock();
				close(newfd);
				socket_unlock();
			}
		}
	} 

	/*
	 * Now handle UDP socket
	 */
	if (FD_ISSET(udp_sock, fds)) {
		socket_lock();
		svc_getreqset(fds);
		socket_unlock();
	}
}


#endif

/*
 * The replacement for the standard svc_run() provided by the RPC library
 * so that we can keep track of the LRU descriptors and perform periodic
 * actions.
 */
void
svc_run(
	int maxlisteners
	)
{
	fd_set readfds;
	fd_set orig;
	fd_set fds;
	fd_set save;
	fd_set shut;
	unsigned maxfds;
	extern int errno;
	int nused;
	int event_fd;
	lrustuff lru;
	long cleanuptime;

	cleanuptime = sys_time() + CLEANUPWAIT;
	orig = svc_fdset;
	maxfds = getdtablesize();
	lru = lru_init(maxfds);
	nused = 0;
	while (!shutdown) {
		readfds = svc_fdset;
		event_fd = event_pipe[0];
		if (event_fd >= 0) {
			FD_SET(event_fd, &readfds);
		}
		switch (select(maxfds, &readfds, NULL, NULL, NULL)) {
		case -1:
			if (errno != EINTR) {
				sys_errmsg("unexpected errno: %m");
				sleep(10);
			}
			break;
		case 0:
			break;
		default:
			if (event_fd >= 0 && FD_ISSET(event_fd, &readfds)) {
				FD_CLR(event_fd, &readfds);
				event_handle();
			}
			save = svc_fdset;

			/*
			 * First find out if any of our listener
			 * sockets want to open another socket
			 */
			fd_and(maxfds, &orig, &readfds, &fds);
#if CONNECTION_CHECK
			open_connections(maxfds, &fds);
#else
			socket_lock();
			svc_getreqset(&fds);
			socket_unlock();
#endif

			/*
			 * Now see if we have any newly opened 
			 * descriptors and then put them in the
			 * lru list if we do.
			 */
			fd_clr(maxfds, &save, &svc_fdset, &fds);
			nused += fd_count(maxfds, &fds);
			lru_mark(maxfds, &lru, &fds);

			/*
			 * Update the lru list with any sockets that
			 * need service and service them.
			 */
			fd_clr(maxfds, &orig, &readfds, &fds);
			lru_mark(maxfds, &lru, &fds);
			socket_lock();
			svc_getreqset(&fds);
			socket_unlock();

			/*
			 * Discover if any sockets were shut and
			 * clear them from the lru list
			 */
			fd_clr(maxfds, &svc_fdset, &save, &shut);
			nused -= fd_count(maxfds, &shut);
			lru_unmark(maxfds, &lru, &shut);

			while (nused > maxlisteners) {
				/*
				 * If we have reached the maximum number
				 * of descriptors, close the lru ones off
				 */
				lru_close(&lru);
				nused--;
			}
		}
		if (sys_time() > cleanuptime) {
#ifdef FLUSHCACHE
			/*
			 * Clean out memory
			 * XXX: Turned off for now
			 * because this may take a long time and can
			 * cause clients to be locked out. We need a better
			 * strategy for cleaning up memory.
			 */
			ni_forget(db_ni);
#endif
			cleanuptime = sys_time() + CLEANUPWAIT;
			/*
			 * Check for database synchronization on both
			 * master and clone.
			 */
			if (i_am_clone) {
				/*
				 * If clone, check to see if still in sync
				 */
				dir_clonecheck();
				/*
				 * Clear have_transferred flag to allow
				 * possible transfers in next cleanup 
				 * period.
				 */
				have_transferred = 0;
			} else {
				/*
				 * If master, force resynchronization, in case 
				 * any clones are out of date.
				 */
				notify_resync();
			} 
		}
	}
}

	 

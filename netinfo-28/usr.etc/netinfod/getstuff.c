/*
 * Lookup various things
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include "ni_server.h"
#include <stdio.h>
#include <arpa/inet.h>
#include <ctype.h>
#include "ni_globals.h"
#include "getstuff.h"
#include "clib.h"
#include "system.h"

#define NI_SEPARATOR '/'

/*
 * Lookup "name"s address
 */
unsigned long
getaddress(
	   void *ni,
	   ni_name name
	   )
{
	ni_id node;
	ni_id root;
	ni_idlist idlist;
	ni_namelist nl;
	u_long addr;
	
	if (ni_root(ni, &root) != NI_OK) {
		return (0);
	}
	if (ni_lookup(ni, &root, NAME_NAME, NAME_MACHINES, &idlist) != NI_OK) {
		return (0);
	}
	node.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);
	
	if (ni_lookup(ni, &node, NAME_NAME, name, &idlist) != NI_OK) {
		return (0);
	}
	node.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);

	if (ni_lookupprop(ni, &node, NAME_IP_ADDRESS, &nl) != NI_OK) {
		return (0);
	}
	if (nl.ninl_len > 0) {
		addr = inet_addr(nl.ninl_val[0]);
	} else {
		addr = 0;
	}
	ni_namelist_free(&nl);
	return (addr);
}


/*
 * Get name and tag of master server
 */
int 
getmaster(
	  void *ni,
	  ni_name *master,
	  ni_name *domain
	  )
{
	ni_id root;
	ni_namelist nl;
	ni_name sep;

	if (ni_root(ni, &root) != NI_OK) {
		return (0);
	}
	if (ni_lookupprop(ni, &root, NAME_MASTER, &nl) != NI_OK) {
		return (0);
	}
	if (nl.ninl_len == 0) {
		ni_namelist_free(&nl);
		return (0);
	}
	sep = index(nl.ninl_val[0], NI_SEPARATOR);
	if (sep == NULL) {
		ni_namelist_free(&nl);
		return (0);
	}
	*sep = 0;
	if (master != NULL) {
		*master = ni_name_dup(nl.ninl_val[0]);
	}
	if (domain != NULL) {
		*domain = ni_name_dup(sep + 1);
	}
	ni_namelist_free(&nl);
	return (1);
}

/*
 * Get master server's address (and domain)
 */
unsigned long
getmasteraddr(
	      void *ni,
	      ni_name *domain
	      )
{
	unsigned long addr;
	ni_name master;

	if (getmaster(ni, &master, domain)) {
		addr = getaddress(ni, master);
		ni_name_free(&master);
	} else {
		addr = 0;
	}
	return (addr);
}


/*
 * Lookup "name"s network
 */
static struct in_addr
getnetwork(
	   void *ni,
	   ni_name name
	   )
{
	ni_id node;
	ni_id root;
	ni_idlist idlist;
	ni_namelist nl;
	struct in_addr addr;

	addr.s_addr = 0;
	if (ni_root(ni, &root) != NI_OK) {
		return (addr);
	}
	if (ni_lookup(ni, &root, NAME_NAME, NAME_NETWORKS, &idlist) != NI_OK) {
		return (addr);
	}
	node.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);
	
	if (ni_lookup(ni, &node, NAME_NAME, name, &idlist) != NI_OK) {
		return (addr);
	}
	node.nii_object = idlist.niil_val[0];
	ni_idlist_free(&idlist);

	if (ni_lookupprop(ni, &node, NAME_ADDRESS, &nl) != NI_OK) {
		return (addr);
	}
	if (nl.ninl_len > 0) {
		addr = inet_makeaddr(inet_network(nl.ninl_val[0]), 0);
	}
	ni_namelist_free(&nl);
	return (addr);
}

static int
network_match(
	      struct in_addr net,
	      struct in_addr host
	      )
{
#	define s_b1 S_un.S_un_b.s_b1
#	define s_b2 S_un.S_un_b.s_b2
#	define s_b3 S_un.S_un_b.s_b3

	if (net.s_b1 != host.s_b1) {
		return (0);
	}
	if (net.s_b2 == 0) {
		/*
		 * One byte network match
		 */
		return (1);
	}
	if (net.s_b2 != host.s_b2) {
		return (0);
	}
	if (net.s_b3 == 0) {
		/*
		 * Two byte network match
		 */
		return (1);
	}
	if (net.s_b3 != host.s_b3) {
		return (0);
	}
	/*
	 * Three byte network match
	 */
	return (1);

#	undef s_b1
#	undef s_b2
#	undef s_b3
}

int
is_trusted_network(
		   void *ni,
		   struct sockaddr_in *host
		   )
{
	struct in_addr network;
	ni_id root;
	int i;
	char *val;
	ni_namelist nl;

	if (ni_root(ni, &root) != NI_OK) {
		/*
		 * Something is seriously wrong. Don't trust anybody.
		 */
		return (0);
	}
	if (ni_lookupprop(ni, &root, NAME_TRUSTED_NETWORKS, &nl) != NI_OK) {
		/*
		 * Property doesn't exist, so we trust everybody
		 */
		return (1);
	}
	for (i = 0; i < nl.ninl_len; i++) {
		val = nl.ninl_val[i];
		if (isdigit(*val)) {
			/*
			 * Network address in line
			 */
			network = inet_makeaddr(inet_network(val), 0);
		} else {
			/*
			 * Network specified by name
			 */
			network = getnetwork(ni, val);
		}
		if (network_match(network, host->sin_addr)) {
			ni_namelist_free(&nl);
			return (1);
		}
	}
	ni_namelist_free(&nl);

	if (sys_ismyaddress(host->sin_addr.s_addr)) {
		/*
		 * Always trust local connections
		 */
		return (1);
	}
	return (0);
}

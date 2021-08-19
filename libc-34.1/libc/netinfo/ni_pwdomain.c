/*
 * Copyright (C) 1990 by NeXT, Inc. All rights reserved.
 */

/*
 * ni_pwdomain function: present working domain for a netinfo handle
 *
 * usage:
 * 	ni_status ni_pwdomain(void *ni, ni_name *buf)
 *
 * pwd is returned in buf, which can be freed with ni_name_free
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <netinfo/ni.h>

extern char *inet_ntoa();

static const char NAME_NAME[] = "name";
static const char NAME_MACHINES[] = "machines";
static const char NAME_IP_ADDRESS[] = "ip_address";
static const char NAME_SERVES[] = "serves";
static const char NAME_UNKNOWN[] = "###UNKNOWN###";

static ni_name
escape_domain(
	      ni_name name
	      )
{
	int extra;
	char *p;
	char *s;
	ni_name newname;

	extra = 0;
	for (p = name; *p; p++) {
		if (*p == '/' || *p == '\\') {
			extra++;
		}
	}
	
	newname = malloc(strlen(name) + extra + 1);
	s = newname;
	for (p = name; *p; p++) {
		if (*p == '/' || *p == '\\') {
			*s++ = '\\';
		}
		*s++ = *p;
	}
	*s = 0;
	return (newname);
	
}


static char *
finddomain(
	   void *ni,
	   struct in_addr addr,
	   ni_name tag
	   )
{
	ni_id nid;
	ni_idlist idl;
	ni_namelist nl;
	ni_index i;
	ni_name slash;
	ni_name domain;

	if (ni_root(ni, &nid) != NI_OK) {
		return (NULL);
	}
	if (ni_lookup(ni, &nid, NAME_NAME, NAME_MACHINES, &idl) != NI_OK) {
		return (NULL);
	}
	nid.nii_object = idl.niil_val[0];
	ni_idlist_free(&idl);
	if (ni_lookup(ni, &nid, NAME_IP_ADDRESS, inet_ntoa(addr), 
		      &idl) != NI_OK) {
		return (NULL);
	}
	nid.nii_object = idl.niil_val[0];
	ni_idlist_free(&idl);
	if (ni_lookupprop(ni, &nid, NAME_SERVES, &nl) != NI_OK) {
		return (NULL);
	}
	for (i = 0; i < nl.ninl_len; i++) {
		slash = rindex(nl.ninl_val[i], '/');
		if (slash == NULL) {
			continue;
		}
		if (ni_name_match(slash + 1, tag)) {
			*slash = 0;
			domain = escape_domain(nl.ninl_val[i]);
			ni_namelist_free(&nl);
			return (domain);
		}
	}
	ni_namelist_free(&nl);
	return (NULL);
}

static char *
ni_domainof(
	    void *ni, 
	    void *parent
	    )
{
	struct sockaddr_in addr;
	ni_name tag;
	ni_name dom;
	
	if (ni_addrtag(ni, &addr, &tag) != NI_OK) {
		return (ni_name_dup(NAME_UNKNOWN));
	}
	dom = finddomain(parent, addr.sin_addr, tag);
	ni_name_free(&tag);
	if (dom == NULL) {
		return (ni_name_dup(NAME_UNKNOWN));
	}
	return (dom);
}

static ni_status
_ni_pwdomain(
	     void *ni,
	     ni_name *buf
	     )
{
	void *nip;
	ni_status status;
	int len;
	char *dom;

	nip = ni_new(ni, "..");
	if (nip == NULL) {
		(*buf) = malloc(2);
		(*buf)[0] = 0;
		return (NI_OK);
	}
	status = _ni_pwdomain(nip, buf);
	if (status != NI_OK) {
		return (status);
	}
	len = strlen(*buf);
	dom = ni_domainof(ni, nip);
	*buf = realloc(*buf, len + 1 + strlen(dom) + 1);
	(*buf)[len] = '/';
	strcpy(&(*buf)[len + 1], dom);
	ni_name_free(&dom);
	ni_free(nip);
	return (NI_OK);
}

/*
 * Just call the recursive ni_pwdomain above, and then fix for case of root
 * domain or error
 */
ni_status
ni_pwdomain(
	    void *ni,
	    ni_name *buf
	    )
{
	ni_status status;

	*buf = NULL;
	status = _ni_pwdomain(ni, buf);
	if (status != NI_OK) {
		if (*buf != NULL) {
			ni_name_free(buf);
		}
		return (status);
	}
	if ((*buf)[0] == 0) {
		(*buf)[0] = '/';
		(*buf)[1] = 0;
	}
	return (NI_OK);
}





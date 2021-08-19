/*
 * Useful stuff for programming netinfo
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdlib.h>
#include <netinfo/ni.h>
#include "clib.h"
#include <ctype.h>

extern void *_ni_dup(void *);

static const char *
eatslash(
	 const char *path
	 )
{
	while (*path == '/') {
		path++;
	}
	return (path);
}

static void
unescape(
	 ni_name *name
	 )
{
	ni_name newname;
	ni_name p;
	int len;
	int i;

	p = *name;
	len = strlen(p);
	newname = malloc(len + 1);
	for (i = 0; *p != 0; i++) {
		if (*p == '\\') {
			p++;
		}
		newname[i] = *p++;
	}
	ni_name_free(name);
	newname[i] = 0;
	*name = newname;
}

static const char *
escindex(
	 const char *str,
	 char ch
	 )
{
	char *p;

	p = index(str, ch);
	if (p == NULL) {
		return (NULL);
	}
	if (p == str) {
		return (p);
	}
	if (p[-1] == '\\') {
		return (escindex(p + 1, ch));
	}
	return (p);
}

static void
setstuff(
	 void *ni,
	 ni_fancyopenargs *args
	 )
{
	if (args != NULL) {
	  	ni_setabort(ni, args->abort);
	  	if (args->rtimeout) {
	    		ni_setreadtimeout(ni, args->rtimeout);
		}
	  	if (args->wtimeout) {
			ni_setwritetimeout(ni, args->wtimeout);
		}
	}
}

static void *
ni_relopen(
	   void *ni,
	   const char *domain,
	   int freeold,
	   ni_fancyopenargs *args
	   )
{
	void *newni;
	void *tmpni;
	char *slash;
	char *component;

	component = ni_name_dup(domain);
	slash = (char *)escindex(component, '/');
	if (slash != NULL) {
		*slash = 0;
	}
	unescape(&component);

	tmpni = NULL;
	if (ni != NULL && args != NULL) {
	  	tmpni = _ni_dup(ni);
		if (freeold) {
			ni_free(ni);
		}
		ni = tmpni;
		setstuff(ni, args);
	}

	newni = ni_new(ni, component);
	free(component);

	if (tmpni != NULL) {
		ni_free(ni);
		ni = NULL;
	}

	if (ni != NULL && freeold) {
		ni_free(ni);
	}


	if (newni == NULL) {
		return (NULL);
	}
	setstuff(newni, args);
	ni = newni;
	if (slash != NULL) {
	  	slash = (char *)escindex(domain, '/');
		domain = eatslash(slash + 1);
		return (ni_relopen(ni, domain, TRUE, NULL));
	} else {
		return (ni);
	}
}

static void *
ni_rootopen(
	    ni_fancyopenargs *args
	    )
{
	void *ni;
	void *newni;

	ni = ni_new(NULL, ".");
	if (ni == NULL) {
		return (NULL);
	}
	setstuff(ni, args);
	for (;;) {
		newni = ni_new(ni, "..");
		if (newni == NULL) {
			break;
		}
		ni_free(ni);
		ni = newni;
	}
	return (ni);
}

static ni_name
ni_name_dupn(
	     ni_name_const start,
	     ni_name_const stop
	     )
{
	int len;
	ni_name new;

	if (stop != NULL) {
	  	len = stop - start;
	} else {
		len = strlen(start);
	}
	new = malloc(len + 1);
	bcopy(start, new, len);
	new[len] = 0;
	return (new);
}

       
static ni_status
ni_relsearch(
	     void *ni,
	     const char *path,
	     ni_id *id
	     )
{
	char *slash;
	char *equal;
	ni_name key;
	ni_name val;
	ni_idlist idl;
	ni_status status;

	slash = (char *)escindex(path, '/');
	equal = (char *)escindex(path, '=');
	if (equal != NULL && (slash == NULL || equal < slash)) {
		key = ni_name_dupn(path, equal);
		val = ni_name_dupn(equal + 1, slash);
	} else {
		if (equal == NULL || (slash != NULL && slash < equal)) {
			key = ni_name_dup("name");
			val = ni_name_dupn(path, slash);
		} else {
			key = ni_name_dupn(path, equal);
			val = ni_name_dupn(equal + 1, slash);
		}
	}
	unescape(&key);
	unescape(&val);
	status = ni_lookup(ni, id, key, val, &idl);
	if (status != NI_OK) {
	  	ni_name_free(&key);
		ni_name_free(&val);
		return (status);
	}
	id->nii_object = idl.niil_val[0];
	ni_name_free(&key);
	ni_name_free(&val);
	ni_idlist_free(&idl);
	if (slash == NULL) {
		ni_self(ni, id);
		return (NI_OK);
	}
	path = eatslash(slash);
	return (ni_relsearch(ni, path, id));
}

ni_status
ni_open(
	void *ni,
	const char *domain,
	void **newni
	)
{
	return (ni_fancyopen(ni, domain, newni, NULL));
}

ni_status
ni_fancyopen(
	     void *ni,
	     const char *domain,
	     void **newni,
	     ni_fancyopenargs *args
	     )
{
	void *tmp = NULL;
	int rootopen = 0;

	if (*domain == '/') {
		tmp = ni_rootopen(args);
		if (tmp == NULL) {
		    return (NI_FAILED); /* XXX: should return real error */
		}
		domain = eatslash(domain);
		ni = tmp;
		rootopen++;
	}
	if (*domain != 0) {
		tmp = ni_relopen(ni, domain, FALSE, args);
		if (rootopen) {
			ni_free(ni);
		}
	}
	if (tmp == NULL) {
	    return (NI_FAILED);
	}
	*newni = tmp;
	ni_needwrite(*newni, args == NULL ? 0 : args->needwrite);
	return (NI_OK);
}

ni_status
ni_pathsearch(
	      void *ni, 
	      ni_id *id,
	      const char *path
	      )
{
	ni_status status;

	if (*path == '/') {
		status = ni_root(ni, id);
		if (status != NI_OK) {
			return (status);
		}
	}
	path = eatslash(path);
	if (*path != 0) {
		status = ni_relsearch(ni, path, id);
		if (status != NI_OK) {
			return (status);
		}
	}
	return (NI_OK);
}

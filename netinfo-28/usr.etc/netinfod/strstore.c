/*
 * String storage
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * Simple scheme to store strings in binary tree and reference count them.
 */
#include <stdlib.h>
#include <string.h>
#include "strstore.h"
#include "ranstrcmp.h"

#define strdup(x)  strcpy(malloc(strlen(x) + 1), (x))

#ifdef STRSTORE_DEBUG
#include <stdio.h>
#define debug(msg) fprintf(stderr, "ERROR: %s\n", msg)
#else
#define debug(msg)
#endif

typedef struct strtreenode *strtree;

typedef struct strtreenode {
	char *string;
	int refcount;
	strtree left;
	strtree right;
} strtreenode;


static strtree
treealloc(const char *string) 
{
	strtree res;

	res = malloc(sizeof(*res));
	res->string = strdup(string);
	res->refcount = 1;
	res->left = NULL;
	res->right = NULL;
	return (res);
}

static const char *
strstore(strtree *thetree, const char *string)
{
	int res;

	if (*thetree == NULL) {
		*thetree = treealloc(string);
		return ((*thetree)->string);
	}
	res = ranstrcmp(string, (*thetree)->string);
	if (res < 0) {
		return (strstore(&(*thetree)->left, string));
	} else if (res == 0) {
		(*thetree)->refcount++;
		return ((*thetree)->string);
	} else if (res > 0) {
		return (strstore(&(*thetree)->right, string));
	}
	debug("impossible");
	return (NULL);
}


static void
insertnode(strtree *thetree, strtree thenode)
{
	int res;

	if (*thetree == NULL) {
		*thetree = thenode;
		return;
	}
	res = ranstrcmp(thenode->string, (*thetree)->string);
	if (res < 0) {
		insertnode(&(*thetree)->left, thenode);
	} else if (res == 0) {
		debug("inserting something already there");
	} else if (res > 0) {
		insertnode(&(*thetree)->right, thenode);
	}
}

static unsigned
pickleft(strtree *thetree)
{
	unsigned long x = (unsigned long)thetree;
	unsigned count;

	for (count = 0; x > 0; x >>= 1) {
		count += x & 1;
	}
	return (count & 1);
}

static void
removenode(strtree *thetree)
{
	strtree save;

	save = *thetree;
	if ((*thetree)->left == NULL) {
		*thetree = (*thetree)->right;
	} else if ((*thetree)->right == NULL) {
		*thetree = (*thetree)->left;
	} else {
		if (pickleft(thetree)) {
			*thetree = (*thetree)->left;
			insertnode(thetree, save->right);
		} else {
			*thetree = (*thetree)->right;
			insertnode(thetree, save->left);
		}
	}
	free(save->string);
	free(save);
}

static void
strdelete(strtree *thetree, const char *string)
{
	int res;

	if (*thetree == NULL) {
		debug("already deleted");
		return;
	}
	res = ranstrcmp(string, (*thetree)->string);
	if (res < 0) {
		strdelete(&(*thetree)->left, string);
	} else if (res == 0) {
		if ((*thetree)->refcount > 0) {
			(*thetree)->refcount--;
		}
		if ((*thetree)->refcount == 0) {
			removenode(thetree);
		}
	} else if (res > 0) {
		strdelete(&(*thetree)->right, string);
	}
}


static strtree handle = NULL;

const char *
ss_alloc(
	 const char *string
	 )
{
	return (strstore(&handle, string));
}

void
ss_unalloc(
	   const char *string
	   )
{
	strdelete(&handle, string);
}


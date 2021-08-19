/*
 * Be a little smarter and preload some entries so things like "ls" and
 * Workspace can show you something reasonable.
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <sys/param.h>
#include "nfs_prot.h"
typedef nfs_fh fhandle_t;
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
#include <sys/mount.h>
#ifdef NeXT
#include <nfs/nfs_mount.h>
#ifndef M_NEWTYPE
#define OLDMOUNT
#endif
#endif
#include "automount.h"

char PRELOAD[] = "preload=";

void
preload_all(dir)
	struct autodir *dir;
{
	char **ent;
	struct vnode *vp;

	for (ent = dir->dir_preload; ent != NULL && *ent != NULL; ent++) {
		(void)lookup(dir, *ent, &vp);
	}
}

int
preload_entry(dir, name)
	struct autodir *dir;
	char *name;
{
	char **ent;

	for (ent = dir->dir_preload; ent != NULL && *ent != NULL; ent++) {
		if (strcmp(*ent, name) == 0) {
			return (1);
		}
	}
	return (0);
}

char **
loadit(fname)
	char *fname;
{
	FILE *f;
	int c;
	char hostname[MAXHOSTNAMELEN + 1];
	int len;
	char **res;
	int reslen;

	f = fopen(fname, "r");
	if (f == NULL) {
		return (NULL);
	}
	res = (char **)malloc(sizeof(char *));
	reslen = 0;
	len = 0;
	while ((c = getc(f)) != EOF) {
		if (isspace(c)) {
			if (len > 0) {
				hostname[len] = 0;
				len = 0;

				res[reslen++] = strdup(hostname);
				res = (char **)realloc(res, ((reslen + 1) * 
							     sizeof(char *)));
			}
		} else {
			if (len < MAXHOSTNAMELEN) {
				hostname[len++] = c;
			}
		}
	}
	fclose(f);
	if (len > 0) {
		hostname[len] = 0;
		res[reslen++] = strdup(hostname);
		res = (char **)realloc(res, ((reslen + 1) * sizeof(char *)));
	}
	res[reslen] = NULL;
	return (res);
}


char **
preload_try(opts)
	char *opts;
{
	char *end;
	char **res;
	char *fname;
	int len;

	while (opts && *opts) {
		if (strncmp(opts, PRELOAD, strlen(PRELOAD)) == 0) {
			opts += strlen(PRELOAD);
			for (end = opts; *end && *end != ','; end++) {
			}
			len = end - opts;
			fname = malloc(len + 1);
			if (fname == NULL) {
				return (NULL);
			}
			strncpy(fname, opts, len);
			fname[len] = 0;
			res = loadit(fname);
			free(fname);
			return (res);
		}
		opts = index(opts, ',');
		if (opts != NULL) {
			opts++; /* skip comma */
		}
	}
	return (NULL);
}

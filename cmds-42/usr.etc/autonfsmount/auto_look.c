#ifndef lint
static char sccsid[] = 	"@(#)auto_look.c	1.2 88/05/10 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * Copyright (c) 1987 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <ctype.h>
#include <syslog.h>
#include <sys/types.h>
#include <pwd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <rpc/types.h>
#include <rpc/auth.h>
#include <rpc/auth_unix.h>
#include <rpc/xdr.h>
#include <rpcsvc/ypclnt.h>
#include "nfs_prot.h"
#define NFSCLIENT
#define NFS                     /* for backwards compatibility */
typedef nfs_fh fhandle_t;
#include <sys/mount.h>
#ifdef NeXT
#include <nfs/nfs_mount.h>
#endif
#include "automount.h"

#define	MAXHOSTS 20

nfsstat
lookup(dir, name, vpp)
	struct autodir *dir;
	char *name;
	struct vnode **vpp;
{
	struct mapent *me;
	struct mapfs *mfs;
	struct link *link;
	struct filsys *fs;
	char *subdir;
	char linkpath[1024];
	int entrycount;
	struct hostent *hp;
	struct in_addr addrs[MAXHOSTS], addr, *addrp, trymany();
	nfsstat status;
	char hissite[64];

	if (name[0] == '.' &&
	    (name[1] == '\0' || (name[1] == '.' && name[2] == '\0'))) {
		*vpp = &dir->dir_vnode;
		return (NFS_OK);
	}

	if ((link = findlink(dir, name)) && 
	    (link->link_fs || 
	     (link->link_dir->dir_maptype == MAPTYPE_FSTAB &&
	      link->link_death != 0))) {
		link->link_death = time_now + max_link_time;
		*vpp =  &link->link_vnode;
		return (NFS_OK);
	}


	if (dir->dir_maptype == MAPTYPE_HOSTS ||
	    dir->dir_maptype == MAPTYPE_FSTAB) {
		if (strcmp(name, self) == 0) {
			link = makelink(dir, name, (struct filsys *)NULL, "/");
			if (link == NULL) {
				return (NFSERR_NOSPC);
			}
			*vpp = &link->link_vnode;
			return (NFS_OK);
		}
		if (dir->dir_maptype == MAPTYPE_FSTAB) {
			if (!fstab_inlist(dir, name)) {
				return (NFSERR_NOENT);
			}
		}
		if (nomounts)
			return (NFSERR_PERM);
		if (link && freeall(dir, name) == 0) { 
			link->link_death = time_now + max_link_time;
			*vpp = &link->link_vnode;
			return (NFS_OK);
		}
		if (dir->dir_maptype == MAPTYPE_HOSTS) {
			status = mountall(dir, name);
			if (status != NFS_OK) {
				return (status);
			}
		}
		(void) sprintf(linkpath, "%s%s/%s", tmpdir,
							dir->dir_name, name);
		link = makelink(dir, name, (struct filsys *)NULL, linkpath);
		if (link == NULL)
			return (NFSERR_NOSPC);
		if (dir->dir_maptype == MAPTYPE_FSTAB) {
			link->link_vnode.vn_fattr.mode |= NFSMODE_STICKY;
		}
		*vpp = &link->link_vnode;
		return (NFS_OK);
	}

	me = getmapent(dir, name);
	if (me == NULL)
		return (NFSERR_NOENT);
	/* see if any fs's are ourself */
	entrycount = 0;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		entrycount++;
		if (strcmp(mfs->mfs_host, self))
			continue;
		/* yes! */
		(void) strcpy(linkpath, mfs->mfs_dir);
		(void) strcat(linkpath, "/");
		(void) strcat(linkpath, mfs->mfs_subdir);
		subdir = linkpath;
		fs = NULL;
		goto ok;
	}
	/* see if any already mounted */
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (fs = already_mounted(mfs->mfs_host, mfs->mfs_dir,
		    me->map_mntopts)) {
			subdir = mfs->mfs_subdir;
			goto ok;
		}
	}
	if (nomounts) {
		free_mapent(me);
		return (NFSERR_PERM);
	}
	if (entrycount == 1) {	/* no replication involved */
		status = trymount(me->map_fs, me->map_mntopts, &fs);
		if (status != NFS_OK) {
			free_mapent(me);
			return (status);
		}
		subdir = me->map_fs->mfs_subdir;
		goto ok;
	}
	/* get addresses */
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		hp = gethostbyname(mfs->mfs_host);
		if (hp == NULL) {
			mfs->mfs_addr.s_addr = 0;
			entrycount--;
			continue;
		}
		bcopy(hp->h_addr, (char *)&mfs->mfs_addr, hp->h_length);
	}
	if (entrycount == 0) {
		free_mapent(me);
		return (NFSERR_NOENT);
	}
	/* try local net */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		if (inet_netof(mfs->mfs_addr) == inet_netof(my_addr)) {
			*addrp++ = mfs->mfs_addr;
		}
	}
	if (addrp != addrs) {	/* got some */
		(*addrp).s_addr = 0;	/* terminate list */
		addr = trymany(addrs);
		if (addr.s_addr) {	/* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
			if (addr.s_addr == mfs->mfs_addr.s_addr)  {
				if (trymount(mfs, me->map_mntopts, &fs) ==
				    NFS_OK) {
					subdir = mfs->mfs_subdir;
					goto ok;
				}
			}
		}
	}
	/* try local site */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next) {
		getsite(mfs->mfs_addr, hissite);
		if (strcmp(mysite, hissite) == 0) {
			*addrp++ = mfs->mfs_addr;
		}
	}
	if (addrp != addrs) {	/* got some */
		(*addrp).s_addr = 0;	/* terminate list */
		addr = trymany(addrs);
		if (addr.s_addr) {	/* got one */
			for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
			if (addr.s_addr == mfs->mfs_addr.s_addr)  {
				if (trymount(mfs, me->map_mntopts, &fs) ==
				    NFS_OK) {
					subdir = mfs->mfs_subdir;
					goto ok;
				}
			}
		}
	}
	/* now try them all */
	addrp = addrs;
	for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		*addrp++ = mfs->mfs_addr;
	(*addrp).s_addr = 0;	/* terminate list */
	addr = trymany(addrs);
	if (addr.s_addr) {	/* got one */
		for (mfs = me->map_fs; mfs; mfs = mfs->mfs_next)
		if (addr.s_addr == mfs->mfs_addr.s_addr)  {
			if (trymount(mfs, me->map_mntopts, &fs) == NFS_OK) {
				subdir = mfs->mfs_subdir;
				goto ok;
			}
		}
	}
	free_mapent(me);
	return (NFSERR_NOENT);
ok:
	link = makelink(dir, name, fs, subdir);
	free_mapent(me);
	if (link == NULL)
		return (NFSERR_NOSPC);
	*vpp = &link->link_vnode;
	return (NFS_OK);
}

struct mapent *
getmapent(dir, key)
	struct autodir *dir;
	char *key;
{
	FILE *fp = NULL;
	char line[1000], *lineptr;
	char *ypline = NULL;
	char word[256];
	char *index();
	char *malloc();
	char *getword();
	char *p, *q;
	struct mapfs *mfs;
	struct mapent *me;
	int reason;
	int maptype;
	int len;

	maptype = dir->dir_maptype;
        if (maptype == MAPTYPE_PASSWD) {
                struct passwd *pw;
                char *rindex();
                int c;

                pw = getpwnam(key);
                if (pw == NULL)
                        return ((struct mapent *)NULL);
                for (c = 0, p = pw->pw_dir ; *p ; p++)
                        if (*p == '/')
                                c++;
                if (c != 3)     /* expect "/dir/host/user" */
                        return ((struct mapent *)NULL);

                me = (struct mapent *)malloc(sizeof *me);
		if (me == NULL)
			goto alloc_failed;
                me->map_mntopts = strdup(dir->dir_opts);
		if (me->map_mntopts == NULL) {
			free((char *)me);
			goto alloc_failed;
		}
		me->map_fs = NULL;
                mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL) {
			free_mapent(me);
			goto alloc_failed;
		}
                p = rindex(pw->pw_dir, '/');
                mfs->mfs_subdir = strdup(p+1);
		if (mfs->mfs_subdir == NULL) {
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
                *p = '\0';
                p = rindex(pw->pw_dir, '/');
                mfs->mfs_host = strdup(p+1);
		if (mfs->mfs_host == NULL) {
			free(mfs->mfs_subdir);
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
                mfs->mfs_dir = strdup(pw->pw_dir);
		if (mfs->mfs_dir == NULL) {
			free(mfs->mfs_host);
			free(mfs->mfs_subdir);
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
                mfs->mfs_next = NULL;
                me->map_fs = mfs;
                (void) endpwent();
                return (me);
        }
	if (maptype == MAPTYPE_FILE &&
	    (fp = fopen(dir->dir_map, "r")) != NULL) {
		/*
		 * The map is in a file, and the file exists; scan it.
		 */
		lineptr = line;
		for (;;) {
			if (fgets(lineptr, sizeof line - (lineptr-line),
			    fp) == NULL) {
				fclose(fp);
				return ((struct mapent *)0);
			}
			/* ignore # and beyond */
			if (lineptr = index(line, '#'))
				*lineptr = '\0';
			len = strlen(line);
			if (len <= 0) {
				lineptr = line;
				continue;
			}
			/* trim trailing white space */
			lineptr = &line[len - 1];
			while (lineptr > line && isspace(*lineptr))
				*lineptr-- = '\0';
			if (lineptr == line)
				continue;
			/* if continued, get next line */
			if (*lineptr == '\\')
				continue;
			/* now have complete line */
			lineptr = getword(word, line);
			if (strcmp(word, key) == 0)
				break;
			if (word[0] == '+') {
				reason = yp_match(mydomain, word+1,
					key, strlen(key), &ypline, &len);
				if (reason == 0)
					break;
				if (reason != YPERR_KEY)
					syslog(LOG_ERR, "%s: %s", word+1,
						yperr_string(reason));
			}
			lineptr = line;
		}
		if (fp)
			fclose(fp);
	} else {
		/*
		 * The map is a YP map, or is claimed to be a file but the
		 * file does not exist; just lookup the YP entry.
		 */
		reason = yp_match(mydomain, dir->dir_map, key, strlen(key),
		    &ypline, &len);
		if (reason) {
			if (reason != YPERR_KEY)
				syslog(LOG_ERR, "%s: %s", dir->dir_map,
					yperr_string(reason));
			return ((struct mapent *)NULL);
		}
	}
	if (ypline) {	/* trim YP entries */
		/* ignore # and beyond */
		if (lineptr = index(ypline, '#'))
			*lineptr = '\0';
		len = strlen(ypline);
		if (len <= 0)
			goto failed;
		/* trim trailing white space */
		lineptr = &ypline[len - 1];
		while (lineptr > ypline && isspace(*lineptr))
			*lineptr-- = '\0';
		if (lineptr == ypline)
			goto failed;
		lineptr = ypline;
	}
	/* now have correct line */
	me = (struct mapent *)malloc(sizeof *me);
	if (me == NULL) {
		syslog(LOG_ERR, "Memory allocation failed: %m");
		return ((struct mapent *)NULL);
	}
	lineptr = getword(word, lineptr);
	if (word[0] == '-') {	/* mount options */
		me->map_mntopts = strdup(word+1);
		lineptr = getword(word, lineptr);
	} else
		me->map_mntopts = strdup(dir->dir_opts);
	if (me->map_mntopts == NULL) {
		free((char *)me);
		goto alloc_failed;
	}
	me->map_fs = NULL;
	while (*word) {
		p = index(word, ':');
		if (p == NULL)
			break;
		*p++ = '\0';
		q = index(p, ':');
		if (q)
			*q++ = '\0';
		else
			q = "";
		mfs = (struct mapfs *)malloc(sizeof *mfs);
		if (mfs == NULL) {
			free_mapent(me);
			goto alloc_failed;
		}
		mfs->mfs_host = strdup(word);
		if (mfs->mfs_host == NULL) {
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
		mfs->mfs_dir = strdup(p);
		if (mfs->mfs_dir == NULL) {
			free((char *)mfs->mfs_host);
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
		mfs->mfs_subdir = strdup(q);
		if (mfs->mfs_subdir == NULL) {
			free((char *)mfs->mfs_dir);
			free((char *)mfs->mfs_host);
			free((char *)mfs);
			free_mapent(me);
			goto alloc_failed;
		}
		mfs->mfs_next = me->map_fs;
		me->map_fs = mfs;
		lineptr = getword(word, lineptr);
	}
	if (ypline)
		free((char *)ypline);
	return ((struct mapent *)me);

alloc_failed:
	syslog(LOG_ERR, "Memory allocation failed: %m");

failed:
	if (ypline)
		free((char *)ypline);
	return ((struct mapent *)NULL);
}

free_mapent(me)
	struct mapent *me;
{
	struct mapfs *mfs;

	while (mfs = me->map_fs) {
		me->map_fs = mfs->mfs_next;
		if (mfs->mfs_host)
			free(mfs->mfs_host);
		if (mfs->mfs_dir)
			free(mfs->mfs_dir);
		if (mfs->mfs_subdir)
			free(mfs->mfs_subdir);
		free((char *)mfs);
	}
	if (me->map_mntopts)
		free(me->map_mntopts);
	free((char *)me);	/* from all this misery */
}

char *
getword(w, p)
	char *w, *p;
{
	while (isspace(*p))
		p++;
	while (*p && !isspace(*p))
		*w++ = *p++;
	*w = 0;
	return (p);
}


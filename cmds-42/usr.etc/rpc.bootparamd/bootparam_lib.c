/*	@(#)bootparam_lib.c	1.6 88/01/29 D/NFS */
#ifndef lint
static  char sccsid[] = "@(#)bootparam_lib.c 1.2 87/10/14 SMI";
#endif

/*
 * Library routines of the bootparam server.
 */

#include <stdio.h>
#include <rpcsvc/ypclnt.h>
#include <sys/types.h>
#include <sys/stat.h>

#define iseol(c)	(c == '\0' || c == '\n' || c == '#')
#define issep(c)	(index(sep,c) != NULL)
#define isignore(c) (index(ignore,c) != NULL)

#define BOOTPARAMS	"/etc/bootparams"
#define	LINESIZE	512
#define NAMESIZE	256
#define DOMAINSIZE	256

extern int useni();

static char domain[DOMAINSIZE];			/* yp domain name */
static int useyp();
static int getclntent();
static int getfileent();
static int getfilekey();

/*
 * getline()
 * Read a line from a file.
 * What's returned is a cookie to be passed to getname
 */
char **
getline(line,maxlinelen,f)
	char *line;
	int maxlinelen;
	FILE *f;
{
	char *p;
	static char *lp;
	do {
		if (! fgets(line, maxlinelen, f)) {
			return(NULL);
		}
	} while (iseol(line[0]));
	p = line;
	for (;;) {	
		while (*p) {	
			p++;
		}
		if (*--p == '\n' && *--p == '\\') {
			if (! fgets(p, maxlinelen, f)) {
				break;	
			}
		} else {
			break;	
		}
	}
	lp = line;
	return(&lp);
}

/*
 * getname()
 * Get the next entry from the line.
 * You tell getname() which characters to ignore before storing them 
 * into name, and which characters separate entries in a line.
 * The cookie is updated appropriately.
 * return:
 *	   1: one entry parsed
 *	   0: partial entry parsed, ran out of space in name
 *    -1: no more entries in line
 */
int
getname(name, namelen, ignore, sep, linep)
	char *name;
	int namelen;
	char *ignore;	
	char *sep;
	char **linep;
{
	char c;
	char *lp;
	char *np;
	int maxnamelen;

	lp = *linep;
	do {
		c = *lp++;
	} while (isignore(c) && !iseol(c));
	if (iseol(c)) {
		*linep = lp - 1;
		return(-1);
	}
	np = name;
	while (! issep(c) && ! iseol(c) && np - name < namelen) {	
		*np++ = c;	
		c = *lp++;
	} 
	lp--;
	if (issep(c) || iseol(c)) {
		if (np - name != namelen) {
			*np = 0;
		}
		if (iseol(c)) {
			*lp = 0;
		} else {
			lp++; 	/* advance over separator */
		}
	} else {
		*linep = lp;
		return(0);
	}
	*linep = lp;
	return(1);
}

/*
 * getclntent reads the line buffer and returns a string of client entry
 * in yellow pages or the "/etc/bootparams" file. Called by bp_getclntent.
 */
static int
getclntent(lp, clnt_entry)
	register char **lp;			/* function input */
	register char *clnt_entry;		/* function output */
{
	char name[NAMESIZE];
	int append = 0;

	while (getname(name, sizeof(name), " \t", " \t", lp) >= 0) {
		if (!append) {
			strcpy(clnt_entry, name);
			append++;
		} else {
			strcat(clnt_entry, " ");
			strcat(clnt_entry, name);
		}
	}
	return (0);
}	

/*
 * getfileent returns the client entry in the "/etc/bootparams"
 * file given the client name.
 */
static int
getfileent(clnt_name, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_entry;		/* function output */
{
	FILE *fp; 
	char line[LINESIZE];
	char name[NAMESIZE];
	char **lp;
	int reason;
 
	reason = -1;
	if ((fp = fopen(BOOTPARAMS, "r")) == NULL) {
		return (-1);
	}
	while (lp = getline(line, sizeof(line), fp)) {
		if ((getname(name, sizeof(name), " \t", " \t",
		    lp) >= 0) && (strcmp(name,clnt_name) == 0)) {
			if (getclntent(lp, clnt_entry) == 0)
				reason = 0;
			break;
		}
	}
	fclose(fp);
	return (reason);
}

/*
 * bp_getclntent returns the client entry in either the yellow pages map or
 * the "/etc/bootparams" file given the client name.
 */
int
bp_getclntent(clnt_name, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_entry;		/* function output */
{
	char *val, *buf;
	int vallen;
	int reason;
	int len;
	int i;
	char **values;
	int nread;

	if (useni()) {
		nread = ni_bplookup(clnt_name, &values);
		if (nread >= 0) {
			for (i = 0; i < nread; i++) {
				strcpy(&clnt_entry[len], values[i]);
				len += strlen(values[i]);
				clnt_entry[len] = ' ';
				len++;
			}
			clnt_entry[len] = 0;
			return (0);
		}
		if (useyp()) {
			if (reason = yp_match(domain, "bootparams", clnt_name,
					      strlen(clnt_name), &val, 
					      &vallen)) {
				/*
				 * if no such map, or clnt_name not found in 
				 * map,
				 * try local bootparams file.
				 */
				if (reason == YPERR_MAP || reason == YPERR_KEY)
					return(getfileent(clnt_name, 
							  clnt_entry));	
				else
					return (reason);
			} else {
				buf = val;
				reason = getclntent(&buf, clnt_entry);
				free(val);
				return(reason);
			}
		}
		return (-1);
	} else {
		return (getfileent(clnt_name, clnt_entry));
	}
}

/*
 * getclntkey reads the line buffer and returns a string of pathname
 * in yellow pages or the "/etc/bootparams" file. Called by bp_getclntkey.
 */
static int
getclntkey(lp, clnt_key, clnt_entry)
	register char **lp;			/* function input */
	register char *clnt_key;		/* function input */
	register char *clnt_entry;		/* function output */
{
	char name[NAMESIZE];
	char *cp;

	while (getname(name, sizeof(name), " \t", " \t", lp) >= 0) {
		if ((cp = (char *)index(name, '=')) == 0)
			return (-1);
		*cp++ = '\0';
		if (strcmp(name, clnt_key) == 0) {
			strcpy(clnt_entry, cp);
			return (0);
		}
	}
	if (strcmp(clnt_key, "dump") == 0) {
		/*
		 * This is a gross hack to handle the case where
		 * no dump file exists in bootparams. The null
		 * server and path names indicate this fact to the
		 * client.
		 */
		strcpy(clnt_entry, ":");
		return (0);
	}
	return (-1);
}	

/*
 * getfilekey returns the client's server name and its pathname from
 * the "/etc/bootparams" file given the client name and the key.
 */
static int
getfilekey(clnt_name, clnt_key, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_key;		/* function input */
	register char *clnt_entry;		/* function output */
{
	FILE *fp; 
	char line[LINESIZE];
	char name[NAMESIZE];
	char **lp;
	int reason;
 
	reason = -1;
	if ((fp = fopen(BOOTPARAMS, "r")) == NULL) {
		return (-1);
	}
	while (lp = getline(line, sizeof(line), fp)) {
		if ((getname(name, sizeof(name), " \t", " \t",
		    lp) >= 0) && (strcmp(name,clnt_name) == 0)) {
			if (getclntkey(lp, clnt_key, clnt_entry) == 0)
				reason = 0;
			break;
		}
	}
	fclose(fp);
	return (reason);
}

/*
 * bp_getclntkey returns the client's server name and its pathname from either
 * the yellow pages or the "/etc/bootparams" file given the client name and
 * the key.
 */
int
bp_getclntkey(clnt_name, clnt_key, clnt_entry)
	register char *clnt_name;		/* function input */
	register char *clnt_key;		/* function input */
	register char *clnt_entry;		/* function output */
{
	char *val, *buf;
	int vallen;
	int reason;
	int i;
	int len;
	char **values;
	int nread;

	if (useni()) {
		nread = ni_bplookup(clnt_name, &values);
		if (nread >= 0) {
			len = strlen(clnt_key);
			for (i = 0; i < nread; i++) {
				if (strncmp(values[i], clnt_key, 
					    len) == 0 &&
				    values[i][len] == '=') {
					strcpy(clnt_entry,
					       &values[i][len + 1]);
					return (0);
				}
			}
			return (-1);
		}
		if (useyp()) {
			if (reason = yp_match(domain, "bootparams", clnt_name,
					      strlen(clnt_name), &val, 
					      &vallen)) {
				/*
				 * if no such map, or clnt_name not found in 
				 * map,
				 * try local bootparams file.
				 */
				if (reason == YPERR_MAP || reason == YPERR_KEY)
					return(getfilekey(clnt_name, clnt_key,
							  clnt_entry));	
				else
					return (reason);
			} else {
				buf = val;
				reason = getclntkey(&buf, clnt_key, 
						    clnt_entry);
				free(val);
				return(reason);
			}
		}
		return (-1);
	} else {
		return(getfilekey(clnt_name, clnt_key, clnt_entry));
	}
}

/*
 * Determine whether or not to use the yellow pages service to do lookups.
 */
static int initted;
static int usingyp;
static int
useyp()
{
	if (!initted) {
		if (getdomainname(domain, sizeof(domain)) < 0)
			usingyp = 0;		/* not using yp */
		else
			usingyp = !yp_bind(domain);
		initted = 1;
	}
	return (usingyp);
}

/*
 * Determine if a descriptor belongs to a socket or not
 */
issock(fd)
	int fd;
{
	struct stat st;

	if (fstat(fd, &st) < 0) {
		return (0);
	} 
	/*	
	 * SunOS returns S_IFIFO for sockets, while 4.3 returns 0 and
	 * does not even have an S_IFIFO mode.  Since there is confusion 
	 * about what the mode is, we check for what it is not instead of 
	 * what it is.
	 */
	switch (st.st_mode & S_IFMT) {
	case S_IFCHR:
	case S_IFREG:
	case S_IFLNK:
	case S_IFDIR:
	case S_IFBLK:
		return (0);
	default:	
		return (1);
	}
}

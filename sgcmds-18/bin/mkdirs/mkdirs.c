#include <stdio.h>
#include <ctype.h>
#include <pwd.h>
#include <grp.h>
#include <strings.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <sys/dir.h>

#define	streq(a,b)	(strcmp(a,b) == 0)

char *progname;
int um;
int have_warned;

void init_path();
char *next_path();
extern struct passwd *getpwnam();

main(argc, argv)
int argc;
char **argv;
{
	char curdir[MAXPATHLEN];
	char *dir, *basedir;
	char *owner = NULL;
	char *group = NULL;
	char *modes = NULL;
#ifdef CROSS
	char *root;
	char passwdfile[512];
	char groupfile[512];
#endif CROSS

	/*
	 * mkdirs [-o OWNER] [-g GROUP] [-m MODE] [-r ROOT] DIR_PATHS
	 *
	 *  Creates all directories listed in DIR_PATHS
	 *  Optionally sets owner, group, and mode for created
	 *  directories.
	 *
	 *  If compiled with CROSS defined, handles searching for
	 *  passwd and group files starting from ROOT.
	 */

	progname = *argv++;

	um = umask(0);
	umask(um);

	if (getwd(curdir) == 0)
		error(curdir);

	for (; *argv && **argv == '-'; argv++) {
		switch ((*argv)[1]) {
		case 'o':
			owner = *++argv;
			break;
		case 'g':
			group = *++argv;
			break;
		case 'm':
			modes = *++argv;
			break;
#ifdef CROSS
		case 'r':
			root = *++argv;
			strcpy(passwdfile, root);
			strcat(passwdfile, "/etc/passwd");
			setpwfile(passwdfile);
			strcpy(groupfile, root);
			strcat(groupfile, root);
			setgrfile(groupfile);
			break;
#endif CROSS
		case '\0':
			goto out;
		default:
			error("illegal option");
		}
	}
out:
	for (; *argv; argv++) {
		have_warned = 0;
		basedir = (**argv == '/') ? "/" : curdir;
		if (chdir(basedir) < 0)
			syserror(basedir);
		init_path(*argv);
		while ((dir = next_path()) != NULL) {
			if (chdir(dir) < 0) {
				if (streq(dir, ".") || streq(dir, ".."))
					syserror(dir);
				mkdir(dir);
				if (owner != NULL || group != NULL)
					do_chown(dir, owner, group);
				if (modes != NULL)
					do_chmod(dir, modes);
				if (chdir(dir) < 0)
					syserror(dir);
			}
		}
	}
}

mkdir(dir)
char *dir;
{
	char command[MAXPATHLEN+120];
	char msg[MAXPATHLEN+200];
	int status;

	sprintf(command, "mkdir %s", dir);
	if ((status = system(command)) != 0) {
		sprintf(msg, "%s failed, status 0x%x", command, status);
		error(msg);
	}
}

static char curpath[MAXPATHLEN];
static char *pathp = NULL;

void
init_path(path)
char *path;
{
	strcpy(curpath, path);
	pathp = curpath;
}

char *
next_path()
{
	char *dirp;
	extern char *index();

	if (pathp == NULL)
		return(NULL);
	while (*pathp == '/')
		pathp++;
	dirp = pathp;
	if ((pathp = index(pathp, '/')) != NULL)
		*pathp++ = '\0';
	return(dirp);
}

static char *ms;

do_chmod(file, modes)
char *file;
char *modes;
{
	register int nm;
	struct stat st;

	/* do stat for directory arguments */
	if (lstat(file, &st) < 0)
		syserror(file);
	if ((st.st_mode&S_IFMT) == S_IFLNK && stat(file, &st) < 0)
		syserror(file);
	nm = newmode(st.st_mode, modes);
	if (chmod(file, nm) < 0)
		syswarn(file);
}

newmode(nm, modes)
	unsigned nm;
	char *modes;
{
	register o, m, b;
	int savem;

	ms = modes;
	savem = nm;
	m = abs();
	if (*ms == '\0')
		return (m);
	do {
		m = who();
		while (o = what()) {
			b = where(nm);
			switch (o) {
			case '+':
				nm |= b & m;
				break;
			case '-':
				nm &= ~(b & m);
				break;
			case '=':
				nm &= ~m;
				nm |= b & m;
				break;
			}
		}
	} while (*ms++ == ',');
	if (*--ms)
		error("invalid mode");
	return (nm);
}

abs()
{
	register c, i;

	i = 0;
	while ((c = *ms++) >= '0' && c <= '7')
		i = (i << 3) + (c - '0');
	ms--;
	return (i);
}

#define	USER	05700	/* user's bits */
#define	GROUP	02070	/* group's bits */
#define	OTHER	00007	/* other's bits */
#define	ALL	01777	/* all (note absence of setuid, etc) */

#define	READ	00444	/* read permit */
#define	WRITE	00222	/* write permit */
#define	EXEC	00111	/* exec permit */
#define	SETID	06000	/* set[ug]id */
#define	STICKY	01000	/* sticky bit */

who()
{
	register m;

	m = 0;
	for (;;) switch (*ms++) {
	case 'u':
		m |= USER;
		continue;
	case 'g':
		m |= GROUP;
		continue;
	case 'o':
		m |= OTHER;
		continue;
	case 'a':
		m |= ALL;
		continue;
	default:
		ms--;
		if (m == 0)
			m = ALL & ~um;
		return (m);
	}
}

what()
{

	switch (*ms) {
	case '+':
	case '-':
	case '=':
		return (*ms++);
	}
	return (0);
}

where(om)
	register om;
{
	register m;

 	m = 0;
	switch (*ms) {
	case 'u':
		m = (om & USER) >> 6;
		goto dup;
	case 'g':
		m = (om & GROUP) >> 3;
		goto dup;
	case 'o':
		m = (om & OTHER);
	dup:
		m &= (READ|WRITE|EXEC);
		m |= (m << 3) | (m << 6);
		++ms;
		return (m);
	}
	for (;;) switch (*ms++) {
	case 'r':
		m |= READ;
		continue;
	case 'w':
		m |= WRITE;
		continue;
	case 'x':
		m |= EXEC;
		continue;
	case 'X':
		if ((om & S_IFDIR) || (om & EXEC))
			m |= EXEC;
		continue;
	case 's':
		m |= SETID;
		continue;
	case 't':
		m |= STICKY;
		continue;
	default:
		ms--;
		return (m);
	}
}

do_chown(file, owner, group)
char *file;
char *owner;
char *group;
{
	register int c, gid, uid;
	register char *cp;
	struct group *grp;
	struct	passwd *pwd;
	struct	stat stbuf;

	gid = -1;
	if (group != NULL && *group != '\0') {
		if (!isnumber(group)) {
			if ((grp = getgrnam(group)) == NULL)
				error("unknown group: %s",group);
			gid = grp -> gr_gid;
			(void) endgrent();
		} else
			gid = atoi(group);
	}
	uid = -1;
	if (owner != NULL && *owner != '\0') {
		if (!isnumber(owner)) {
			if ((pwd = getpwnam(owner)) == NULL)
				error("unknown user id: %s",owner);
			uid = pwd->pw_uid;
		} else
			uid = atoi(owner);
	}
	/* do stat for directory arguments */
	if (lstat(file, &stbuf) < 0)
		syserror(file);
	if (gid != -1) {
		if (chown(file, -1, gid))
			syswarn(file);
	}
	if (uid != -1) {
		if (chown(file, uid, -1))
			syswarn(file);
	}
}

isnumber(s)
	char *s;
{
	register c;

	while(c = *s++)
		if (!isdigit(c))
			return (0);
	return (1);
}

error(msg, arg)
char *msg;
char *arg;
{
	fprintf(stderr, "%s: ", progname);
	fprintf(stderr, msg, arg);
	fprintf(stderr, "\n");
	exit(1);
}

syserror(msg)
char *msg;
{
	fprintf(stderr, "%s: ", progname);
	perror(msg);
	exit(1);
}

syswarn(msg)
char *msg;
{
	if (have_warned)
		return;
	have_warned++;
	fprintf(stderr, "%s: ", progname);
	perror(msg);
}

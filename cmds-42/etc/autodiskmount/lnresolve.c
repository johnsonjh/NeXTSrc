#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/param.h>

/* #define DEBUG	1 */

/*
 * lnresolve(opath)
 * if opath is a symbolic link, output it's final destination
 * otherwise output opath
 */
char *
lnresolve(opath)
char *opath;
{
	static char tgtdir[MAXPATHLEN+1];
	static char path[MAXPATHLEN+1];
	char curdir[MAXPATHLEN+1];
	char symlink[MAXPATHLEN+1];
	char file[MAXPATHLEN+1];
	struct stat st;
	char *cp;
	int cc;
	extern char *rindex();

	if (strlen(opath) > MAXPATHLEN) {
		fprintf(stderr, "path \"%s\" too long\n", opath);
		exit(1);
	}
	strcpy(path, opath);
	/*
	 * resolve last element of path until we know it's not
	 * a symbolic link
	 */
	while (lstat(path, &st) >= 0
	    && (st.st_mode & S_IFMT) == S_IFLNK
	    && (cc = readlink(path, symlink, sizeof(symlink)-1)) > 0) {
		symlink[cc] = '\0';
		if ((cp = rindex(path, '/')) != NULL && symlink[0] != '/')
			*++cp = '\0';
		else
			path[0] = '\0';
		strcat(path, symlink);
	}
	if ((cp = rindex(path, '/')) == NULL) {
		/*
		 * last element of path is only element and it
		 * now not a symbolic link, so we're done
		 */
		return(path);
	}
	/*
	 * We cheat a little bit here and let getwd() do the
	 * dirty work of resolving everything before the last
	 * element of the path
	 */
	if (getwd(curdir) == NULL) {
		fprintf(stderr, "%s\n", curdir);
		exit(1);
	}
	*cp++ = 0;
	strcpy(file, cp); /* save last element */
	if (chdir(path) < 0) {
		perror(path);
		exit(1);
	}
	if (getwd(tgtdir) == NULL) {
		fprintf(stderr, "%s\n", tgtdir);
		exit(1);
	}
	if (chdir(curdir) < 0) {
		perror(path);
		exit(1);
	}
	if (tgtdir[strlen(tgtdir)-1] != '/')
		strcat(tgtdir, "/");
	strcat(tgtdir, file);
	if (curdir[strlen(curdir)-1] != '/')
		strcat(curdir, "/");
	/*
	 * If original path was relative to curdir then
	 * try to strip our original current directory if we're
	 * still relative to it
	 */
#ifdef DEBUG
	printf("curdir=\"%s\"\n", curdir);
	printf("tgtdir=\"%s\"\n", tgtdir);
#endif DEBUG
	if (*opath != '/' && strncmp(curdir, tgtdir, strlen(curdir)) == 0) {
		cp = &tgtdir[strlen(curdir)];
		while (*cp == '/')
			cp++;
		return(cp);
	}
	return(tgtdir);
}

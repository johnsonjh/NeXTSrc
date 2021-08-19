/* dosetpath -- setpath built-in command */

#include "sh.h"

static char *syspaths[] = {"PATH","CPATH","LPATH","MPATH","EPATH",0};
#define LOCALSYSPATH	"/usr/local"

int dosetpath (arglist)
char **arglist;
{
	char aline[2000],*ap,*val,*paths[50],*cmds[50];
	char *append();
	char *getenv();
	int npaths,narg,i,ncmds,sysflag;

	ap = aline;
	npaths = 0;
	for (narg=1; arglist[narg] && arglist[narg][0] != '-'; narg++) {
		val = getenv (arglist[narg]);
		if (val == 0)  val = "";
		paths[npaths] = ap;
		ap = append (ap,arglist[narg],0);
		ap = append (ap,val,'=');
		*ap++ = '\0';
		npaths++;
	}
	sysflag = 0;
	if (npaths == 0) {	/* no pathnames specified */
		for (i=0; syspaths[i]; i++) {
			val = getenv (syspaths[i]);
			if (val == 0)  val = "";
			paths[npaths] = ap;
			ap = append (ap,syspaths[i],0);
			ap = append (ap,val,'=');
			*ap++ = '\0';
			npaths++;
		}
		sysflag++;
	}
	paths[npaths] = 0;
	ncmds = 0;
	while (arglist[narg]) {
		val = globone (arglist[narg]);
		if (val == 0)  return;
		cmds[ncmds] = val;
		ncmds++;
		narg++;
	}
	cmds[ncmds] = 0;
	if (setpath(paths, cmds, LOCALSYSPATH, sysflag, 1) < 0)
		return;
	for (i=0; i<npaths; i++) {
		for (val=paths[i]; val && *val && *val != '='; val++) ;
		if (val && *val == '=') {
			*val++ = '\0';
			setenv (paths[i],val);
			if (strcmp(paths[i],"PATH") == 0) {
				importpath (val);
				if (havhash) dohash();
			}
			*--val = '=';
		}
		xfree(paths[i]);
	}
}

char *append (to,from,separator)
char *to,*from,separator;
{
	register char *t,*f;
	t = to;
	f = (from ? from : "");
	if (separator) *t++ = separator;
	while (*t++ = *f++) ;
	return (t-1);
}

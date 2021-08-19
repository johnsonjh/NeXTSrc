/*  openp, fopenp  --  search pathlist and open file
 *
 *  Usage:
 *	i = openp (path,file,complete,flags,mode)
 *	f = fopenp (path,file,complete,type)
 *	int i,flags,mode;
 *	FILE *f;
 *	char *path,*file,*complete,*type;
 *
 *  Openp searches for "file" in the pathlist "path";
 *  when the file is found and can be opened by open()
 *  with the specified "flags" and "mode", then the full filename
 *  is copied into "complete" and openp returns the file
 *  descriptor.  If no such file is found, openp returns -1.
 *  Fopenp performs the same function, using fopen() instead
 *  of open() and type instead of flags/mode; it returns 0 if no
 *  file is found.
 *
 *  HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX.  Added new parameter to openp.c;
 *	changed names of flags, mode, and type parameters to reflect
 *	current manual entries for open and fopen.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

#include <stdio.h>

int open();
int searchp();

static int flgs,mod,value;
static char *ftyp;
static FILE *fvalue;

static int func (fnam)
char *fnam;
{
	value = open (fnam,flgs,mod);
	return (value < 0);
}

static int ffunc (fnam)
char *fnam;
{
	fvalue = fopen (fnam,ftyp);
	return (fvalue == 0);
}

int openp (path,file,complete,flags,mode)
char *path, *file, *complete;
int flags, mode;
{
	register char *p;
	flgs = flags;
	mod = mode;
	if (searchp(path,file,complete,func) < 0)  return (-1);
	return (value);
}

FILE *fopenp (path,file,complete,ftype)
char *path, *file, *complete, *ftype;
{
	register char *p;
	ftyp = ftype;
	if (searchp(path,file,complete,ffunc) < 0)  return (0);
	return (fvalue);
}

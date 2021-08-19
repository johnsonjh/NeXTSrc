/*  fwantwrite  --  attempt to open file for output
 *
 *  Usage:  f = fwantwrite (path,file,fullname,prompt,warn);
 *    FILE *f;
 *    int warn;
 *    char *path,*file,*fullname,*prompt;
 *
 *  Fwantwrite will attempt to open "file" for output somewhere in
 *  the pathlist "path".  If no such file can be created, the user
 *  is given an oportuity to enter a new file name (after the
 *  prompt is printed).  The new file will then be sought using
 *  the same path.
 *  If the path is the null string, the file will be created with
 *  no prefix to the file name.  If the file name is null, the
 *  user will be prompted for a file name immediately.  The user
 *  can always abort fwantwrite by typing just a carriage return
 *  to the prompt.
 *  Fwantwrite will put the name of the successfully created file
 *  into the "fullname" string (which must therefore be long enough to
 *  hold a complete file name), and return its file pointer;
 *  if no file is created, 0 will be returned.
 *  Fwantwrite examines each entry in the path, to see if the
 *  desired file can be created there.  If "warn" is true, 
 *  fwantwrite will first check to ensure that no such file
 *  already exists (if it does, the user is allowed to keep it).
 *  If no such file exists (or the user wants to delete it), then
 *  fwantwrite attempts to create the file.  If it is unsuccessful,
 *  the next entry in the pathlist is examined in the same way.
 *
 *  HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX:  Changed output messages to stderr.
 *
 * 21-Oct-81  Fil Alleva (faa) at Carnegie-Mellon University
 *	Fixed bug which caused an infinite loop when getstr() got
 *	an EOT error and returned NULL. The error return was ignored
 *	and the value of "answer" was not changed which caused the loop.
 *
 * 28-Aug-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug which used "file" instead of "myfile" in call to
 *	searchp.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Created for VAX.
 *
 */

#include <stdio.h>

int strcmp();
FILE *fopen();
int searchp();
char *getstr();

static int warnflag;
static FILE *filptr;

static int func (fnam)
char *fnam;
{		/* attempt to create fnam */
	register int goahead;
	register int fildes;

	goahead = 1;
	fflush (stdout);
	if (warnflag) {
		fildes = open (fnam,1);
		if (fildes >= 0) {
			close (fildes);
			fprintf (stderr,"%s already exists!  ",fnam);
			goahead = getbool ("Delete old file?",0);
		}
	}
	if (goahead) {
		filptr = fopen (fnam,"w");
		if (filptr == 0) {
			goahead = 0;
		}
	}
	return (!goahead);
}

FILE *fwantwrite (path,file,fullname,prompt,warn)
char *path,*file,*fullname,*prompt;
int warn;
{
	register int i;
	char myfile[200], *retval;

	if (*file == '\0') {
		getstr (prompt,"no file",myfile);
		if (strcmp(myfile,"no file") == 0)  return (0);
	}
	else strcpy (myfile,file);

	warnflag = warn;
	do {
		i = searchp (path,myfile,fullname,func);
		if (i < 0) {
			if (*path && (*myfile != '/')) {
				fprintf (stderr,"%s in path \"%s\":  Can't create.\n",myfile,path);
			} 
			else {
				perror (myfile);
			}
			retval = getstr (prompt,"no file",myfile);
			if ((strcmp(myfile,"no file") == 0) || retval == NULL)
			    return (0);
		}
	} 
	while (i < 0);

	return (filptr);
}

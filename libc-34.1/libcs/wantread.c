/*  wantread  --  attempt to open file for input
 *
 *  Usage:  i = wantread (path,file,fullname,prompt);
 *	int i;
 *	char *path,*file,*fullname,*prompt;
 *
 *  Wantread will search through the specified path for the
 *  specified file, attempting to open it for input if it is
 *  found.  If no such file is found, the user is given
 *  an opportunity to enter a new file name (after the prompt is
 *  printed).  The new file will then be sought using the same
 *  path.
 *  If the path is the null string, the file will be searched for
 *  with no prefix.  If the file name is null, the user will be
 *  prompted for a file name immediately.  The user can always
 *  abort wantread by typing just a carriage return to the prompt.
 *  Wantread will put the name of the successfully opened file into
 *  the "fullname" string (which therefore must be long enough to hold a
 *  complete file name), and return its file descriptor; if no file
 *  is successfully found, -1 is returned.
 *
 *  HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for 4.2 BSD UNIX:  New openp routine to conform to new open call.
 *
 * 21-Oct-81  Fil Alleva (faa) at Carnegie-Mellon University
 *	Fixed bug which caused an infinite loop when getstr() got
 *	an EOT error and returned NULL. The error return was ignored
 *	and the value of "answer" was not changed which caused the loop.
 *
 * 28-Aug-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Fixed bug which used the "file" string to hold name typed at terminal
 *	even though "file" may have been passed as a constant.
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten for VAX.
 *
 */

#include <stdio.h>
#include <sys/file.h>

int strcmp(),openp();
char *getstr();

int wantread (path,file,fullname,prompt)
char *path,*file,*fullname,*prompt;
{
	register int value;
	char myfile[2000], *retval;

	fflush (stdout);
	if (*file == '\0') {
		getstr (prompt,"no file",myfile);
		if (strcmp(myfile,"no file") == 0)  return (-1);
	}
	else
	    strcpy(myfile, file);

	do {
		value = openp (path,myfile,fullname,O_RDONLY,0);
		if (value < 0) {
			if (*path && (*myfile != '/')) {
				sprintf (fullname,"Want to read %s in path \"%s\"",myfile,path);
			}
			else {
				sprintf (fullname,"Want to read %s",myfile);
			}
			perror (fullname);
			retval = getstr (prompt,"no file",myfile);
			if ((strcmp(myfile,"no file") == 0) || retval == NULL)
			    return (-1);
		}
	} 
	while (value < 0);

	return (value);
}

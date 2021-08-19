/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>
#include <stdlib.h>
#include <sys/file.h>
#include <errno.h>

#undef tmpnam
char *tmpnam(char *tfn) {
	register unsigned int pid;
	register int fd, i;
	static char tmpfilename[14];
	FILE *iop;
	char *s;

	pid = getpid();
	strcpy(tmpfilename, "/tmp/t.XXXXXX");
	s = &tmpfilename[13];
	while (*--s == 'X') {
		*s = (pid % 10) + '0';
		pid /= 10;
	}
	s++;
	i = 'a';
	while ((fd = access(tmpfilename, F_OK)) == -1) {
		if (errno == ENOENT)
			break;
		else if (i == 'z')
			return NULL;
		else
			*s = i++;
	}
	if (tfn != (char *)NULL)  {
		strcpy (tfn, tmpfilename);
		return (tfn);
	}
	else
		return (tmpfilename);
}



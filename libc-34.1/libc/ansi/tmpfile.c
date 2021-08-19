/* Copyright (c) 1988 NeXT, Inc. - 9/12/88 CCH */

#include <stdio.h>
#include <stdlib.h>

#include <sys/file.h>

extern FILE *_findiop();

#undef tmpfile
FILE *tmpfile(void) {
	register char *s;
	register unsigned int pid;
	register int fd, i;
	char tmpfilename[14];
	FILE *iop;

	pid = getpid();
	strcpy(tmpfilename, "/tmp/u.XXXXXX");
	s = &tmpfilename[13];
	while (*--s == 'X') {
		*s = (pid % 10) + '0';
		pid /= 10;
	}
	s++;
	i = 'a';
	while ((fd = open(tmpfilename, O_CREAT|O_EXCL|O_RDWR, 0600)) == -1) {
		if (i == 'z')
			return NULL;
		*s = i++;
	}
	iop = _findiop();
	if (iop == NULL)
		return (NULL);

	iop->_cnt = 0;
	iop->_file = fd;
	iop->_bufsiz = 0;
	iop->_base = iop->_ptr = NULL;

	iop->_flag = _IORW;

	remove (tmpfilename);
	return (iop);
}


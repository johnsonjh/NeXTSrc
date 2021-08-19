#if defined(LIBC_SCCS) && !defined(lint)
static char sccsid[] = "@(#)ungetc.c	5.3 (Berkeley) 3/26/86";
#endif LIBC_SCCS and not lint

#if	NeXT
#include <stdlib.h>
#endif	NeXT
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>

ungetc(c, iop)
	register FILE *iop;
{
#if	NeXT
	if (c == EOF || (iop->_flag & (_IOREAD|_IORW)) == 0)
		return (EOF);
	if (iop->_base==NULL) {
		int size;
		struct stat stbuf;

		if (iop->_flag&_IONBF) {
			iop->_base = &iop->_smallbuf;
			size = 1;
		} else {
			if (fstat(fileno(iop), &stbuf) < 0 || stbuf.st_blksize <= 0)
				size = BUFSIZ;
			else
				size = stbuf.st_blksize;
			if ((iop->_base = malloc(size)) == NULL) {
				iop->_flag |= _IONBF;
				iop->_base = &iop->_smallbuf;
				size = 1;
			} else {
				iop->_flag |= _IOMYBUF;
			}
		}
		iop->_bufsiz = size;
		iop->_ptr = iop->_base;
		iop->_cnt = 0;
	}
#else
	if (c == EOF || (iop->_flag & (_IOREAD|_IORW)) == 0 ||
	    iop->_ptr == NULL || iop->_base == NULL)
		return (EOF);
#endif	NeXT
	if (iop->_ptr == iop->_base)
		if (iop->_cnt == 0)
			iop->_ptr++;
		else
			return (EOF);

	iop->_cnt++;
#if	NeXT
	if (*--iop->_ptr != (char)c)
		*iop->_ptr = c;
#else
	*--iop->_ptr = c;
#endif	NeXT

	return (c);
}

#ifndef lint
static char sccsid[] = "@(#)gename.c	5.6 (Berkeley) 10/9/85";
#endif

#include "uucp.h"

#define SEQLEN 4
#define SLOCKTIME 10L
#define SLOCKTRIES 5
/*
 * the alphabet can be anything, but if it's not in ascii order,
 * sequence ordering is not preserved
 */
static char alphabet[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

#ifdef BSD4_2
#include <sys/file.h>
#endif BSD4_2

/*LINTLIBRARY*/

/*
 *	generate file name
 */
gename(pre, sys, grade, file)
char pre, *sys, grade, *file;
{
	register int i, fd;
	static char snum[5];
	static char *lastchar = NULL;

	if (lastchar == NULL || (snum[SEQLEN-1] = *(lastchar++)) == '\0') {
#ifndef BSD4_2
		for (i = 0; i < SLOCKTRIES; i++) {
			if (!ulockf(SEQLOCK, SLOCKTIME))
				break;
			sleep(5);
		}

		if (i >= SLOCKTRIES) {
			logent(SEQLOCK, "CAN NOT LOCK");
			goto getrandseq;
		}
#endif !BSD4_2

		if ((fd = open(SEQFILE, 2)) >= 0) {
			register char *p;
#ifdef BSD4_2
			flock(fd, LOCK_EX);
#endif !BSD4_2
			read(fd, snum, SEQLEN);
			/* increment the penultimate character */
			for (i = SEQLEN - 2; i >= 0; --i) {
				if ((p = index(alphabet, snum[i])) == NULL)
					goto getrandseq;
				if (++p < &alphabet[sizeof alphabet - 1]) {
					snum[i] = *p;
					break;
				} else		/* carry */
					snum[i] = alphabet[0];	/* continue */
			}
			snum[SEQLEN-1] = alphabet[0];
		} else {
			extern int errno;
#ifdef NeXT_MOD
			int oldumask;
			oldumask = umask(0);
			fd = open(SEQFILE, O_CREAT, 0600);
			umask(oldumask);
#else
			fd = creat(SEQFILE, 0666);
#endif
getrandseq:		srand((int)time((time_t *)0));
#ifdef NeXT_MOD
			if(fd < 0)
				assert(SEQFILE, "is missing or trashed\n", errno);
			else
				logent(SEQFILE, "CREATED");
#else
			assert(SEQFILE, "is missing or trashed\n", errno);
#endif
			for (i = 0; i < SEQLEN; i++)
				snum[i] = alphabet[rand() % (sizeof alphabet - 1)];
			snum[SEQLEN-1] = alphabet[0];
		}

		if (fd >= 0) {
			lseek(fd, 0L, 0);
			write(fd, snum, SEQLEN);
			close(fd);
		}
#ifndef BSD4_2
		rmlock(SEQLOCK);
#endif !BSD4_2
		lastchar = alphabet + 1;
	}
	sprintf(file,"%c.%.*s%c%.*s", pre, SYSNSIZE, sys, grade, SEQLEN, snum);
	DEBUG(4, "file - %s\n", file);
}

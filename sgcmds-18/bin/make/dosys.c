#include "defs.h"
#include <signal.h>
#include <sys/wait.h>
#ifndef sigmask
#define sigmask(m)	(1 << ((m)-1))
#endif

FSTATIC int waitpid = 0;
FSTATIC int waitmsk = 0;


int
dosys(comm, nohalt)
	register char *comm;
	int nohalt;
{
	register int status;

#ifdef NeXT_MOD
	/*
	 * Can't cache archive opens once we do a command, it may
	 * unlink the archive and possibly rebuild it!
	 */
	purgearch();
#endif NeXT_MOD
	if (metas(comm))
		doshell(comm, nohalt);
	else
		doexec(comm);
	waitmsk = sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT));
	status = await(nohalt);
	(void) sigsetmask(waitmsk);
	return status;
}


/*
 * Are there are any Shell meta-characters?
 */
int
metas(s)
	register char *s;
{
	register char c;

	while ((funny[c = *s++] & META) == 0)
		;
	return c;
}


doshell(comm, nohalt)
	char *comm;
	int nohalt;
{
	(void) fflush(stdout);
	(void) fflush(stderr);
	switch (waitpid = vfork()) {
	case -1:
		fatal("Cannot fork");
	case 0:
		enbint(SIG_DFL);
		shexec(comm, nohalt);
	}
}


#define MAXARGV	1000

doexec(comm)
	register char *comm;
{
	register char *t;
	register char **p;
	char *argv[MAXARGV];

	while (*comm == ' ' || *comm == '\t')
		++comm;
	if (*comm == 0)
		fatal("no command");

	p = argv;
	for (t = comm; *t; ) {
		if (p >= argv + MAXARGV)
			fatal1("%s: Too many arguments.", comm);
		*p++ = t;
		while (*t && *t != ' ' && *t != '\t')
			++t;
		if (*t)
			for (*t++ = 0; *t== ' ' || *t == '\t'; ++t)
				;
	}
	*p = 0;

	(void) fflush(stdout);
	(void) fflush(stderr);
	switch (waitpid = vfork()) {
	case -1:
		fatal("Cannot fork");
	case 0:
		enbint(SIG_DFL);
		execvp(*argv, argv);
		fatal1("Cannot load %s", *argv);
	}
}


shexec(comm, nohalt)
	char *comm;
	int nohalt;
{
#ifdef SHELLENV
	char *shellcom, *shellstr;
	extern char *getenv();

	if ((shellcom = getenv("SHELL")) == 0)
		shellcom = SHELLCOM;
	if ((shellstr = rindex(shellcom, '/')) == 0)
		shellstr = shellcom;
	else
		shellstr += 1;
	execl(shellcom, shellstr, nohalt ? "-c" : "-ce", comm, (char *)0);
#else
	execl(SHELLCOM, "sh", nohalt ? "-c" : "-ce", comm, (char *)0);
#endif
	fatal("Couldn't load Shell");
}


int
await(nohalt)
	int nohalt;
{
	register int pid;
	union wait status;
	unsigned int sig;
	extern char *sys_siglist[];

	while ((pid = wait(&status)) != waitpid)
		if (pid == -1)
			fatal("bad wait code");
	waitpid = 0;
	if (status.w_status) {
		if (sig = status.w_termsig) {
			if (sig < NSIG && sys_siglist[sig] && *sys_siglist[sig])
				printf("*** %s", sys_siglist[sig]);
			else
				printf("*** Signal %d", sig);
			if (status.w_coredump)
				printf(" - core dumped");
		} else
			printf("*** Exit %d", status.w_retcode);
		if (nohalt)
			printf(" (ignored)\n");
		else if (keepgoing)
			printf("\n");
		else
			fatal((char *) 0);
		(void) fflush(stdout);
	}
	return status.w_status;
}


touch(force, name)
	int force;
	char *name;
{
	register int fd;
	struct stat stbuff;
	char junk[1];
	extern long lseek();

	if (stat(name, &stbuff) == -1) {
		if (force)
			goto create;
		fprintf(stderr, "touch: file %s does not exist.\n", name);
		return;
	}
	if (stbuff.st_size == 0)
		goto create;

	if ((fd = open(name, 2)) == -1)
		goto bad;

	if (read(fd, junk, 1) != 1
	|| lseek(fd, 0L, 0) != 0L
	|| write(fd, junk, 1) != 1) {
		(void) close(fd);
		goto bad;
	}
	(void) close(fd);
	return;

bad:
	fprintf(stderr, "Cannot touch %s\n", name);
	return;

create:
	if ((fd = creat(name, 0666)) == -1)
		goto bad;
	(void) close(fd);
}


FILE *
pfopen(comm, nohalt)
	char *comm;
	int nohalt;
{
	FILE *f;
	int fds[2];

	if (pipe(fds) == -1)
		fatal("Cannot make pipe");
	(void) fflush(stdout);
	(void) fflush(stderr);
	switch (waitpid = vfork()) {
	case -1:
		fatal("Cannot fork");
	case 0:
		enbint(SIG_DFL);
		(void) close(fds[0]);
		(void) dup2(fds[1], 1);
		(void) close(fds[1]);
		shexec(comm, nohalt);
	}
	(void) close(fds[1]);
	if ((f = fdopen(fds[0], "r")) == NULL)
		fatal("Cannot allocate file structure");
	waitmsk = sigblock(sigmask(SIGHUP)|sigmask(SIGINT)|sigmask(SIGQUIT));
	return f;
}


int
pfclose(f, nohalt)
	FILE *f;
	int nohalt;
{
	register int status;

	(void) fclose(f);
	status = await(nohalt);
	(void) sigsetmask(waitmsk);
	return status;
}

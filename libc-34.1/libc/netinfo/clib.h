/*
 * Standard libc stuff used by source files in this directory
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>

int _filbuf(FILE *);
void _exit(int);
void abort(void);
int access(char *, int);
int chdir(char *);
int close(int);
char *crypt(char *, char *);
int dup(int);
int dup2(int, int);
int execv(char *, char **);
int fcntl(int, int, int);
int fork(void);
int fsync(int);
int ftruncate(int, off_t);
int getdtablesize(void);
int gethostname(char *, unsigned);
int getpid(void);
int getuid(void);
unsigned long htonl(unsigned long);
unsigned short htons(unsigned short);
unsigned long inet_addr(char *);
char *inet_ntoa(struct in_addr);
int ioctl(int, int, void *);
enum clnt_stat many_cast_args(unsigned,
			      struct in_addr *, u_long, u_long, u_long,
			      bool_t (*)(), void *, unsigned, bool_t (*)(), 
			      void *,
			      bool_t (*)(), int);
int mkdir(char *, int);
int open(char *, int, int);
int pipe(int *);
int pmap_unset(unsigned, unsigned);
unsigned short pmap_getport();
int random(void);
int read(int, char *, int);
int rmdir(char *);
int select(unsigned, fd_set *, fd_set *, fd_set *, struct timeval *);
int sleep(int);
#ifdef notdef
bool_t svcerr_systemerr(SVCXPRT *);
#endif
void syslog(int, char *, ...);
int unlink(char *);
int write(int, char *, int);
bool_t xdr_free(xdrproc_t, void *);
int kill(int, int);
int wait(union wait *);

#include <stdio.h>
#include <string.h>

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
int fork(void);
int fsync(int);
int ftruncate(int, off_t);
int getdtablesize(void);
int gethostname(char *, unsigned);
int getpid(void);
unsigned long htonl(unsigned long);
unsigned short htons(unsigned short);
int ioctl(int, int, void *);
enum clnt_stat many_cast_args(unsigned,
			      struct in_addr *, u_long, u_long, u_long,
			      bool_t (*)(), void *, unsigned, bool_t (*)(), 
			      void *,
			      bool_t (*)());
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
bool_t svcerr_systemerr(SVCXPRT *);
void syslog(int, char *, ...);
int unlink(char *);
int write(int, char *, int);
bool_t xdr_free(xdrproc_t, void *);
int kill(int, int);
int wait(union wait *);
char *ether_ntoa(struct ether_addr *);
struct ether_addr ether_aton(char *);
char *inet_ntoa(struct in_addr);
unsigned long inet_addr(char *);

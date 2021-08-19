#include <stdio.h>

#ifdef	__STRICT_BSD__
#include <strings.h>
#else
#include <string.h>
#include <stdlib.h>
#include <time.h>
#endif

#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/resource.h>
#include <sys/param.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/time_stamp.h>
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/socket.h>
#include <sys/vfs.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <machine/vm_types.h>
#include <machine/boolean.h>
#include <machine/kern_return.h>

/*
 * The above include files should be idempotent. We don't ifndef LIBC_H
 * around them to make sure that they are indeed idempotent.
 */
#ifndef LIBC_H
#define LIBC_H

extern char	**environ;
/*
 * UNIX system calls.
 */

extern int	sethostid(long hostid), gethostid(void);
extern int	sethostname(const char *, int), gethostname(char *, int);
extern int	getpid(void);
extern int	setdomainname(const char *, int), getdomainname(char *, int);
extern int	fork(void);
extern int	vfork(void);
extern int	execve (char *name, char **argv, char **envp);
extern int	wait(union wait *), wait3(union wait *, int, struct rusage *);
extern uid_t	getuid(void), geteuid(void);
extern gid_t	getgid(void), getegid(void);
extern int	setreuid(int, int), setregid();
extern int	getgroups(int, int *),setgroups(int, int *);
extern int	getpgrp(int), setpgrp(int, int);

extern int	getpagesize(void);

extern int	sigvec(int , struct sigvec *, struct sigvec *);
extern int	sigblock(int), sigsetmask(int);
extern int	sigpause(int), sigstack(struct sigstack *, struct sigstack *);
extern int	kill(int, int), killpg(int, int);

extern int	gettimeofday(struct timeval *, struct timezone *);
extern int	settimeofday(struct timeval *, struct timezone *);
extern int	getitimer(int, struct itimerval *);
extern int	setitimer(int, struct itimerval *, struct itimerval *);
extern int 	adjtime(struct timeval *, struct timeval *);

extern int	getdtablesize(void), dup(int), dup2(int, int), close(int);
extern int	select(int, fd_set *, fd_set *, fd_set *, struct timeval *);
extern int	fcntl(int, int, int), flock(int, int);

extern int	getpriority(int, int), setpriority(int, int, int);
extern int	getrusage(int, struct rusage *);
extern int	getrlimit(int, struct rlimit *), setrlimit(int, struct rlimit *);

extern int	unmount(const char *), mount(int, const char *, int, caddr_t);
extern int	sync(void), reboot(int);

extern int	read(int, void *, int), write(int, const void *, int);
extern int	readv(int, struct iovec *, int), writev(int, struct iovec *, int);
extern int	ioctl(int, long, ...);

extern int	chdir(const char *), chroot(const char *);
extern int	mkdir(const char *, int), rmdir(const char *);
extern int	getdirentries(int, char *, int, long *);
extern int	creat(const char *, int), open(const char *, int, ...);
extern int	mknod(const char *, int, int);
extern int	unlink(const char *), stat(const char *, struct stat *);
extern int	fstat(int, struct stat *), lstat(const char *, struct stat *);
extern int	chown(const char *, int, int), fchown(int, int, int);
extern int	chmod(const char *, int), fchmod(int, int);
extern int	utimes(const char *, struct timeval [2]);
extern int	link(const char *, const char *), symlink(const char *, const char *);
extern int	readlink(const char *, char *, int);
extern int	rename(const char *, const char *);
extern off_t	lseek(int, off_t, int);
extern int	truncate(const char *, off_t), ftruncate(int, off_t);
extern int	access(const char *, int), fsync(int);
extern int	statfs(const char *, struct statfs *), fstatfs(int, struct statfs *);

extern int	socket(int, int, int), bind(int, struct sockaddr *, int);
extern int	listen(int, int), accept(int, struct sockaddr *, int *);
extern int	connect(int, struct sockaddr *, int);
extern int	socketpair(int, int, int, int [2]);
extern int	sendto(int, void *, int, int, struct sockaddr *, int);
extern int	send(int, void *, int, int);
extern int	recvfrom(int, void *, int, int, struct sockaddr *, int *);
extern int	recv(int, void *, int, int);
extern int	sendmsg(int, struct msghdr *, int);
extern int	recvmsg(int, struct msghdr *, int);
extern int	shutdown(int, int);
extern int	setsockopt(int, int, int, void *, int);
extern int	getsockopt(int, int, int, void *, int *);
extern int	getsockname(int, struct sockaddr *, int *);
extern int	getpeername(int, struct sockaddr *, int *);
extern int	pipe(int [2]);

extern int	umask(int);
extern void	_exit(int);

extern kern_return_t map_fd (int fd, vm_offset_t offset, 
	vm_offset_t *addr, boolean_t find_space, vm_size_t numbytes);
extern int	profile (char *, int, int, int);

/*
 *	Other libc entry points.
 */

extern char	*crypt(char *, char *);
extern void	syslog(int, char *, ...);

/*
 * Organize these respective to manpage in /usr/man/man3.
 */
extern unsigned long inet_addr(char *cp);

extern int ruserok (char *rhost, int superuser, char *ruser, char *luser);





extern void setbuf (FILE *stream, char *buf);
extern int setbuffer (FILE *stream, char *buf, int size);
extern int setlinebuf (FILE *stream);

/* abort(3) 	*/
#ifdef	__GNUC__
extern volatile void abort(void);
#else
extern void	abort(void);
#endif	__GNUC__
/* abs(3)	*/
extern int abs (int i);
/* addr(3n)	*/
extern unsigned long inet_addr (char *cp);
extern unsigned long inet_network (char *cp);
extern char *inet_ntoa (struct in_addr in);
extern struct in_addr inet_makeaddr (int net, int lna);
extern int inet_lnaof (struct in_addr in);
extern int inet_netof (struct in_addr in);
/* alarm(3c)	*/
extern int alarm (unsigned int seconds);
/* atof(3)	*/
/* double atof (char *nptr); */
#ifdef	__STRICT_BSD__
extern int atoi (char *nptr);
extern long atol (char *nptr);
#endif	__STRICT_BSD__
/* atoh(3)	*/
extern unsigned int atoh (char *ap);
extern unsigned int atoo (char *ap);
int ffs (int i);
/* byteorder (3n) */

#ifndef htonl
extern unsigned long htonl (unsigned long hostlong);
#endif
#ifndef htons
extern unsigned short htons (unsigned short hostshort);
#endif
#ifndef nthol
extern unsigned long ntohl (unsigned long netlong);
#endif
#ifndef ntohs
extern unsigned short ntohs (unsigned short netshort);
#endif

/* convert_ts_to_tv(3) */
extern void convert_ts_to_tv (int ts_format, struct tsval *tsp, struct timeval *tvp);
/* crypt(3)	*/
extern char *crypt (char *key, char *salt);
extern void setkey (char *key);
extern void encrypt (char *block, int edflag);
/* ecvt(3)	*/
extern char *ecvt (double value, int ndigit, int *decpt, int *sign);
extern char *fcvt (double value, int ndigit, int *decpt, int *sign);
extern char *gcvt (double value, int ndigit, char *buf);
/* execl(3)	*/
extern int execv(const char *, const char **);
extern int execl(const char *, ...);
extern int execvp(const char *, const char **);
extern int execlp(const char *, ...);
extern int execle(const char *, ...);
extern int exect(const char *, const char **, const char **);
/* exit(3)	*/
#ifdef	__STRICT_BSD__
extern void	exit(int);
/* getenv(3)	*/
extern char *getenv (char *name);
/* system(3)	*/
extern int system (char *string);
#endif	__STRICT_BSD__
/* getlogin(3)	*/
extern char *getlogin(void);
/* getmachheaders(3)	*/
extern struct mach_header **getmachheaders(void);
/* getopt(3)	*/
extern int getopt (int argc, char **argv, char *optstring);
extern char *optarg;
extern int optind;
/* getpass(3)	*/
extern char *getpass (char *prompt);
/* getpw(3)	*/
extern char getpw(int uid, char *buf);
/* getsecbyname(3)	*/
extern const struct section *getsectbyname(
	const char *segname, 
	const char *sectname);
extern char *getsectdata(
	const char *segname, 
	const char *sectname, 
	int *size);
extern const struct section *getsectbynamefromheader(
	const struct mach_header *mhp,
	const char *segname,
	const char *sectname);
extern char *getsectdatafromheader(
	const struct mach_header *mhp,
	const char *segname,
	const char *sectname,
	int *size);
extern char *getsectdatafromlib(
	const char *libname, 
	const char *segname, 
	const char *sectname,
	int *size);
/* getsegbyname(3)	*/
extern const struct segment_command *getsegbyname(const char *segname);
/* getusershell(3)	*/
extern char *getusershell(void);
extern void setusershell(void);
extern void endusershell(void);
/* getwd(3)	*/
extern char *getwd (char *pathname);
/* initgroups(3)	*/
extern int initgroups (char *name, int basegid);
/* insque (3)	*/
struct qelem {
	struct qelem *q_forw;
	struct qelem *q_back;
	char *q_data;
};
extern void insque (struct qelem *elem, struct qelem *pred);
extern void remque (struct qelem *elem);
/* mktemp(3)	*/
extern int mkstemp (char *template);
extern char *mktemp (char *template);
/* monitor(3)	*/
extern void monitor (char *lowpc, char *highpc, char *buf, int bufsiz, int cntsiz);
extern void monstartup (char *lowpc, char *highpc);
extern void moncontrol (int mode);
/* nice(3)	*/
extern int nice(int incr);
/* pause(3)	*/
extern void pause(void);
/* perror(3)	*/
extern void perror (const char *s);
/* psignal(3)	*/
extern void psignal (unsigned int sig, char *string);
/* */
extern int putpwpasswd (char *login, char *cleartext, char *encrypted);
/* rand(3c)	*/
extern int rand(void);
/* random(3)	*/
extern long random(void);
extern void srandom (int seed);
extern char *initstate (unsigned int seed, char *state, int n);
extern char *setstate (char *state);
/* rcmd(3)	*/
extern int rcmd (char **ahost, int inport, char *locuser, char *remuser,
	  char *cmd, int *fd2p);
extern int rresvport (int *port);
extern int ruserok (char *rhost, int superuser, char *ruser, char *luser);
/* rexec(3)	*/
extern int rexec (char **ahort, int inport, char *user, char *passwd, 
	   char *cmd, int *fd2p);	 
/* setuid(3)	*/
extern int setuid (uid_t uid);
extern int seteuid (uid_t euid);
extern int setruid (uid_t ruid);
extern int setgid (gid_t gid);
extern int setegid (gid_t egid);
extern int setrgid (gid_t rgid);
/* siginterrupt(3) */
extern int siginterrupt (int sig, int flag);
/* sleep(3)	*/
extern int	sleep(unsigned int seconds);
extern int	sleep(unsigned int useconds);
/* stty(3c)	*/
extern int stty (int fd, struct sgttyb *buf);
extern int gtty (int fd, struct sgttyb *buf);
/* swab(3)	*/
extern void swab (char *from, char *to, int nbytes);
/* time(3c)	*/
extern long time (long *tloc);
/* times(3c)	*/
extern int times (struct tms *buffer);
/* ttyname(3)	*/
extern int ttyslot (void);
extern int isatty (int filedes);
extern char *ttyname (int filedes);
/* ualarm(3)	*/
extern unsigned ualarm (unsigned value, unsigned interval);
/* usleep(3)	*/
extern void usleep (unsigned int useconds);
/* utime(3c)	*/
extern int utime (char *file, time_t timep[2]);
/* valloc(3c)	*/
/* vlimit(3c)	*/
extern int vlimit(int resource, int value);
 
#endif



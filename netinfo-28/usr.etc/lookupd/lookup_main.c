/*
 * lookupd main
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <mach.h>
#include <sys/message.h>
#include <mig_errors.h>
#include <sys/file.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/signal.h>
#include <sys/wait.h>
#include <syslog.h>
#include <servers/bootstrap.h>
#include "lookup.h"
#include "clib.h"

#define TIMEOUT_MSECONDS (2000) /* 2 second timeout on sends) */
#define MSECS_PER_MIN (1000 * 60)

#define CACHE_TIMEOUT_MINS 30

#define CACHE_TIMEOUT_MSECS (CACHE_TIMEOUT_MINS * MSECS_PER_MIN)

static void recomputelists(void);
static void detach(void);

extern kern_return_t __lookup_link(port_t, const char *, int *);
extern int lookup_getpwent(unsigned, char *, unsigned *, char **);
extern int lookup_alias_getent(unsigned, char *, unsigned *, char **);
extern int checksum_unchanged(void);

void parentexit(int);
void catch(int);
void goodbye(int);
int msectime(void);

/*
 * The size of the largest message we expect to handle
 */
typedef struct lookup_msg {
	msg_header_t head;
	msg_type_t itype;
	int i;
	msg_type_t dtype;
	inline_data data;
} lookup_msg;
extern kern_return_t lookup_server(lookup_msg *, lookup_msg *);

extern port_t bootstrap_port;

static int nfork;

int
main(int argc, char **argv)
{
	lookup_msg request_msg;
	lookup_msg reply_msg;
	port_t port;
	kern_return_t ret;
	int pid;
	int timeout;
	int when;
	int mins;
	struct rlimit rlim;
	int std_timeout = CACHE_TIMEOUT_MSECS;
	boolean_t docaching = TRUE;

	if (argc > 2 && strcmp(argv[1], "-m") == 0) {
		mins = atoi(argv[2]);
		if (mins > 0) {
			std_timeout = mins * MSECS_PER_MIN;
		} else {
			docaching = FALSE;
		}
	}
#ifndef DEBUG
	if ((pid = fork()) > 0) {
		(void)signal(SIGTERM, parentexit);
		for (;;) {
			sleep(1);
		}
	}
#endif
#ifdef MALLOC_DEBUG
	{
		extern void catch_malloc_problems(int);

		(void)malloc_error(catch_malloc_problems);
		malloc_debug(31);
	}
#endif
	openlog("lookupd", LOG_PID, LOG_DAEMON);

	rlim.rlim_cur = rlim.rlim_max = RLIM_INFINITY;
	setrlimit(RLIMIT_CORE, &rlim);
	ret = port_allocate(task_self(), &port);
	if (ret != KERN_SUCCESS) {
		exit(1);
	}
	ret = bootstrap_register(bootstrap_port, LOOKUP_SERVER_NAME, port);
	if (ret != KERN_SUCCESS) {
		exit(1);
	}
	request_msg.head.msg_local_port = port;
	reply_msg.head.msg_local_port = port;

#ifndef DEBUG
	if (pid == 0) {
		kill(getppid(), SIGTERM);
	}
	detach();
#endif

	(void)signal(SIGCHLD, catch);
	(void)signal(SIGTERM, goodbye);
	if (docaching) {
		recomputelists();
	}
	timeout = std_timeout;
	when = msectime();
	for (;;) {
		request_msg.head.msg_size = sizeof(request_msg);
		ret = msg_receive((msg_header_t *)&request_msg, 
				  docaching ? RCV_TIMEOUT : 0, 
				  timeout);
		timeout = std_timeout - (msectime() - when);
		if (timeout <= 0) {
			if (docaching) {
				recomputelists();
			}
			timeout = std_timeout;
			when = msectime();
		}
		if (ret != KERN_SUCCESS) {
			continue;
		}
		ret = lookup_server(&request_msg, &reply_msg);
		if (ret == MIG_NO_REPLY) {
			continue;
		}
		ret = msg_send((msg_header_t *)&reply_msg, 
			       SEND_NOTIFY | SEND_TIMEOUT, 
			       TIMEOUT_MSECONDS);
		if (ret != KERN_SUCCESS) {
			if (ret != SEND_INVALID_PORT) {
				syslog(LOG_ERR, mach_errormsg(ret));
			}
		}
	}
}

int 
msectime(void)
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return ((tv.tv_sec * 1000) + (tv.tv_usec / 1000));
}

static void
recomputelists(void)
{
	unsigned outlen;
	char *outdata;
	unsigned nolen;
	char *noval;
	port_t port;
	kern_return_t ret;
	int proc;

	if (checksum_unchanged()) {
#ifdef TEST
		syslog(LOG_ERR, "checksum match");
#endif
		return;
	}
#ifdef TEST
	syslog(LOG_ERR, "checksum mismatch");
#endif
	if (nfork > 0) {
		return;
	}
	switch (fork()) {
	case -1:
		return;
	case 0:
		break;
	default:
		nfork++;
		return;
	}
	ret = bootstrap_look_up(bootstrap_port, LOOKUP_SERVER_NAME, &port);
	if (ret != KERN_SUCCESS) {
		return;
	}
	(void)lookup_getpwent(0, NULL, NULL, NULL); /* flush cache */
	if (lookup_getpwent(0, NULL, &outlen, &outdata)) {
		ret = __lookup_link(PORT_NULL, "setpwent", &proc);
		if (ret == KERN_SUCCESS) {
			noval = NULL;
			nolen = 0;
			outlen /= UNIT_SIZE;
			ret = _lookup_ooall(port, proc, outdata, outlen, 
					    &noval, &nolen);
			if (ret != KERN_SUCCESS) {
				syslog(LOG_ERR, mach_errormsg(ret));
			}
		}
	}
#ifdef CACHE_ALIASINFO
	(void)lookup_alias_getent(0, NULL, NULL, NULL); /* flush cache */
	if (lookup_alias_getent(0, NULL, &outlen, &outdata)) {
		ret = __lookup_link(PORT_NULL, "alias_setent", &proc);
		if (ret == KERN_SUCCESS) {
			noval = NULL;
			nolen = 0;
			outlen /= UNIT_SIZE;
			ret = _lookup_ooall(port, proc, outdata, outlen, 
					    &noval, &nolen);
			if (ret != KERN_SUCCESS) {
				syslog(LOG_ERR, mach_errormsg(ret));
			}
		}
	}
#endif
	exit(0);
}

static void
detach(void)
{
	int ttyfd;

	ttyfd = open("/dev/tty", O_RDWR, 0);
	if (ttyfd > 0) {
		ioctl(ttyfd, TIOCNOTTY, (char *)0);
		close(ttyfd);
	}
}

void
parentexit(int x)
{
	exit(0);
}

void
catch(int x)
{
	(void)wait3(NULL, WNOHANG, NULL);
	nfork--;
}

void
goodbye(int x)
{
	exit(1);
}

#ifdef MALLOC_DEBUG
void
catch_malloc_problems(
		      int problem
		      )
{
	abort();
}
#endif

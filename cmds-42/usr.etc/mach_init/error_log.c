/*
 * bootstrap -- fundamental service initiator and port server
 * Mike DeMoney, NeXT, Inc.
 * Copyright, 1990.  All rights reserved.
 *
 * error_log.c -- implementation of logging routines
 *
 * Routines may be safely invoked from multiple threads
 */

#import "bootstrap_internal.h"
#import "error_log.h"

#import <mach_error.h>
#import <stdio.h>
#import <cthreads.h>
#import <syslog.h>
#import <libc.h>

static mutex_t errlog_lock;

void
init_errlog(boolean_t is_init)
{
	int nfds, fd;

	if (is_init) {
		close(0);
		freopen("/dev/console", "r", stdin);
		setbuf(stdin, NULL);
		close(1);
		freopen("/dev/console", "w", stdout);
		setbuf(stdout, NULL);
		close(2);
		freopen("/dev/console", "w", stderr);
		setbuf(stderr, NULL);
	}

	nfds = getdtablesize();
	for (fd = 3; fd < nfds; fd++)
		close(fd);
	openlog((char *)program_name, LOG_PID, LOG_DAEMON);
	setlogmask(LOG_UPTO(LOG_INFO));

	errlog_lock = mutex_alloc();
	mutex_set_name(errlog_lock, "errlog lock");
}

void
close_errlog(void)
{
	closelog();
}

void
debug(const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: ", program_name, getpid());
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_DEBUG, "%s", buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
info(const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: ", program_name, getpid());
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_INFO, "%s", buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
log(const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: ", program_name, getpid());
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_NOTICE, "%s", buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
error(const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: ", program_name, getpid());
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
	 	vsprintf(buf, format, ap);
		syslog(LOG_CRIT, "%s", buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
kern_error(kern_return_t result, const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: %s:", program_name, getpid(),
		 mach_error_string(result));
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_CRIT, "%s: %s", mach_error_string(result), buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
unix_error(const char *format, ...)
{
	va_list ap;
	char *error_msg;
	char buf[1000];
	extern unsigned sys_nerr;
	extern char *sys_errlist[];
	
	va_start(ap, format);
	error_msg = errno < sys_nerr ? sys_errlist[errno] : "Unknown error";
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: %s:", program_name, getpid(), error_msg);
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_CRIT, "%s: %s", error_msg, buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
parse_error(const char *token_string, const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: %s unexpected: ", program_name, getpid(),
		 token_string);
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_CRIT, "%s unexpected: %s", token_string, buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
}

void
fatal(const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: FATAL: ", program_name, getpid());
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_ALERT, "%s", buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
	exit(1);
}

void
kern_fatal(kern_return_t result, const char *format, ...)
{
	va_list ap;
	char buf[1000];
	
	va_start(ap, format);
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: FATAL: %s:", program_name, getpid(),
		 mach_error_string(result));
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_ALERT, "%s: %s", mach_error_string(result), buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
	exit(1);
}

void
unix_fatal(const char *format, ...)
{
	va_list ap;
	const char *error_msg;
	char buf[1000];
	extern unsigned sys_nerr;
	extern char *sys_errlist[];
	
	va_start(ap, format);
	error_msg = errno < sys_nerr ? sys_errlist[errno] : "Unknown error";
	mutex_lock(errlog_lock);
	if (debugging) {
		fprintf(stderr, "%s[%d]: FATAL: %s:", program_name, getpid(), error_msg);
		vfprintf(stderr, format, ap);
		fprintf(stderr, "\n");
	} else {
		vsprintf(buf, format, ap);
		syslog(LOG_ALERT, "%s: %s", error_msg, buf);
	}
	mutex_unlock(errlog_lock);
	va_end(ap);
	exit(1);
}






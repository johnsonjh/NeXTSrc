/*
	logErrorInc.c
  	Copyright 1988, NeXT, Inc.
	Responsibility: Trey Matteson
  
	Contains code for writing error messages to stderr or syslog.
  
	This code is included in errors.m in the kit, and in pbs.c
	so pbs can use it also.
*/

#import <syslog.h>

void NXLogError(const char *format, ...)
{
    va_list ap;
    char bigBuffer[4*1024];
    static char hasTerminal = -1;
    int fd;

    if (hasTerminal == -1) {
	fd = open("/dev/tty", O_RDWR, 0);
	if (fd >= 0) {
	    hasTerminal = 1;
	    (void)close(fd);
	} else
	    hasTerminal = 0;
    }
    va_start(ap, format);
    vsprintf(bigBuffer, format, ap);
    va_end(ap);
    if (hasTerminal) {
	fwrite(bigBuffer, sizeof(char), strlen(bigBuffer), stderr);
	if (bigBuffer[strlen(bigBuffer)-1] != '\n')
	    fputc('\n', stderr);
    } else
	syslog(LOG_ERR, "%s", bigBuffer);
}


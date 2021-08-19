/* 
 * Debugging utilities
 * Copyright (C) 1989 by NeXT, Inc.
 */
#include <syslog.h>

void
debug(char *msg)
{
	syslog(LOG_ERR, "%s", msg);
}

void
debug2(char *msg1, char *msg2)
{
	syslog(LOG_ERR, "%s %s", msg1, msg2);
}



#include <sys/time.h>

/*
 * from yp_bind.c 1.1 86/09/24 Copyr 1985 Sun Micro
 */
#ifdef  DEBUG
#define YPTIMEOUT 120			/* Total seconds for timeout */
#else
#define YPTIMEOUT 20			/* Total seconds for timeout */
#endif
#define YPSLEEPTIME 5			/* Time to sleep between tries */

unsigned int _ypsleeptime = YPSLEEPTIME;
struct timeval _ypserv_timeout = {
	YPTIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

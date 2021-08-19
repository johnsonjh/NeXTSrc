#ifndef	_km
#define	_km

/* Module km */

#include <sys/kern_return.h>
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(LINTLIBRARY)
#include <sys/port.h>
#include <sys/message.h>
#endif

#ifndef	mig_external
#define mig_external extern
#endif

#include "key_defs.h"
#include "nm_defs.h"

/* Routine km_dummy */
mig_external kern_return_t km_dummy
#if	defined(LINTLIBRARY)
    (server_port)
	port_t server_port;
{ return km_dummy(server_port); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port
);
#else
    ();
#endif
#endif

#endif	_km

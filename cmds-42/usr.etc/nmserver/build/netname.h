#ifndef	_netname
#define	_netname

/* Module netname */

#include <sys/kern_return.h>
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(LINTLIBRARY)
#include <sys/port.h>
#include <sys/message.h>
#endif

#ifndef	mig_external
#define mig_external extern
#endif

#include <servers/netname_defs.h>

/* Routine netname_check_in */
mig_external kern_return_t netname_check_in
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature, port_id)
	port_t server_port;
	netname_name_t port_name;
	port_t signature;
	port_t port_id;
{ return netname_check_in(server_port, port_name, signature, port_id); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	netname_name_t port_name,
	port_t signature,
	port_t port_id
);
#else
    ();
#endif
#endif

/* Routine netname_look_up */
mig_external kern_return_t netname_look_up
#if	defined(LINTLIBRARY)
    (server_port, host_name, port_name, port_id)
	port_t server_port;
	netname_name_t host_name;
	netname_name_t port_name;
	port_t *port_id;
{ return netname_look_up(server_port, host_name, port_name, port_id); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	netname_name_t host_name,
	netname_name_t port_name,
	port_t *port_id
);
#else
    ();
#endif
#endif

/* Routine netname_check_out */
mig_external kern_return_t netname_check_out
#if	defined(LINTLIBRARY)
    (server_port, port_name, signature)
	port_t server_port;
	netname_name_t port_name;
	port_t signature;
{ return netname_check_out(server_port, port_name, signature); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	netname_name_t port_name,
	port_t signature
);
#else
    ();
#endif
#endif

/* Routine netname_version */
mig_external kern_return_t netname_version
#if	defined(LINTLIBRARY)
    (server_port, version)
	port_t server_port;
	netname_name_t version;
{ return netname_version(server_port, version); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	netname_name_t version
);
#else
    ();
#endif
#endif

#endif	_netname

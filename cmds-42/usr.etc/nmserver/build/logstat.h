#ifndef	_logstat
#define	_logstat

/* Module logstat */

#include <sys/kern_return.h>
#if	(defined(__STDC__) || defined(c_plusplus)) || defined(LINTLIBRARY)
#include <sys/port.h>
#include <sys/message.h>
#endif

#ifndef	mig_external
#define mig_external extern
#endif

#include "ls_defs.h"

/* Routine ls_sendlog */
mig_external kern_return_t ls_sendlog
#if	defined(LINTLIBRARY)
    (server_port, old_log_ptr, old_log_ptrCnt, cur_log_ptr, cur_log_ptrCnt)
	port_t server_port;
	log_ptr_t *old_log_ptr;
	unsigned int *old_log_ptrCnt;
	log_ptr_t *cur_log_ptr;
	unsigned int *cur_log_ptrCnt;
{ return ls_sendlog(server_port, old_log_ptr, old_log_ptrCnt, cur_log_ptr, cur_log_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	log_ptr_t *old_log_ptr,
	unsigned int *old_log_ptrCnt,
	log_ptr_t *cur_log_ptr,
	unsigned int *cur_log_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_resetlog */
mig_external kern_return_t ls_resetlog
#if	defined(LINTLIBRARY)
    (server_port)
	port_t server_port;
{ return ls_resetlog(server_port); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port
);
#else
    ();
#endif
#endif

/* Routine ls_writelog */
mig_external kern_return_t ls_writelog
#if	defined(LINTLIBRARY)
    (server_port)
	port_t server_port;
{ return ls_writelog(server_port); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port
);
#else
    ();
#endif
#endif

/* Routine ls_sendstat */
mig_external kern_return_t ls_sendstat
#if	defined(LINTLIBRARY)
    (server_port, stat_ptr, stat_ptrCnt)
	port_t server_port;
	stat_ptr_t *stat_ptr;
	unsigned int *stat_ptrCnt;
{ return ls_sendstat(server_port, stat_ptr, stat_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	stat_ptr_t *stat_ptr,
	unsigned int *stat_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_resetstat */
mig_external kern_return_t ls_resetstat
#if	defined(LINTLIBRARY)
    (server_port)
	port_t server_port;
{ return ls_resetstat(server_port); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port
);
#else
    ();
#endif
#endif

/* Routine ls_senddebug */
mig_external kern_return_t ls_senddebug
#if	defined(LINTLIBRARY)
    (server_port, debug_ptr, debug_ptrCnt)
	port_t server_port;
	debug_ptr_t *debug_ptr;
	unsigned int *debug_ptrCnt;
{ return ls_senddebug(server_port, debug_ptr, debug_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	debug_ptr_t *debug_ptr,
	unsigned int *debug_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_setdebug */
mig_external kern_return_t ls_setdebug
#if	defined(LINTLIBRARY)
    (server_port, debug_ptr, debug_ptrCnt)
	port_t server_port;
	debug_ptr_t debug_ptr;
	unsigned int debug_ptrCnt;
{ return ls_setdebug(server_port, debug_ptr, debug_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	debug_ptr_t debug_ptr,
	unsigned int debug_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_sendparam */
mig_external kern_return_t ls_sendparam
#if	defined(LINTLIBRARY)
    (server_port, param_ptr, param_ptrCnt)
	port_t server_port;
	param_ptr_t *param_ptr;
	unsigned int *param_ptrCnt;
{ return ls_sendparam(server_port, param_ptr, param_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	param_ptr_t *param_ptr,
	unsigned int *param_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_setparam */
mig_external kern_return_t ls_setparam
#if	defined(LINTLIBRARY)
    (server_port, param_ptr, param_ptrCnt)
	port_t server_port;
	param_ptr_t param_ptr;
	unsigned int param_ptrCnt;
{ return ls_setparam(server_port, param_ptr, param_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	param_ptr_t param_ptr,
	unsigned int param_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_sendportstat */
mig_external kern_return_t ls_sendportstat
#if	defined(LINTLIBRARY)
    (server_port, port_stat_ptr, port_stat_ptrCnt)
	port_t server_port;
	log_ptr_t *port_stat_ptr;
	unsigned int *port_stat_ptrCnt;
{ return ls_sendportstat(server_port, port_stat_ptr, port_stat_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	log_ptr_t *port_stat_ptr,
	unsigned int *port_stat_ptrCnt
);
#else
    ();
#endif
#endif

/* Routine ls_mem_list */
mig_external kern_return_t ls_mem_list
#if	defined(LINTLIBRARY)
    (server_port, class_ptr, class_ptrCnt, nam_ptr, nam_ptrCnt, bucket_ptr, bucket_ptrCnt)
	port_t server_port;
	mem_class_ptr_t *class_ptr;
	unsigned int *class_ptrCnt;
	mem_nam_ptr_t *nam_ptr;
	unsigned int *nam_ptrCnt;
	mem_bucket_ptr_t *bucket_ptr;
	unsigned int *bucket_ptrCnt;
{ return ls_mem_list(server_port, class_ptr, class_ptrCnt, nam_ptr, nam_ptrCnt, bucket_ptr, bucket_ptrCnt); }
#else
#if	(defined(__STDC__) || defined(c_plusplus))
(
	port_t server_port,
	mem_class_ptr_t *class_ptr,
	unsigned int *class_ptrCnt,
	mem_nam_ptr_t *nam_ptr,
	unsigned int *nam_ptrCnt,
	mem_bucket_ptr_t *bucket_ptr,
	unsigned int *bucket_ptrCnt
);
#else
    ();
#endif
#endif

#endif	_logstat

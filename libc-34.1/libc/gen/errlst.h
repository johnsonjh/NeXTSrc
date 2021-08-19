/* @(#)errlst.c	1.2 87/07/06 3.2/4.3NFSSRC */
/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 **********************************************************************
 * HISTORY
 * 30-Apr-85  Steven Shafer (sas) at Carnegie-Mellon University
 *	Adapted for CS library.  Changed message #16.
 *
 **********************************************************************
 */

el(	"Error 0",				0, 0)
el(	"Not owner",				1, EPERM)
el(	"No such file or directory",		2, ENOENT)
el(	"No such process",			3, ESRCH)
el(	"Interrupted system call",		4, EINTR)
el(	"I/O error",				5, EIO)
el(	"No such device or address",		6, ENXIO)
el(	"Arg list too long",			7, E2BIG)
el(	"Exec format error",			8, ENOEXEC)
el(	"Bad file number",			9, EBADF)
el(	"No children",				10, ECHILD)
el(	"No more processes",			11, EAGAIN)
el(	"Not enough memory",			12, ENOMEM)
el(	"Permission denied",			13, EACCES)
el(	"Bad address",				14, EFAULT)
el(	"Block device required",		15, ENOTBLK)
#ifdef	CMUCS
el(	"File or device busy",			16, EBUSY)
#else	CMUCS
el(	"Device busy",				16, EBUSY)
#endif	CMUCS
el(	"File exists",				17, EEXIST)
el(	"Cross-device link",			18, EXDEV)
el(	"No such device",			19, ENODEV)
el(	"Not a directory",			20, ENOTDIR)
el(	"Is a directory",			21, EISDIR)
el(	"Invalid argument",			22, EINVAL)
el(	"File table overflow",			23, ENFILE)
el(	"Too many open files",			24, EMFILE)
el(	"Inappropriate ioctl for device",	25, ENOTTY)
el(	"Text file busy",			26, ETXTBSY)
el(	"File too large",			27, EFBIG)
el(	"No space left on device",		28, ENOSPC)
el(	"Illegal seek",				29, ESPIPE)
el(	"Read-only file system",		30, EROFS)
el(	"Too many links",			31, EMLINK)
el(	"Broken pipe",				32, EPIPE)

/* math software */
el(	"Argument too large",			33, EDOM)
el(	"Result too large",			34, ERANGE)

/* non-blocking and interrupt i/o */
el(	"Operation would block",		35, EWOULDBLOCK)
el(	"Operation now in progress",		36, EINPROGRESS)
el(	"Operation already in progress",	37, EALREADY)

/* ipc/network software */

	/* argument errors */
el(	"Socket operation on non-socket",	38, ENOTSOCK)
el(	"Destination address required",		39, EDESTADDRREQ)
el(	"Message too long",			40, EMSGSIZE)
el(	"Protocol wrong type for socket",	41, EPROTOTYPE)
el(	"Option not supported by protocol",	42, ENOPROTOOPT)
el(	"Protocol not supported",		43, EPROTONOSUPPORT)
el(	"Socket type not supported",		44, ESOCKTNOSUPPORT)
el(	"Operation not supported on socket",	45, EOPNOTSUPP)
el(	"Protocol family not supported",	46, EPFNOSUPPORT)
el(	"Address family not supported by protocol family",
						47, EAFNOSUPPORT)
el(	"Address already in use",		48, EADDRINUSE)
el(	"Can't assign requested address",	49, EADDRNOTAVAIL)

	/* operational errors */
el(	"Network is down",			50, ENETDOWN)
el(	"Network is unreachable",		51, ENETUNREACH)
el(	"Network dropped connection on reset",	52, ENETRESET)
el(	"Software caused connection abort",	53, ECONNABORTED)
el(	"Connection reset by peer",		54, ECONNRESET)
el(	"No buffer space available",		55, ENOBUFS)
el(	"Socket is already connected",		56, EISCONN)
el(	"Socket is not connected",		57, ENOTCONN)
el(	"Can't send after socket shutdown",	58, ESHUTDOWN)
el(	"Too many references: can't splice",	59, ETOOMANYREFS)
el(	"Connection timed out",			60, ETIMEDOUT)
el(	"Connection refused",			61, EREFUSED)
el(	"Too many levels of symbolic links",	62, ELOOP)
el(	"File name too long",			63, ENAMETOOLONG)
el(	"Host is down",				64, EHOSTDOWN)
el(	"Host is unreachable",			65, EHOSTUNREACH)
el(	"Directory not empty",			66, ENOTEMPTY)
el(	"Too many processes",			67, EPROCLIM)
el(	"Too many users",			68, EUSERS)
el(	"Disc quota exceeded",			69, EDQUOT)

/* NFS/RPC software */

el(	"Stale NFS file handle",                70, ESTALE)
el(	"Too many levels of remote in path",    71, EREMOTE)
el(	"Not a stream device",                  72, ENOSTR)
el(	"Timer expired",                        73, ETIME)
el(	"Out of stream resources",              74, ENOSR)
el(	"No message of desired type",           75, ENOMSG)
el(	"Not a data message",                   76, EBADMSG)
el(	"Identifier removed",                   77, EIDRM)
el(	"Deadlock situation detected/avoided",  78, EDEADLK)
el(	"No record locks available",            79, ENOLCK)

/* intelligent devices */

el(	"Device power is off",			80, EPWROFF)
el(	"Device error",				81, EDEVERR)
el(	"Device not initialized",		82, ENOINIT)

/* program loading errors */
el(	"Bad executable (or shared library)",	83, EBADEXEC)

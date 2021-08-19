/*
 * Safe stdio
 * Copyright (C) 1989 by NeXT, Inc.
 *
 * Stdio does not lock its iob slots. We must do the locking
 * ourselves here instead because we use multiple threads which
 * stdio does not expect.
 */
#include <stdio.h>
#include <cthreads.h>

#ifdef STDIO_DEBUG
#include <sys/stat.h>
#endif

/*
 * The UNIX open/close calls are locked using the following
 * primitives. The "socket" name is an anachronism.
 */
extern void socket_lock(void);
extern void socket_unlock(void);

/*
 * fopen() a file thread-safe way.
 */
FILE *
safe_fopen(
	   char *fname,
	   char *mode
	   )
{
	FILE *f;

	socket_lock();
	f = fopen(fname, mode);
#ifdef STDIO_DEBUG
	/*
	 * If a problem occurs because descriptors are not being locked
	 * correctly, this code will often catch the problem (with enough
	 * beating up on the server).
	 */
	if (f != NULL) {
		struct stat st;
		extern void sys_errmsg(const char *);

		if (fstat(fileno(f), &st) < 0) {
			fclose(f);
			socket_unlock();
			sys_errmsg("fopen returned bogus descriptor");
			return (NULL);
		}
	}
#endif
	socket_unlock();
	return (f);
}

/*
 * fclose() a file in a thread-safe way.
 */
int
safe_fclose(
	    FILE *f
	    )
{
	int res;

	socket_lock();
	res = fclose(f);
	socket_unlock();
	return (res);
}

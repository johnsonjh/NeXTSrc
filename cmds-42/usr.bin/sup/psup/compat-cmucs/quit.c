/*  quit  --  print message and exit
 *
 *  Usage:  quit (status,format [,arg]...);
 *	int status;
 *	(... format and arg[s] make up a printf-arglist)
 *
 *  Quit is a way to easily print an arbitrary message and exit.
 *  It is most useful for error exits from a program:
 *	if (open (...) < 0) then quit (1,"Can't open...",file);
 *
 *  HISTORY
 * 20-Mar-81  Dale Moore (dwm) at Carnegie-Mellon University
 *	Inserting assembly statements into high level languages is particularly
 *	gross, especially in this case, where it is totally avoidable.
 *	In both Vax and pdp11 UNIX (version 7) printf(III) and fprintf(III)
 *	are implemented with a routine named _doprnt.  Therefore, rather
 *	than implementing quit on top of fprintf, I decided that it should be
 *	implemented along side of fprintf, on top of the same stuff that printf
 *	and fprintf are implemented.
 *
 * 10-Nov-80  Mike Accetta (mja) at Carnegie-Mellon University
 *	Well Steve, it looks like you were right. The %r feature appears
 *	to have disappeared from printf() so we now use insert a CALLG
 *	instruction in-line after saving the status and setting it to
 *	point to the STDERR buffer.  Now if this isn't machine specific,
 *	then nothing is (and you thought you were apprehensive before)!
 *
 * 20-Nov-79  Steven Shafer (sas) at Carnegie-Mellon University
 *	Rewritten in C for VAX (code due to Dave Smith).  I'm real apprehensive
 *	about the use of the "%r" format in printf(), which is not documented by
 *	Bell Labs.  On the other hand, this format was present in Version 6
 *	of UNIX, and is still there, so maybe it will remain forever...?
 *	Another bit of sleaziness is the use of "&" to pass the parm list to
 *	printf;  however, the precedent for this is the "execlp" routine from
 *	Bell Labs which does the same thing.  Paul Hilfinger assures me that
 *	the underlying assumption about argument-passing is reasonable across
 *	a wide variety of machines.
 *
 */

#include <stdio.h>

quit (status, fmt, args)
int status;
char *fmt;
{
	_doprnt(fmt, &args, stderr);
	exit (status);
}

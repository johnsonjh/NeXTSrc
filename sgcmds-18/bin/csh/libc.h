/*
 **********************************************************************
 * HISTORY
 * 01-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added getname() extern.
 *
 * 29-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added lseek() extern.
 *
 * 02-Nov-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added salloc() extern.
 *
 * 14-Aug-81  Mike Accetta (mja) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

/*  CMU stdio additions  */
#ifndef	FILE
#include	<stdio.h>
#endif
extern FILE *fopenp();
extern FILE *fwantread();
extern FILE *fwantwrite();

/* CMU string routines */
extern char *foldup(), *folddown();
extern char *sindex(), *skipto(), *skipover(), *nxtarg();
extern char *getstr(), *strarg();
extern long getlong(), longarg();
extern short getshort(), shortarg();
extern float getfloat(), floatarg();
extern double getdouble(), doublearg();
extern unsigned int getoct(), octarg(), gethex(), hexarg();
extern unsigned int atoo(), atoh();
extern char *salloc();

/* CMU library routines */
extern char *getname();

/* 4.2 BSD standard library routines */
#include <strings.h>
extern FILE *popen();
extern long atol(), random();
extern long lseek();
extern double atof();
extern char *crypt(), *ecvt(), *fcvt(), *gcvt(), *getenv(), *getlogin();
extern char *getpass(), *getwd(), *malloc(), *realloc(), *calloc();
extern char *alloca(), *mktemp(), *initstate(), *setstate();
extern char *re_comp(), *ttyname(), *valloc();

/*  CMU time additions */
extern long gtime();
extern char *cdate();
extern long atot();

/* 4.2 BSD time routines */
extern struct tm *localtime();
extern struct tm *gmtime();
extern char *ctime();
extern char *asctime();
extern char *timezone();

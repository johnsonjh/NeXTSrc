/*	@(#)mac.h	1.3	*/
/*
 *	UNIX shell
 *
 *	Bell Telephone Laboratories
 *
 */

#define TRUE	(-1)
#define FALSE	0
#define LOBYTE	0377
#define STRIP	0177
#define QUOTE	0200

#define EOF	0
#define NL	'\n'
#define SP	' '
#define LQ	'`'
#define RQ	'\''
#define MINUS	'-'
#define COLON	':'
#define TAB	'\t'
#ifdef TILDE_SUB
#define	SQUIGGLE '~'	/* should be called TILDE but BERKELEY tty handler uses that */
#endif

#define MAX(a,b)	((a)>(b)?(a):(b))

#define blank()		prc(SP)
#define	tab()		prc(TAB)
#define newline()	prc(NL)

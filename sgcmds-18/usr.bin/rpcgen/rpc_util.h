/*	@(#)rpc_util.h	1.2 88/05/08 4.0NFSSRC SMI	*/

/* 
 * Copyright (c) 1988 by Sun Microsystems, Inc.
 * @(#) from SUN 1.9
 */

/*
 * rpc_util.h, Useful definitions for the RPC protocol compiler 
 */
extern char *malloc();

#define alloc(size)		malloc((unsigned)(size))
#define ALLOC(object)   (object *) malloc(sizeof(object))

extern char *sprintf();

#define s_print	(void) sprintf
#define f_print (void) fprintf

struct list {
	char *val;
	struct list *next;
};
typedef struct list list;

/*
 * Global variables 
 */
#define MAXLINESIZE 1024
extern char curline[MAXLINESIZE];
extern char *where;
extern int linenum;

extern char *infilename;
extern FILE *fout;
extern FILE *fin;

extern list *defined;

/*
 * rpc_util routines 
 */
void storeval();

#define STOREVAL(list,item)	\
	storeval(list,(char *)item)

char *findval();

#define FINDVAL(list,item,finder) \
	findval(list, (char *) item, finder)

char *fixtype();
char *stringfix();
void pvname();
void ptype();
int isvectordef();
int streq();
void error();
void expected1();
void expected2();
void expected3();
void tabify();
void record_open();

/*
 * rpc_cout routines 
 */
void cprint();
void emit();

/*
 * rpc_hout routines 
 */
void print_datadef();

/*
 * rpc_svcout routines 
 */
void write_most();
void write_register();
void write_rest();
void write_programs();

/*
 * rpc_clntout routines
 */
void write_stubs();

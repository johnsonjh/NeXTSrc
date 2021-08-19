/*      @(#)getgroup.h	1.1 88/03/07 4.0NFSSRC SMI   */

struct grouplist {		
	char *gl_machine;
	char *gl_name;
	char *gl_domain;
	struct grouplist *gl_nxt;
};

struct grouplist *my_getgroup();

			

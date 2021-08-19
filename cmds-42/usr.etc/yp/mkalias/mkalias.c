#ifndef lint
static char sccsid[] = 	"@(#)mkalias.c	1.1 88/03/07 4.0NFSSRC Copyr 1988 Sun Micro";
#endif

/*
 * mkmap - program to convert the mail.aliases map into an
 * inverse map of <user@host> back to <preferred-alias>
 */

# include <sys/file.h>
# include <ndbm.h>
# include <stdio.h>
# include <ctype.h>

#ifdef NeXT_MOD
# include "h/ypdefs.h"
#else
# include "ypdefs.h"
#endif NeXT_MOD

USE_YP_PREFIX
USE_YP_MASTER_NAME
USE_YP_LAST_MODIFIED

char *index(), *rindex();

int Verbose = 0;	/* to get the gory details */

DBM *Indbm, *Scandbm, *Outdbm;

IsMailingList(s)
  {
    /* 
     * returns true if the given string is a mailing list
     */
     char *p;

     if ( index(s,',') ) return(1);
     if ( index(s,'|') ) return(1);
     p = index(s,':');
     if (p && strncmp(p,":include:")) return(1);
     return(0);
  }


IsQualified(s,p)
     char *s;  /* input string */
     char *p;  /* output: user part */
  {
    /* 
     * returns true if the given string is qualified with a host name
     */
     register char *middle;

     middle = index(s,'@');
     if ( middle ) {
         for (middle=s; *middle != '@'; *p++ = *middle++) continue;
	 *p = '\0';
         return(1);
       }
     middle = rindex(s,'!');
     if ( middle ) {
	 strcpy(p, middle+1);
         return(1);
       }
     return(0);
  }


IsMaint(s)
    char *s;
  {
    /*
     * returns true if the given string is one of the maintenence
     * strings used in sendmail or YP.
     */
    if (*s=='@') return(1);
    if (strncmp(s, yp_prefix, yp_prefix_sz)==0) return(1);
    return(0);
  }

CopyName(dst, src)
    register char *dst, *src;
  {
     /*
      * copy a string, but ignore white space
      */
     while (*src != '\0' && isspace(*src)) src++;
     while (*src) *dst++ = *src++;
     *dst = '\0';
  }

Compare(s1, s2)
    register char *s1, *s2;
  {
     /*
      * compare strings, but ignore white space
      */
     while (*s1 != '\0' && isspace(*s1)) s1++;
     while (*s2 != '\0' && isspace(*s2)) s2++;
     return(strcmp(s1,s2));
  }


ProcessMap()
  {
    datum key, value, part, partvalue;
    char address[PBLKSIZ];	/* qualified version */
    char user[PBLKSIZ];		/* unqualified version */
    char userpart[PBLKSIZ];	/* unqualified part of qualified addr. */
    
    for (key = dbm_firstkey(Scandbm); key.dptr != NULL;
      key = dbm_nextkey(Scandbm)) {
	value = dbm_fetch(Indbm, key);
	CopyName(address, value.dptr);
	CopyName(user, key.dptr);
	if (address == NULL) continue;
	if (IsMailingList(address)) continue;
	if (!IsQualified(address, userpart)) continue;
	if (IsMaint(user)) continue;
	part.dptr = userpart;
	part.dsize = strlen(userpart) + 1;
	partvalue = dbm_fetch(Indbm, part);
	value.dptr = address;
	value.dsize = strlen(address) + 1;
	if (partvalue.dptr != NULL && Compare(partvalue.dptr,user)==0 ) {
	    dbm_store(Outdbm, value, part, DBM_REPLACE);
	    if (Verbose) printf("%s --> %s --> %s\n", 
	    	userpart, user, address);
	  }
	else {
	    key.dptr = user;
	    key.dsize = strlen(user) + 1;
	    dbm_store(Outdbm, value, key, DBM_REPLACE);
	    if (Verbose) printf("%s --> %s\n", user, address);
	  }
    }
  }

AddYPEntries()
  {
    datum key, value;
    char last_modified[PBLKSIZ];
    char host_name[PBLKSIZ];
    long now;
	    /*
	     * Add the special Yellow pages entries. 
	     */
    key.dptr = yp_last_modified;
    key.dsize = yp_last_modified_sz;
    time(&now);
    sprintf(last_modified,"%10.10d",now);
    value.dptr = last_modified;
    value.dsize = strlen(value.dptr);
    dbm_store(Outdbm, key, value);

    key.dptr = yp_master_name;
    key.dsize = yp_master_name_sz;
    gethostname(host_name, sizeof(host_name) );
    value.dptr = host_name;
    value.dsize = strlen(value.dptr);
    dbm_store(Outdbm, key, value);
  }

main(argc, argv)
	int argc;
	char **argv;
{
    while (argc > 1 && argv[1][0] == '-') {
        switch (argv[1][1]) {
	  case 'v':
	  	Verbose = 1;
		break;
	  default:
	  	printf("Unknown option %c\n", argv[1][1]);
		break;
	}
	argc--; argv++;
      }    
    if (argc != 3) {
      printf("Usage: mkalias <input> <output>\n");
      exit(1);
    }
    Indbm = dbm_open(argv[1], O_RDONLY, 0);
    if (Indbm==NULL) {
      printf("Unable to open input database %s\n", argv[1]);
      exit(1);
    }
    Scandbm = dbm_open(argv[1], O_RDONLY, 0);
    if (Scandbm==NULL) {
      printf("Unable to open input database %s\n", argv[1]);
      exit(1);
    }
    Outdbm = dbm_open(argv[2], O_RDWR|O_CREAT|O_TRUNC, 0644);
    if (Outdbm==NULL) {
      printf("Unable to open output database %s\n", argv[2]);
      exit(1);
    }
    ProcessMap();
    AddYPEntries();
    dbm_close(Indbm);
    dbm_close(Scandbm);
    dbm_close(Outdbm);
}

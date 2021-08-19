#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <mach.h>
#include "errors.h"
#include "misc.h"

/*
 * A hash function used for converting a string into a single number.  It is
 * then usually mod'ed with the hash table size to get an index into the hash
 * table.
 */
long
hash_string(
char *key)
{
    char *cp;
    long k;

	cp = key;
	k = 0;
	while(*cp)
	    k = (((k << 1) + (k >> 14)) ^ (*cp++)) & 0x3fff;
	return(k);
}

/*
 * xmalloc is just a wrapper around malloc that prints and error message and
 * exits if the malloc fails.
 */
void *
xmalloc(
long size)
{
    void *p;

	if((p = malloc(size)) == (char *)0)
	    perror_fatal("virtual memory exhausted");
	return(p);
}

/*
 * xrealloc is just a wrapper around realloc that prints and error message and
 * exits if realloc fails.
 */
void *
xrealloc(
void *p,
long size)
{
	if((p = realloc(p, size)) == (char *)0)
	    perror_fatal("virtual memory exhausted");
	return(p);
}

/*
 * savestr() malloc's space for the string passed to it, copys the string into
 * the space and returns a pointer to that space.
 */
char *
savestr(
char *s)
{
    long len;
    char *r;

	len = strlen(s) + 1;
	r = (char *)xmalloc(len);
	strcpy(r, s);
	return(r);
}

/*
 * Mkstr() creates a string that is the concatenation of a variable number of
 * strings.  It is pass a variable number of pointers to strings and the last
 * pointer is NULL.  It returns the pointer to the string it created.  The
 * storage for the string is malloc()'ed can be free()'ed when nolonger needed.
 */
char *
makestr(
const char *args,
...)
{
    va_list ap;
    char *s, *p;
    long size;

	size = 0;
	if(args != NULL){
	    size += strlen(args);
	    va_start(ap, args);
	    p = (char *)va_arg(ap, char *);
	    while(p != NULL){
		size += strlen(p);
		p = (char *)va_arg(ap, char *);
	    }
	}
	s = (char *)xmalloc(size + 1);
	*s = '\0';

	if(args != NULL){
	    (void)strcat(s, args);
	    va_start(ap, args);
	    p = (char *)va_arg(ap, char *);
	    while(p != NULL){
		(void)strcat(s, p);
		p = (char *)va_arg(ap, char *);
	    }
	    va_end(ap);
	}
	return(s);
}

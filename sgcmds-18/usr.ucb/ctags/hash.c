/* hash table for common words */
#include <stdio.h>
#include "tags.h"
#ifdef	NeXT_MOD
#define Hent register HashEntry
#define notequal strcmp
#define hnull (HashEntry *)0
#define Alloc(x) (x *)malloc(sizeof(x))
#else	NeXT_MOD
#d Hent register HashEntry
#d notequal strcmp
#d hnull (HashEntry *)0
#d Alloc(x) (x *)malloc(sizeof(x))
#endif	NeXT_MOD
typedef struct HashEntry { 
	char *k;
	struct HashEntry *next; /* next entry in chained list */
} HashEntry;

#ifdef	NeXT_MOD
#define SIZE 1017   /* make it almost any size if you like */
#else	NeXT_MOD
#d SIZE 1017   /* make it almost any size if you like */
#endif	NeXT_MOD
static HashEntry *table[SIZE];	/* the hash table */

static hash(s) Char *s; {
        Int i;
	if (!*s) return 0;
        for (i = *s; *s; i += (*s)*(*(++s)));
        return (i+*(s-1))%SIZE;
}

HashEntry *
lookup(k) Char *k; {
        Hent *e = table[hash(k)];
        while (e && notequal(k,e->k)) e=e->next;
        return e;
}

enter(k) Char *k; {
        Hent *e = lookup(k);
	Int i;
        if (!e && (e=Alloc(HashEntry)))
                i = hash(e->k=k),
                e->next = table[i],
                table[i] = e;
}

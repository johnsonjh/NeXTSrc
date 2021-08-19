#import <stdio.h>
#import "define.h"
#import <text/text.h>
#import "query.h"


/* addQuery(), prevQuery(), nextQuery() -- simple history list */
#define Alloc(x) (x *)calloc(1,sizeof(x))
#define same(a,b) (strcmp(a,b)==0)

typedef struct List {
	char           *s;
	struct List    *next, *prev;
}               List;
static List    *Current = (List *) 0;

addQuery(s)
	char           *s;

/*
 * Remember 's' in the queue.  Do this each time a new query is found.
 */
{
	List           *l;

	if (!Current) {
		Current = Alloc(List);
		Current->s = strsave(s);
		Current->prev = Current->next = Current;
		return;
	}

	if (Current && same(s, Current->s))
		return;

	l = Alloc(List);
	l->s = strsave(s);
	l->prev = Current->prev;
	l->prev->next = l;
	l->next = Current;
	Current->prev = l;
	Current = l;
}
static
printQueryList()
{
	List           *l = Current;

	fprintf(stderr, "->\t%s\n", l && l->s ? l->s : "(nil)");
	for (l = l->next; l != Current; l = l->next) {
		fprintf(stderr, "\t%s\n", l->s ? l->s : "(nil)");
	}
}

char *
nextQuery()
{
	if (!Current || !Current->prev)
		return (char *)0;
	Current = Current->prev;
	return Current->s;
}

char *
prevQuery()
{
	if (!Current || !Current->next)
		return (char *)0;
	Current = Current->next;
	return Current->s;
}

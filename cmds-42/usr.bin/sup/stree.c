/*
 * stree.c -- SUP Tree Routines
 *
 **********************************************************************
 * HISTORY
 * 29-Dec-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Added code to initialize new fields.  Added Tfree routine.
 *
 * 27-Sep-85  Glenn Marcy (gm0w) at Carnegie-Mellon University
 *	Created.
 *
 **********************************************************************
 */

#include <libc.h>
#include <c.h>
#include <sys/types.h>
#include "sup.h"

#define Static	/* static		/* comment for debugging */

/*************************************************************
 ***    T R E E   P R O C E S S I N G   R O U T I N E S    ***
 *************************************************************/

Tfree (t)
register TREE **t;
{
	if (*t == NULL)  return;
	Tfree (&((*t)->Tlink));
	Tfree (&((*t)->Texec));
	Tfree (&((*t)->Tlo));
	Tfree (&((*t)->Thi));
	if ((*t)->Tname)  free ((*t)->Tname);
	if ((*t)->Tuser)  free ((*t)->Tuser);
	if ((*t)->Tgroup)  free ((*t)->Tgroup);
	free (*(char **)t);
	*t = NULL;
}

Static
TREE *Tmake (p)
char *p;
{
	register TREE *t;
	t = (TREE *) malloc (sizeof (TREE));
	t->Tname = (p == NULL) ? NULL : salloc (p);
	t->Tflags = 0;
	t->Tuid = 0;
	t->Tgid = 0;
	t->Tuser = NULL;
	t->Tgroup = NULL;
	t->Tmode = 0;
	t->Tctime = 0;
	t->Tmtime = 0;
	t->Tlink = NULL;
	t->Texec = NULL;
	t->Tbf = 0;
	t->Tlo = NULL;
	t->Thi = NULL;
	return (t);
}

Static
TREE *Trotll (tp,tl)
register TREE *tp,*tl;
{
    tp->Tlo = tl->Thi;
    tl->Thi = tp;
    tp->Tbf = tl->Tbf = 0;
    return(tl);
}

Static
TREE *Trotlh (tp,tl)
register TREE *tp,*tl;
{
    register TREE *th;

    th = tl->Thi;
    tp->Tlo = th->Thi;
    tl->Thi = th->Tlo;
    th->Thi = tp;
    th->Tlo = tl;
    tp->Tbf = tl->Tbf = 0;
    if (th->Tbf == 1)
	tp->Tbf = -1;
    else if (th->Tbf == -1)
	tl->Tbf = 1;
    th->Tbf = 0;
    return(th);
}

Static
TREE *Trothl (tp,th)
register TREE *tp,*th;
{
    register TREE *tl;

    tl = th->Tlo;
    tp->Thi = tl->Tlo;
    th->Tlo = tl->Thi;
    tl->Tlo = tp;
    tl->Thi = th;
    tp->Tbf = th->Tbf = 0;
    if (tl->Tbf == -1)
	tp->Tbf = 1;
    else if (tl->Tbf == 1)
	th->Tbf = -1;
    tl->Tbf = 0;
    return(tl);
}

Static
TREE *Trothh (tp,th)
register TREE *tp,*th;
{
    tp->Thi = th->Tlo;
    th->Tlo = tp;
    tp->Tbf = th->Tbf = 0;
    return(th);
}

Static
Tbalance (t)
TREE **t;
{
    if ((*t)->Tbf < 2 && (*t)->Tbf > -2)
	return;
    if ((*t)->Tbf > 0) {
	if ((*t)->Tlo->Tbf > 0)
	    *t = Trotll(*t, (*t)->Tlo);
	else
	    *t = Trotlh(*t, (*t)->Tlo);
    } else {
	if ((*t)->Thi->Tbf > 0)
	    *t = Trothl(*t, (*t)->Thi);
	else
	    *t = Trothh(*t, (*t)->Thi);
    }
}

Static
TREE *Tinsertavl (t,p,find,dh)
TREE **t;
char *p;
int find;
int *dh;
{
	register TREE *newt;
	register int cmp;
	int deltah;

	if (*t == NULL) {
	    *t = Tmake (p);
	    *dh = 1;
	    return (*t);
	}
	if ((cmp = strcmp(p, (*t)->Tname)) == 0) {
	    if (!find)  return (NULL);	/* node already exists */
	    *dh = 0;
	    return (*t);
	} else if (cmp < 0) {
	    if ((newt = Tinsertavl (&((*t)->Tlo),p,find,&deltah)) == NULL)
		return (NULL);
	    (*t)->Tbf += deltah;
	} else {
	    if ((newt = Tinsertavl (&((*t)->Thi),p,find,&deltah)) == NULL)
		return (NULL);
	    (*t)->Tbf -= deltah;
	}
	Tbalance(t);
	if ((*t)->Tbf == 0) deltah = 0;
	*dh = deltah;
	return (newt);
}

TREE *Tinsert (t,p,find)
TREE **t;
register char *p;
int find;
{
	int deltah;

	if (p != NULL && p[0] == '.' && p[1] == '/') {
		p += 2;
		while (*p == '/') p++;
		if (*p == 0) p = ".";
	}
	return (Tinsertavl (t,p,find,&deltah));
}

TREE *Tsearch (t,p)
TREE *t;
char *p;
{
	register TREE *x;
	register int cmp;

	x = t;
	while (x) {
		cmp = strcmp (p,x->Tname);
		if (cmp == 0)  return (x);
		if (cmp < 0)	x = x->Tlo;
		else		x = x->Thi;
	}
	return (NULL);
}

TREE *Tlookup (t,p)
TREE *t;
char *p;
{
	register TREE *x;
	register char *a, *b;

	x = t;
	while (x) {
		for (a = x->Tname, b = p; *a == *b && *a != '\0'; a++, b++) ;
		if (*a == '\0' && (*b == '\0' || *b == '/'))
			return(x);
		if (*a > *b)
			x = x->Tlo;
		else
			x = x->Thi;
	}
	return (NULL);
}

Static
int Tsubprocess (t,reverse,f,argp)
TREE *t;
int reverse;
int (*f)();
int *argp;
{
	register int x = SCMOK;
	if (reverse?t->Thi:t->Tlo)  x = Tsubprocess (reverse?t->Thi:t->Tlo,reverse,f,argp);
	if (x != SCMOK)  return (x);
	x = (*f) (t,argp);
	if (x != SCMOK)  return (x);
	if (reverse?t->Tlo:t->Thi)  x = Tsubprocess (reverse?t->Tlo:t->Thi,reverse,f,argp);
	return (x);
}

/* VARARGS2 */
int Trprocess (t,f,args)
TREE *t;
int (*f)();
int args;
{
	if (t == NULL)  return (SCMOK);
	return (Tsubprocess (t,TRUE,f,&args));
}

/* VARARGS2 */
int Tprocess (t,f,args)
TREE *t;
int (*f)();
int args;
{
	if (t == NULL)  return (SCMOK);
	return (Tsubprocess (t,FALSE,f,&args));
}

Static
int Tprintone (t)
TREE *t;
{
	printf ("Node at %X name '%s' flags %o hi %X lo %X\n",t,t->Tname,t->Tflags,t->Thi,t->Tlo);
	return (SCMOK);
}

Tprint (t,p)		/* print tree -- for debugging */
TREE *t;
char *p;
{
	printf ("%s\n",p);
	(void) Tprocess (t,Tprintone);
	printf ("End of tree\n");
	fflush (stdout);
}
